#include "../single_include/tgen.h"

#include <iostream>
#include <limits>

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	// Random 20 distinct values from 1 to 100.
	std::cout << tgen::sequence<int>(20, 1, 100).distinct().gen() << std::endl;

	// Random Palindrome of length 7.
	auto s1 = tgen::sequence<int>(7, 0, 9);
	for (int i = 0; i <= 2; ++i)
		s1.equal(i, 6 - i);
	std::cout << s1.gen() << std::endl;

	// Random 3 runs of 4 equal numbers. Values between runs are distinct.
	std::cout << tgen::sequence<int>(12, 1, 10)
					 .equal_range(0, 3)
					 .equal_range(4, 7)
					 .equal_range(8, 11)
					 .distinct({0, 4, 8})
					 .gen()
			  << std::endl;

	// Random DNA sequence of length 8 with no equal adjacent values.
	auto s2 = tgen::sequence<char>(8, {'A', 'C', 'G', 'T'});
	for (int i = 1; i < 8; i++)
		s2.different(i - 1, i);
	std::cout << s2.gen() << std::endl;

	// Random binary sequence of length 10 with 5 1's that start with 1.
	std::cout << tgen::sequence<int>(10, 0, 1).fix(0, 1).gen_until(
					 [](const auto &inst) {
						 auto vec = inst.to_std();
						 return accumulate(vec.begin(), vec.end(), 0) == 5;
					 },
					 100)
			  << std::endl;

	// Prints a random 1-based permutation of size 10 that start with 2.
	std::cout << tgen::permutation(10).fix(0, 1).gen().add_1() << std::endl;

	// Random permutation of size 5 with only one cycle.
	std::cout << tgen::permutation(5).gen({5}) << std::endl;

	// Inverse of a random odd permutation of size 5.
	std::cout << tgen::permutation(5)
					 .gen_until(
						 [](const auto &perm) { return perm.parity() == -1; },
						 100)
					 .inverse()
			  << std::endl;

	// Random swap permutation of size 20.
	std::cout << tgen::permutation(20)
					 .gen(tgen::sequence<int>(19, 1, 2)
							  .fix(0, 1)
							  .equal_range(0, 17)
							  .fix(18, 2)
							  .gen()
							  .to_std())
					 .add_1()
			  << std::endl;

	std::cout << tgen::math::gen_prime(1, 1e18) << std::endl;

	std::cout << tgen::math::prime_upto(std::numeric_limits<uint64_t>::max())
			  << std::endl;

	auto [l, r] =
		tgen::math::prime_gap_upto(std::numeric_limits<uint64_t>::max());
	std::cout << l << " " << r << " " << r - l << std::endl;

	std::cout << tgen::math::gen_even(1, 10) << std::endl;

	std::cout << tgen::println(tgen::math::gen_partition(10));

	std::cout << tgen::print(tgen::math::gen_partition_fixed_size(10, 2, 3, 7))
			  << std::endl;

	std::vector<std::vector<int>> mat = {{1, 2}, {3, 4}};
	std::cout << tgen::println(mat);

	std::cout << tgen::println(tgen::shuffled(std::set<int>({1, 2, 3})));
	std::cout << tgen::println(
		tgen::shuffled(tgen::sequence<int>({1, 2, 3}).gen()));

	tgen::str leq1e30("0 | [1-9][0-9]{0,%d} | 10{%d}", 30 - 1, 30);
	std::cout << leq1e30.gen() << " " << leq1e30.gen() << " " << leq1e30.gen()
			  << std::endl;
	std::cout << tgen::str("[a-zA-Z]{10}").gen() << std::endl;

	std::cout << tgen::any_by_distribution({"a", "b", "c"}, {1, 2, 3})
			  << std::endl;

	tgen::distinct_range d10(1, 10);
	for (int i = 0; i < 5; i++)
		std::cout << d10.gen() << " \n"[i + 1 == 5];
}
