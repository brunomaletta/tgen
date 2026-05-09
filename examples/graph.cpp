#include "../single_include/tgen.h"

using namespace tgen;

int main() {
	register_gen();

	std::cout << (!K(4) + K(2) + C(3)).add_1();
	std::cout << std::endl;

	std::cout << graph(4, 4)
					 .gen()
					 .glue(C(3), {0, 1})
					 .link(K(1), 2, 0)
					 .add_1()
					 .print_nm()
					 .set_edge_weights(str("ab{0,2}a").gen_list(7).to_std());
	std::cout << std::endl;

	std::cout << graph(10, 12).get_connected();
	std::cout << std::endl;

	std::cout << graph(10, 15, true).get_acyclic();
}