#include <gtest/gtest.h>

#include "tgen.h"

inline void EXPECT_STARTS_WITH(const std::string &msg,
							   const std::string &prefix) {
	EXPECT_TRUE(msg.rfind(prefix, 0) == 0)
		<< "Expected prefix: \"" << prefix << "\"\n"
		<< "Actual message: \"" << msg << "\"";
}

TEST(general_test, sequence_constructor_size_zero) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	try {
		auto s = tgen::sequence<int>(0, 1, 10);
		FAIL() << "Expected std::runtime_error, but no error ocurred";
	} catch (const std::runtime_error &e) {
		EXPECT_STARTS_WITH(e.what(), "tgen: size must be positive");
	} catch (...) {
		FAIL() << "Expected std::runtime_error, but caught a different error";
	}
}

TEST(general_test, sequence_constructor_invalid_range) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	try {
		auto s = tgen::sequence<int>(10, 2, 1);
		FAIL() << "Expected std::runtime_error, but no error ocurred";
	} catch (const std::runtime_error &e) {
		EXPECT_STARTS_WITH(e.what(), "tgen: value range must be valid");
	} catch (...) {
		FAIL() << "Expected std::runtime_error, but caught a different error";
	}
}

TEST(general_test, sequence_constructor_empty_set) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	try {
		auto s = tgen::sequence<int>(10, {});
		FAIL() << "Expected std::runtime_error, but no error ocurred";
	} catch (const std::runtime_error &e) {
		EXPECT_STARTS_WITH(e.what(), "tgen: value set must be non-empty");
	} catch (...) {
		FAIL() << "Expected std::runtime_error, but caught a different error";
	}
}

TEST(general_test, gen_no_restrictions) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	auto s = tgen::sequence<int>(10, 1, 10);

	for (int i = 0; i < 100; ++i) {
		auto v = s.gen();
		for (int j = 0; j < 10; ++j)
			EXPECT_TRUE(1 <= v[j] and v[j] <= 10);
	}
}

TEST(general_test, gen_no_restrictions_corners) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

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

TEST(general_test, gen_with_set) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	for (int i = 0; i < 100; ++i) {
		int num_op = tgen::next(1, 5);
		std::vector<int> set_idx(10, 0), vals(10);
		for (int j = 0; j < num_op; ++j)
			set_idx[j] = 1;
		tgen::shuffle(set_idx);

		auto s = tgen::sequence<int>(10, 1, 10);
		for (int j = 0; j < num_op; ++j) if (set_idx[j]) {
			vals[j] = tgen::next(1, 10);
			s.set(j, vals[j]);
		}

		auto v = s.gen();
		for (int j = 0; j < num_op; ++j) if (set_idx[j])
			EXPECT_EQ(v[j], vals[j]);
	}
}
