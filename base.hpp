#ifndef DHTSIM_BASE_H
#define DHTSIM_BASE_H

#include "application.hpp"
#include "message.hpp"
#include "time.hpp"
#include "callback.hpp"

#include <map>
#include <iostream>
#include <optional>
#include <functional>

namespace dhtsim {
template <typename A> class BaseApplication : public Application<A> {
public:
	using SendCallbackSet = CallbackSet<Message<A>, Message<A>>;

	// Contructor
        BaseApplication() : epoch(0), 
                            inqueue(), outqueue(),
                            callbacks() {}

        virtual void recv(Message<A> m) { this->queueIn(m); }
	virtual std::optional<Message<A>> unqueueOut();
	virtual void handleMessage(const Message<A>& m);
	virtual void tick(Time time);
        /**
         * Send a message.
         * @param {m} The message to send.
         * @param {callback} The function to execute with the response.
         * @param {timeout} The timeout before message is considered "lost"
         * @param {maxRetries} how many times to retry? (exponential backoff, starting with timeout)
         */
	virtual void send(Message<A> m,
	                  SendCallbackSet callback = SendCallbackSet(),
                          unsigned int maxRetries = 16,
                          unsigned long timeout = 0);

        virtual void die() { this->dead = true; }
        virtual bool isDead() { return this->dead; }

protected:
        /** The current network's time. */
        Time epoch;

	/** Is this node dead? */
	bool dead = false;

	std::queue<Message<A>> inqueue, outqueue;
	void queueIn(Message<A> m);
	void queueOut(Message<A> m);
private:

	/* This section is for variables that configure this
	 * application's network behavior. */

	const unsigned int inqueueLimit = 1 << 15;
	const unsigned int outqueueLimit = 1 << 15;

	/**
	 * The number of retry messages this application is willing to
	 * store.
	 */
	const int retryBufferLimit = 1024;

	/**
	 * The number of ticks to wait before re-sending for the first
	 * time.
	 */
	const unsigned long defaultTimeout = 20;

	/** The base factor of exponential backoff for retrying. */
	const int backoffFactor = 2;

	/**
	 * A type that represents a message that has been sent.
	 */
	struct SentMessage {
		/** The message that was sent. */
		Message<A> message;

		/** The function to be called when the response arrives. */
		SendCallbackSet callback;

		/** The time at which this message was last sent. */
		Time timeSent;

		/** The time at which this message is scheduled to be sent next. */
		Time nextSend = std::numeric_limits<Time>::max();

		/** The number of times we have retried. */
		unsigned long retries;

		/** How many times to retry before giving up? */
		unsigned long maxRetries;

		SentMessage() = default;
                SentMessage(Message<A> m, SendCallbackSet cbf, Time time,
                            unsigned long timeout, unsigned int maxRetries)
			: message(m), callback(cbf), timeSent(time),
			  nextSend(time + timeout), retries(0), maxRetries(maxRetries) {}

		bool needsRetry() {
			return this->retries < this->maxRetries;
		}
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

		void success(Message<A> m) {
			this->callback.success(m);
		}

		void failure() {
			this->callback.failure(this->message);
		}
	};

	void attemptRetry(SentMessage& record);

	/**
	 * A map of message tags to "sent message" records. When a
	 * message is received and its tag matches one of the keys,
	 * the callback function is called with the message as its
	 * parameter.
	 */
	std::map<unsigned long, SentMessage> callbacks;

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

template <typename A> void BaseApplication<A>::attemptRetry(SentMessage& record) {
	// Send the message again, but this time
	// without a callback!.
	this->send(record.message);
	// Let the record know about the retry.
	record.retry(this->epoch, this->backoffFactor);
}

template <typename A> void BaseApplication<A>::tick(Time time) {
	// Dead nodes send no messages.
	if (this->dead) return;

	// Update our record of the time.
	this->epoch = time;

	// handle inbound messages
	while (!this->inqueue.empty()) {
		auto message = this->inqueue.front();
		// std::clog << this->getAddress()
		//           << " got a message from "
		//           << message.originator
		//           << " tagged " << message.tag << std::endl;
		this->inqueue.pop();

		this->handleMessage(message);
	}

	// Check for messages whose responses are overdue
	auto it = this->callbacks.begin();
	for ( ; it != this->callbacks.end(); ) {
		auto& [idx, record] = *it;
		// Is the message overdue for re-sending?
		if (this->epoch >= record.nextSend) {
			if (record.needsRetry()) {
				this->attemptRetry(record);
			} else {
				record.failure();
				it = this->callbacks.erase(it);
				continue;
			}
		}

		it++;
	}
	
}

template <typename A> void BaseApplication<A>::send(
	Message<A> m,
	BaseApplication<A>::SendCallbackSet callback,
	unsigned int maxRetries,
	unsigned long timeout) {

	// Dead nodes send no messages, but we'll be nice and call the
	// failure function. It doesn't really make much sense, but
	// this becomes a lot harder otherwise.
	if (this->dead) {
		callback.failure(m);
		return;
	}

	if (m.tag == 0) {
		m.tag = this->randomTag();
	}

	if (timeout == 0) {
		// Use default value
		timeout = this->defaultTimeout;
	}

	if (!callback.empty()) {
		SentMessage sentmsg(m, callback, this->epoch, timeout, maxRetries);
		this->callbacks[m.tag] = sentmsg;
	}

	this->queueOut(m);
}

template <typename A> void BaseApplication<A>::handleMessage(const Message<A>& m) {
	auto tag = m.tag;
	auto it = this->callbacks.find(tag);
	if (it != this->callbacks.end()) {
		auto [tag, sentrecord] = *it;
		sentrecord.success(m);
		this->callbacks.erase(it);
	}
}

}

#endif
