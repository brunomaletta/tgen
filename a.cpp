#include "tgen.h"
#include "tgen_array.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n", 12);

	auto array_gen = tgen::array_gen<int>(/*size=*/n, /*l=*/1, /*r=*/5)
						 .distinct_idx_set(/*indices=*/{0, 1, 2})
						 .value_at_idx(/*idx=*/1, /*value=*/2)
						 .distinct_idx_set(/*indices=*/{11, 10, 9, 8, 7});

	for (int i = 0; i < 10; i++)
		array_gen().print();

	vector<int> nxt_array = array_gen().choose(5).shuffle()();
	for (int i : nxt_array)
		cout << i << " ";
	cout << endl;
}
