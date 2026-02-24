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

const uint64_t largest_prime_64 = 18446744073709551557ULL;
const uint64_t largest_number_64 = std::numeric_limits<uint64_t>::max();
const uint64_t fft_mod = 998244353;

/*
 * Tests.
 */

TEST(math_test, is_prime) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int n = -10; n < 2; ++n)
		EXPECT_EQ(tgen::math::is_prime(n), false);
	EXPECT_EQ(tgen::math::is_prime(2), true);
	EXPECT_EQ(tgen::math::is_prime(3), true);
	EXPECT_EQ(tgen::math::is_prime(4), false);
	EXPECT_EQ(tgen::math::is_prime(5), true);
	EXPECT_EQ(tgen::math::is_prime(6), false);
	EXPECT_EQ(tgen::math::is_prime(101), true);
	EXPECT_EQ(tgen::math::is_prime(1e9 + 7), true);
	EXPECT_EQ(tgen::math::is_prime(fft_mod), true);
	EXPECT_EQ(tgen::math::is_prime(largest_prime_64), true);
	EXPECT_EQ(tgen::math::is_prime(largest_number_64), false);
}

TEST(math_test, factor) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	using vec = std::vector<uint64_t>;

	EXPECT_THROW_TGEN_PREFIX(tgen::math::factor(0),
							 "number to factor must be positive");

	EXPECT_EQ(tgen::math::factor(1), vec({}));
	EXPECT_EQ(tgen::math::factor(2), vec({2}));
	EXPECT_EQ(tgen::math::factor(4), vec({2, 2}));
	EXPECT_EQ(tgen::math::factor(5), vec({5}));
	EXPECT_EQ(tgen::math::factor(6), vec({2, 3}));
	EXPECT_EQ(tgen::math::factor(12), vec({2, 2, 3}));
	EXPECT_EQ(tgen::math::factor(largest_prime_64), vec({largest_prime_64}));
	EXPECT_EQ(tgen::math::factor(largest_number_64),
			  vec({3, 5, 17, 257, 641, 65537, 6700417}));
	EXPECT_EQ(tgen::math::factor(fft_mod - 1),
			  vec({2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
				   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7, 17}));
}

TEST(math_test, factor_by_prime) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	using vec = std::vector<std::pair<uint64_t, int>>;

	EXPECT_THROW_TGEN_PREFIX(tgen::math::factor_by_prime(0),
							 "number to factor must be positive");

	EXPECT_EQ(tgen::math::factor_by_prime(1), vec({}));
	EXPECT_EQ(tgen::math::factor_by_prime(2), vec({{2, 1}}));
	EXPECT_EQ(tgen::math::factor_by_prime(4), vec({{2, 2}}));
	EXPECT_EQ(tgen::math::factor_by_prime(5), vec({{5, 1}}));
	EXPECT_EQ(tgen::math::factor_by_prime(6), vec({{2, 1}, {3, 1}}));
	EXPECT_EQ(tgen::math::factor_by_prime(12), vec({{2, 2}, {3, 1}}));
	EXPECT_EQ(tgen::math::factor_by_prime(largest_prime_64),
			  vec({{largest_prime_64, 1}}));
	EXPECT_EQ(tgen::math::factor_by_prime(largest_number_64),
			  vec({{3, 1},
				   {5, 1},
				   {17, 1},
				   {257, 1},
				   {641, 1},
				   {65537, 1},
				   {6700417, 1}}));
	EXPECT_EQ(tgen::math::factor_by_prime(fft_mod - 1),
			  vec({{2, 23}, {7, 1}, {17, 1}}));
}

TEST(math_test, modular_inverse_invalid) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::modular_inverse(0, 5),
		"remainder must be positive and smaller than the mod");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::modular_inverse(5, 5),
		"remainder must be positive and smaller than the mod");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::modular_inverse(2, 6),
							 "remainder and mod must be coprime");
}

TEST(math_test, modular_inverse) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::math::modular_inverse(1, 2), 1);
	EXPECT_EQ(tgen::math::modular_inverse(5, 6), 5);
	EXPECT_EQ(tgen::math::modular_inverse(10, 1e9 + 7), 700000005);
}

TEST(math_test, totient) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::math::totient(0), "totient(0) is undefined");

	EXPECT_EQ(tgen::math::totient(1), 1);
	EXPECT_EQ(tgen::math::totient(2), 1);
	EXPECT_EQ(tgen::math::totient(3), 2);
	EXPECT_EQ(tgen::math::totient(17), 16);
	EXPECT_EQ(tgen::math::totient(100), 40);
	EXPECT_EQ(tgen::math::totient(largest_prime_64), largest_prime_64 - 1);
	EXPECT_EQ(tgen::math::totient(largest_number_64), 9208981628670443520);
}

TEST(math_test, gen_prime_no_prime) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_prime(2, 1),
							 "there is no prime in range [2, 1]");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_prime(0, 1),
							 "there is no prime in range [0, 1]");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_prime(6787988999657777798, 6787988999657779306),
		"there is no prime in range [6787988999657777798, "
		"6787988999657779306]");
}

TEST(math_test, gen_prime) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		EXPECT_TRUE(tgen::math::is_prime(
			tgen::math::gen_prime(0, largest_number_64, i % 2 == 0)));
	}
}

TEST(math_test, next_prime) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::math::next_prime(largest_number_64 - 58),
							 "invalid bound");
	EXPECT_EQ(tgen::math::next_prime(largest_number_64 - 59), largest_prime_64);

	auto p = tgen::math::gen_prime(0, largest_number_64 / 100);
	EXPECT_TRUE(tgen::math::is_prime(p));
	for (int i = 0; i < 100; ++i) {
		EXPECT_THROW_TGEN_PREFIX(
			tgen::math::gen_prime(p + 1, tgen::math::next_prime(p) - 1),
			"there is no prime in range");
		p = tgen::math::next_prime(p);
		EXPECT_TRUE(tgen::math::is_prime(p));
	}
}

TEST(math_test, prev_prime) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::math::prev_prime(2),
							 "there is no prime up to 2");

	auto p = tgen::math::gen_prime(largest_number_64 / 100, largest_number_64);
	EXPECT_TRUE(tgen::math::is_prime(p));
	for (int i = 0; i < 100; ++i) {
		EXPECT_THROW_TGEN_PREFIX(
			tgen::math::gen_prime(tgen::math::prev_prime(p) + 1, p - 1),
			"there is no prime in range");
		p = tgen::math::prev_prime(p);
		EXPECT_TRUE(tgen::math::is_prime(p));
	}
}
