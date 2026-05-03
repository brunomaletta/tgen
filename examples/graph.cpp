#include "../single_include/tgen.h"

using namespace tgen;

int main() {
	register_gen();

	std::cout << (!K(4) + K(2) + C(3)).add_1().print_nm();
	std::cout << std::endl;

	auto G = C(3).glue(C(3), {0});
	std::cout << G.shuffle_except({0});
}