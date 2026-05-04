#include "../single_include/tgen.h"

using namespace tgen;

int main() {
	register_gen();

	std::cout << (!K(4) + K(2) + C(3)).add_1().print_nm();
	std::cout << std::endl;

	auto G = C(3).glue(C(3), {0});
	std::cout << G.shuffle_except({0});

	std::cout << std::endl;

	std::vector<int> weights = {1, 2, 3, 4, 5, 6, 7};
	std::cout << C(4).glue(C(3), {0, 1})
					 .glue(P(2), {{2, 0}})
					 .add_1()
					 .print_nm()
					 .set_edge_weights(weights);
}