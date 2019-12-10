#ifndef DHTSIM_EXPERIMENT_HPP
#define DHTSIM_EXPERIMENT_HPP

#include "network.hpp"
#include "callback.hpp"

#include <cstdint>
#include <iostream> //temporary


namespace dhtsim {
template <typename Node>
class Experiment {
public:
	using Key = typename Node::Key;
	using FetchCallbackSet = CallbackSet<std::vector<unsigned char>, int>;
	// there is no way to do that through the generic DHTNode
	// interface.
	Experiment(CentralizedNetwork<uint32_t>& net,
	           const std::vector<std::shared_ptr<Application<uint32_t>>>& nodes) :
		net(net), nodes(nodes), waiting(nodes.size(), 0), current_epoch(0) {}

	virtual void init() = 0;
	virtual void run() = 0;

protected:

	void addNode(std::shared_ptr<Node> n) {
		this->net.add(n);
		this->nodes.push_back(n);
		this->waiting.push_back(false);
	}


	CentralizedNetwork<uint32_t> net;
	std::vector<std::shared_ptr<Application<uint32_t>>> nodes;
	// a bit mask over each node, "is this node waiting for something?"
	std::vector<bool> waiting;

	// the keys data for each node we stored
	std::vector<Key> stored_data_keys;
	unsigned int current_epoch;

	void recordFind(size_t node_index, size_t target_data_index, Time since) {
		this->waiting[node_index] = false;
		std::cout << "[E] S " << node_index << " " << target_data_index
		          << " " << this->net.current_epoch() - since << std::endl;
	}
	void recordFail(size_t node_index, size_t target_data_index, Time since) {
		this->waiting[node_index] = false;
		std::cout << "[E] F " << node_index << " " << target_data_index
		          << " " << this->net.current_epoch() - since << std::endl;
	}

	std::shared_ptr<Node> create();

	void introduce(const std::shared_ptr<Application<uint32_t>>& n1,
		       const std::shared_ptr<Application<uint32_t>>& n2);
	Key store(const std::shared_ptr<Application<uint32_t>>& storer,
	           const std::vector<unsigned char>& data);
	void fetch(const std::shared_ptr<Application<uint32_t>>& fetcher,
	           const Key& data, FetchCallbackSet cb);
	           
	
};
}


#endif
