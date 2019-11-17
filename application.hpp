#ifndef DHTSIM_APPLICATION_H
#define DHTSIM_APPLICATION_H

#include "message.hpp"
#include "time.hpp"
#include "callback.hpp"

#include <queue>
#include <functional>
#include <random>
#include <map>
#include <optional>

namespace dhtsim {

/**
 * The following class represents an abstract application on a network
 * that has addresses of type A. A distributed hash table node will be
 * an instance of a subclass of this class.
 */
template <typename A> class Application {
private:
	// RNG stuff
	std::random_device dev;
	std::mt19937 rng;
	std::uniform_int_distribution<std::mt19937::result_type> dist;
protected:
	A address = 0;
	unsigned long randomTag();
public:

	template <typename KeyType, typename ValueType> class DHTNode {
	public:
		using GetCallbackSet = CallbackSet<ValueType, ValueType>;
		virtual KeyType put(const ValueType& v) = 0;
		virtual void get(const KeyType& k, GetCallbackSet callback) = 0;
		virtual KeyType getKey(const ValueType& v) = 0;
	};

        Application();
	virtual ~Application() = default;

	/**
	 * Send a message with an optional callback for the
	 * response. This can be called by the application or the
	 * network.
	 */
	virtual void send(Message<A> m,
	                  CallbackSet<Message<A>> callback = CallbackSet<Message<A>>(),
	                  unsigned int maxRetries = 16,
	                  unsigned long timeout = 0) = 0;

	/** Receive a message. This is called by the network. */
        virtual void recv(Message<A> m) = 0;

	/**
	 * Somehow (or not) produce a message to be sent. This is
	 * called by the network.
	 */
	virtual std::optional<Message<A>> unqueueOut() = 0;

	/**
	 * Entry point of this application during the event loop.
	 * @param time The current time, measured as network ticks
	 *             since startup.
	 */
	virtual void tick(Time time) = 0;

	/**
	 * A setter to be possibly overriden, as changing addresses
	 * should be handled with care.
	 */
	void setAddress(A newAddress) { this->address = newAddress; }
        A getAddress() { return this->address; }

	/**
	 * Kill this node. It will no longer do anything.
	 */
	virtual void die() = 0;
};

template <typename A> Application<A>::Application() {
	this->rng = std::mt19937(dev());
	this->dist = std::uniform_int_distribution<std::mt19937::result_type>(
		0, std::numeric_limits<unsigned long>::max());
}

template <typename A> unsigned long Application<A>::randomTag() {
	return this->dist(this->rng);
}

}

#endif
