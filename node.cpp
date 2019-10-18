#include "application.hpp"
#include "node.hpp"

#include <openssl/sha.h>

using namespace dhtsim;

Node::Node() {
	// Generate a random key with SHA1
	unsigned long randval = this->randomTag();
	SHA1((unsigned char *)&randval, sizeof(unsigned long), this->key.key);

	this->buckets.reserve(Node::KEY_LEN);
}

static unsigned longest_matching_prefix(const Node::Key& k1, const Node::Key& k2) {
	unsigned i;
	for (i = 0; i < Node::KEY_LEN; i++) {
		if (k1.key[i] != k2.key[i]) {
			break;
		}
	}
	return i;
}

void Node::add_to_bucket(unsigned bucket_index, BucketEntry entry) {
	(void)bucket_index, (void)entry;
	/** NOT IMPLEMENTED */
}

void Node::observe(uint32_t other_address, const Node::Key& other_key) {
	unsigned which_bucket = longest_matching_prefix(this->key, other_key);
	BucketEntry entry;
	entry.key = other_key;
	entry.address = other_address;
	entry.lastSeen = this->epoch;
	add_to_bucket(which_bucket, entry);
}
