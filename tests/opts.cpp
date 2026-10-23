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

	EXPECT_THROW_TGEN_PREFIX(
		tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data()),
		"invalid opt");
}

TEST(opts_test, invalid_opts_empty_name_2) {
	auto argv = get_argv({"./executable", "--", "n", "10"});

	EXPECT_THROW_TGEN_PREFIX(
		tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data()),
		"invalid opt");
}

TEST(opts_test, invalid_opts_empty_key_before_eq) {
	auto argv = get_argv({"./executable", "-=10"});

	EXPECT_THROW_TGEN_PREFIX(
		tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data()),
		"expected non-empty key/value in opt");
}

TEST(opts_test, invalid_opts_empty_value_after_eq) {
	auto argv = get_argv({"./executable", "-n="});

	EXPECT_THROW_TGEN_PREFIX(
		tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data()),
		"expected non-empty key/value in opt");
}

TEST(opts_test, invalid_opts_empty_value_after_space) {
	auto argv = get_argv({"./executable", "-n"});

	EXPECT_THROW_TGEN_PREFIX(
		tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data()),
		"value cannot be empty");
}

TEST(opts_test, invalid_opts_repeated_key_equal) {
	auto argv = get_argv({"./executable", "-n", "10", "-n=20"});

	EXPECT_THROW_TGEN_PREFIX(
		tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data()),
		"cannot have repeated keys");
}

TEST(opts_test, invalid_opts_repeated_key_space) {
	auto argv = get_argv({"./executable", "-n", "10", "-n", "20"});

	EXPECT_THROW_TGEN_PREFIX(
		tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data()),
		"cannot have repeated keys");
}

