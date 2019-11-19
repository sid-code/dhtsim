#include "kademlia.hpp"
#include "key.hpp"
#include "bucket.hpp"
#include "message_structs.hpp"
#include "application.hpp"
#include "message.hpp"

#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <queue>
#include <chrono>
#include <thread>

#include <nop/structure.h>
#include <nop/serializer.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>
#include <openssl/sha.h>

using namespace dhtsim;

KademliaNode::KademliaNode() {
	// Generate a random key with SHA1
	unsigned long randval = this->randomTag();
	SHA1((unsigned char *)&randval, sizeof(unsigned long), this->key.key);

	this->buckets.resize(KEY_LEN_BITS);
	for (unsigned i = 0; i < KEY_LEN_BITS; i++){
		this->buckets[i].reserve(this->k);
	}
}


static KademliaNode::Key getSHA1(const std::vector<unsigned char>& value) {
	KademliaNode::Key result;
	SHA1(&value[0], value.size(), result.key);
	return result;
}

/** How many trailing zeros does c have in binary? */
static unsigned l2c(unsigned char c) {
	unsigned result = 0;
	for ( ; c > 0; c >>= 1, result++);
	return result;
}

/** Longest matching binary prefix of the two keys. */
static unsigned longest_matching_prefix(const KademliaNode::Key& k1, const KademliaNode::Key& k2) {
	unsigned i;
	for (i = 0; i < KademliaNode::KEY_LEN; i++) {
		unsigned char cmp = k1.key[i] ^ k2.key[i];
		if (cmp != 0) {
			return (i + 1) * 8 - l2c(cmp);
		}
	}
	return KademliaNode::KEY_LEN_BITS;
}

/** Returns true if k1 is closer to target, false otherwise. */
static bool key_distance_cmp(const KademliaNode::Key& target,
			     const KademliaNode::Key& k1,
			     const KademliaNode::Key& k2) {
	unsigned i;
	for (i = 0; i < KademliaNode::KEY_LEN; i++) {
	        unsigned char k1t = k1.key[i] ^ target.key[i];
		unsigned char k2t = k2.key[i] ^ target.key[i];
		if (k1t != k2t) {
			return k1t < k2t;
		}
	}

	return false;
}



void KademliaNode::tick(Time time) {
	BaseApplication<uint32_t>::tick(time);
}

static void sortByDistanceTo(const KademliaNode::Key& target,
                             std::vector<BucketEntry>& bucket) {

        auto cmp_fn = [target](const BucketEntry &e1,
                            const BucketEntry &e2) {
	                      return key_distance_cmp(target, e1.key, e2.key);
                      };

	std::sort(bucket.begin(), bucket.end(), cmp_fn);
}
// static void insertSortedByDistanceTo(const KademliaNode::Key& target,
//                                      std::vector<BucketEntry>& entries,
//                                      const BucketEntry& entry) {
// 	for (auto it = entries.begin(); it != entries.end(); it++) {
// 		if (!key_distance_cmp(target, it->key, entry.key)) {
// 			entries.insert(it, entry);
// 			return;
// 		}
// 	}

// 	entries.push_back(entry);
// }

std::vector<BucketEntry> KademliaNode::getNearest(
	unsigned n, const Key& target) {
	return this->getNearest(n, target, this->getKey());
}

std::vector<BucketEntry> KademliaNode::getNearest(
	unsigned n, const Key& target, const Key& exclude) {

	std::vector<BucketEntry> entries, result;
	unsigned i;
	for (i = 0; i < KEY_LEN; i++) {
		for (const auto &entry : this->buckets[i]) {
			if (!(entry.key == exclude)) {
				entries.push_back(entry);
			}
		}
	}
	sortByDistanceTo(target, entries);
	for (i = 0; i < entries.size(); i++) {
		if (i >= n) break;
		result.push_back(entries[i]);
	}
	return result;
}

void KademliaNode::ping(uint32_t other_address, PingCallbackSet callback) {
	Message<uint32_t> m(KM_PING, this->getAddress(), other_address, 0,
	                    std::vector<unsigned char>());
	PingMessage pm = PingMessage::ping();
	pm.sender = this->getKey();
	m.data.resize(KEY_LEN + 20);
	writeToMessage(pm, m);

	auto cb_success = [callback](Message<uint32_t> m) {
		                 (void) m;
		                 callback.success(0);
	                 };
	auto cb_failure = [this, callback, other_address](Message<uint32_t> m) {
				 (void) m;
				 this->unobserve(other_address);
				 callback.failure(1);
			 };

	this->send(m, SendCallbackSet(cb_success, cb_failure));
}

