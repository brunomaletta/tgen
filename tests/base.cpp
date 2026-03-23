#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <algorithm>

TEST(base_test, print_scalar) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	testing::internal::CaptureStdout();
	std::cout << tgen::print(10);
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("10"));

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::string("str"));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("str"));
}

TEST(base_test, print_pair) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::pair<std::string, double>("str", 0.1));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("str 0.1"));
}

TEST(base_test, print_tuple) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	testing::internal::CaptureStdout();
	std::cout << tgen::print(
		std::tuple<int, double, std::string>(5, 0.1, "str"));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("5 0.1 str"));
}

TEST(base_test, print_1d_container) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	testing::internal::CaptureStdout();
	std::cout << tgen::print({1, 2, 3});
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("1 2 3"));

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::vector<int>({1, 2, 3}));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("1 2 3"));

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::set<int>({1, 2, 3}));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("1 2 3"));

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::array<int, 3>({1, 2, 3}));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("1 2 3"));
}

TEST(base_test, print_2d_container) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	testing::internal::CaptureStdout();
	std::cout << tgen::println({{1, 2, 3}, {4, 5, 6}, {7, 8, 9}});
	EXPECT_EQ(testing::internal::GetCapturedStdout(),
			  std::string("1 2 3\n4 5 6\n7 8 9\n"));

	std::vector<int> r1 = {1, 2, 3}, r2 = {4, 5, 6}, r3 = {7, 8, 9};

	testing::internal::CaptureStdout();
	std::cout << tgen::print({r1, r2, r3});
	EXPECT_EQ(testing::internal::GetCapturedStdout(),
			  std::string("1 2 3\n4 5 6\n7 8 9"));

	// Complex container 1.
	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::vector<std::vector<int>>({r1, r2, r3}));
	EXPECT_EQ(testing::internal::GetCapturedStdout(),
			  std::string("1 2 3\n4 5 6\n7 8 9"));

	// Complex container 2.
	testing::internal::CaptureStdout();
	std::cout << tgen::print(
		std::vector<std::pair<int, int>>({{1, 2}, {3, 4}}));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("1 2\n3 4"));

	// Complex container 3.
	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::vector<std::tuple<char, int, double>>(
		{{'a', 1, 0.1}, {'b', 2, 0.2}}));
	EXPECT_EQ(testing::internal::GetCapturedStdout(),
			  std::string("a 1 0.1\nb 2 0.2"));

	// Complex tuple.
	testing::internal::CaptureStdout();
	std::cout << tgen::println(
		std::tuple<int, std::pair<int, int>, double>({2, {3, 4}, 5.1}));
	EXPECT_EQ(testing::internal::GetCapturedStdout(),
			  std::string("2\n3 4\n5.1\n"));

	// Complex pair.
	testing::internal::CaptureStdout();
	std::cout << tgen::print(
		std::pair<std::pair<int, int>, std::pair<int, int>>({{1, 2}, {3, 4}}));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("1 2\n3 4"));
}

TEST(base_test, next_invalid_range) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::next<int>(2, 1),
							 "range for `next` bust be valid");
}

TEST(base_test, next_val) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		auto n = tgen::next(10);
		EXPECT_TRUE(0 <= n and n < 10);
	}
	check_function_uniform([]() -> int { return tgen::next<int>(10); }, 10);
	check_function_uniform([]() -> int { return tgen::next<double>(2) < 1; },
						   2);
}

TEST(base_test, next_range) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		auto n = tgen::next(10, 20);
		EXPECT_TRUE(10 <= n and n <= 20);
	}
	check_function_uniform([](int l, int r) { return tgen::next<int>(l, r); },
						   11, 10, 20);
}

TEST(base_test, next_by_distribution) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		auto n = tgen::next_by_distribution({1, 2, 3});
		EXPECT_TRUE(0 <= n and n < 3);
	}
	check_function_uniform(
		[]() { return tgen::next_by_distribution({2, 2, 2}); }, 3);
}

TEST(base_test, shuffle) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 10);
	auto v_sorted = v;
	sort(v_sorted.begin(), v_sorted.end());

	for (int i = 0; i < 100; ++i) {
		tgen::shuffle(v.begin(), v.end());
		std::sort(v.begin(), v.end());
		EXPECT_TRUE(v == v_sorted);
	}
}

TEST(base_test, shuffled) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::vector<int> a = tgen::shuffled(std::vector<int>({1, 2, 3}));
	std::string b = tgen::shuffled(std::string("str"));
	std::vector<int> c = tgen::shuffled(std::set<int>({1, 2, 3}));
	std::vector<int> d = tgen::shuffled({1, 2, 3});
}

TEST(base_test, any) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 10);

	for (int i = 0; i < 100; ++i) {
		int value = tgen::any(v);
		EXPECT_TRUE(find(v.begin(), v.end(), value) != v.end());
	}
}

TEST(base_test, any_by_distribution) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::vector<int> v(3);
	for (int &i : v)
		i = tgen::next(1, 10);

	for (int i = 0; i < 10; ++i) {
		int value = tgen::any_by_distribution(v, {1, 2, 3});
		EXPECT_TRUE(find(v.begin(), v.end(), value) != v.end());
	}
	check_function_uniform(
		[]() { return tgen::any_by_distribution({1, 2, 3}, {2, 2, 2}); }, 3);
}

TEST(base_test, choose_invalid_ammount) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 100);

	EXPECT_THROW_TGEN_PREFIX(tgen::choose(v.size() + 1, v),
							 "number of elements to choose must be valid");
}

TEST(base_test, choose) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 10);

	for (int i = 0; i < 100; ++i) {
		int k = tgen::next<int>(1, v.size());
		auto subseq = tgen::choose(k, v);
		auto subseq_it = subseq.begin();
		// Tests if subseq is a subsequence of v.
		for (int j : v)
			if (subseq_it != subseq.end() and *subseq_it == j)
				++subseq_it;
		EXPECT_TRUE(subseq_it == subseq.end());
	}
}

TEST(base_test, distinct_range_generate_too_many_values) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	tgen::distinct_range d(1, 10);
	for (int i = 0; i < 10; ++i)
		d.gen();
	EXPECT_THROW_TGEN_PREFIX(d.gen(),
							 "distinct_range: no more values to generate");
}

TEST(base_test, distinct_range) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	tgen::distinct_range d(1, 10);
	std::set<int> s;
	for (int i = 0; i < 10; ++i) {
		int value = d.gen();
		EXPECT_TRUE(1 <= value and value <= 10);
		s.insert(value);
		EXPECT_EQ(d.size(), 10 - i - 1);
	}
	EXPECT_EQ(s.size(), 10);
}

TEST(base_test, distinct_container) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 100);
	tgen::distinct_container d(v);
	for (int i = 0; i < 10; ++i) {
		int value = d.gen();
		EXPECT_TRUE(find(v.begin(), v.end(), value) != v.end());
		EXPECT_EQ(d.size(), v.size() - i - 1);
	}
}