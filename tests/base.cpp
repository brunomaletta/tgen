#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <algorithm>

TEST(base_test, distinct) {
	tgen::register_gen();

	tgen::distinct g([]() { return tgen::next(1, 10); });
	std::set<int> st;
	while (!g.empty()) {
		int val = g.gen();
		EXPECT_TRUE(1 <= val and val <= 10);
		st.insert(val);
	}

	EXPECT_EQ(st.size(), 10);
	EXPECT_THROW_TGEN_PREFIX(g.gen(), "distinct: no more distinct values");
}

TEST(base_test, distinct_gen_list) {
	tgen::register_gen();

	tgen::distinct g([]() { return tgen::next(1, 10); });
	std::vector<int> values = g.gen_list(5).to_std();
	std::set<int> st;
	for (int val : values) {
		EXPECT_TRUE(1 <= val and val <= 10);
		st.insert(val);
	}

	EXPECT_EQ(st.size(), 5);
}

TEST(base_test, distinct_gen_all) {
	tgen::register_gen();

	tgen::distinct g([]() { return tgen::next(1, 10); });
	std::vector<int> values = g.gen_all().to_std();
	std::set<int> st;
	for (int val : values) {
		EXPECT_TRUE(1 <= val and val <= 10);
		st.insert(val);
	}

	EXPECT_EQ(st.size(), 10);
	EXPECT_THROW_TGEN_PREFIX(g.gen(), "distinct: no more distinct values");
}

TEST(base_test, print_scalar) {
	tgen::register_gen();

	testing::internal::CaptureStdout();
	std::cout << tgen::print(10);
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("10"));

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::string("str"));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("str"));
}

TEST(base_test, print_pair) {
	tgen::register_gen();

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::pair<std::string, double>("str", 0.1));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("str 0.1"));

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::pair<std::string, double>("str", 0.1), ',');
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("str,0.1"));
}

TEST(base_test, print_tuple) {
	tgen::register_gen();

	testing::internal::CaptureStdout();
	std::cout << tgen::print(
		std::tuple<int, double, std::string>(5, 0.1, "str"));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("5 0.1 str"));

	testing::internal::CaptureStdout();
	std::cout << tgen::print(
		std::tuple<int, double, std::string>(5, 0.1, "str"), ',');
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("5,0.1,str"));
}

TEST(base_test, print_1d_container) {
	tgen::register_gen();

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

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::array<int, 3>({1, 2, 3}), ',');
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("1,2,3"));
}

TEST(base_test, print_2d_container) {
	tgen::register_gen();

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
	std::cout << tgen::print(std::vector<std::pair<int, int>>({{1, 2}, {3, 4}}),
							 ',');
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("1,2\n3,4"));

	// Complex container 2.
	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::map<std::string, int>({{"ab", 2}, {"c", 4}}),
							 ',');
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("ab,2\nc,4"));

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

TEST(base_test, print_cols) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(std::cout
								 << tgen::print_cols(std::vector<int>({1, 2}),
													 std::vector<int>({1})),
							 "print_cols: sizes should be the same");

	testing::internal::CaptureStdout();
	std::cout << tgen::print_cols(std::vector<int>({1, 2}),
								  std::vector<int>({3, 4}));
	EXPECT_EQ(testing::internal::GetCapturedStdout(),
			  std::string("1 3\n2 4\n"));

	testing::internal::CaptureStdout();
	std::cout << tgen::print_cols(std::vector<int>({1, 2}),
								  std::vector<int>({3, 4}),
								  tgen::list<int>::value({5, 6}));
	EXPECT_EQ(testing::internal::GetCapturedStdout(),
			  std::string("1 3 5\n2 4 6\n"));
}

TEST(base_test, next_invalid_range) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::next<int>(2, 1),
							 "range for `next` must be valid");
}

TEST(base_test, next_val) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		auto n = tgen::next(10);
		EXPECT_TRUE(0 <= n and n < 10);
	}
	check_function_uniform([]() -> int { return tgen::next<int>(10); }, 10);
	check_function_uniform([]() -> int { return tgen::next<double>(2) < 1; },
						   2);
}

TEST(base_test, next_range) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		auto n = tgen::next(10, 20);
		EXPECT_TRUE(10 <= n and n <= 20);
	}
	check_function_uniform([](int l, int r) { return tgen::next<int>(l, r); },
						   11, 10, 20);
}

TEST(base_test, wnext_half_open_range) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		int a = tgen::wnext<int>(100, 0);
		int b = tgen::wnext<int>(100, 2);
		int c = tgen::wnext<int>(100, -2);
		EXPECT_TRUE(0 <= a and a < 100);
		EXPECT_TRUE(0 <= b and b < 100);
		EXPECT_TRUE(0 <= c and c < 100);
	}
	for (int i = 0; i < 100; ++i) {
		double x = tgen::wnext<double>(1.0, 0);
		double y = tgen::wnext<double>(1.0, 1);
		double z = tgen::wnext<double>(1.0, -1);
		EXPECT_TRUE(0 <= x and x < 1.0);
		EXPECT_TRUE(0 <= y and y < 1.0);
		EXPECT_TRUE(0 <= z and z < 1.0);
	}
}

