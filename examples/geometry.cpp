#include "../single_include/tgen.h"

using namespace tgen;

int main(int argc, char **argv) {
	tgen::register_gen(0);

	constexpr bool use_random_polygon = false;

	std::vector<geometry::point<long long>> poly;
	if (use_random_polygon) {
		const int n = 200;
		const long long max_coord = 10 * n;
		poly = geometry::random_simple_polygon(n, 0, max_coord);
	} else {
		const int rows = 10, cols = 10;
		std::vector<geometry::point<long long>> pts;
		pts.reserve(static_cast<size_t>(rows) * cols);
		for (int y = 0; y < rows; ++y)
			for (int x = 0; x < cols; ++x)
				pts.emplace_back(x, y);
		poly = geometry::random_simple_polygon_through_points(pts);
	}

	std::cout << print(poly, '\n') << std::endl;
	return 0;
}
