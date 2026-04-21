#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <iostream>
#include <set>
#include <utility>
#include <vector>

struct list_test {
	int l, r;
	tgen::list<int> s;
	std::vector<std::pair<int, int>> defs;
	std::vector<std::pair<int, int>> equals;
	std::vector<std::set<int>> distincts;

	list_test(int n, int l_, int r_) : l(l_), r(r_), s(n, l, r) {}

	list_test &fix(int idx, int val) {
		s.fix(idx, val);
		defs.emplace_back(idx, val);
		return *this;
	}
	list_test &equal(int idx_1, int idx_2) {
		s.equal(idx_1, idx_2);
		equals.emplace_back(idx_1, idx_2);
		return *this;
	}
	list_test &different(std::set<int> indices) {
		s.different(indices);
		distincts.push_back(indices);
		return *this;
	}

	void check() {
		auto v = s.gen();
		for (int i = 0; i < v.size(); ++i)
			EXPECT_TRUE(l <= v[i] and v[i] <= r);
		for (auto [idx, val] : defs)
			EXPECT_TRUE(v[idx] == val);
		for (auto [idx_1, idx_2] : equals)
			EXPECT_TRUE(v[idx_1] == v[idx_2]);
		for (auto distinct : distincts) {
			std::set<int> vals;
			for (int i : distinct) {
				EXPECT_TRUE(vals.find(v[i]) == vals.end());
				vals.insert(v[i]);
			}
		}
	}
};

TEST(list_test, constructor_size_zero) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(0, 1, 10),
							 "list: size must be positive");
}

TEST(list_test, constructor_invalid_range) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 2, 1),
							 "list: value range must be valid");
}

TEST(list_test, constructor_empty_set) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, {}),
							 "list: value set must be non-empty");
}

TEST(list_test, gen_no_restrictions) {
	tgen::register_gen();

	auto s = tgen::list<int>(10, 1, 10);

	for (int i = 0; i < 100; ++i) {
		auto v = s.gen();
		for (int j = 0; j < 10; ++j)
			EXPECT_TRUE(1 <= v[j] and v[j] <= 10);
	}
}

TEST(list_test, gen_no_restrictions_corners) {
	tgen::register_gen();

	{
		auto v = tgen::list<int>(1, 1, 10).gen();
		EXPECT_TRUE(1 <= v[0] and v[0] <= 10);
	}
	{
		auto v = tgen::list<int>(1, 1, 1).gen();
		EXPECT_TRUE(v[0] == 1);
	}
	{
		auto v = tgen::list<int>(10, 1, 1).gen();
		for (int i = 0; i < 10; ++i)
			EXPECT_TRUE(v[i] == 1);
	}
}

TEST(list_test, set_invalid_idx) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 1, 10).fix(-1, 5),
							 "list: index must be valid");
	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 1, 10).fix(10, 5),
							 "list: index must be valid");
}

TEST(list_test, set_range_invalid_value) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 1, 10).fix(3, 20),
							 "list: value must be in the defined range");
}

TEST(list_test, set_range_twice) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 1, 10).fix(3, 5).fix(3, 6),
							 "list: must not set to two different values");
}

TEST(list_test, set_value_set_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, {5, 10, 15}).fix(3, 3),
							 "list: value must be in the set of values");
}

TEST(list_test, set_value_set_twice) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::list<int>(10, {5, 10, 15}).fix(3, 5).fix(3, 10),
		"list: must not set to two different values");
}

TEST(list_test, set_twice_valid) {
	tgen::register_gen();

	tgen::list<int>(10, 1, 10).fix(3, 5).fix(3, 5);
	tgen::list<int>(10, {5, 10, 15}).fix(3, 5).fix(3, 5);
}

TEST(list_test, equal_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 1, 10).fix(-1, 5),
							 "list: index must be valid");
}

TEST(list_test, value_ops) {
	tgen::register_gen();

	tgen::list<int>::value inst = {4, 1, 3, 2};

	EXPECT_EQ(inst.size(), 4);
	EXPECT_EQ(inst[2], 3);

	tgen::list<int>::value extra = {5, 6};
	inst = inst + extra;
	EXPECT_EQ(inst.to_std(), std::vector<int>({4, 1, 3, 2, 5, 6}));

	inst.reverse();
	EXPECT_EQ(inst.to_std(), std::vector<int>({6, 5, 2, 3, 1, 4}));

	tgen::shuffle(inst);
	inst.sort();
	EXPECT_EQ(inst.to_std(), std::vector<int>({1, 2, 3, 4, 5, 6}));
	EXPECT_EQ(inst.to_std(), tgen::shuffled(inst).sort().to_std());

	EXPECT_TRUE(tgen::pick(inst) > 0);
	EXPECT_TRUE(tgen::pick_by_distribution(inst, {1, 2, 3, 4, 5, 6}) > 0);
	EXPECT_EQ(tgen::choose(inst, 3).size(), 3);

	tgen::list<tgen::list<int>::value>::value nested = {{1, 2}, {3}};
	EXPECT_EQ(nested.to_std(), std::vector<std::vector<int>>({{1, 2}, {3}}));

	testing::internal::CaptureStdout();
	std::cout << inst;
	EXPECT_EQ(testing::internal::GetCapturedStdout(),
			  std::string("1 2 3 4 5 6"));

	testing::internal::CaptureStdout();
	std::cout << inst.separator(',');
	EXPECT_EQ(testing::internal::GetCapturedStdout(),
			  std::string("1,2,3,4,5,6"));
}

