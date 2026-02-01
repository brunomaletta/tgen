#include "tgen.h"
#include "tgen_sequence.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n", 12);

	auto seq_gen = tgen::sequence<int>(/*size=*/n, /*l=*/1, /*r=*/5)
					   .distinct_idx_set(/*indices=*/{0, 1, 2})
					   .value_at_idx(/*idx=*/1, /*value=*/2)
					   .distinct_idx_set(/*indices=*/{11, 10, 9, 8, 7});

	for (int i = 0; i < 10; i++)
		cout << seq_gen.gen() << endl;

	cout << endl;

	cout << tgen::sequence<char>(3, 'a', 'd').gen().sort().reverse() << " "
		 << tgen::sequence<int>(10, 20, 30).equal_range(0, 9).gen() << endl;

	vector<char> chars =
		tgen::sequence_op::choose(
			3, tgen::sequence<char>(5, 'A', 'E').distinct().gen())
			.stdvec();

	cout << tgen::sequence<char>(10, chars).gen() << endl;

	cout << tgen::sequence<int>(20, {1, 50, 100, 250, 1000})
				.value_at_idx(0, 50)
				.equal_idx_pair(0, 2)
				.distinct_idx_set({19, 18, 17, 16, 15})
				.gen()
		 << endl;

	auto v = tgen::choose(5, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
	for (int i : v)
		cout << i << " ";
	cout << endl;
}