/* One step in the find_nodes operation.  This function is quite
 * convoluted. It is called every time new nodes are learned of during
 * the find_nodes process. I'll come back to that later.
 *
 * It first retrieves the "node finder" from the table stored on the
 * node. This holds temporary information about the status of the
 * find_nodes operation. Then, it adds the newly learned-of nodes to
 * the list of uncontacted nodes. If there are no more uncontacted
 * nodes and no more nodes are being waited on, then we mark the
 * process as over and return early.
 *
 * Otherwise, we repeatedly remove an element from the list of
 * uncontacted nodes (sorted by distance to the target) until we get
 * one we haven't queried before, and then send a FIND_NODES message
 * to it. When it responds with its k-nearest nodes, this function is
 * called semi-recursively. I say semi-recursively because it's
 * scheduled to be called in a callback. Therefore, it doesn't
 * actually deepen the stack, as callbacks are all resolved at the
 * same stack level. */
void KademliaNode::findNodesStep(const Key& target, const std::vector<BucketEntry>& new_nodes) {
	// Retrive the node finder
	auto nf_it = this->nodes_being_found.find(target);
	if (nf_it == this->nodes_being_found.end()) {
		std::clog << "Attempt to call findNodesStep without valid key" << std::endl;
		return;
	}

	// Add the newly learned-of nodes to the uncontacted list, but
	// only if they're unseen. They will be contacted later.
	NodeFinder& nf = nf_it->second;
	for (auto& entry : new_nodes) {
		if (nf.seen.find(entry.key) == nf.seen.end()) {
			nf.uncontacted.push_back(entry);
			nf.seen.insert(entry.key);
		}
	}

	// Stopping condition
	if (nf.uncontacted.empty()) {
		if (nf.waiting == 0) {
			if (nf.find_value) {
				this->findNodesFail(target);
			} else {
				this->findNodesFinish(target);
			}
		} else{
			std::cout << "Dry return." << std::endl;}
		return;
	}

	sortByDistanceTo(target, nf.uncontacted);

	// Pull off as many as necessary to get a new one, and query it.
	BucketEntry top;
	while (true) {
		if (nf.uncontacted.empty()) {
			return;
		}

		auto first_it = nf.uncontacted.begin();
		top = *first_it;
		nf.uncontacted.erase(first_it);

		break;
	}

	// This is to tell whether we have more callbacks that need to
	// complete.
        nf.waiting++;

        // Message building boilerplate.
	Message<uint32_t> m;
	m.type = KademliaNode::KM_FIND_NODES;
	m.originator = this->getAddress();
	m.destination = top.address;
	m.tag = 0; // the send function will set this to a random value.

	FindNodesMessage fm;
	fm.request = true;
	fm.sender = this->getKey();
	fm.target = target;
	fm.find_value = nf.find_value;
	fm.num_found = 0;

	writeToMessage(fm, m);

	// lambda captures kept to a minimum
	auto cbSuccess =
		[this, target, top](Message<uint32_t> m) {
			FindNodesMessage fm;
			auto nf_it = this->nodes_being_found.find(target);
			if (nf_it == this->nodes_being_found.end()) return;
			auto& nf = nf_it->second;
			nf.contacted.push_back(top);
                        nf.waiting--;
                        readFromMessage(fm, m);
                        if (fm.find_value && fm.value_found) {
	                        this->findNodesFinish(target, fm.value);
                        } else {
				this->findNodesStep(target, fm.nearest);
                        }
		};
	auto cbFailure =
		[this, target, top](Message<uint32_t> m) {
			(void) m;
			// Remove it from our buckets
			this->unobserve(top.address);

			auto nf_it = this->nodes_being_found.find(target);
			if (nf_it == this->nodes_being_found.end()) return;
			auto& nf = nf_it->second;
                        nf.waiting--;
			this->findNodesStep(target, {});
		};

	this->send(m, SendCallbackSet(cbSuccess, cbFailure), 1, 2);
}
void KademliaNode::findNodesStart(const Key& target) {
	auto nearest = this->getNearest(this->k, target);
	this->findNodesStep(target, nearest);
}

void KademliaNode::findNodesFail(const Key& target) {
	auto nf_it = this->nodes_being_found.find(target);
	FindNodesMessage fm;
	fm.find_value = true;
	fm.value_found = false;
	nf_it->second.find_nodes_callback.failure(fm);
}

void KademliaNode::findNodesFinish(const Key& target) {
	auto nf_it = this->nodes_being_found.find(target);
	sortByDistanceTo(target, nf_it->second.contacted);
	FindNodesMessage result;
	result.request = false;
	result.find_value = false;
	result.num_found = nf_it->second.contacted.size();
	result.nearest = nf_it->second.contacted;
	nf_it->second.find_nodes_callback.success(result);
	this->nodes_being_found.erase(target);
}

