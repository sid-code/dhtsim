
#include "network.hpp"
#include "application.hpp"
#include "base.hpp"
#include "pingonly.hpp"


using namespace dhtsim;

int main() {
	std::cout << "[startup]" << std::endl;
	CentralizedNetwork<uint32_t> net(10);
	unsigned int tick = 0;

	unsigned int nodes = 2, i;

	std::vector<std::shared_ptr<PingOnlyApplication<uint32_t>>> apps;
	for (int i = 0; i < nodes; i++) {
		auto app = std::make_shared<PingOnlyApplication<uint32_t>>();
		apps.push_back(app);
		net.add(app);
		std::clog << "App " << i << " address: " << app->getAddress()
		          << std::endl;

	}

	while (true) {
		net.tick();
		if (tick == 10) {
			auto addr = apps[1]->getAddress();
			apps[0]->ping(
				addr,
				[](auto m){
					std::clog<<"GOT BACK PONG"<<std::endl;
				});
		}
		tick++;
	}
}
