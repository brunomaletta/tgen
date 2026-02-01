#include "tgen.h"
#include "tgen_array.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n", 12);

	auto array_gen = tgen::array<int>(/*size=*/n, /*l=*/1, /*r=*/5)
						 .distinct_idx_set(/*indices=*/{0, 1, 2})
						 .value_at_idx(/*idx=*/1, /*value=*/2)
						 .distinct_idx_set(/*indices=*/{11, 10, 9, 8, 7});

	for (int i = 0; i < 10; i++)
		cout << array_gen.gen() << endl;

	cout << endl;

	cout << tgen::array<char>(3, 'a', 'd').gen().sort().reverse() << " "
		 << tgen::array<int>(10, 20, 30).equal_range(0, 9).gen() << endl;

	vector<char> chars = tgen::array_op::choose(
							 3, tgen::array<char>(5, 'A', 'E').distinct().gen())
							 .stdvec();

	cout << tgen::array<char>(10, chars).gen() << endl;

	cout << tgen::array<int>(20, {1, 50, 100, 250, 1000}).gen() << endl;
}