TEST(base_test, wnext_closed_range) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		int a = tgen::wnext<int>(10, 20, 0);
		int b = tgen::wnext<int>(10, 20, 2);
		int c = tgen::wnext<int>(10, 20, -2);
		EXPECT_TRUE(10 <= a and a <= 20);
		EXPECT_TRUE(10 <= b and b <= 20);
		EXPECT_TRUE(10 <= c and c <= 20);
	}
}

TEST(base_test, wnext_weight_zero_uniform) {
	tgen::register_gen();

	check_function_uniform([]() -> int { return tgen::wnext<int>(10, 0); }, 10);
	check_function_uniform([]() { return tgen::wnext<int>(10, 20, 0); }, 11);
}

TEST(base_test, next_by_distribution) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		auto n = tgen::next_by_distribution({1, 2, 3});
		EXPECT_TRUE(0 <= n and n < 3);
	}
	check_function_uniform(
		[]() { return tgen::next_by_distribution({2, 2, 2}); }, 3);
}

TEST(base_test, many_by_distribution) {
	tgen::register_gen();

	for (int i = 0; i < 5; ++i) {
		auto vals = tgen::many_by_distribution<int>(3, {1, 2, 3});
		for (int j : vals) {
			EXPECT_TRUE(0 <= j and j < 3);
		}
	}
	check_function_uniform(
		[]() { return tgen::many_by_distribution<int>(2, {2, 2, 2}); }, 9);
}

TEST(base_test, shuffle) {
	tgen::register_gen();

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
	tgen::register_gen();

	std::vector<int> a = tgen::shuffled(std::vector<int>({1, 2, 3}));
	std::string b = tgen::shuffled(std::string("str"));
	std::vector<int> c = tgen::shuffled(std::set<int>({1, 2, 3}));
	std::vector<int> d = tgen::shuffled({1, 2, 3});
}

TEST(base_test, any) {
	tgen::register_gen();

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 10);

	for (int i = 0; i < 100; ++i) {
		int value = tgen::pick(v);
		EXPECT_TRUE(find(v.begin(), v.end(), value) != v.end());
	}
}

TEST(base_test, any_by_distribution) {
	tgen::register_gen();

	std::vector<int> v(3);
	for (int &i : v)
		i = tgen::next(1, 10);

	for (int i = 0; i < 10; ++i) {
		int value = tgen::pick_by_distribution(v, {1, 2, 3});
		EXPECT_TRUE(find(v.begin(), v.end(), value) != v.end());
	}
	check_function_uniform(
		[]() { return tgen::pick_by_distribution({1, 2, 3}, {2, 2, 2}); }, 3);
}

TEST(base_test, choose_invalid_ammount) {
	tgen::register_gen();

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 100);

	EXPECT_THROW_TGEN_PREFIX(tgen::choose(v, v.size() + 1),
							 "number of elements to choose must be valid");
}

TEST(base_test, choose) {
	tgen::register_gen();

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 10);

	for (int i = 0; i < 100; ++i) {
		int k = tgen::next<int>(1, v.size());
		auto subseq = tgen::choose(v, k);
		auto subseq_it = subseq.begin();
		// Tests if subseq is a subsequence of v.
		for (int j : v)
			if (subseq_it != subseq.end() and *subseq_it == j)
				++subseq_it;
		EXPECT_TRUE(subseq_it == subseq.end());
	}
}

TEST(base_test, distinct_range_generate_too_many_values) {
	tgen::register_gen();

	tgen::distinct_range d(1, 10);
	for (int i = 0; i < 10; ++i)
		d.gen();
	EXPECT_THROW_TGEN_PREFIX(d.gen(),
							 "distinct_range: no more values to generate");
}

TEST(base_test, distinct_range) {
	tgen::register_gen();

	tgen::distinct_range d(1, 10);
	std::set<int> s;
	for (int i = 0; i < 10; ++i) {
		int value = d.gen();
		EXPECT_TRUE(1 <= value and value <= 10);
		s.insert(value);
		EXPECT_EQ(d.size(), 10 - i - 1);
	}
	EXPECT_EQ(s.size(), 10);
	auto vec = tgen::distinct_range(1, 10).gen_all().to_std();
	EXPECT_EQ(std::set<int>(vec.begin(), vec.end()).size(), 10);
	vec = tgen::distinct_range(1, 10).gen_list(5).to_std();
	EXPECT_EQ(std::set<int>(vec.begin(), vec.end()).size(), 5);
}

TEST(base_test, distinct_container) {
	tgen::register_gen();

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 100);
	tgen::distinct_container d(v);
	for (int i = 0; i < 10; ++i) {
		int value = d.gen();
		EXPECT_TRUE(find(v.begin(), v.end(), value) != v.end());
		EXPECT_EQ(d.size(), v.size() - i - 1);
	}
	auto vec = tgen::distinct_container(v).gen_all().to_std();
	EXPECT_EQ(vec.size(), 10);
	vec = tgen::distinct_container(v).gen_list(5).to_std();
	EXPECT_EQ(vec.size(), 5);
}