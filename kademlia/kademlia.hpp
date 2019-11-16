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

namespace dhtsim {

static const unsigned int KADEMLIA_KEY_LEN = SHA_DIGEST_LENGTH;

struct KademliaKey {
	unsigned char key[KADEMLIA_KEY_LEN];
	friend bool operator==(const KademliaKey& l, const KademliaKey& r) {
		return memcmp(&l.key, &r.key, KADEMLIA_KEY_LEN) == 0;
	}
	friend bool operator<(const KademliaKey& l, const KademliaKey& r) {
		return memcmp(&l.key, &r.key, KADEMLIA_KEY_LEN) < 0;
	}
	friend bool operator>(const KademliaKey& l, const KademliaKey& r) {
		return memcmp(&l.key, &r.key, KADEMLIA_KEY_LEN) > 0;
	}
	friend std::ostream &operator<<(std::ostream &os,
					const KademliaKey &key) {
		const unsigned PRINT_MAX = 4;
		unsigned i;
		for (i = 0; i < PRINT_MAX; i++) {
			os << std::hex << (int)key.key[i];
			os << std::dec;
		}
		return os;
	}
};

/**
 * A distributed hash table node in a network where addresses are 32
 * bits.
 *
 * We inherit from BaseApplication<> for the simple message passing
 * and receiving capabilities. We also inherit from
 * Application<>::DHTNode<> to provide an abstraction for the DHT.
 */
class KademliaNode
    : public BaseApplication<uint32_t>,
      public Application<uint32_t>::DHTNode<KademliaKey, std::vector<unsigned char>> {
public:
	struct BucketEntry;

	/////// TYPES

	/* A bunch of type aliases for callback types. */

	using CallbackSet = Application<uint32_t>::CallbackSet<>;
	using PingCallbackSet = Application<uint32_t>::CallbackSet<int, int>;
	using FindNodesCallbackSet =
		Application<uint32_t>::CallbackSet<std::vector<KademliaNode::BucketEntry>, int>;
	using FindValueCallbackSet = Application<uint32_t>::CallbackSet<void, void>;
	using StoreCallbackSet = Application<uint32_t>::CallbackSet<void, void>;

	/* Aliases for the DHTNode callback types */
	using GetCallbackSet = Application<uint32_t>::DHTNode<KademliaKey, std::vector<unsigned char>>::GetCallbackSet;

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

	/* This is one entry in a k-bucket. It holds a node id, a
	 * network address, and a time when this node was last
	 * seen. */
	struct BucketEntry {
		Key key;
		uint32_t address;
		Time lastSeen;
		friend bool operator<(const BucketEntry& l, const BucketEntry& r) {
			return l.key < r.key;
		}
		friend bool operator>(const BucketEntry& l, const BucketEntry& r) {
			return l.key > r.key;
		}
	};

	/**
	 * A node finder. This is used by the find_nodes operation to
	 * keep track of progress.
	 */
	struct NodeFinder {
		NodeFinder() = default;
		NodeFinder(Key target, FindNodesCallbackSet callback)
			: target(target), callback(callback) {};
		Key target; // the key of the node being searched for
		FindNodesCallbackSet callback; // the callback set to call when done
		uint32_t waiting = 0; // number of recursive find nodes pending
		std::vector<BucketEntry> uncontacted; // nodes yet to contact
		std::vector<BucketEntry> contacted; // nodes to be returned
		std::set<Key> seen; // nodes already seen
	};


	/////// Public API
	KademliaNode();

	/* Accessors */
        Key getKey() { return this->key; }

        /* Virtual (inherited) functions */

	virtual void tick(Time time);
	virtual void handleMessage(const Message<uint32_t>& m);

	/* High level DHT node functions */
	virtual Key put(const std::vector<unsigned char>& v);
	virtual void get(const Key& k, GetCallbackSet callback);
	virtual Key getKey(const std::vector<unsigned char>& value);

        /* RPCS */

	void findNodes(const Key& target, FindNodesCallbackSet callback);
	void findValue(const Key& target, const Key& stored_key, FindValueCallbackSet callback);
	void store(const Key& target, std::vector<unsigned char> data,
	           StoreCallbackSet callback);
	void ping(uint32_t target_address, PingCallbackSet callback);
	void observe(uint32_t other_address, const Key& other_key);
private:
	/** How many entries in each routing bucket? */
	const unsigned k = 10;

	/** FIND_NODES concurrency parameter. */
	const unsigned alpha = 3;

	Key key;
	std::vector<std::vector<BucketEntry>> buckets;


	/** Called every time we see another node */
	void updateOrAddToBucket(unsigned bucket_index, BucketEntry entry);

	std::vector<BucketEntry> getNearest(unsigned n, const Key& key);
	std::vector<BucketEntry> getNearest(unsigned n, const Key& key, const Key& exclude);

	/** findNodes helpers */
	std::map<Key, NodeFinder> nodes_being_found;
	void findNodesStart(const Key& target);
        void findNodesStep(const Key &target,
                           const std::vector<BucketEntry> &new_nodes = {});
        void findNodesFinish(const Key& target);
};

} // namespace dhtsim
#endif
