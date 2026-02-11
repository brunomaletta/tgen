#include <gtest/gtest.h>

#include "tgen.h"

#include <set>
#include <utility>
#include <vector>

#define EXPECT_THROW_TGEN_PREFIX(stmt, prefix)                                 \
	EXPECT_THROW(                                                              \
		{                                                                      \
			try {                                                              \
				stmt;                                                          \
				FAIL() << "Expected std::runtime_error, but no error ocurred"; \
			} catch (const std::runtime_error &e) {                            \
				std::string msg = e.what();                                    \
				std::string tgen_pref = std::string("tgen: ") + prefix;        \
				EXPECT_TRUE(msg.rfind(tgen_pref, 0) == 0)                      \
					<< "Expected message to start with: \"" << tgen_pref       \
					<< "\"\n"                                                  \
					<< "Actual message: \"" << msg << "\"";                    \
				throw e;                                                       \
			}                                                                  \
		},                                                                     \
		std::runtime_error)

inline std::vector<char *> get_argv(std::initializer_list<const char *> list) {
	std::vector<char *> v;
	for (auto s : list)
		v.push_back(const_cast<char *>(s));
	v.push_back(nullptr);
	return v;
}

/*
 * Tests.
 */

TEST(permutation_test, constructor_size_zero) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::permutation(0), "size must be positive");
}

TEST(permutation_test, set_invalid_index) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::permutation(5).set(-1, 0),
							 "index must be valid");
	EXPECT_THROW_TGEN_PREFIX(tgen::permutation(5).set(5, 0),
							 "index must be valid");
}

TEST(permutation_test, instance_invalid) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::vector<int> invalid = {1, 2};
	EXPECT_THROW_TGEN_PREFIX(tgen::permutation::instance inst = invalid,
							 "permutation values must be from `0` to `size-1`");
	invalid = {1, 1};
	EXPECT_THROW_TGEN_PREFIX(tgen::permutation::instance inst = invalid,
							 "cannot have repeated values in permutation");
}

TEST(permutation_test, instance_ops) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	tgen::permutation::instance inst = {1, 0, 2};

	EXPECT_EQ(inst.size(), 3);
	EXPECT_EQ(inst[2], 2);

	inst.reverse();
	EXPECT_EQ(inst.to_std(), std::vector<int>({2, 0, 1}));

	inst.inverse();
	EXPECT_EQ(inst.to_std(), std::vector<int>({1, 2, 0}));

	inst.sort();
	EXPECT_EQ(inst.to_std(), std::vector<int>({0, 1, 2}));

	testing::internal::CaptureStdout();
	std::cout << inst;
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("0 1 2"));

	testing::internal::CaptureStdout();
	std::cout << inst.add_1();
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("1 2 3"));
}

TEST(permutation_test, gen) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

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
				perm.set(j, set_val[j]);

		auto inst = perm.gen();
		for (int j = 0; j < num_op; ++j)
			if (set_idx[j])
				EXPECT_EQ(inst[j], set_val[j]);
	}
}

TEST(permutation_test, gen_cycles) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		int n = 10, num_op = tgen::next(1, 10);
		std::vector<int> cycles;
		while (true) {
			int left = n - std::accumulate(cycles.begin(), cycles.end(), 0);
			if (left == 0)
				break;
			cycles.push_back(tgen::next(1, left));
		}

		auto inst = tgen::permutation(n).gen(cycles);
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
