#ifndef DHTSIM_MESSAGE_H
#define DHTSIM_MESSAGE_H

#include <random>
#include <vector>
#include <sstream>

#include <nop/structure.h>
#include <nop/serializer.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>


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
};


template <typename T, typename A> static void writeToMessage(T msg_data, Message<A>& m) {
	nop::Serializer<nop::StreamWriter<std::stringstream>> serializer;
	serializer.Write(msg_data);
	const std::string data = serializer.writer().take().str();
	m.data.resize(data.length());
	std::copy(data.begin(), data.end(), m.data.begin());
}
template <typename T, typename A> static void readFromMessage(T& msg_data, const Message<A>& m) {
	std::string data(m.data.begin(), m.data.end());
	std::stringstream ss;
	ss.str(data);
	nop::Deserializer<nop::StreamReader<std::stringstream>> deserializer{std::move(ss)};
	deserializer.Read(&msg_data);
}

}
#endif
