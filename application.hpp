#ifndef DHTSIM_APPLICATION_H
#define DHTSIM_APPLICATION_H

#include "message.hpp"
#include "time.hpp"

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
	template <typename SuccessParameter = Message<A>,
	          typename FailureParameter = SuccessParameter>
	class CallbackSet
	{
	public:
		using SuccessFn = std::function<void(SuccessParameter)>;
		using FailureFn = std::function<void(FailureParameter)>;
		// Main constructors
		CallbackSet() = default;
		CallbackSet(std::optional<SuccessFn> successFn,
		            std::optional<FailureFn> failureFn)
			: successFn(successFn),
			  failureFn(failureFn) {};
		CallbackSet(const CallbackSet& other) = default;
		~CallbackSet() = default;

		// Static constructors
		static CallbackSet onSuccess(SuccessFn fn) {
			return CallbackSet(fn, std::nullopt);
		}
		static CallbackSet onFailure(FailureFn fn) {
			return CallbackSet(std::nullopt, fn);
		}

		void success(SuccessParameter m) const {
			if (this->successFn.has_value()) {
				(*this->successFn)(m);
			}
		}
		void failure(FailureParameter m) const {
			if (this->failureFn.has_value()) {
				(*this->failureFn)(m);
			}
		}

		bool isEmpty() const {
			return this->successFn == std::nullopt &&
				this->failureFn == std::nullopt;
		}
	private:
		std::optional<SuccessFn> successFn = std::nullopt;
		std::optional<FailureFn> failureFn = std::nullopt;
	};
	Application();
	virtual ~Application() = default;

	/**
	 * Send a message with an optional callback for the
	 * response. This can be called by the application or the
	 * network.
	 */
	virtual void send(Message<A> m,
	                  CallbackSet<> callback = CallbackSet<>(),
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