void KademliaNode::findNodesFinish(const Key& target, const std::vector<unsigned char>& value) {
	auto nf_it = this->nodes_being_found.find(target);
	FindNodesMessage result;
	result.request = false;
	result.find_value = true;
	result.value_found = true;
	result.value = value;
	nf_it->second.find_nodes_callback.success(result);
	this->nodes_being_found.erase(target);
}

void KademliaNode::findNodes(const Key& target, FindNodesCallbackSet callback) {
	auto loc = this->nodes_being_found.find(target);
	if (loc != this->nodes_being_found.end()) {
		loc->second.find_nodes_callback += callback;
		return;
	}

	NodeFinder nf(target, callback);
	this->nodes_being_found[target] = nf;
	this->findNodesStart(target);
}
void KademliaNode::findValue(const Key& target, FindNodesCallbackSet callback) {
	auto loc = this->nodes_being_found.find(target);
	if (loc != this->nodes_being_found.end()) {
		loc->second.find_nodes_callback += callback;
		return;
	}

	NodeFinder nf(target, callback);
	nf.find_value = true;
	this->nodes_being_found[target] = nf;
	this->findNodesStart(target);
}

void KademliaNode::store(const std::vector<unsigned char>& value) {

	auto store_under = this->getKey(value);
	auto cb_success = [this, value](FindNodesMessage m) {
		                  // We must sleep here for 1
		                  // nanosecond. Otherwise this
		                  // doesn't work.
				  std::this_thread::sleep_for(std::chrono::nanoseconds(1));
				  for (const auto& entry : m.nearest) {
					  this->store(entry.address, value);
				  }
			  };

	this->findNodes(store_under, FindNodesCallbackSet::onSuccess(cb_success));
}
void KademliaNode::store(uint32_t target_address,
                         const std::vector<unsigned char>& value) {
	StoreMessage sm;
	sm.sender = this->getKey();
	sm.value = value;

	Message<uint32_t> m;
	m.type = KM_STORE;
	m.originator = this->getAddress();
	m.destination = target_address;

	writeToMessage(sm, m);

	this->send(m);
}

void KademliaNode::storeValue(const Key& store_under, const std::vector<unsigned char>& value) {
	auto loc = this->table.find(store_under);
	if (loc != this->table.end()) {
		loc->second.last_touch = this->epoch;
		return;
	}
	KademliaNode::TableEntry table_entry;
	table_entry.value = value;
	table_entry.last_touch = this->epoch;

	this->table[store_under] = table_entry;
}

void KademliaNode::handleMessage(const Message<uint32_t>& m, FindNodesMessage& fm) {
	auto resp = m;
	if (fm.request) {
		// First, check the request is for a value and if we have that value.
		auto loc = this->table.find(fm.target);
		if (fm.find_value && loc != this->table.end()) {
			std::cout << "[" << fm.sender << "] " << this->getKey()
				  << ".find_value(" << fm.target << ") FOUND!" << std::endl;

			fm.value_found = true;
			fm.value = loc->second.value;
		} else {
			std::cout << "[" << fm.sender << "] " << this->getKey()
			          << (fm.find_value ? ".find_value(" : ".find_nodes(")
			          << fm.target << ")" << std::endl;

			auto entries = this->getNearest(this->k, fm.target, fm.sender);
			fm.num_found = entries.size();
			fm.nearest = entries;
		}
		fm.request = 0;
		fm.sender = this->getKey();
		writeToMessage(fm, resp);
		std::swap(resp.destination, resp.originator);
		this->send(resp);
	} else {
		for (const auto &entry : fm.nearest) {
			// std::clog << "Observed " << entry.key
			//          << " at " << entry.address << std::endl;
			this->observe(entry.address, entry.key);
		}
	}
}

