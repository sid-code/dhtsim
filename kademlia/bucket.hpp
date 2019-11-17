#ifndef DHTSIM_KADEMLIA_BUCKET_HPP
#define DHTSIM_KADEMLIA_BUCKET_HPP

#include <cstdint>

#include "key.hpp"
#include "time.hpp"

namespace dhtsim {
/* This is one entry in a k-bucket. It holds a node id, a
 * network address, and a time when this node was last
 * seen. */
struct BucketEntry {
	KademliaKey key;
	uint32_t address;
	Time lastSeen;
	friend bool operator<(const BucketEntry &l, const BucketEntry &r) {
		return l.key < r.key;
	}
	friend bool operator>(const BucketEntry &l, const BucketEntry &r) {
		return l.key > r.key;
	}

	BucketEntry() {}

	NOP_STRUCTURE(BucketEntry, key, address, lastSeen);
};
} // namespace dhtsim

#endif
