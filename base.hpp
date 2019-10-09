#ifndef _BASE_H
#define _BASE_H

#include "application.hpp"
#include "message.hpp"
#include "time.hpp"
#include <map>
#include <iostream>

namespace dhtsim {
template <typename A> class BaseApplication : public Application<A> {
	using CallbackFunction = std::function<void(Message<A>)>;
private:

	/* This section is for variables that configure this
	 * application's network behavior. */

	unsigned int inqueueLimit = 1024;
	unsigned int outqueueLimit = 1024;



	/**
	 * The number of retry messages this application is willing to
	 * store.
	 */
	int retryBufferLimit = 1024;

	/**
	 * The number of ticks to wait before re-sending for the first
	 * time.
	 */
	unsigned long timeout = 20;

	/** The base factor of exponential backoff for retrying. */
	int backoffFactor = 2;

	/**
	 * A type that represents a message that has been sent.
	 */
	struct SentMessage {
		/** The message that was sent. */
		Message<A> message;

		/** The function to be called when the response arrives. */
		CallbackFunction callback;

		/** The time at which this message was last sent. */
		Time timeSent;

		/** The time at which this message is scheduled to be sent next. */
		Time nextSend;

		/** The number of times we have retried. */
		unsigned long retries;

		SentMessage() = default;
                SentMessage(Message<A> m, CallbackFunction cbf, Time time,
                            unsigned long timeout)
			: message(m), callback(cbf), timeSent(time),
			  nextSend(time + timeout), retries(0) {}

		/**
		 * Record a retry.
		 * @param time The time of retry.
		 * @param backoffFactor How many times longer will the next retry interval be?
		 */
		void retry(Time time, int backoffFactor) {
			Time diff = this->nextSend - this->timeSent;
			this->timeSent = time;
			this->nextSend = this->timeSent + diff * backoffFactor;
			this->retries++;
		}

		void resolve() {this->callback(this->message);}
	};

	/**
	 * A map of message tags to "sent message" records. When a
	 * message is received and its tag matches one of the keys,
	 * the callback function is called with the message as its
	 * parameter.
	 */
	std::map<unsigned long, SentMessage> callbacks;

protected:
	/** The current network's time. */
	Time epoch;

	std::queue<Message<A>> inqueue, outqueue;
	void queueIn(Message<A> m);
	void queueOut(Message<A> m);
	
public:
	virtual void recv(Message<A> m) { this->queueIn(m); }
	virtual std::optional<Message<A>> unqueueOut();
	virtual void handleMessage(Message<A> m);
	virtual void tick(Time time);
	virtual void send(Message<A> m, std::optional<CallbackFunction> callback = std::nullopt);
};

template <typename A> std::optional<Message<A>> BaseApplication<A>::unqueueOut() {
	if (this->outqueue.empty()) {
		return {};
	}

	auto message = this->outqueue.front();
	this->outqueue.pop();
	
	return message;
}

template <typename A> void BaseApplication<A>::queueIn(Message<A> m) {
	if (this->inqueue.size() < this->inqueueLimit) {
		this->inqueue.push(m);
	} else {
		std::clog << "[" << this->getAddress() << "]"
		          << " Input queue full, dropping packet."
		          << std::endl;
	}
}

template <typename A> void BaseApplication<A>::queueOut(Message<A> m) {
	if (this->outqueue.size() < this->outqueueLimit) {
		this->outqueue.push(m);
	} else {
		std::clog << "[" << this->getAddress() << "]"
		          << " Output queue full, dropping packet."
		          << std::endl;
	}
}

template <typename A> void BaseApplication<A>::tick(Time time) {
	// Update our record of the time.
	this->epoch = time;

	// handle inbound messages
	while (!this->inqueue.empty()) {
		auto message = this->inqueue.front();
		std::clog << this->getAddress()
		          << " got a message from "
		          << message.originator
		          << " tagged " << message.tag << std::endl;
		this->inqueue.pop();

		this->handleMessage(message);
	}

	// Check for messages whose responses are overdue
	auto it = this->callbacks.begin();
	for ( ; it != this->callbacks.end(); it++) {
		auto& [idx, record] = *it;
		// Is the message overdue for re-sending?
		if (this->epoch >= record.nextSend) {
			// Send the message again, but this time
			// without a callback!.
			this->send(record.message);
			// Let the record know about the retry.
			record.retry(time, this->backoffFactor);
		}
	}
	
}

template <typename A> void BaseApplication<A>::send(Message<A> m, std::optional<CallbackFunction> callback) {
	if (callback.has_value()) {
		SentMessage sentmsg(m, *callback, this->epoch, this->timeout);
		this->callbacks[m.tag] = sentmsg;
	}

	this->queueOut(m);
}

template <typename A> void BaseApplication<A>::handleMessage(Message<A> m) {
	auto tag = m.tag;
	auto it = this->callbacks.find(tag);
	if (it != this->callbacks.end()) {
		auto [tag, sentrecord] = *it;
		sentrecord.resolve();
		this->callbacks.erase(it);
	}
}

}

#endif
