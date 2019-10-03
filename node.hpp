#ifndef _NODE_H
#define _NODE_H

#include "application.hpp"
#include "base.hpp"

#include <vector>

namespace dhtsim {

uint32_t keyDistance(uint32_t x, uint32_t y) {
	return x ^ y;
}

/**
 * A distributed hash table node in a network where addresses are 32
 * bits.
 */
class Node : public BaseApplication<uint32_t> {

private:
	/** Is this node part of the network yet? */
	bool joined;

	/** What is this node's key? */
	uint64_t key;
	/** How deep is this node in the Huffman tree. */
	uint32_t key_depth;

	/** A list of peers that this node know about. */
	std::vector<uint32_t> peers;

public:
	Node() : joined(false), key(0), key_depth(0) {}
};

} // namespace dhtsim
#endif
