#ifndef _PINGONLY_H
#define _PINGONLY_H

#include "application.hpp"
#include "base.hpp"
#include "message.hpp"

#include <iostream>
#include <optional>

namespace dhtsim {


enum PingMessageType {
	PM_PING, PM_PONG,
};

template <typename A> class PingOnlyApplication : public BaseApplication<A> {
	using CallbackFunction = std::function<void(Message<A>)>;

public:
	PingOnlyApplication(){};
	void ping(A other, std::optional<CallbackFunction> callback = std::nullopt);
	virtual void send(Message<A> m, std::optional<CallbackFunction> callback = std::nullopt);
	virtual void handleMessage(Message<A> m);
	virtual ~PingOnlyApplication(){};

};

template <typename A> void PingOnlyApplication<A>::send(Message<A> m, std::optional<CallbackFunction> callback) {
	BaseApplication<A>::send(m, callback);
}

template <typename A> void PingOnlyApplication<A>::ping(A other, std::optional<CallbackFunction> callback) {
	auto tag = this->randomTag();
	std::vector<unsigned char> data;
	Message<A> m(PM_PING, this->getAddress(), other, tag, data);
	this->send(m, callback);
}


template <typename A> void PingOnlyApplication<A>::handleMessage(Message<A> m) {
	BaseApplication<A>::handleMessage(m);
	switch (m.type) {
	case PM_PING:
		// Received a ping! Send a message back.
		m.type = PM_PONG;
		m.destination = m.originator;
		m.originator = this->getAddress();
		this->send(m);
		break;
	case PM_PONG:
		// Received a pong.
		break;
	default:
		std::clog << "received an unknown message!" << std::endl;
	}
}

}

#endif