TEST(list_test, gen_with_set) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		int n = 10, num_op = tgen::next(1, 5);
		std::vector<int> set_idx(n, 0);
		for (int j = 0; j < num_op; ++j)
			set_idx[j] = 1;
		tgen::shuffle(set_idx.begin(), set_idx.end());

		list_test test(n, 1, n);

		for (int j = 0; j < num_op; ++j)
			if (set_idx[j])
				test.fix(j, tgen::next(1, n));

		test.check();
	}
}

TEST(list_test, gen_with_equal) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		int n = 10;
		list_test test(n, 1, n);

		int q = tgen::next(1, 2 * n);
		for (int j = 0; j < q; ++j)
			test.equal(tgen::next(0, n - 1), tgen::next(0, n - 1));

		test.check();
	}
}

TEST(list_test, gen_with_equal_range) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		std::vector<std::pair<int, int>> equals;
		int n = 10;
		int q = tgen::next(1, 2 * n);
		for (int j = 0; j < q; ++j)
			equals.emplace_back(tgen::next(0, n - 1), tgen::next(0, n - 1));
		tgen::shuffle(equals.begin(), equals.end());

		auto s = tgen::list<int>(n, 1, n);
		for (auto &[a, b] : equals) {
			if (b < a)
				std::swap(a, b);
			s.equal_range(a, b);
		}

		auto v = s.gen();
		for (auto [a, b] : equals) {
			for (int k = a + 1; k < b; ++k)
				EXPECT_EQ(v[k - 1], v[k]);
		}
	}
}

TEST(list_test, gen_with_distinct) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		int n = 5;
		list_test test(n, 1, n);

		int q = 2;
		for (int j = 0; j < q; ++j) {
			int sz = tgen::next(1, n);
			std::set<int> idx;
			for (int k = 0; k < n; ++k)
				idx.insert(k);
			idx = tgen::choose(idx, sz);
			test.different(idx);
		}

		test.check();
	}
}

TEST(list_test, gen_with_all_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::list<int>(10, 1, 10).fix(0, 5).equal(0, 1).fix(1, 6).gen(),
		"list: invalid list (contradicting restrictions)");

	EXPECT_THROW_TGEN_PREFIX(
		tgen::list<int>(10, 1, 10).fix(0, 5).fix(1, 5).different(0, 1).gen(),
		"list: invalid list (contradicting restrictions)");

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 1, 9).all_different().gen(),
							 "list: invalid list (contradicting restrictions)");

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 1, 10)
								 .fix(0, 1)
								 .fix(2, 1)
								 .different({0, 1, 2})
								 .gen(),
							 "list: invalid list (contradicting restrictions)");

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 0, 2)
								 .equal(0, 1)
								 .equal(2, 3)
								 .fix(0, 0)
								 .fix(2, 1)
								 .different({0, 2, 3})
								 .gen(),
							 "list: invalid list (contradicting restrictions)");
}

TEST(list_test, gen_with_all_complex) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::list<int>(10, 1, 10)
			.different({0, 1, 2})
			.different({2, 3, 4})
			.different({4, 5, 0})
			.gen(),
		"list: cannot represent list (complex restrictions)");

	EXPECT_THROW_TGEN_PREFIX(
		tgen::list<int>(10, 1, 10)
			.different({0, 1})
			.different({1, 2})
			.fix(0, 5)
			.fix(2, 6)
			.gen(),
		"list: cannot represent list (complex restrictions)");

	EXPECT_THROW_TGEN_PREFIX(
		tgen::list<int>(10, 1, 10)
			.different({0, 1})
			.different({0, 1})
			.different({0, 1})
			.gen(),
		"list: cannot represent list (complex restrictions)");

	EXPECT_THROW_TGEN_PREFIX(
		tgen::list<int>(10, 1, 10)
			.different({0, 1})
			.different({1, 2, 3})
			.different({3, 4})
			.equal(0, 4)
			.gen(),
		"list: cannot represent list (complex restrictions)");
}

