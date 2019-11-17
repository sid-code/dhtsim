#include <iostream>
#include <chrono>
#include <thread>

#include "network.hpp"
#include "application.hpp"
#include "base.hpp"
#include "kademlia/kademlia.hpp"


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
	while (!done) {
		net.tick();
		tick++;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		if (tick % 20 == 0) {
			nodes[2]->die();
			nodes[1]->findNodes(
				nodes[2]->getKey(),
				KademliaNode::FindNodesCallbackSet::onSuccess(
					[&done](std::vector<KademliaNode::BucketEntry> results) {
						std::cout << results.size()
						          << " results."
						          << std::endl;
						for (const auto& entry : results) {
							std::cout << entry.address << " "
							          << entry.key << std::endl;

						}

					}));
			
		}
	}
}
