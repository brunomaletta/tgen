#include "../single_include/tgen.h"

using namespace tgen;

int main(int argc, char **argv) {
	register_gen(argc, argv);

	std::cout << (!K(4) + K(2) + C(3)).add_1();
	std::cout << std::endl;

	std::cout << graph(10, 12).get_connected();
	std::cout << std::endl;

	std::cout << graph(10, 15, true).get_acyclic();
	std::cout << std::endl;

	std::cout << graph(10, 15).get_skewed(100, 2);
	std::cout << std::endl;

	std::cout << tgen::tree(4).gen().set_vertex_weights<std::string>(
					 {"a", "ab", "b", "abc"})
			  << std::endl;

	std::cout << graph(10, 15, true)
					 .add_edges_from(tgen::P(10, true))
					 .get_acyclic()
					 .shuffle_except({0, 9})
			  << std::endl;
}