#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

TEST(opts_test, did_not_register_opt) {
	// Fake reset registered status.
	tgen::detail::registered = false;
	EXPECT_THROW_TGEN_PREFIX(
		tgen::has_opt(0),
		"tgen was not registered! You should call tgen::register_gen(argc, "
		"argv) before running tgen functions");
}

TEST(opts_test, did_not_register_next) {
	// Fake reset registered status.
	tgen::detail::registered = false;
	EXPECT_THROW_TGEN_PREFIX(
		tgen::next(0, 1),
		"tgen was not registered! You should call tgen::register_gen(argc, "
		"argv) before running tgen functions");
}

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

TEST(opts_test, has_opt_named) {
	auto argv = get_argv({"./executable", "-n", "10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::has_opt("n"), true);
	EXPECT_EQ(tgen::has_opt("m"), false);
}

TEST(opts_test, has_opt_named_char) {
	auto argv = get_argv({"./executable", "-n", "10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::has_opt('n'), true);
	EXPECT_EQ(tgen::has_opt('m'), false);
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

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>("m"), "cannot find opt with key m");
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

TEST(opts_test, opt_named_char) {
	auto argv = get_argv({"./executable", "-n", "10", "-10", "-m", "true"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::opt<int>('n'), 10);
	EXPECT_EQ(tgen::opt<bool>('m'), true);
}

TEST(opts_test, opt_named_default) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::opt<int>("m", 20), 20);
}

TEST(opts_test, opt_positional_not_found) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>(1), "cannot find opt at index 1");
}

TEST(opts_test, opt_positional) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::opt<int>(0), -10);
}

TEST(opts_test, register_gen_with_seed) {
	// Fake reset registered status.
	tgen::detail::registered = false;
	tgen::register_gen(42);

	EXPECT_TRUE(tgen::detail::registered);
}

TEST(opts_test, register_gen_with_default_seed) {
	// Fake reset registered status.
	tgen::detail::registered = false;
	tgen::register_gen();

	EXPECT_TRUE(tgen::detail::registered);
}

TEST(opts_test, set_cpp_version) {
	tgen::register_gen();
	tgen::set_cpp_version(17);

	EXPECT_EQ(tgen::detail::cpp.version_, 17);
}

TEST(opts_test, set_cpp_version_opt) {
	auto argv = get_argv({"./executable", "tgen::CPP:20"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::detail::cpp.version_, 20);
}

TEST(opts_test, set_compiler) {
	tgen::register_gen();

	tgen::set_compiler(tgen::gcc());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::gcc().kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::gcc().major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::gcc().minor_);

	tgen::set_compiler(tgen::clang());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::clang().kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::clang().major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::clang().minor_);

	tgen::set_compiler(tgen::gcc(17));
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::gcc(17).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::gcc(17).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::gcc(17).minor_);

	tgen::set_compiler(tgen::clang(17));
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::clang(17).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::clang(17).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::clang(17).minor_);

	tgen::set_compiler(tgen::gcc(17, 2));
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::gcc(17, 2).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::gcc(17, 2).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::gcc(17, 2).minor_);

	tgen::set_compiler(tgen::clang(17, 2));
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::clang(17, 2).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::clang(17, 2).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::clang(17, 2).minor_);
}

TEST(opts_test, set_compiler_opt) {
	auto argv = get_argv({"./executable", "tgen::GCC"});
	tgen::register_gen(argv.size() - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::gcc().kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::gcc().major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::gcc().minor_);

	argv = get_argv({"./executable", "tgen::CLANG"});
	tgen::register_gen(argv.size() - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::clang().kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::clang().major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::clang().minor_);

	argv = get_argv({"./executable", "tgen::GCC:17"});
	tgen::register_gen(argv.size() - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::gcc(17).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::gcc(17).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::gcc(17).minor_);

	argv = get_argv({"./executable", "tgen::CLANG:17"});
	tgen::register_gen(argv.size() - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::clang(17).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::clang(17).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::clang(17).minor_);

	argv = get_argv({"./executable", "tgen::GCC:17.2"});
	tgen::register_gen(argv.size() - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::gcc(17, 2).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::gcc(17, 2).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::gcc(17, 2).minor_);

	argv = get_argv({"./executable", "tgen::CLANG:17.2"});
	tgen::register_gen(argv.size() - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::clang(17, 2).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::clang(17, 2).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::clang(17, 2).minor_);
}