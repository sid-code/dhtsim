#include "application.hpp"
#include "kademlia.hpp"

#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <queue>
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


static unsigned l2c(unsigned char c) {
	unsigned result = 0;
	for ( ; c > 0; c >>= 1, result++);
	return result;
}

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

// Returns true if k1 is closer to target, false otherwise.
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

struct PingMessage {
        bool ping_or_pong;
	KademliaNode::Key sender;
	PingMessage(bool ping) : ping_or_pong(ping) {};
	bool is_ping() { return ping_or_pong; }
        static PingMessage ping() {
		return PingMessage(true);
	}
	static PingMessage pong() {
		return PingMessage(false);
	}
};
struct FindNodesMessage {
	bool request;
	KademliaNode::Key sender;
	KademliaNode::Key target;
	uint32_t num_found;
	std::vector<KademliaNode::BucketEntry> nearest;
	FindNodesMessage() = default;
};

static void write(std::vector<unsigned char>::iterator& it, bool x) {
	*it = x;
	it++;
}
static void read(std::vector<unsigned char>::const_iterator& it, bool& x) {
	x = *it;
	it++;
}
static void write(std::vector<unsigned char>::iterator& it, uint32_t x) {
	*it++ = x & 0xFF; x >>= 8;
	*it++ = x & 0xFF; x >>= 8;
	*it++ = x & 0xFF; x >>= 8;
	*it++ = x & 0xFF; x >>= 8;
}
static void read(std::vector<unsigned char>::const_iterator& it, uint32_t& x) {
	x = 0;
	x |= *it++;
	x |= *it++ << 8;
	x |= *it++ << (8*2);
	x |= *it++ << (8*3);
}
static void write(std::vector<unsigned char>::iterator& it, uint64_t x) {
	*it++ = x & 0xFF; x >>= 8;
	*it++ = x & 0xFF; x >>= 8;
	*it++ = x & 0xFF; x >>= 8;
	*it++ = x & 0xFF; x >>= 8;
	*it++ = x & 0xFF; x >>= 8;
	*it++ = x & 0xFF; x >>= 8;
	*it++ = x & 0xFF; x >>= 8;
	*it++ = x & 0xFF; x >>= 8;
}
static void read(std::vector<unsigned char>::const_iterator& it, uint64_t& x) {
	x = 0;
	x |= (uint64_t) *it++;
	x |= (uint64_t) (*it++) << 8;
	x |= (uint64_t) (*it++) << (8*2);
	x |= (uint64_t) (*it++) << (8*3);
	x |= (uint64_t) (*it++) << (8*4);
	x |= (uint64_t) (*it++) << (8*5);
	x |= (uint64_t) (*it++) << (8*6);
	x |= (uint64_t) (*it++) << (8*7);
}
static void write(std::vector<unsigned char>::iterator& it, const KademliaNode::Key& key) {
	std::copy(key.key,
	          key.key + KademliaNode::KEY_LEN,
	          it);
	it += KademliaNode::KEY_LEN;
}
static void read(std::vector<unsigned char>::const_iterator& it, KademliaNode::Key& key) {
	std::copy(it, it + KademliaNode::KEY_LEN, key.key);
	it += KademliaNode::KEY_LEN;
}
static void write(std::vector<unsigned char>::iterator& it,
                  const KademliaNode::BucketEntry& entry) {
	write(it, entry.key);
	write(it, entry.address);
	write(it, entry.lastSeen);
}
static void read(std::vector<unsigned char>::const_iterator& it,
		 KademliaNode::BucketEntry& entry) {
	read(it, entry.key);
	read(it, entry.address);
	read(it, entry.lastSeen);
}
static void write(std::vector<unsigned char>::iterator& it,
                  const PingMessage& pm) {
	write(it, pm.ping_or_pong);
	write(it, pm.sender);
}
static void read(std::vector<unsigned char>::const_iterator& it,
		 PingMessage& pm) {
	read(it, pm.ping_or_pong);
	read(it, pm.sender);
}
static void write(std::vector<unsigned char>::iterator& it,
                  const FindNodesMessage& fm) {
	write(it, fm.request);
	write(it, fm.sender);
	write(it, fm.target);
	write(it, fm.num_found);
	unsigned i;
	for (i = 0; i < fm.num_found; i++) {
		write(it, fm.nearest[i]);
	}
}
static void read(std::vector<unsigned char>::const_iterator& it,
		 FindNodesMessage& fm) {
	read(it, fm.request);
	read(it, fm.sender);
	read(it, fm.target);
	read(it, fm.num_found);
	unsigned i;
	fm.nearest.resize(fm.num_found);
	for (i = 0; i < fm.num_found; i++) {
		read(it, fm.nearest[i]);
	}
}

