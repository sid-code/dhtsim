#include "application.hpp"
#include "kademlia.hpp"

#include <openssl/sha.h>

using namespace dhtsim;

KademliaNode::KademliaNode() {
	// Generate a random key with SHA1
	unsigned long randval = this->randomTag();
	SHA1((unsigned char *)&randval, sizeof(unsigned long), this->key.key);

	this->buckets.reserve(KademliaNode::KEY_LEN);
}

static unsigned longest_matching_prefix(const KademliaNode::Key& k1, const KademliaNode::Key& k2) {
	unsigned i;
	for (i = 0; i < KademliaNode::KEY_LEN; i++) {
		if (k1.key[i] != k2.key[i]) {
			break;
		}
	}
	return i;
}

void KademliaNode::add_to_bucket(unsigned bucket_index, BucketEntry entry) {
	(void)bucket_index, (void)entry;
	/** NOT IMPLEMENTED */
}

void KademliaNode::observe(uint32_t other_address, const KademliaNode::Key& other_key) {
	unsigned which_bucket = longest_matching_prefix(this->key, other_key);
	BucketEntry entry;
	entry.key = other_key;
	entry.address = other_address;
	entry.lastSeen = this->epoch;
	add_to_bucket(which_bucket, entry);
}
