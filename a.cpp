#include "tgen.h"
#include "tgen_array.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n", 12);

	auto array_gen = tgen::array_gen<char>(n, 'a', 'd')
						 .set_value_at_idx(1, 'b')
						 .set_equal_idx_pair(0, 2);
	array_gen.set_palindromic_substring(5, 9).set_equal_substring(9, 11);

	auto array1 = array_gen();
	auto array2 = array_gen();
	array1.print();
	array2.sort().reverse().print();
	array_gen().print();
}
