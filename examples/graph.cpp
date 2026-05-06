#include "../single_include/tgen.h"

using namespace tgen;

int main() {
	register_gen();

	std::cout << (!K(4) + K(2) + C(3)).add_1().print_nm();
	std::cout << std::endl;

	std::vector<std::string> weights = {"1", "2", "3", "4", "5", "6", "7"};
	std::cout << graph(4, 4)
					 .gen()
					 .glue(C(3), {0, 1})
					 .link(K(1), 2, 0)
					 .add_1()
					 .print_nm()
					 .set_edge_weights(weights);
}