#include <gtest/gtest.h>

#include "tgen.h"

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

TEST(opts_test, invalid_opts_empty_name_1) {
	char arg0[] = "./executable";
	char arg1[] = "-";
	char arg2[] = "n";
	char arg3[] = "10";
	char *argv[] = {arg0, arg1, arg2, arg3, nullptr};

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(4, argv), "invalid opt");
}

TEST(opts_test, invalid_opts_empty_name_2) {
	char arg0[] = "./executable";
	char arg1[] = "--";
	char arg2[] = "n";
	char arg3[] = "10";
	char *argv[] = {arg0, arg1, arg2, arg3, nullptr};

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(4, argv), "invalid opt");
}

TEST(opts_test, invalid_opts_empty_key_before_eq) {
	char arg0[] = "./executable";
	char arg1[] = "-=10";
	char *argv[] = {arg0, arg1, nullptr};

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(2, argv),
							 "expected non-empty key/value in opt");
}

TEST(opts_test, invalid_opts_empty_value_after_eq) {
	char arg0[] = "./executable";
	char arg1[] = "-n=";
	char *argv[] = {arg0, arg1, nullptr};

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(2, argv),
							 "expected non-empty key/value in opt");
}

TEST(opts_test, invalid_opts_empty_value_after_space) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char *argv[] = {arg0, arg1, nullptr};

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(2, argv),
							 "value cannot be empty");
}

TEST(opts_test, invalid_opts_repeated_key_equal) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "10";
	char arg3[] = "-n=20";
	char *argv[] = {arg0, arg1, arg2, arg3, nullptr};

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(4, argv),
							 "cannot have repeated keys");
}

TEST(opts_test, invalid_opts_repeated_key_space) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "10";
	char arg3[] = "-n";
	char arg4[] = "20";
	char *argv[] = {arg0, arg1, arg2, arg3, arg4, nullptr};

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(5, argv),
							 "cannot have repeated keys");
}

TEST(opts_test, invalid_opts_empty_value) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char *argv[] = {arg0, arg1, nullptr};

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(2, argv),
							 "value cannot be empty");
}

TEST(opts_test, has_opt_named) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "10";
	char *argv[] = {arg0, arg1, arg2, nullptr};
	tgen::register_gen(3, argv);

	EXPECT_EQ(tgen::has_opt("n"), true);
}

TEST(opts_test, has_opt_positional) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "10";
	char arg3[] = "-10";
	char *argv[] = {arg0, arg1, arg2, arg3, nullptr};
	tgen::register_gen(4, argv);

	EXPECT_EQ(tgen::has_opt(0), true);
	EXPECT_EQ(tgen::has_opt(1), false);
}

TEST(opts_test, opt_named_not_found) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "10";
	char arg3[] = "-10";
	char *argv[] = {arg0, arg1, arg2, arg3, nullptr};
	tgen::register_gen(4, argv);

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>("m"), "cannot find key with key m");
}

TEST(opts_test, opt_named_invalid_conversion) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "value";
	char arg3[] = "-10";
	char *argv[] = {arg0, arg1, arg2, arg3, nullptr};
	tgen::register_gen(4, argv);

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>("n"),
							 "invalid value `value` for type i");
}

TEST(opts_test, opt_named_invalid_conversion_bool) {
	char arg0[] = "./executable";
	char arg1[] = "-b";
	char arg2[] = "value";
	char arg3[] = "tru";
	char *argv[] = {arg0, arg1, arg2, arg3, nullptr};
	tgen::register_gen(4, argv);

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<bool>("b"),
							 "invalid value `value` for type b");
}

TEST(opts_test, opt_named) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "10";
	char arg3[] = "-10";
	char arg4[] = "-m";
	char arg5[] = "true";
	char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5, nullptr};
	tgen::register_gen(6, argv);

	EXPECT_EQ(tgen::opt<int>("n"), 10);
	EXPECT_EQ(tgen::opt<bool>("m"), true);
}

TEST(opts_test, opt_named_default) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "10";
	char arg3[] = "-10";
	char *argv[] = {arg0, arg1, arg2, arg3, nullptr};
	tgen::register_gen(4, argv);

	EXPECT_EQ(tgen::opt<int>("m", 20), 20);
}

TEST(opts_test, opt_positional_not_found) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "10";
	char arg3[] = "-10";
	char *argv[] = {arg0, arg1, arg2, arg3, nullptr};
	tgen::register_gen(4, argv);

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>(1), "cannot find key with index 1");
}

TEST(opts_test, opt_positional) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "10";
	char arg3[] = "-10";
	char *argv[] = {arg0, arg1, arg2, arg3, nullptr};
	tgen::register_gen(4, argv);

	EXPECT_EQ(tgen::opt<int>(0), -10);
}
