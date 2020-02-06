#ifndef DHTSIM_NETWORK_H
#define DHTSIM_NETWORK_H

#include "application.hpp"
#include "time.hpp"

#include <map>
#include <cstdint>
#include <random>
#include <memory>

namespace dhtsim {
/**
 * A simple abstract network. It is parametrized by a numeric type A,
 * which will be the address space. Applications will live in this
 * network.
 */
template <typename A> class CentralizedNetwork {
private:
	std::map<A, std::shared_ptr<Application<A>>> inhabitants;
        A getNewAddress();
	Time epoch;
public:
	// The bytes-per-tick limit of a single link on this network
	unsigned int linkLimit;
	CentralizedNetwork(unsigned int linkLimit = 1024);
	A add(std::shared_ptr<Application<A>> x);
	void remove(std::shared_ptr<Application<A>> x);
	void tick();
	void passAlongMessage(Message<A> message);
        Time current_epoch() { return this->epoch; };
};


}


#endif
