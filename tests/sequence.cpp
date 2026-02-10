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

struct sequence_test {
	int l, r;
	tgen::sequence<int> s;
	std::vector<std::pair<int, int>> sets;
	std::vector<std::pair<int, int>> equals;
	std::vector<std::set<int>> distincts;

	sequence_test(int n, int l_, int r_) : l(l_), r(r_), s(n, l, r) {}

	void set(int idx, int val) {
		s.set(idx, val);
		sets.emplace_back(idx, val);
	}
	void equal(int idx_1, int idx_2) {
		s.equal(idx_1, idx_2);
		equals.emplace_back(idx_1, idx_2);
	}
	void distinct(std::set<int> distinct) {
		s.distinct(distinct);
		distincts.push_back(distinct);
	}

	void check() {
		auto v = s.gen();
		for (int i = 0; i < v.size(); ++i)
			EXPECT_TRUE(l <= v[i] and v[i] <= r);
		for (auto [idx, val] : sets)
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

/*
 * Tests.
 */

TEST(sequence_test, sequence_constructor_size_zero) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(0, 1, 10),
							 "size must be positive");
}

TEST(sequence_test, sequence_constructor_invalid_range) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, 2, 1),
							 "value range must be valid");
}

TEST(sequence_test, sequence_constructor_empty_set) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, {}),
							 "value set must be non-empty");
}

TEST(sequence_test, gen_no_restrictions) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	auto s = tgen::sequence<int>(10, 1, 10);

	for (int i = 0; i < 100; ++i) {
		auto v = s.gen();
		for (int j = 0; j < 10; ++j)
			EXPECT_TRUE(1 <= v[j] and v[j] <= 10);
	}
}

TEST(sequence_test, gen_no_restrictions_corners) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	{
		auto v = tgen::sequence<int>(1, 1, 10).gen();
		EXPECT_TRUE(1 <= v[0] and v[0] <= 10);
	}
	{
		auto v = tgen::sequence<int>(1, 1, 1).gen();
		EXPECT_TRUE(v[0] == 1);
	}
	{
		auto v = tgen::sequence<int>(10, 1, 1).gen();
		for (int i = 0; i < 10; ++i)
			EXPECT_TRUE(v[i] == 1);
	}
}

TEST(sequence_test, set_invalid_idx) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, 1, 10).set(-1, 5),
							 "index must be valid");
}

TEST(sequence_test, set_range_invalid_value) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, 1, 10).set(3, 20),
							 "value must be in the defined range");
}

TEST(sequence_test, set_range_twice) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, 1, 10).set(3, 5).set(3, 6),
							 "value must be in the defined range");
}

TEST(sequence_test, set_value_set_invalid) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, {5, 10, 15}).set(3, 3),
							 "value must be in the set of values");
}

TEST(sequence_test, set_value_set_twice) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(
		tgen::sequence<int>(10, {5, 10, 15}).set(3, 5).set(3, 10),
		"must not set to two different values");
}

TEST(sequence_test, set_twice_valid) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	tgen::sequence<int>(10, 1, 10).set(3, 5).set(3, 5);
	tgen::sequence<int>(10, {5, 10, 15}).set(3, 5).set(3, 5);
}

TEST(sequence_test, gen_with_set) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		int n = 10, num_op = tgen::next(1, 5);
		std::vector<int> set_idx(n, 0);
		for (int j = 0; j < num_op; ++j)
			set_idx[j] = 1;
		tgen::shuffle(set_idx.begin(), set_idx.end());

		sequence_test test(n, 1, n);

		for (int j = 0; j < num_op; ++j)
			if (set_idx[j])
				test.set(j, tgen::next(1, n));

		test.check();
	}
}

TEST(sequence_test, equal_invalid) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, 1, 10).set(-1, 5),
							 "index must be valid");
}

TEST(sequence_test, gen_with_equal) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		int n = 10;
		sequence_test test(n, 1, n);

		int q = tgen::next(1, 2 * n);
		for (int i = 0; i < q; ++i)
			test.equal(tgen::next(0, n - 1), tgen::next(0, n - 1));

		test.check();
	}
}

TEST(sequence_test, gen_with_equal_range) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		std::vector<std::pair<int, int>> equals;
		int n = 10;
		int q = tgen::next(1, 2 * n);
		for (int i = 0; i < q; ++i)
			equals.emplace_back(tgen::next(0, n - 1), tgen::next(0, n - 1));
		tgen::shuffle(equals.begin(), equals.end());

		auto s = tgen::sequence<int>(n, 1, n);
		for (auto &[i, j] : equals) {
			if (j < i)
				std::swap(i, j);
			s.equal_range(i, j);
		}

		auto v = s.gen();
		for (auto [i, j] : equals) {
			for (int k = i + 1; k < j; ++k)
				EXPECT_EQ(v[k - 1], v[k]);
		}
	}
}