TEST(list_test, gen_two_distincts_one_set) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		int n = 5;
		list_test test(n, 1, n);

		int q = 2;
		for (int j = 0; j < q; ++j) {
			int sz = tgen::next(1, n);
			std::set<int> idx;
			for (int k = 0; k < n; ++k)
				idx.insert(k);
			idx = tgen::choose(idx, sz);
			test.different(idx);
		}

		test.fix(tgen::next(0, n - 1), tgen::next(1, n));

		test.check();
	}
}

TEST(list_test, gen_with_all) {
	tgen::register_gen();

	list_test(10, 1, 10)
		.different({0, 1})
		.different({1, 2, 3})
		.different({3, 4})
		.fix(1, 1)
		.fix(3, 2)
		.check();
	list_test(10, 1, 10)
		.equal(0, 1)
		.equal(1, 2)
		.different({0, 5, 6})
		.fix(6, 10)
		.check();
	list_test(10, 1, 10)
		.equal(0, 1)
		.equal(1, 2)
		.different({0, 5, 6})
		.fix(5, 10)
		.fix(6, 9)
		.check();
}

TEST(list_test, gen_uniform) {
	tgen::register_gen();

	check_generator_uniform(tgen::list<int>(5, 0, 1), 1 << 5);
	check_generator_uniform(tgen::list<int>(5, 0, 1).fix(0, 1), 1 << 4);
	check_generator_uniform(tgen::list<int>(5, 0, 1).equal(0, 1), 1 << 4);
	check_generator_uniform(tgen::list<int>(5, 0, 1).fix(0, 1).equal(0, 1),
							1 << 3);
	check_generator_uniform(tgen::list<int>(5, 0, 1).equal(0, 1).equal(1, 2),
							1 << 3);
	check_generator_uniform(tgen::list<int>(3, 1, 3).all_different(), 6);
	check_generator_uniform(tgen::list<int>(5, 1, 5).different({0, 1, 2}),
							60 * 5 * 5);
	check_generator_uniform(
		tgen::list<int>(5, 1, 5).different({0, 1, 2}).equal(1, 4), 60 * 5);
	check_generator_uniform(
		tgen::list<int>(5, 1, 5).different({0, 1, 2}).equal(1, 4).fix(4, 3),
		12 * 5);
}

TEST(list_test, gen_until_not_found) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 0, 1).fix(0, 1).gen_until(
								 [](const auto &inst) {
									 auto vec = inst.to_std();
									 return std::accumulate(vec.begin(),
															vec.end(), 0) == 0;
								 },
								 100),
							 "could not generate value matching predicate");
}

TEST(list_test, gen_until) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		auto inst = tgen::list<int>(10, 0, 1).fix(0, 1).gen_until(
			[](const auto &inst2) {
				auto vec = inst2.to_std();
				return std::accumulate(vec.begin(), vec.end(), 0) == 5;
			},
			100);

		EXPECT_TRUE(inst[0] == 1);
		auto vec = inst.to_std();
		EXPECT_TRUE(std::accumulate(vec.begin(), vec.end(), 0) == 5);
	}
}

TEST(list_test, gen_list) {
	tgen::register_gen();

	auto insts = tgen::list<int>(5, 0, 1).fix(0, 1).equal(0, 1).gen_list(5);
	for (auto &inst : insts.to_std()) {
		EXPECT_TRUE(inst[0] == 1);
		EXPECT_EQ(inst[0], inst[1]);
	}
}

TEST(list_test, distinct) {
	tgen::register_gen();

	auto insts =
		tgen::list<int>(5, 0, 1).fix(0, 1).equal(0, 1).distinct().gen_list(5);
	for (auto &inst : insts.to_std()) {
		EXPECT_TRUE(inst[0] == 1);
		EXPECT_EQ(inst[0], inst[1]);
	}

	auto vec = insts.to_std();
	EXPECT_EQ(std::set<tgen::list<int>::value>(vec.begin(), vec.end()).size(),
			  5);
}

TEST(list_test, list_choose) {
	tgen::register_gen();

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 10);
	tgen::list<int>::value inst(v);

	for (int i = 0; i < 100; ++i) {
		int k = tgen::next<int>(1, v.size());
		auto subseq = tgen::choose(inst, k);
		int idx = 0;
		// Tests if subseq is a sublist of inst.
		for (int j = 0; j < inst.size(); ++j)
			if (idx < subseq.size() and subseq[idx] == inst[j])
				++idx;
		EXPECT_TRUE(idx == subseq.size());
	}
}