void KademliaNode::handleMessage(const Message<uint32_t>& m) {
	Key sender;
	BaseApplication<uint32_t>::handleMessage(m);
	auto resp = m;
	switch (m.type) {
	case KM_PING: {
		if (m.data.size() < 1) {
			std::clog << "malformed ping (empty body)" << std::endl;
			break;
		}
		PingMessage pm(true);
		readFromMessage(pm, m);
		auto pingpong = pm.is_ping() ? "ping" : "pong";
		std::cout << "[" << pm.sender << "] " << pingpong
		          <<"(" << this->getKey() << ")" << std::endl;

		// Observe
		sender = pm.sender;
		this->observe(m.originator, sender);

		if (pm.is_ping()) {
			PingMessage outbound = PingMessage::pong();
			outbound.sender = this->getKey();
			resp.destination = m.originator;
			resp.originator = this->getAddress();
			writeToMessage(outbound, resp);
			this->send(resp);
		}
		break;
	}
	case KM_FIND_NODES: {
		if (m.data.size() < 1+KEY_LEN+4) {
			std::clog << "malformed find_nodes (too short)" << std::endl;
			break;
		}
		FindNodesMessage fm;
		// This is unsafe. Out of bounds reads may happen.
		readFromMessage(fm, m);
		// Observe
		sender = fm.sender;
		this->observe(m.originator, sender);

		this->handleMessage(m, fm);
		break;
	}
	case KM_STORE: {
		StoreMessage sm;
		readFromMessage(sm, m);
		this->observe(m.originator, sm.sender);

		if (sm.request) {
			auto store_under = this->getKey(sm.value);
			std::cout << "[" << sm.sender << "] " << this->getKey() << ".store("
			          << store_under << ")" << std::endl;
			this->storeValue(store_under, sm.value);

			sm.request = false;
			sm.value.clear();
			sm.sender = this->getKey();
			auto resp = m;
			std::swap(resp.originator, resp.destination);
			writeToMessage(sm, resp);
			this->send(resp);
		}

		break;
	}
	default:
		std::clog << "received an unknown message!" << std::endl;
	}
}

void KademliaNode::updateOrAddToBucket(unsigned bucket_index, BucketEntry new_entry) {
	if (bucket_index == KEY_LEN_BITS) {
		return;
	}

	auto& bucket = this->buckets[bucket_index];

	// Case 1: We have already seen the key of the new entry: we
	// just need to delete that entry and add this one.

	auto it = bucket.begin();
	for ( ; it != bucket.end(); it++) {
		if (it->key == new_entry.key) {
			it = bucket.erase(it);
			bucket.push_back(new_entry);
			//std::clog << "[" << this->getKey() << "]"
			//	  << " hoisted in bucket " << bucket_index << ": "
			//	  << new_entry.key << std::endl;
			return;
		}
	}

	// Case 2: We have not seen the key of the new entry, but there is
	// still space left to add a new key.

	if (bucket.size() < k) {
		bucket.push_back(new_entry);
		//std::clog << "[" << this->getKey() << "]"
		//          << " added to bucket " << bucket_index << ": "
		//          << new_entry.key << std::endl;
		return;
	}

	// Case 3: We have not esen the key of the new entry, and
	// there is no space left.

	// Ping the least-recently seen node:
	auto lrs_entry = bucket[0];
	auto lrs_address = bucket[0].address;
	auto tag = this->randomTag();
	std::vector<unsigned char> data;

	PingMessage pm = PingMessage::ping();
	pm.sender = this->getKey();


	auto msg = Message<uint32_t>(
		KM_PING, this->getAddress(), lrs_address, tag, data);
	writeToMessage(pm, msg);

	// If the node responds, we hoist it to the most recently seen
	// position, and drop the new entry.
        auto cbSuccess =
	        [this, bucket_index, lrs_entry](auto m) {
		        (void) m; // unused
		};

        // If the node fails to respond, we delete it and add the new
        // entry.
        auto cbFail =
	        [this, bucket_index, new_entry] (auto m) {
		        (void) m; // unused
		        this->buckets[bucket_index].erase(
			        this->buckets[bucket_index].begin());
		        this->buckets[bucket_index].push_back(new_entry);
		};

        this->send(msg, SendCallbackSet(cbSuccess, cbFail));
}

void KademliaNode::observe(uint32_t other_address, const KademliaNode::Key& other_key) {
	unsigned which_bucket = longest_matching_prefix(this->key, other_key);
	BucketEntry entry;
	entry.key = other_key;
	entry.address = other_address;
	entry.lastSeen = this->epoch;
	updateOrAddToBucket(which_bucket, entry);
}
void KademliaNode::unobserve(uint32_t other_address) {
	// Search all buckets for that address:
	for (auto& bucket : this->buckets) {
		for (auto it = bucket.begin(); it != bucket.end(); ) {
			if (it->address == other_address) {
				it = bucket.erase(it);
			} else {
				++it;
			}
		}
	}
}

void KademliaNode::get(const Key& stored_key, GetCallbackSet callback) {
	this->findValue(stored_key, FindNodesCallbackSet::onSuccess(
		                [callback](FindNodesMessage fm) {
			                if (fm.value_found) {
						callback.success(fm.value);
			                } else {
						callback.failure(fm.value);
			                }
		                }) + FindNodesCallbackSet::onFailure(
		                [callback](FindNodesMessage fm) {
			                callback.failure(fm.value);
		                }));
}
KademliaNode::Key KademliaNode::put(const std::vector<unsigned char>& value) {
	auto key = this->getKey(value);
	this->store(value);
	return key;
}
KademliaNode::Key KademliaNode::getKey(const std::vector<unsigned char>& value) {
	return getSHA1(value);
}
