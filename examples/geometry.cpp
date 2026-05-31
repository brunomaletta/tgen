#include "../single_include/tgen.h"

using namespace tgen;

int main(int argc, char **argv) {
	tgen::register_gen(2);

	int n = 5000;
	std::cout << print(geometry::general_position(n, 0, 10 * n), '\n')
			  << std::endl;

	return 0;
}