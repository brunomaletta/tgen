#include "tgen.h"
#include "tgen_array.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n", 12);

	auto array_gen = tgen::array_gen<int>(/*size=*/n, /*l=*/1, /*r=*/5)
						 .set_distinct_idx_set(/*indices=*/{0, 1, 2})
						 .set_value_at_idx(/*idx=*/1, /*value=*/2);

	for (int i = 0; i < 10; i++)
		array_gen().print();
}
