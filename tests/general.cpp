#include <gtest/gtest.h>

#include "tgen.h"

inline void EXPECT_STARTS_WITH(const std::string &msg,
							   const std::string &prefix) {
	EXPECT_TRUE(msg.rfind(prefix, 0) == 0)
		<< "Expected prefix: \"" << prefix << "\"\n"
		<< "Actual message: \"" << msg << "\"";
}

TEST(general_test, next_invalid_range) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	try {
		int nxt = tgen::next<int>(2, 1);
		FAIL() << "Expected std::runtime_error, but no error ocurred";
	} catch (const std::runtime_error &e) {
		EXPECT_STARTS_WITH(e.what(), "tgen: range for `next` bust be valid");
	} catch (...) {
		FAIL() << "Expected std::runtime_error, but caught a different error";
	}
}

TEST(general_test, shuffle_check_values) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 10);
	auto v_sorted = v;
	sort(v_sorted.begin(), v_sorted.end());

	for (int i = 0; i < 100; ++i) {
		tgen::shuffle(v);
		std::sort(v.begin(), v.end());
		EXPECT_TRUE(v == v_sorted);
	}
}

TEST(general_test, any_check_value) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 10);

	for (int i = 0; i < 100; ++i) {
		int value = tgen::any(v);
		EXPECT_TRUE(find(v.begin(), v.end(), value) != v.end());
	}
}

TEST(general_test, choose_invalid_ammount) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 100);

	try {
		v = tgen::choose(v.size() + 1, v);
		FAIL() << "Expected std::runtime_error, but no error ocurred";
	} catch (const std::runtime_error &e) {
		EXPECT_STARTS_WITH(e.what(),
						   "tgen: number of elements to choose must be valid");
	} catch (...) {
		FAIL() << "Expected std::runtime_error, but caught a different error";
	}
}

TEST(general_test, choose_check_subsequence) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	std::vector<int> v(10);
	for (int &i : v)
		i = tgen::next(1, 10);

	for (int i = 0; i < 100; ++i) {
		int k = tgen::next<int>(1, v.size());
		auto subseq = tgen::choose(k, v);
		auto subseq_it = subseq.begin();
		// Tests if subseq is a subsequence of v.
		for (int i : v)
			if (subseq_it != subseq.end() and *subseq_it == i)
				++subseq_it;
		EXPECT_TRUE(subseq_it == subseq.end());
	}
}
