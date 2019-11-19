#ifndef DHTSIM_NODE_H
#define DHTSIM_NODE_H

#include "application.hpp"
#include "base.hpp"
#include "time.hpp"
#include "dhtnode.hpp"

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
    : public BaseApplication<uint32_t>,
      public DHTNode<KademliaKey, std::vector<unsigned char>> {
public:
	/////// TYPES

	/* A bunch of type aliases for callback types. */

	using SendCallbackSet = CallbackSet<Message<uint32_t>, Message<uint32_t>>;
	using PingCallbackSet = CallbackSet<int, int>;
	using FindNodesCallbackSet = CallbackSet<FindNodesMessage, FindNodesMessage>;

	/* Aliases for the DHTNode callback types */
	using GetCallbackSet = DHTNode<KademliaKey, std::vector<unsigned char>>::GetCallbackSet;

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


	/** An entry in this node's hash table */
	struct TableEntry {
		std::vector<unsigned char> value;
		Time last_touch;
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
	KademliaNode();

	/* Accessors */
        Key getKey() { return this->key; }

        /* Virtual (inherited) functions */

	virtual void tick(Time time);
	virtual void handleMessage(const Message<uint32_t>& m);
	virtual void handleMessage(const Message<uint32_t>& m, FindNodesMessage& fm);

	/* High level DHT node functions */
	virtual Key put(const std::vector<unsigned char>& v);
	virtual void get(const Key& k, GetCallbackSet callback);
	virtual Key getKey(const std::vector<unsigned char>& value);
        virtual void die() { BaseApplication<uint32_t>::die(); }

        /* RPCS */

	void findNodes(const Key& target, FindNodesCallbackSet callback);
	void findValue(const Key& target, FindNodesCallbackSet callback);

	// This just does findNodes and then calls the other overload
	// of store with the addresses that were returned.
	void store(const std::vector<unsigned char>& value);
	void store(uint32_t target_address,
	           const std::vector<unsigned char>& data);

	void ping(uint32_t target_address, PingCallbackSet callback);

	void observe(uint32_t other_address, const Key& other_key);
private:
	/** How many entries in each routing bucket? */
	const unsigned k = 10;

	/** FIND_NODES concurrency parameter. */
	const unsigned alpha = 3;

	Key key;
	std::vector<std::vector<BucketEntry>> buckets;

	/** The table of data that this node stores */
	std::map<Key, TableEntry> table;


	/** Called every time we see another node */
	void updateOrAddToBucket(unsigned bucket_index, BucketEntry entry);

	std::vector<BucketEntry> getNearest(unsigned n, const Key& key);
	std::vector<BucketEntry> getNearest(unsigned n, const Key& key, const Key& exclude);

	/** findNodes helpers */
	std::map<Key, NodeFinder> nodes_being_found;
	void findNodesStart(const Key& target);
        void findNodesStep(const Key &target,
                           const std::vector<BucketEntry> &new_nodes = {});
	void findNodesFail(const Key& target);
        void findNodesFinish(const Key& target);
	void findNodesFinish(const Key& target, const std::vector<unsigned char>& value);

	/** store helper */
	void storeValue(const Key& store_under, const std::vector<unsigned char>& value);
};

} // namespace dhtsim
#endif
