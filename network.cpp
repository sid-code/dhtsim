#include "network.hpp"
#include "application.hpp"
#include "random.h"
#include <iostream>
#include <map>
#include <climits>
#include <functional>
#include <memory>


using namespace dhtsim;

template <typename A> CentralizedNetwork<A>::CentralizedNetwork(unsigned int linkLimit) {
	this->linkLimit = linkLimit;
	this->epoch = 0;
}
template <typename A> A CentralizedNetwork<A>::getNewAddress() {
	A attempt;
	const uint32_t max_tries = 1000;
	uint32_t tries;

	for (tries = 0; tries < max_tries; tries++) {
		attempt = global_rng.Number<A>(std::pair(0, std::numeric_limits<A>::max()));
	        auto it = this->inhabitants.find(attempt);
	        if (it == this->inhabitants.end()) {
		        return attempt;
	        }
        }

	return 0;
}

template <typename A> A CentralizedNetwork<A>::add(std::shared_ptr<Application<A>> app) {
	A address = this->getNewAddress();
	if (address == 0) return address;

	this->inhabitants[address] = app;
	app->setAddress(address);
	app->tick(this->epoch);
	return address;
}

template <typename A> void CentralizedNetwork<A>::remove(std::shared_ptr<Application<A>> app) {
	this->inhabitants.erase(app->getAddress());
}

template <typename A> void CentralizedNetwork<A>::tick() {
	// Keeps of the total bytes transferred per link
	unsigned long totalLinkTransfer;

	// Keeps track of the total bytes transferred during this tick.
	unsigned long totalTransferred = 0;

	// In the following for loop, app is a shared_ptr to the
	// application
	for (const auto [address, app] : this->inhabitants) {
		totalLinkTransfer = 0;

		// handle inbound messages
		app->tick(this->epoch);

		// handle outbound messages
		std::optional<Message<A>> outboundMessage = app->unqueueOut();
		while (outboundMessage.has_value()) {
			auto size = outboundMessage->data.size();
			totalLinkTransfer += size;
			if (size > this->linkLimit) {
				std::cerr << "DROPPED message of length " << size << std::endl;
				break;
			}
			if (totalLinkTransfer > this->linkLimit) {
				std::cerr << "RETRYING message of length " << size << std::endl;
				app->send(*outboundMessage);
				break;
			}

			this->passAlongMessage(*outboundMessage);

		        outboundMessage = app->unqueueOut();
		}

		totalTransferred += totalLinkTransfer;

	}

	std::cout << "[E] T " << this->epoch << " " << totalTransferred << std::endl;

	this->epoch++;
}

template <typename A> void CentralizedNetwork<A>::passAlongMessage(Message<A> message) {
	message.hops++;
	A dest = message.destination;
	auto it = this->inhabitants.find(dest);
	if (it != this->inhabitants.end()) {
		auto [address, app] = *it;
		app->recv(message);
	}

	// The destination doesn't exist on this network, so just drop
	// the message.
}

template class dhtsim::CentralizedNetwork<uint32_t>;