TEST(sequence_test, gen_with_distinct) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		int n = 5;
		sequence_test test(n, 1, n);

		int q = 2;
		for (int i = 0; i < q; ++i) {
			int sz = tgen::next(1, n);
			std::set<int> idx;
			for (int j = 0; j < n; ++j)
				idx.insert(j);
			idx = tgen::choose(sz, idx);
			test.distinct(idx);
		}

		test.check();
	}
}

TEST(sequence_test, gen_with_all_invalid) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(
		tgen::sequence<int>(10, 1, 10).set(0, 5).equal(0, 1).set(1, 6).gen(),
		"invalid sequence (contradicting constraints)");

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, 1, 10)
								 .set(0, 5)
								 .set(1, 5)
								 .different(0, 1)
								 .gen(),
							 "invalid sequence (contradicting constraints)");

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, 1, 9).distinct().gen(),
							 "invalid sequence (contradicting constraints)");

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, 1, 10)
								 .set(0, 1)
								 .set(2, 1)
								 .distinct({0, 1, 2})
								 .gen(),
							 "invalid sequence (contradicting constraints)");

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, 0, 2)
								 .equal(0, 1)
								 .equal(2, 3)
								 .set(0, 0)
								 .set(2, 1)
								 .distinct({0, 2, 3})
								 .gen(),
							 "invalid sequence (contradicting constraints)");
}

TEST(sequence_test, gen_with_all_complex) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(
		tgen::sequence<int>(10, 1, 10)
			.distinct({0, 1, 2})
			.distinct({2, 3, 4})
			.distinct({4, 5, 0})
			.gen(),
		"failed to generate sequence: complex constraints");

	EXPECT_THROW_TGEN_PREFIX(
		tgen::sequence<int>(10, 1, 10)
			.distinct({0, 1})
			.distinct({1, 2})
			.set(0, 5)
			.set(2, 6)
			.gen(),
		"failed to generate sequence: complex constraints");

	EXPECT_THROW_TGEN_PREFIX(
		tgen::sequence<int>(10, 1, 10)
			.distinct({0, 1})
			.distinct({0, 1})
			.distinct({0, 1})
			.gen(),
		"failed to generate sequence: complex constraints");

	EXPECT_THROW_TGEN_PREFIX(
		tgen::sequence<int>(10, 1, 10)
			.distinct({0, 1})
			.distinct({1, 2, 3})
			.distinct({3, 4})
			.equal(0, 4)
			.gen(),
		"failed to generate sequence: complex constraints");
}

TEST(sequence_test, gen_two_distincts_one_set) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		int n = 5;
		sequence_test test(n, 1, n);

		int q = 2;
		for (int i = 0; i < q; ++i) {
			int sz = tgen::next(1, n);
			std::set<int> idx;
			for (int j = 0; j < n; ++j)
				idx.insert(j);
			idx = tgen::choose(sz, idx);
			test.distinct(idx);
		}

		test.set(tgen::next(0, n - 1), tgen::next(1, n));

		test.check();
	}
}

TEST(sequence_test, gen_with_all) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	{
		sequence_test test(10, 1, 10);
		test.distinct({0, 1});
		test.distinct({1, 2, 3});
		test.distinct({3, 4});
		test.set(1, 1);
		test.set(3, 2);

		test.check();
	}
	{
		sequence_test test(10, 1, 10);
		test.equal(0, 1);
		test.equal(1, 2);
		test.distinct({0, 5, 6});
		test.set(6, 10);

		test.check();
	}
	{
		sequence_test test(10, 1, 10);
		test.equal(0, 1);
		test.equal(1, 2);
		test.distinct({0, 5, 6});
		test.set(5, 10);
		test.set(6, 9);

		test.check();
	}
}

TEST(sequence_test, gen_until_not_found) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::sequence<int>(10, 0, 1).set(0, 1).gen_until(
								 [](const auto &inst) {
									 auto vec = inst.to_std();
									 return std::accumulate(vec.begin(),
															vec.end(), 0) == 0;
								 },
								 100),
							 "could not generate instance matching predicate");
}

TEST(sequence_test, gen_until) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		auto inst = tgen::sequence<int>(10, 0, 1).set(0, 1).gen_until(
			[](const auto &inst) {
				auto vec = inst.to_std();
				return std::accumulate(vec.begin(), vec.end(), 0) == 5;
			},
			100);

		EXPECT_TRUE(inst[0] == 1);
		auto vec = inst.to_std();
		EXPECT_TRUE(std::accumulate(vec.begin(), vec.end(), 0) == 5);
	}
}

/*
 * sequence_op.
 */

TEST(sequence_test, sequence_op_choose) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 10);
	tgen::sequence<int>::instance inst(v);

	for (int i = 0; i < 100; ++i) {
		int k = tgen::next<int>(1, v.size());
		auto subseq = tgen::sequence_op::choose(k, inst);
		int idx = 0;
		// Tests if subseq is a subsequence of inst.
		for (int j = 0; j < inst.size(); ++j)
			if (idx < subseq.size() and subseq[idx] == inst[j])
				++idx;
		EXPECT_TRUE(idx == subseq.size());
	}
}
