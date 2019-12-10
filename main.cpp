#include <iostream>
#include <chrono>
#include <thread>

#include "argh.h"

#include "network.hpp"
#include "application.hpp"
#include "base.hpp"
#include "experiment.hpp"
#include "kademlia/kademlia.hpp"
#include "kademlia/message_structs.hpp"



using namespace dhtsim;

KademliaNode::Config global_kademlia_config;

template <typename Node>
class ChurnExperiment : public Experiment<Node> {

public:
	ChurnExperiment(CentralizedNetwork<uint32_t> &net,
	                const std::vector<std::shared_ptr<Application<uint32_t>>> &nodes)
		: Experiment<Node>(net, nodes) {}

	virtual void init() {
		this->stored_data_keys.clear();
		unsigned int i;
		for (i = 0; i < this->nodes.size(); i++) {
			std::string is = std::to_string(i);
			std::vector<unsigned char> d(is.begin(), is.end());
			auto key = this->store(this->nodes[i], d);
			this->stored_data_keys.push_back(key);
		}
		for (i = 0; i < 500; i++) {
			this->net.tick();
		}
	}

	virtual void run() {
		unsigned int max_iterations = 50000;
		unsigned int i;

		for (i = 0; i < max_iterations; i++) {
			this->current_epoch = i;

			// We kill off a random node and replace it
			// with another.
			if (i % 10 == 0) {
				// we do not kill node zero
				auto node_index = global_rng.Size_T(1, this->nodes.size()-1);
				std::cout << "[E] R " << node_index << std::endl;
				auto new_node = this->create();
				this->nodes[node_index]->die();
				this->net.remove(std::static_pointer_cast<Application<uint32_t>>(this->nodes[node_index]));
				this->net.add(new_node);
				this->nodes[node_index] = new_node;
				this->waiting[node_index] = false;
				this->introduce(this->nodes[node_index], this->nodes[0]);
			}


			auto node_index = global_rng.Size_T(0, this->nodes.size()-1);
			auto target_data_index = global_rng.Size_T(0, this->nodes.size()-1);
#ifdef DEBUG
			std::cout << node_index << " wants to find " << target_data_index << std::endl;
#endif
			if (!this->waiting[node_index]) {
				this->waiting[node_index] = true;
				auto key = this->stored_data_keys[target_data_index];
				auto now = this->net.current_epoch();
				this->fetch(this->nodes[node_index], key,
					typename ChurnExperiment::FetchCallbackSet(
						[=](auto d) {(void)d;this->recordFind(node_index, target_data_index, now);},
						[=](auto d) {(void)d;this->recordFail(node_index, target_data_index, now);}));
			} else {
#ifdef DEBUG
				std::clog << node_index << " is waiting." << std::endl;
#endif
			}

			this->net.tick();
		}

	}

};

template<>
std::shared_ptr<KademliaNode> Experiment<KademliaNode>::create() {
	return std::make_shared<KademliaNode>(global_kademlia_config);
}

template <>
void Experiment<KademliaNode>::introduce(const std::shared_ptr<Application<uint32_t>>& n1,
					 const std::shared_ptr<Application<uint32_t>>& n2) {
	auto n1_cast = std::static_pointer_cast<KademliaNode>(n1);
	n1_cast->ping(n2->getAddress(), KademliaNode::PingCallbackSet());
}
template <>
KademliaNode::Key Experiment<KademliaNode>::store(const std::shared_ptr<Application<uint32_t>>& storer,
                                     const std::vector<unsigned char>& data) {
	auto storer_cast = std::static_pointer_cast<KademliaNode>(storer);
	return storer_cast->store(data);
}
template <>
void Experiment<KademliaNode>::fetch(const std::shared_ptr<Application<uint32_t>>& storer,
                                     const KademliaNode::Key& key,
                                     Experiment::FetchCallbackSet cb) {
	auto storer_cast = std::static_pointer_cast<KademliaNode>(storer);
        auto cb_success = [=](FindNodesMessage fm) {
				  if (fm.value_found) {
					  cb.success(fm.value);
				  }
			  };
	auto cb_fail = [=](FindNodesMessage fm) {
		               (void) fm;
			       cb.failure(1);
		       };

	storer_cast->findValue(key, KademliaNode::FindNodesCallbackSet(cb_success, cb_fail));
}


int main(int, char* argv[]) {
	unsigned long link_limit, n_nodes;
	argh::parser cmdl(argv);
	cmdl("k", 10) >> global_kademlia_config.k;
	cmdl("alpha", 3) >> global_kademlia_config.alpha;
	cmdl("mp", 10000) >> global_kademlia_config.maintenance_period;
	cmdl("rp", 1000) >> global_kademlia_config.bucket_refresh_period;

	cmdl("ll", 1<<16) >> link_limit;

	cmdl("nn", 400) >> n_nodes;
	std::clog << "Global network options: " << std::endl
	          << "Link limit: " << link_limit << std::endl
	          << "# nodes...: " << n_nodes << std::endl;
	std::clog << "Kademlia options:" << std::endl
		  << global_kademlia_config << std::endl;

	std::clog << "[startup]" << std::endl;
	CentralizedNetwork<uint32_t> net(link_limit);

	unsigned long i;

	std::vector<std::shared_ptr<Application<uint32_t>>> nodes;
        auto node_zero = std::make_shared<KademliaNode>(global_kademlia_config);
        net.add(node_zero);
        nodes.push_back(node_zero);
        auto node_zero_address = node_zero->getAddress();

	for (i = 1; i < n_nodes; i++) {
		auto node = std::make_shared<KademliaNode>(global_kademlia_config);
		net.add(node);
		node->ping(node_zero_address, KademliaNode::PingCallbackSet());
		nodes.push_back(node);
		std::clog << "Node " << i << " address: " << node->getAddress()
		          << std::endl;
		std::clog << "Node " << i << " key: " << node->getKey()
		          << std::endl;

	}

	// complete the warmup
	for (i = 0; i < 100; i++) {
		net.tick();
	}

	auto exp = ChurnExperiment<KademliaNode>(net, nodes);
	exp.init();
	exp.run();

}
