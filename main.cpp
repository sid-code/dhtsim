#include <iostream>
#include <chrono>
#include <thread>

#include "network.hpp"
#include "application.hpp"
#include "base.hpp"
#include "kademlia/kademlia.hpp"
#include "kademlia/message_structs.hpp"


using namespace dhtsim;


int main() {
	std::cout << "[startup]" << std::endl;
	CentralizedNetwork<uint32_t> net(1024000);
	unsigned int tick = 0;

	unsigned int n_nodes = 200, i;

	std::vector<std::shared_ptr<KademliaNode>> nodes;
	for (i = 0; i < n_nodes; i++) {
		auto node = std::make_shared<KademliaNode>();
		nodes.push_back(node);
		net.add(node);
		std::clog << "Node " << i << " address: " << node->getAddress()
		          << std::endl;
		std::clog << "Node " << i << " key: " << node->getKey()
		          << std::endl;

	}

	for (i = 1; i < n_nodes; i++) {
		nodes[i]->ping(nodes[0]->getAddress(),
		               KademliaNode::PingCallbackSet());
	}


	bool done = false;
	std::vector<KademliaNode::Key> keys(n_nodes);
	std::vector<std::vector<unsigned char>> things_to_store(n_nodes);
        for (i = 0; i < n_nodes; i++) {
	        things_to_store[i].push_back('a' + (unsigned char)(i%26));
        }

        auto cbs = KademliaNode::GetCallbackSet::onSuccess(
				              [](std::vector<unsigned char> value) {
					              std::string s(value.begin(), value.end());
					              std::cout << "Value: " << s << std::endl;
				              }) + KademliaNode::GetCallbackSet::onFailure(
				              [](std::vector<unsigned char> value) {
					              (void)value;
						      std::cout << "we failed." << std::endl;
				              });
	while (!done) {
		net.tick();
		tick++;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		if (tick == 10) {
			for (i = 0; i < n_nodes; i++) {
				keys[i] = nodes[0]->put(things_to_store[i]);
			}
		}

		if (tick == 100) {
			nodes[0]->get(keys[30], cbs);
			nodes[1]->get(keys[20], cbs);
			nodes[2]->get(keys[10], cbs);
		}

	}
}