void test_rw() {
	std::vector<unsigned char> data(256, 0);
	std::vector<unsigned char>::iterator write_it = data.begin();
	std::vector<unsigned char>::const_iterator read_it;

	KademliaNode n;
	FindNodesMessage fm, fm_copy;

	fm.sender = n.getKey();
	fm.target = n.getKey();
	fm.num_found = 1;
	auto entry = KademliaNode::BucketEntry();
	entry.address = 2121212;
	entry.key = n.getKey();
	std::cout<<entry.key<<std::endl;
	std::cout<<entry.address<<std::endl;
	entry.lastSeen = 0;
	fm.nearest.push_back(entry);
	write(write_it, fm);
	read_it = data.begin();
	read(read_it, fm_copy);

	if (fm.sender == fm_copy.sender) {
		std::cout << "Test was a success." << std::endl;
	} else {
		std::cout << "Test was a failure." << std::endl;
		std::cout << "Stored key: " << fm.sender << std::endl;
		std::cout << "Read key:   " << fm_copy.sender << std::endl;
		std::exit(EXIT_FAILURE);
	}
	if (fm.num_found == fm_copy.num_found) {
		std::cout << "Test was a success." << std::endl;
	} else {
		std::cout << "Test was a failure." << std::endl;
		std::cout << "Stored nr: " << fm.num_found << std::endl;
		std::cout << "Read nr:   " << fm_copy.num_found << std::endl;
		std::exit(EXIT_FAILURE);
	}

}

void KademliaNode::tick(Time time) {
	BaseApplication<uint32_t>::tick(time);
}

static void sortByDistanceTo(const KademliaNode::Key& target,
                             std::vector<KademliaNode::BucketEntry>& bucket) {
	
        auto cmp_fn = [target](const KademliaNode::BucketEntry &e1,
                            const KademliaNode::BucketEntry &e2) {
	                      return key_distance_cmp(target, e1.key, e2.key);
                      };

	std::sort(bucket.begin(), bucket.end(), cmp_fn);
}
// static void insertSortedByDistanceTo(const KademliaNode::Key& target,
//                                      std::vector<KademliaNode::BucketEntry>& entries,
//                                      const KademliaNode::BucketEntry& entry) {
// 	for (auto it = entries.begin(); it != entries.end(); it++) {
// 		if (!key_distance_cmp(target, it->key, entry.key)) {
// 			entries.insert(it, entry);
// 			return;
// 		}
// 	}

// 	entries.push_back(entry);
// }

void KademliaNode::getNearest(unsigned n, const KademliaNode::Key& target,
			      std::vector<KademliaNode::BucketEntry>& out) {
	std::vector<KademliaNode::BucketEntry> entries;
	unsigned i;
	for (i = 0; i < KEY_LEN; i++) {
		for (const auto &entry : this->buckets[i]) {
			entries.push_back(entry);
		}
	}
	sortByDistanceTo(target, entries);
	for (i = 0; i < entries.size(); i++) {
		if (i >= n) break;
		out.push_back(entries[i]);
	}
}

