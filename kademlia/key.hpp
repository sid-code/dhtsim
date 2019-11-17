#ifndef DHTSIM_KADEMLIA_KEY_HPP
#define DHTSIM_KADEMLIA_KEY_HPP

#include <cstring>
#include <iostream>

#include <openssl/sha.h>
#include <nop/structure.h>

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

        KademliaKey() {}

        NOP_STRUCTURE(KademliaKey, key);
};
}

#endif
