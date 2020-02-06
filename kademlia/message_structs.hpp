#ifndef DHTSIM_KADEMLIA_MESSAGE_STRUCTS_HPP
#define DHTSIM_KADEMLIA_MESSAGE_STRUCTS_HPP
#include "key.hpp"
#include "bucket.hpp"

#include <vector>

#include <nop/structure.h>

namespace dhtsim {
/** Ping message data structure */
struct PingMessage {
        bool ping_or_pong;
	KademliaKey sender;
	PingMessage(bool ping) : ping_or_pong(ping) {};
	bool is_ping() { return ping_or_pong; }
        static PingMessage ping() {
		return PingMessage(true);
	}
	static PingMessage pong() {
		return PingMessage(false);
	}
	PingMessage() = default;

	NOP_STRUCTURE(PingMessage, ping_or_pong, sender);
};

/**
 * Find nodes message data structure.
 * This is also used for find_value.
 */
struct FindNodesMessage {
	KademliaKey sender;

	bool request; // is this a request or response?
	bool find_value = false; // is this a find_value request?

	// The key we're searching for. This can either be the key of
	// a node we want or the key of a value we want.
	KademliaKey target;

        // For finding nodes:
	uint32_t num_found;
	std::vector<BucketEntry> nearest;
	// For finding values:
	bool value_found = false; // was a value found?
	std::vector<unsigned char> value; // the value that was found

	FindNodesMessage() = default;

	NOP_STRUCTURE(FindNodesMessage, sender, request, find_value, target,
	              num_found, nearest, value_found, value);

};

/**
 * Store message data structure.
 */
struct StoreMessage {
	bool request;
	KademliaKey sender;

	std::vector<unsigned char> value; // the value

	NOP_STRUCTURE(StoreMessage, request, sender, value);
};
}
#endif
