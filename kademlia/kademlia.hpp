#ifndef DHTSIM_NODE_H
#define DHTSIM_NODE_H

#include "application.hpp"
#include "base.hpp"
#include "time.hpp"

#include <vector>
#include <queue>
#include <map>
#include <set>
#include <optional>
#include <openssl/sha.h>
#include <cstring>

#include <nop/structure.h>

#include "key.hpp"
#include "message_structs.hpp"

namespace dhtsim {

/**
 * A distributed hash table node in a network where addresses are 32
 * bits.
 *
 * We inherit from BaseApplication<> for the simple message passing
 * and receiving capabilities. We also inherit from
 * Application<>::DHTNode<> to provide an abstraction for the DHT.
 */
class KademliaNode
	: public BaseApplication<uint32_t> {
public:
	/////// TYPES

	/* A bunch of type aliases for callback types. */

	using SendCallbackSet = CallbackSet<Message<uint32_t>, Message<uint32_t>>;
	using PingCallbackSet = CallbackSet<int, int>;
	using FindNodesCallbackSet = CallbackSet<FindNodesMessage, FindNodesMessage>;

	/* As per kademlia, the key length is the length of the SHA1
	 * digest */
	static const unsigned int KEY_LEN = KADEMLIA_KEY_LEN;
	static const unsigned int KEY_LEN_BITS = KEY_LEN * 8;

	/** A node key. As per Kademlia, this is 160 bits and
	 * calculated y SHA1. */
	using Key = KademliaKey;

	/* The message types */
	enum MessageType {
		KM_PING, KM_FIND_NODES, KM_FIND_VALUE, KM_STORE
	};

	/** Configuration object */
	struct Config {
		/** How many entries in each routing bucket? */
		unsigned int k = 20;

		/** FIND_NODES concurrency parameter. */
		unsigned int alpha = 3;

		/** How often should runMaintenance be called? */
		unsigned long maintenance_period = 10000;

		/** How often should refreshBuckets be called? */
		unsigned long bucket_refresh_period = 1000;

		friend std::ostream &operator<<(std::ostream &os,
		                                const Config &conf) {
			os << "KademliaConfig(k=" << conf.k << ", alpha=" << conf.alpha
			   << ", maintenance=" << conf.maintenance_period
			   << ", bucket_refresh=" << conf.bucket_refresh_period << ")";
			return os;
		}
	};

	Config config;

	/** An entry in this node's hash table */
	struct TableEntry {
		std::vector<unsigned char> value;

		/* When was this value last sent to us by a neighbor
		 * node? */
		Time last_touch;

		/* When was this value first added? */
		Time added;
	};

	/**
	 * A node finder. This is used by the find_nodes operation to
	 * keep track of progress.
	 */
	struct NodeFinder {
		NodeFinder() = default;
		NodeFinder(Key target, FindNodesCallbackSet callback)
			: target(target), find_nodes_callback(callback) {};
		bool find_value = false; // was this a find_value call?
		Key target; // the key of the node being searched for
		FindNodesCallbackSet find_nodes_callback; // the callback set to call when done
		uint32_t waiting = 0; // number of recursive find nodes pending
		std::vector<BucketEntry> uncontacted; // nodes yet to contact
		std::vector<BucketEntry> contacted; // nodes to be returned
		std::set<Key> seen; // nodes already seen
	};


	/////// Public API
	KademliaNode(Config config);

	/* Accessors */
        Key getKey() { return this->key; }

        /* Virtual (inherited) functions */

	virtual void tick(Time time);
	virtual void handleMessage(const Message<uint32_t>& m);
	virtual void handleMessage(const Message<uint32_t>& m, FindNodesMessage& fm);

        /* RPCS */

	void findNodes(const Key& target, FindNodesCallbackSet callback);
	void findValue(const Key& target, FindNodesCallbackSet callback);

	// This just does findNodes and then calls the other overload
	// of store with the addresses that were returned.
	Key store(const std::vector<unsigned char>& value);
	Key store(uint32_t target_address,
		  const std::vector<unsigned char>& data);

	void ping(uint32_t target_address, PingCallbackSet callback);

	void observe(uint32_t other_address, const Key& other_key);

	// temp debug func
	void dumpBuckets() {
		for (unsigned i = 0; i < KEY_LEN_BITS; i++) {
			if (this->buckets[i].empty()) continue;
			std::cout << "[" << this->getKey() << "] bucket "
			          << i << ": " << std::endl;
			for (const auto &entry : this->buckets[i]) {
				std::cout << entry.key << " "
				          << entry.address << std::endl;
			}
                }
        }
private:

	Key key;
	std::vector<std::vector<BucketEntry>> buckets;

	/** The table of data that this node stores */
	std::map<Key, TableEntry> table;


	/** Called every time we see another node */
	void updateOrAddToBucket(unsigned bucket_index, BucketEntry entry);
	/** Called every time we fail to contact a node */
	void unobserve(uint32_t other_address);

	std::vector<BucketEntry> getNearest(unsigned n, const Key& key);
	std::vector<BucketEntry> getNearest(unsigned n, const Key& key, const Key& exclude);

	/**
	 * A map of addresses being pinged to their callbacks.
	 * To avoid sending tons of pings, keep track of them and add
	 * callbacks together
	 */
	std::map<uint32_t, PingCallbackSet> pings_in_progress;

	/* findNodes helpers */

	/**
	 * A map of keys that are being searched for to the data
	 * structure that keeps track of the process. This is similar
	 * to the pings_in_progress map but a little more
	 * elaborate.
	 */
	std::map<Key, NodeFinder> nodes_being_found;
	void findNodesStart(const Key& target);
        void findNodesStep(const Key &target,
                           const std::vector<BucketEntry> &new_nodes = {});
	void findNodesFail(const Key& target);
        void findNodesFinish(const Key& target);
	void findNodesFinish(const Key& target, const std::vector<unsigned char>& value);

	/** store helper */
	void storeValue(const Key& store_under, const std::vector<unsigned char>& value);

	/**
	 * This ensures stale table entries are deleted and non-stale
	 * table entries are re-transmitted, so nodes don't hold
	 * values that they shouldn't be holding onto for too long. It
	 * also allows the network to heal after node losses.
	 */
	void runTableMaintenance();

	/**
	 * This ensures that buckets whose node range haven't been
	 * queried in a long time remain fresh.
	 */
	using RefreshCallbackSet = CallbackSet<int, int>;
	void refreshSingleBucket(unsigned int bucket_index, RefreshCallbackSet cb);
	void refreshBuckets(RefreshCallbackSet cb);

	/**
	 * Since nodes are expected to perform maintenance every N
	 * ticks (where N is a parameter chosen by the simulator), we
	 * don't want them to all do it at the same time. Therefore,
	 * we choose a random integer X from [0, maintenance_period)
	 * and have this node run maintenance at that offset every
	 * time to minimize the overlap with other node maintenance.
	 */
	Time maintenance_offset;

};

} // namespace dhtsim
#endif
