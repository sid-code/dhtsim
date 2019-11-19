#ifndef DHTSIM_DHTNODE_H
#define DHTSIM_DHTNODE_H

#include "callback.hpp"

namespace dhtsim {
// The Distributed Hash Table node interface.
template <typename KeyType, typename ValueType> class DHTNode {
public:
	using GetCallbackSet = CallbackSet<ValueType, ValueType>;
	virtual KeyType put(const ValueType& v) = 0;
	virtual void get(const KeyType& k, GetCallbackSet callback) = 0;
	virtual KeyType getKey(const ValueType& v) = 0;
	virtual void die() = 0; // BaseApplication provides this one
};
}

#endif
