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

TEST(opts_test, invalid_opts_empty_name_1) {
	auto argv = get_argv({"./executable", "-", "n", "10"});

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(argv.size() - 1, argv.data()),
							 "invalid opt");
}

TEST(opts_test, invalid_opts_empty_name_2) {
	auto argv = get_argv({"./executable", "--", "n", "10"});

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(argv.size() - 1, argv.data()),
							 "invalid opt");
}

TEST(opts_test, invalid_opts_empty_key_before_eq) {
	auto argv = get_argv({"./executable", "-=10"});

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(argv.size() - 1, argv.data()),
							 "expected non-empty key/value in opt");
}

TEST(opts_test, invalid_opts_empty_value_after_eq) {
	auto argv = get_argv({"./executable", "-n="});

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(argv.size() - 1, argv.data()),
							 "expected non-empty key/value in opt");
}

TEST(opts_test, invalid_opts_empty_value_after_space) {
	auto argv = get_argv({"./executable", "-n"});

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(argv.size() - 1, argv.data()),
							 "value cannot be empty");
}

TEST(opts_test, invalid_opts_repeated_key_equal) {
	auto argv = get_argv({"./executable", "-n", "10", "-n=20"});

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(argv.size() - 1, argv.data()),
							 "cannot have repeated keys");
}

TEST(opts_test, invalid_opts_repeated_key_space) {
	auto argv = get_argv({"./executable", "-n", "10", "-n", "20"});

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(argv.size() - 1, argv.data()),
							 "cannot have repeated keys");
}

TEST(opts_test, invalid_opts_empty_value) {
	auto argv = get_argv({"./executable", "-n"});

	EXPECT_THROW_TGEN_PREFIX(tgen::register_gen(argv.size() - 1, argv.data()),
							 "value cannot be empty");
}

TEST(opts_test, has_opt_named) {
	auto argv = get_argv({"./executable", "-n", "10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::has_opt("n"), true);
}

TEST(opts_test, has_opt_positional) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::has_opt(0), true);
	EXPECT_EQ(tgen::has_opt(1), false);
}

TEST(opts_test, opt_named_not_found) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>("m"), "cannot find key with key m");
}

TEST(opts_test, opt_named_invalid_conversion) {
	auto argv = get_argv({"./executable", "-n", "value", "-10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>("n"),
							 "invalid value `value` for type i");
}

TEST(opts_test, opt_named_invalid_conversion_bool) {
	auto argv = get_argv({"./executable", "-b", "tru"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<bool>("b"),
							 "invalid value `tru` for type b");
}

TEST(opts_test, opt_named) {
	auto argv = get_argv({"./executable", "-n", "10", "-10", "-m", "true"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::opt<int>("n"), 10);
	EXPECT_EQ(tgen::opt<bool>("m"), true);
}

TEST(opts_test, opt_named_default) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::opt<int>("m", 20), 20);
}

TEST(opts_test, opt_positional_not_found) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>(1), "cannot find key with index 1");
}

TEST(opts_test, opt_positional) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::opt<int>(0), -10);
}
