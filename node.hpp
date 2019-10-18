#ifndef DHTSIM_NODE_H
#define DHTSIM_NODE_H

#include "application.hpp"
#include "base.hpp"
#include "time.hpp"

#include <vector>
#include <optional>
#include <openssl/sha.h>

namespace dhtsim {

uint32_t keyDistance(uint32_t x, uint32_t y) {
	return x ^ y;
}

/**
 * A distributed hash table node in a network where addresses are 32
 * bits.
 */
class Node : public BaseApplication<uint32_t> {
public:
	// As per kademlia, the key length is the length of the SHA1
	// digest.
	static const unsigned int KEY_LEN = SHA_DIGEST_LENGTH;

	struct Key {
		unsigned char key[KEY_LEN];
	};

	// This is one entry in a k-bucket. It holds a node id, a
	// network address, and a time when this node was last seen.
	struct BucketEntry {
		Key key;
		uint32_t address;
		Time lastSeen;
	};

	Node();
	/** RPCS */

	void find_nodes(const Key& target, std::optional<CallbackFunction> cb);
	void find_value(const Key& target, std::optional<CallbackFunction> cb);
	void store(const Key& target, std::vector<unsigned char> data,
	           std::optional<CallbackFunction> cb);
	void ping(const Key& target, std::optional<CallbackFunction> cb);
private:
	const unsigned k = 64;

	Key key;
	std::vector<std::vector<BucketEntry>> buckets;

	/** Called every time we see another node */
	void observe(uint32_t other_address, const Key& other_key);
	void add_to_bucket(unsigned bucket_index, BucketEntry entry);
};

} // namespace dhtsim
#endif
