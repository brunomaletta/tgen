#include "../single_include/tgen.h"

#include <iostream>
#include <limits>

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);

	// Random 20 distinct values from 1 to 100.
	std::cout << tgen::list<int>(20, 1, 100).distinct().gen() << std::endl;

	// Random palindrome of length 7.
	std::cout << tgen::str(7, 'a', 'z').palindrome().gen() << std::endl;

	// Random 3 runs of 4 equal numbers. Values between runs are distinct.
	std::cout << tgen::list<int>(12, 1, 10)
					 .equal_range(0, 3)
					 .equal_range(4, 7)
					 .equal_range(8, 11)
					 .different({0, 4, 8})
					 .gen()
			  << std::endl;

	// Random DNA list of length 8 with no equal adjacent values.
	auto s2 = tgen::list<char>(8, {'A', 'C', 'G', 'T'});
	for (int i = 1; i < 8; ++i)
		s2.different(i - 1, i);
	std::cout << s2.gen() << std::endl;

	// Random binary list of length 10 with 5 1's that starts with 1.
	std::cout << tgen::list<int>(10, 0, 1).fix(0, 1).gen_until(
					 [](const auto &inst) {
						 auto vec = inst.to_std();
						 return accumulate(vec.begin(), vec.end(), 0) == 5;
					 },
					 100)
			  << std::endl;

	// Prints a random 1-based permutation of size 10 that starts with 2.
	std::cout << tgen::permutation(10).fix(0, 1).gen().add_1() << std::endl;

	// Random permutation of size 5 with only one cycle.
	std::cout << tgen::permutation(5).cycles({5}).gen() << std::endl;

	// Inverse of a random odd permutation of size 5.
	std::cout << tgen::permutation(5)
					 .gen_until(
						 [](const auto &perm) { return perm.parity() == -1; },
						 100)
					 .inverse()
			  << std::endl;

	// Random swap permutation of size 20.
	std::cout << tgen::permutation(20)
					 .cycles(tgen::list<int>(19, 1, 2)
								 .fix(0, 1)
								 .equal_range(0, 17)
								 .fix(18, 2)
								 .gen()
								 .to_std())
					 .gen()
					 .add_1()
			  << std::endl;

	// Prints a random prime in [1, 1e18].
	std::cout << tgen::math::gen_prime(1, 1e18) << std::endl;

	// Prints the largest prime that fits in uint64_t.
	std::cout << tgen::math::prime_upto(std::numeric_limits<uint64_t>::max())
			  << std::endl;

	// Prints largest prime gap that fits in uint64_t.
	auto [l, r] =
		tgen::math::prime_gap_upto(std::numeric_limits<uint64_t>::max());
	std::cout << l << " " << r << " " << r - l << std::endl;

	// Prints a random even number in [1, 10].
	std::cout << tgen::math::gen_congruent(1, 10, 0, 2) << std::endl;

	// Prints a random partition of 10.
	std::cout << tgen::println(tgen::math::gen_partition(10));

	// Prints a random partition of 10 into 2 parts in [3, 7].
	std::cout << tgen::print(tgen::math::gen_partition_fixed_size(10, 2, 3, 7))
			  << std::endl;

	// Prints a matrix.
	std::vector<std::vector<int>> mat = {{1, 2}, {3, 4}};
	std::cout << tgen::println(mat);

	// Prints a shuffled set.
	std::cout << tgen::println(tgen::shuffled(std::set<int>({1, 2, 3})));

	// Prints random numbers in [0, 1e30].
	tgen::str leq1e30("0 | [1-9][0-9]{0,%d} | 10{%d}", 30 - 1, 30);
	std::cout << leq1e30.gen() << " " << leq1e30.gen() << " " << leq1e30.gen()
			  << std::endl;

	// Prints a random element from {"a", "b", "c"} by distribution {1, 2, 3}.
	std::cout << tgen::pick_by_distribution({"a", "b", "c"}, {1, 2, 3})
			  << std::endl;

	// Prints 5 random distinct numbers in [1, 10].
	std::cout << tgen::distinct_range(1, 10).gen_list(5) << std::endl;

	// Prints 3 random unique strings of length 5.
	std::cout << tgen::str("[ab]{5}").distinct().gen_list(3) << std::endl;

	// Prints 3 random unique primes in [1, 10].
	std::cout << tgen::distinct(tgen::math::gen_prime, 1, 10).gen_list(3)
			  << std::endl;

	// Prints a random perfect matching of K_10.
	auto g = tgen::distinct([&]() { return tgen::next(0, 9); });
	for (int i = 0; i < 5; ++i)
		std::cout << g.gen() << " " << g.gen() << std::endl;

	// Prints all primes in [1, 10], in order.
	std::cout << tgen::distinct(tgen::math::gen_prime, 1, 10).gen_all().sort()
			  << std::endl;

	// Prints 5 random square numbers in [1, 1e4].
	std::cout << tgen::distinct([&]() {
					 int x = tgen::next(1, 100);
					 return x * x;
				 }).gen_list(5)
			  << std::endl;

	// Prints all unique permutations of size 3 and 1 cycles of sizes 1 and 2.
	std::cout
		<< tgen::permutation(3).cycles({1, 2}).distinct().gen_all().separator(
			   '\n')
		<< std::endl;

	// Prints some random parenthesis sequences.
	std::cout << tgen::distinct(tgen::misc::gen_parenthesis, 6).gen_list(5)
			  << std::endl;

	// Computes how many of these there are.
	std::cout << tgen::distinct(tgen::misc::gen_parenthesis, 6).gen_all().size()
			  << std::endl;

	// Prints two strings that force polynomial hash collision for multiple
	// bases and mods.
	std::cout << tgen::print(tgen::hack::polynomial_hash_hack(
								 26, {31, 33}, {(int)1e9 + 7, (int)1e9 + 9}),
							 '\n')
			  << std::endl;

	// Prints all pairs (a, b) in [1, 3] with a <= b.
	std::cout << tgen::pair<int>(1, 3).leq().distinct().gen_all().separator(
					 '\n')
			  << std::endl;
}
