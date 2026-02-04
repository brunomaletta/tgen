#include "tgen.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n", 12);

	auto seq_gen = tgen::sequence<int>(/*size=*/n, /*value_l=*/1, /*value_r=*/5)
					   .distinct(/*indices=*/{0, 1, 2})
					   .set(/*idx=*/1, /*value=*/2)
					   .distinct(/*indices=*/{11, 10, 9, 8, 7});

	cout << endl;

	cout << tgen::sequence<char>(/*size=*/3, /*value_l=*/'a', /*value_r=*/'d')
				.gen()
				.sort()
				.reverse()
		 << " "
		 << tgen::sequence<int>(/*size=*/10, /*value_l=*/20, /*value_r=*/30)
				.equal_range(/*left=*/0, /*right=*/9)
				.gen()
		 << endl;

	vector<char> chars =
		tgen::sequence_op::choose(
			3,
			tgen::sequence<char>(/*size=*/5, /*value_l=*/'A', /*value_r=*/'E')
				.distinct()
				.gen())
			.to_std();

	cout << tgen::sequence<char>(/*size=*/10, /*values=*/chars).gen() << endl;

	cout << tgen::sequence<int>(/*size=*/20, /*values=*/{1, 50, 100, 250, 1000})
				.set(0, 50)
				.equal(0, 2)
				.distinct({19, 18, 17, 16, 15})
				.gen()
		 << endl;

	auto v = tgen::choose(/*k=*/5, /*list=*/{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
	for (int i : v)
		cout << i << " ";
	cout << endl;

	cout << tgen::sequence<int>(/*size=*/10, /*value_l=*/1, /*value_r=*/10)
				.distinct()
				.gen_until(
					/*predicate=*/
					[](const auto &seq) { return seq[0] < seq[9]; },
					/*max_tries=*/100)
		 << endl;

	tgen::sequence<int>::instance inst = {1, 2, 3};
	cout << tgen::sequence_op::any(inst) << endl;

	cout << tgen::sequence<int>(4, 1, 5)
				.equal(0, 1)
				.equal(2, 3)
				.different(1, 2)
				.gen()
		 << std::endl;
}