TEST(opts_test, has_opt_named) {
	auto argv = get_argv({"./executable", "-n", "10"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_EQ(tgen::has_opt("n"), true);
	EXPECT_EQ(tgen::has_opt("m"), false);
}

TEST(opts_test, has_opt_named_char) {
	auto argv = get_argv({"./executable", "-n", "10"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_EQ(tgen::has_opt('n'), true);
	EXPECT_EQ(tgen::has_opt('m'), false);
}

TEST(opts_test, has_opt_positional) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_EQ(tgen::has_opt(0), true);
	EXPECT_EQ(tgen::has_opt(1), false);
}

TEST(opts_test, opt_named_not_found) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>("m"), "cannot find opt with key m");
}

TEST(opts_test, opt_named_invalid_conversion) {
	auto argv = get_argv({"./executable", "-n", "value", "-10"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>("n"),
							 "invalid value `value` for type i");
}

TEST(opts_test, opt_named_invalid_conversion_bool) {
	auto argv = get_argv({"./executable", "-b", "tru"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<bool>("b"),
							 "invalid value `tru` for type b");
}

TEST(opts_test, opt_named) {
	auto argv = get_argv({"./executable", "-n", "10", "-10", "-m", "true"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_EQ(tgen::opt<int>("n"), 10);
	EXPECT_EQ(tgen::opt<bool>("m"), true);
}

TEST(opts_test, opt_named_char) {
	auto argv = get_argv({"./executable", "-n", "10", "-10", "-m", "true"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_EQ(tgen::opt<int>('n'), 10);
	EXPECT_EQ(tgen::opt<bool>('m'), true);
}

TEST(opts_test, opt_named_default) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_EQ(tgen::opt<int>("m", 20), 20);
}

TEST(opts_test, opt_positional_not_found) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>(1), "cannot find opt at index 1");
}

TEST(opts_test, opt_positional) {
	auto argv = get_argv({"./executable", "-n", "10", "-10"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

	EXPECT_EQ(tgen::opt<int>(0), -10);
}

TEST(opts_test, register_gen_with_seed) {
	// Fake reset registered status.
	tgen::detail::registered = false;
	tgen::register_gen(42);

	EXPECT_TRUE(tgen::detail::registered);
}

TEST(opts_test, register_gen_with_stress_seed_offset) {
	const long long old_stress_seed = tgen::test_detail::stress_seed;

	tgen::test_detail::stress_seed = 0;
	tgen::register_gen(43);
	const int expected = tgen::next(1, 1000000);

	tgen::test_detail::stress_seed = 1;
	tgen::register_gen(42);
	EXPECT_EQ(tgen::next(1, 1000000), expected);

	tgen::test_detail::stress_seed = old_stress_seed;
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
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());

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
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::gcc().kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::gcc().major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::gcc().minor_);

	argv = get_argv({"./executable", "tgen::CLANG"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::clang().kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::clang().major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::clang().minor_);

	argv = get_argv({"./executable", "tgen::GCC:17"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::gcc(17).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::gcc(17).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::gcc(17).minor_);

	argv = get_argv({"./executable", "tgen::CLANG:17"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::clang(17).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::clang(17).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::clang(17).minor_);

	argv = get_argv({"./executable", "tgen::GCC:17.2"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::gcc(17, 2).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::gcc(17, 2).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::gcc(17, 2).minor_);

	argv = get_argv({"./executable", "tgen::CLANG:17.2"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_EQ(tgen::detail::compiler.kind_, tgen::clang(17, 2).kind_);
	EXPECT_EQ(tgen::detail::compiler.major_, tgen::clang(17, 2).major_);
	EXPECT_EQ(tgen::detail::compiler.minor_, tgen::clang(17, 2).minor_);
}
TEST(opts_test, regression_numeric_option_narrowing) {
	auto argv =
		get_argv({"./executable", "2147483648", "-2147483649", "256", "128"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>(0), "invalid value");
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>(1), "invalid value");
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<unsigned char>(2), "invalid value");
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<signed char>(3), "invalid value");
}

TEST(opts_test, regression_numeric_option_suffix) {
	auto argv =
		get_argv({"./executable", "--n=12junk", "--x=1.5junk", "--y=12 "});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<int>("n"), "invalid value");
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<double>("x"), "invalid value");
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<unsigned>("y"), "invalid value");
	EXPECT_EQ(tgen::opt<std::string>("n"), "12junk");
}

TEST(opts_test, regression_negative_unsigned_option) {
	auto argv = get_argv({"./executable", "-1", "--n= -1", "--zero=-0"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<unsigned>(0), "invalid value");
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<unsigned long long>("n"),
							 "invalid value");
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<unsigned>("zero"), "invalid value");
}

TEST(opts_test, regression_floating_option_range) {
	auto argv = get_argv({"./executable", "1e40", "1e-100", "--double=1e400"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<float>(0), "invalid value");
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<float>(1), "invalid value");
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<double>("double"), "invalid value");
}

TEST(opts_test, regression_numeric_option_boundaries) {
	auto argv =
		get_argv({"./executable", "-9223372036854775808", "9223372036854775807",
				  "18446744073709551615", "--plus=+12", "--real=1.25e2",
				  "--tiny=1.401298464324817070923729583289916131280e-45",
				  "--inf=inf", "--nan=nan"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_EQ(tgen::opt<long long>(0), std::numeric_limits<long long>::min());
	EXPECT_EQ(tgen::opt<long long>(1), std::numeric_limits<long long>::max());
	EXPECT_EQ(tgen::opt<unsigned long long>(2),
			  std::numeric_limits<unsigned long long>::max());
	EXPECT_EQ(tgen::opt<unsigned>("plus"), 12);
	EXPECT_EQ(tgen::opt<double>("real"), 125);
	EXPECT_EQ(tgen::opt<float>("tiny"),
			  std::numeric_limits<float>::denorm_min());
	EXPECT_TRUE(std::isinf(tgen::opt<double>("inf")));
	EXPECT_TRUE(std::isnan(tgen::opt<double>("nan")));
}

TEST(opts_test, regression_long_double_subnormal_option) {
	std::ostringstream out;
	out << std::scientific
		<< std::setprecision(std::numeric_limits<long double>::max_digits10)
		<< std::numeric_limits<long double>::denorm_min();
	std::string tiny = out.str();
	auto argv = get_argv({"./executable", tiny.c_str(), "--overflow=1e100000",
						  "--underflow=1e-100000"});
	tgen::register_gen(static_cast<int>(argv.size()) - 1, argv.data());
	EXPECT_EQ(tgen::opt<long double>(0),
			  std::numeric_limits<long double>::denorm_min());
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<long double>("overflow"),
							 "invalid value");
	EXPECT_THROW_TGEN_PREFIX(tgen::opt<long double>("underflow"),
							 "invalid value");
}
