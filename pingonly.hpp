#ifndef DHTSIM_PINGONLY_H
#define DHTSIM_PINGONLY_H

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
public:
	using MessageCallbackSet = typename Application<A>::template CallbackSet<>;
	PingOnlyApplication(){};
	void ping(A other, MessageCallbackSet callback = MessageCallbackSet());
	virtual void handleMessage(const Message<A>& m);
	virtual ~PingOnlyApplication(){};

};

template <typename A> void PingOnlyApplication<A>::ping(A other, MessageCallbackSet callback) {
	auto tag = this->randomTag();
	std::vector<unsigned char> data;
	Message<A> m(PM_PING, this->getAddress(), other, tag, data);
	this->send(m, callback);
}


template <typename A> void PingOnlyApplication<A>::handleMessage(const Message<A>& m) {
	BaseApplication<A>::handleMessage(m);
	auto resp = m;
	switch (m.type) {
	case PM_PING:
		// Rhandle_messageg! Send a message back.
		resp.type = PM_PONG;
		resp.destination = m.originator;
		resp.originator = this->getAddress();
		this->send(resp);
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
