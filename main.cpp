#include <iostream>
#include <chrono>
#include <thread>

#include "network.hpp"
#include "application.hpp"
#include "base.hpp"
#include "kademlia/kademlia.hpp"
#include "kademlia/message_structs.hpp"


using namespace dhtsim;

using MyDHTNode = DHTNode<KademliaKey, std::vector<unsigned char>>;

void experiment(const Time start_tick,
		const Time& tick,
                std::vector<std::shared_ptr<MyDHTNode>>& nodes,
                unsigned int n_values,
                std::vector<KademliaKey>& keys,
                unsigned int& waiting,
                Time& total_time,
		bool store) {
	Time find_start_time = 100;
	if (!store) find_start_time = 0;
	unsigned int i;

	auto cbs = KademliaNode::GetCallbackSet::onSuccess(
		[start_tick, &tick, find_start_time, n_values, &waiting, &total_time](std::vector<unsigned char> value) {
			waiting--;
			total_time += tick - find_start_time - start_tick;
			std::string s(value.begin(), value.end());
			std::cout << "Value: " << s << " at "
			          << tick << " (waiting for "
			          << waiting << ")" << " took " << tick - (find_start_time + start_tick)
			          << std::endl;
			if (waiting == 0) {
				std::cout << "Average time "
				          << (double) total_time / n_values
				          << std::endl;
			}
		}) +
		KademliaNode::GetCallbackSet::onFailure(
			[](std::vector<unsigned char> value) {
				(void)value;
				std::cout << "we failed." << std::endl;
			});

	if (tick == start_tick) {
		total_time = 0;
		for (i = 0; i < n_values; i++) {
			std::string x = std::to_string(i);
			std::vector<unsigned char> d(x.begin(), x.end());
			if (store) {
				keys[i] = nodes[i]->put(d);
			} else {
				keys[i] = nodes[i]->getKey(d);
			}
			std::cout << "key " << i << " " << keys[i] << std::endl;
		}
	}

	if (tick == find_start_time + start_tick) {
		for (i = 0; i < n_values; i++) {
			waiting++;
			nodes[0]->get(keys[i], cbs);
		}
	}
}


int main() {
	std::cout << "[startup]" << std::endl;
	CentralizedNetwork<uint32_t> net(1024000);
	Time tick = 0;

	unsigned int n_nodes = 1500, i;

        std::vector<std::shared_ptr<MyDHTNode>> nodes;
        auto node_zero = std::make_shared<KademliaNode>();
        net.add(node_zero);
        nodes.push_back(node_zero);
        auto node_zero_address = node_zero->getAddress();

	for (i = 1; i < n_nodes; i++) {
		auto node = std::make_shared<KademliaNode>();
		net.add(node);
		node->ping(node_zero_address, KademliaNode::PingCallbackSet());
		nodes.push_back(node);
		std::clog << "Node " << i << " address: " << node->getAddress()
		          << std::endl;
		std::clog << "Node " << i << " key: " << node->getKey()
		          << std::endl;

	}


	bool done = false;

	unsigned int waiting = 0;
	unsigned int n_values = 30;
	std::vector<KademliaNode::Key> keys(n_values);
	Time total_time;
	while (!done) {
		net.tick();
		tick++;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		experiment(10, tick, nodes, n_values, keys, waiting, total_time, true);
		experiment(300, tick, nodes, n_values, keys, waiting, total_time, false);
		// Some nodes now die.
		if (tick == 350) {
			for (i = 0; i < n_nodes; i++) {
				// we don't want to kill node 0
				if (i % 3 != 0) {
					nodes[i]->die();
				}
			}
		}
		
		experiment(400, tick, nodes, n_values, keys, waiting, total_time, false);
		experiment(500, tick, nodes, n_values, keys, waiting, total_time, false);
	}
}
