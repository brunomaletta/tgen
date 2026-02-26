#include <gtest/gtest.h>

#include "tgen.h"

#include <algorithm>
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

TEST(general_test, print_scalar) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	testing::internal::CaptureStdout();
	std::cout << tgen::print(10);
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("10"));

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::string("str"));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("str"));
}

TEST(general_test, print_pair) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	testing::internal::CaptureStdout();
	std::cout << tgen::print(std::pair<std::string, double>("str", 0.1));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("str 0.1"));
}

TEST(general_test, print_tuple) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	testing::internal::CaptureStdout();
	std::cout << tgen::print(
		std::tuple<int, double, std::string>(5, 0.1, "str"));
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("5 0.1 str"));
}

TEST(general_test, print_1d_container) {
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

TEST(general_test, print_2d_container) {
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

TEST(general_test, next_invalid_range) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::next<int>(2, 1),
							 "range for `next` bust be valid");
}

TEST(general_test, shuffle_check_values) {
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

TEST(general_test, any_check_value) {
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

TEST(general_test, choose_invalid_ammount) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 100);

	EXPECT_THROW_TGEN_PREFIX(tgen::choose(v.size() + 1, v),
							 "number of elements to choose must be valid");
}

TEST(general_test, choose_check_subsequence) {
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
