#include "tgen.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n", 12);

	auto seq_gen =
		tgen::sequence<int>(/*size=*/n, /*value_l=*/1, /*value_r=*/5);
	for (int i = 1; i < n; i++)
		seq_gen.different(i - 1, i);

	for (int i = 0; i < 10; ++i)
		cout << seq_gen.gen() << endl;

	cout << tgen::sequence(n, 1, n).distinct().gen() << endl;
}
