#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <sstream>
#include <vector>

namespace {

bool is_permutation(const tgen::permutation::value &p) {
	std::vector<bool> vis(p.size(), false);
	for (int i = 0; i < p.size(); ++i) {
		if (p[i] < 0 or p[i] >= p.size() or vis[p[i]])
			return false;
		vis[p[i]] = true;
	}
	return true;
}

} // namespace

TEST(permutation_test, constructor_size_zero) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::permutation(0),
							 "permutation: size must be positive");
}

TEST(permutation_test, fix_invalid_index) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::permutation(5).fix(-1, 0),
							 "permutation: index must be valid");
	EXPECT_THROW_TGEN_PREFIX(tgen::permutation(5).fix(5, 0),
							 "permutation: index must be valid");
}

TEST(permutation_test, cycles_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::permutation(5).cycles({4}),
		"permutation: cycle sizes must add up to size of permutation");
}

TEST(permutation_test, value_invalid) {
	tgen::register_gen();

	std::vector<int> invalid = {1, 2};
	EXPECT_THROW_TGEN_PREFIX(
		tgen::permutation::value inst = invalid,
		"permutation: value: values must be from `0` to `size-1`");
	invalid = {1, 1};
	EXPECT_THROW_TGEN_PREFIX(tgen::permutation::value inst = invalid,
							 "permutation: value: cannot have repeated values");
}

TEST(permutation_test, value_ops) {
	tgen::register_gen();

	tgen::permutation::value inst = {1, 0, 2};

	EXPECT_EQ(inst.size(), 3);
	EXPECT_EQ(inst[2], 2);

	inst.reverse();
	EXPECT_EQ(inst.to_std(), std::vector<int>({2, 0, 1}));

	inst.inverse();
	EXPECT_EQ(inst.to_std(), std::vector<int>({1, 2, 0}));

	inst.sort();
	EXPECT_EQ(inst.to_std(), std::vector<int>({0, 1, 2}));

	EXPECT_EQ((std::ostringstream() << inst).str(), std::string("0 1 2"));

	EXPECT_EQ((std::ostringstream() << inst.print_1_based()).str(),
			  std::string("1 2 3"));

	// print_1_based does not affect to_std().
	EXPECT_EQ(inst.to_std(), std::vector<int>({0, 1, 2}));
	EXPECT_EQ(inst.to_std_1_based(), std::vector<int>({1, 2, 3}));

	EXPECT_EQ(
		(std::ostringstream() << inst.print_1_based().separator(',')).str(),
		std::string("1,2,3"));
}

TEST(permutation_test, add_1_deprecated_alias) {
	tgen::register_gen();

	tgen::permutation::value inst = {0, 1, 2};
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
	EXPECT_EQ((std::ostringstream() << inst.add_1()).str(), "1 2 3");
	EXPECT_EQ(inst.add_1().to_std(), std::vector<int>({0, 1, 2}));
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

TEST(permutation_test, value_shuffle_pick) {
	tgen::register_gen();

	tgen::permutation::value inst({1, 0, 2});
	EXPECT_TRUE(is_permutation(inst));

	for (int i = 0; i < 100; ++i) {
		inst.shuffle();
		EXPECT_TRUE(is_permutation(inst));
	}

	inst.sort();
	for (int i = 0; i < 100; ++i) {
		int x = inst.pick();
		EXPECT_GE(x, 0);
		EXPECT_LT(x, inst.size());
	}
	expect_distribution([&] { return inst.pick_by_distribution({1, 2, 3}); },
						{1, 2, 3});

	EXPECT_THROW_TGEN_PREFIX(inst.pick_by_distribution({1, 2}),
							 "value and distribution must have the same size");
}

TEST(permutation_test, gen_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::permutation(5).fix(0, 1).fix(0, 2).gen(),
		"permutation: cannot set an index to two different values");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::permutation(5).fix(0, 0).fix(1, 0).gen(),
		"permutation: cannot set two indices to the same value");
}

TEST(permutation_test, gen_without_cycles) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		int n = 10, num_op = tgen::next(1, 5);
		std::vector<int> set_idx(n, 0), set_val(n);
		for (int j = 0; j < num_op; ++j)
			set_idx[j] = 1;
		tgen::shuffle(set_idx.begin(), set_idx.end());
		std::iota(set_val.begin(), set_val.end(), 0);
		tgen::shuffle(set_val.begin(), set_val.end());

		tgen::permutation perm(n);

		for (int j = 0; j < num_op; ++j)
			if (set_idx[j])
				perm.fix(j, set_val[j]);

		auto inst = perm.gen();
		for (int j = 0; j < num_op; ++j)
			if (set_idx[j]) {
				EXPECT_EQ(inst[j], set_val[j]);
			}
	}
}

TEST(permutation_test, gen_with_cycles) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		int n = 10;
		std::vector<int> cycles;
		while (true) {
			int left = n - std::accumulate(cycles.begin(), cycles.end(), 0);
			if (left == 0)
				break;
			cycles.push_back(tgen::next(1, left));
		}

		auto inst = tgen::permutation(n).cycles(cycles).gen();
		std::vector<bool> vis(n, false);
		std::vector<int> gen_cycles;
		for (int j = 0; j < n; ++j)
			if (!vis[j]) {
				int cyc_size = 0;
				for (int k = j; !vis[k]; k = inst[k]) {
					vis[k] = true;
					cyc_size++;
				}
				gen_cycles.push_back(cyc_size);
			}

		std::sort(cycles.begin(), cycles.end());
		std::sort(gen_cycles.begin(), gen_cycles.end());
		EXPECT_EQ(cycles, gen_cycles);
	}
}

TEST(permutation_test, gen_uniform) {
	tgen::register_gen();

	expect_generator_uniform(tgen::permutation(4), 24);
	expect_generator_uniform(tgen::permutation(5).fix(0, 3).fix(1, 2), 6);
}
