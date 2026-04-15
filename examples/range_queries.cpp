#include "../single_include/tgen.h"

#include <iostream>

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n", 10);
	int q = tgen::opt<int>("q", 10);

	std::cout << n << " " << q << std::endl;
	std::cout << tgen::pair<int>(1, n).leq().unique().gen_seq(q).separator('\n')
			  << std::endl;
}