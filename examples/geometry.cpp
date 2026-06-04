#include "../single_include/tgen.h"

using namespace tgen;

int main(int argc, char **argv) {
	tgen::register_gen(9);

	int n = 200;
	long long max_coord = 10 * n;
	std::cout << print(geometry::random_simple_polygon(n, 0, max_coord), '\n')
			  << std::endl;

	return 0;
}
