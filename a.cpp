#include "tgen.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	// Random 20 distinct values from 1 to 100.
	std::cout <<
		tgen::sequence<int>(20, 1, 100).distinct().gen() << std::endl;

	// Random Palindrome of length 7.
	auto s1 = tgen::sequence<int>(7, 0, 9);
	for (int i = 0; i <= 2; ++i) s1.equal(i, 6-i);
	cout << s1.gen() << endl;

	// Random 3 runs of 4 equal numbers. Values between runs are distinct.
	std::cout <<
		tgen::sequence<int>(12, 1, 10)
		.equal_range(0, 3).equal_range(4, 7).equal_range(8, 11)
		.distinct({0, 4, 8}).gen() << endl;

	// Random DNA sequence of length 8 with no equal adjacent values.
	auto s2 = tgen::sequence(8, {'A','C','G','T'});
	for (int i = 1; i < 8; i++) s2.different(i-1, i);
	cout << s2.gen() << endl;

	// Random binary sequence of length 10 with 5 1's that start with 1.
	std::cout <<
		tgen::sequence<int>(10, 0, 1)
		.set(0, 1)
		.gen_until([](auto inst) {
			auto vec = inst.to_std();
			return accumulate(vec.begin(), vec.end(), 0) == 5;
		 }, 100) << endl;
}
