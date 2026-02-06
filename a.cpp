#include "tgen.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n", 12);

	auto seq_gen = tgen::sequence<int>(n, 1, 5);
	for (int i = 1; i < n; i++)
		seq_gen.different(i - 1, i);
	cout << seq_gen.gen() << endl;

	cout << tgen::sequence(n, 1, n).distinct().gen() << endl;

	cout << tgen::sequence(n, 1, n)
				.different(0, 1)
				.set(2, 1)
				.set(1, 2)
				.different(1, 2)
				.gen_until([](const auto &seq) { return seq[0] == seq[2]; },
						   100)
		 << endl;

	cout << tgen::sequence<int>(5, 0, 9)
				.equal(0, 1)
				.distinct({1, 2, 3})
				.distinct({3, 4})
				.gen()
		 << endl;
}