void KademliaNode::ping(uint32_t other_address, PingCallbackSet callback) {
	Message<uint32_t> m(KM_PING, this->getAddress(), other_address, 0,
	                    std::vector<unsigned char>());
	PingMessage pm = PingMessage::ping();
	pm.sender = this->getKey();
	m.data.resize(KEY_LEN + 20);
	auto it = m.data.begin();
	write(it, pm);

	auto cbSuccess = [callback](Message<uint32_t> m) {
		                 (void) m;
		                 callback.success(0);
	                 };
	auto cbFailure = [callback](Message<uint32_t> m) {
				 (void) m;
				 callback.failure(1);
			 };

	this->send(m, CallbackSet(cbSuccess, cbFailure));
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
			this->findNodesFinish(target);
		}
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
	fm.num_found = 0;

	std::vector<unsigned char> data(64, 0);

	auto data_it = data.begin();
	write(data_it, fm);
	m.data = data;

	// lambda captures kept to a minimum
	auto cbSuccess = 
		[this, target, top](Message<uint32_t> m) {
			FindNodesMessage fm;
			std::vector<unsigned char>::const_iterator it = m.data.begin();
			read(it, fm);
			auto nf_it = this->nodes_being_found.find(target);
			auto& nf = nf_it->second;
			nf.contacted.push_back(top);
                        nf.waiting--;
			this->findNodesStep(target, fm.nearest);
		};
	this->send(m, CallbackSet::onSuccess(cbSuccess));
}
void KademliaNode::findNodesStart(const Key& target) {
	std::vector<BucketEntry> nearest;
	this->getNearest(this->k, target, nearest);
	this->findNodesStep(target, nearest);
}
void KademliaNode::findNodesFinish(const Key& target) {
	auto nf_it = this->nodes_being_found.find(target);
	sortByDistanceTo(target, nf_it->second.contacted);
	nf_it->second.callback.success(nf_it->second.contacted);
	this->nodes_being_found.erase(target);
}

void KademliaNode::findNodes(const Key& target, FindNodesCallbackSet callback) {
	std::vector<BucketEntry> start_nodes;
	if (this->nodes_being_found.find(target) != this->nodes_being_found.end()) {
		std::clog << "[" << this->getKey()
		          << "] Ignoring duplicate findNodes call."
		          << std::endl;
		return;
	}

	NodeFinder nf(target, callback);
	this->nodes_being_found[target] = nf;
	this->findNodesStart(target);
}

void KademliaNode::handleMessage(const Message<uint32_t>& m) {
	Key sender;
	BaseApplication<uint32_t>::handleMessage(m);
	std::vector<unsigned char>::const_iterator it = m.data.begin();
	auto resp = m;
	switch (m.type) {
	case KM_PING: {
		if (m.data.size() < 1) {
			std::clog << "malformed ping (empty body)" << std::endl;
			break;
		}
		PingMessage pm(true);
		read(it, pm);
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
			auto resp_data_it = resp.data.begin();
			write(resp_data_it, outbound);
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
		read(it, fm); // TODO: make this less unsafe
		// Observe
		sender = fm.sender;
		this->observe(m.originator, sender);

		if (fm.request) {
			std::cout << "[" << fm.sender << "] "
			          << this->getKey() << ".find_nodes(" << fm.target << ")"
			          << std::endl;

			std::vector<KademliaNode::BucketEntry> entries;
			this->getNearest(this->k, fm.target, entries);

			fm.request = 0;
			fm.sender = this->getKey();
			fm.num_found = entries.size();
			fm.nearest = entries;

			resp.data.resize(fm.num_found * (KEY_LEN + 20) + 10*KEY_LEN, 0);
			auto resp_data_it = resp.data.begin();
			write(resp_data_it, fm);
			std::swap(resp.destination, resp.originator);
			this->send(resp);
		} else {
			for (const auto& entry : fm.nearest) {
				//std::clog << "Observed " << entry.key
				//          << " at " << entry.address << std::endl;
				this->observe(entry.address, entry.key);
			}
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
	std::vector<unsigned char> data(64, 0);

	PingMessage pm = PingMessage::ping();
	pm.sender = this->getKey();
	auto data_it = data.begin();
	write(data_it, pm);

	auto msg = Message<uint32_t>(
		KM_PING, this->getAddress(), lrs_address, tag, data);

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
	
        this->send(msg, CallbackSet(cbSuccess, cbFail));
}

void KademliaNode::observe(uint32_t other_address, const KademliaNode::Key& other_key) {
	unsigned which_bucket = longest_matching_prefix(this->key, other_key);
	BucketEntry entry;
	entry.key = other_key;
	entry.address = other_address;
	entry.lastSeen = this->epoch;
	updateOrAddToBucket(which_bucket, entry);
}
