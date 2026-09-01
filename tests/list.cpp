#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

struct list_test {
	int l, r;
	tgen::list<int> s;
	std::vector<std::pair<int, int>> defs;
	std::vector<std::set<int>> equals;
	std::vector<std::set<int>> distincts;

	list_test(int n, int l_, int r_) : l(l_), r(r_), s(n, l, r) {}

	list_test &fix(int idx, int val) {
		s.fix(idx, val);
		defs.emplace_back(idx, val);
		return *this;
	}
	list_test &equal(int idx_1, int idx_2) {
		s.equal(idx_1, idx_2);
		equals.push_back({idx_1, idx_2});
		return *this;
	}
	list_test &equal(std::set<int> indices) {
		s.equal(indices);
		equals.push_back(indices);
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
		for (auto equal : equals) {
			std::optional<int> val;
			for (int i : equal) {
				if (!val)
					val = v[i];
				else
					EXPECT_TRUE(*val == v[i]);
			}
		}
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

	tgen::list<tgen::list<int>::value>::value nested = {{1, 2}, {3}};
	EXPECT_EQ(nested.to_std(), std::vector<std::vector<int>>({{1, 2}, {3}}));

	EXPECT_EQ((std::ostringstream() << inst).str(), std::string("6 5 2 3 1 4"));

	EXPECT_EQ((std::ostringstream() << inst.separator(',')).str(),
			  std::string("6,5,2,3,1,4"));
}

TEST(list_test, value_shuffle) {
	tgen::register_gen();

	tgen::list<int>::value inst({4, 1, 3, 2, 3});
	auto sorted = inst.to_std();
	std::sort(sorted.begin(), sorted.end());

	for (int i = 0; i < 100; ++i) {
		inst.shuffle();
		auto cur = inst.to_std();
		std::sort(cur.begin(), cur.end());
		EXPECT_EQ(cur, sorted);
	}
}

TEST(list_test, value_pick) {
	tgen::register_gen();

	tgen::list<int>::value inst({0, 1, 2});
	for (int i = 0; i < 100; ++i) {
		int x = inst.pick();
		EXPECT_GE(x, 0);
		EXPECT_LE(x, 2);
	}
	expect_distribution([&] { return inst.pick_by_distribution({1, 2, 3}); },
						{1, 2, 3});
}

TEST(list_test, value_pick_by_distribution_invalid) {
	tgen::register_gen();

	tgen::list<int>::value inst({1, 2, 3});
	EXPECT_THROW_TGEN_PREFIX(inst.pick_by_distribution({1, 2}),
							 "value and distribution must have the same size");
}

TEST(list_test, value_choose_invalid) {
	tgen::register_gen();

	tgen::list<int>::value inst({1, 2, 3});
	EXPECT_THROW_TGEN_PREFIX(static_cast<void>(inst.choose(0)),
							 "number of elements to choose must be valid");
	EXPECT_THROW_TGEN_PREFIX(static_cast<void>(inst.choose(4)),
							 "number of elements to choose must be valid");
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

	for (int i = 0; i < 100; ++i) {
		int n = 5;
		list_test test(n, 1, n);

		int q = tgen::next(1, 5);
		for (int j = 0; j < q; ++j) {
			int sz = tgen::next(1, n);
			std::set<int> idx;
			for (int k = 0; k < n; ++k)
				idx.insert(k);
			idx = tgen::choose(idx, sz);
			test.equal(idx);
		}

		test.check();
	}
}

TEST(list_test, gen_with_different) {
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

	auto inst = tgen::list<int>(10, 1, 10).equal({0, 3, 4}).gen();
	EXPECT_TRUE(inst[0] == inst[3] and inst[3] == inst[4]);
}

TEST(list_test, gen_with_all_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::list<int>(10, 1, 10).fix(0, 5).equal(0, 1).fix(1, 6).gen(),
		"list: invalid list (contradictory restrictions)");

	EXPECT_THROW_TGEN_PREFIX(
		tgen::list<int>(10, 1, 10).fix(0, 5).fix(1, 5).different(0, 1).gen(),
		"list: invalid list (contradictory restrictions)");

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 1, 9).all_different().gen(),
							 "list: invalid list (contradictory restrictions)");

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 1, 10)
								 .fix(0, 1)
								 .fix(2, 1)
								 .different({0, 1, 2})
								 .gen(),
							 "list: invalid list (contradictory restrictions)");

	EXPECT_THROW_TGEN_PREFIX(tgen::list<int>(10, 0, 2)
								 .equal(0, 1)
								 .equal(2, 3)
								 .fix(0, 0)
								 .fix(2, 1)
								 .different({0, 2, 3})
								 .gen(),
							 "list: invalid list (contradictory restrictions)");
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

	expect_generator_uniform(tgen::list<int>(5, 0, 1), 1 << 5);
	expect_generator_uniform(tgen::list<int>(5, 0, 1).fix(0, 1), 1 << 4);
	expect_generator_uniform(tgen::list<int>(5, 0, 1).equal(0, 1), 1 << 4);
	expect_generator_uniform(tgen::list<int>(5, 0, 1).fix(0, 1).equal(0, 1),
							 1 << 3);
	expect_generator_uniform(tgen::list<int>(5, 0, 1).equal(0, 1).equal(1, 2),
							 1 << 3);
	expect_generator_uniform(tgen::list<int>(3, 1, 3).all_different(), 6);
	expect_generator_uniform(tgen::list<int>(5, 1, 5).different({0, 1, 2}),
							 60 * 5 * 5);
	expect_generator_uniform(
		tgen::list<int>(5, 1, 5).different({0, 1, 2}).equal(1, 4), 60 * 5);
	expect_generator_uniform(
		tgen::list<int>(5, 1, 5).different({0, 1, 2}).equal(1, 4).fix(4, 3),
		12 * 5);
}

namespace {

void expect_all_different_in_range(const tgen::list<long long>::value &inst,
								   long long left, long long right) {
	EXPECT_EQ(inst.size(), 8);
	std::set<long long> seen;
	for (int i = 0; i < inst.size(); ++i) {
		EXPECT_GE(inst[i], left);
		EXPECT_LE(inst[i], right);
		EXPECT_TRUE(seen.insert(inst[i]).second);
	}
}

} // namespace

TEST(list_test, gen_all_different_huge_value_range) {
	tgen::register_gen(42);

	// Range far above the dense-pool threshold; must not allocate O(range).
	constexpr long long left = 0;
	constexpr long long right = 1'000'000'000'000'000'000LL;

	expect_all_different_in_range(
		tgen::list<long long>(8, left, right).all_different().gen(), left,
		right);

	// Same sampling via the general different-restriction path.
	std::set<int> indices = {0, 1, 2, 3, 4, 5, 6, 7};
	expect_all_different_in_range(
		tgen::list<long long>(8, left, right).different(indices).gen(), left,
		right);
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
		auto subseq = inst.choose(k);
		int idx = 0;
		// Tests if subseq is a subsequence of inst.
		for (int j = 0; j < inst.size(); ++j)
			if (idx < subseq.size() and subseq[idx] == inst[j])
				++idx;
		EXPECT_TRUE(idx == subseq.size());
	}
}

TEST(list_test, adjacent_different) {
	tgen::register_gen();

	for (int i = 0; i < 50; ++i) {
		auto inst = tgen::list<int>(10, 1, 5).adjacent_different().gen();
		EXPECT_EQ(inst.size(), 10);
		for (int j = 1; j < inst.size(); ++j)
			EXPECT_NE(inst[j - 1], inst[j]);
	}
}
