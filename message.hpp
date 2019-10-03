#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <random>
#include <vector>

namespace dhtsim {

template <typename A> class Message {
private:
	
public:
	unsigned int type;
	A originator, destination;
	unsigned long tag;
	unsigned int hops;

	std::vector<unsigned char> data;

        Message() {}

        Message(unsigned int type, A originator, A destination,
	        unsigned long tag, std::vector<unsigned char> data)
            : type(type), originator(originator), destination(destination),
              tag(tag), hops(0), data(data) {}

	Message(const Message& other) = default;
};

}

#endif
