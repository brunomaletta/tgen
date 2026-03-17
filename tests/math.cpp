#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

inline constexpr uint64_t largest_prime_64 = 18446744073709551557ULL;
inline constexpr uint64_t largest_number_64 =
	std::numeric_limits<uint64_t>::max();
inline constexpr uint64_t fft_mod = 998244353;

inline const std::pair<std::vector<uint64_t>, std::vector<uint64_t>> &
prime_gaps() {
	static const std::pair<std::vector<uint64_t>, std::vector<uint64_t>> value{
		/* clang-format off */ {
			2, 3, 7, 23, 89, 113, 523, 887, 1129, 1327, 9551, 15683, 19609,
			31397, 155921, 360653, 370261, 492113, 1349533, 1357201, 2010733,
			4652353, 17051707, 20831323, 47326693, 122164747, 189695659,
			191912783, 387096133, 436273009, 1294268491, 1453168141,
			2300942549, 3842610773, 4302407359, 10726904659, 20678048297,
			22367084959, 25056082087, 42652618343, 127976334671, 182226896239,
			241160624143, 297501075799, 303371455241, 304599508537,
			416608695821, 461690510011, 614487453523, 738832927927,
			1346294310749, 1408695493609, 1968188556461, 2614941710599,
			7177162611713, 13829048559701, 19581334192423, 42842283925351,
			90874329411493, 171231342420521, 218209405436543, 1189459969825483,
			1686994940955803, 1693182318746371, 43841547845541059,
			55350776431903243, 80873624627234849, 203986478517455989,
			218034721194214273, 305405826521087869, 352521223451364323,
			401429925999153707, 418032645936712127, 804212830686677669,
			1425172824437699411, 5733241593241196731, 6787988999657777797
		}, /* clang-format on */
		{1,	   2,	 4,	   6,	 8,	   14,	 18,   20,	 22,   34,	 36,
		 44,   52,	 72,   86,	 96,   112,	 114,  118,	 132,  148,	 154,
		 180,  210,	 220,  222,	 234,  248,	 250,  282,	 288,  292,	 320,
		 336,  354,	 382,  384,	 394,  456,	 464,  468,	 474,  486,	 490,
		 500,  514,	 516,  532,	 534,  540,	 582,  588,	 602,  652,	 674,
		 716,  766,	 778,  804,	 806,  906,	 916,  924,	 1132, 1184, 1198,
		 1220, 1224, 1248, 1272, 1328, 1356, 1370, 1442, 1476, 1488, 1510}};

	return value;
}

inline const std::vector<uint64_t> &highly_composites() {
	/* clang-format off */
	static const std::vector<uint64_t> highly_composites_internal = {
	1, 2, 4, 6, 12, 24, 36, 48, 60, 120, 180, 240, 360, 720, 840, 1260, 1680,
	2520, 5040, 7560, 10080, 15120, 20160, 25200, 27720, 45360, 50400, 55440,
	83160, 110880, 166320, 221760, 277200, 332640, 498960, 554400, 665280,
	720720, 1081080, 1441440, 2162160, 2882880, 3603600, 4324320, 6486480,
	7207200, 8648640, 10810800, 14414400, 17297280, 21621600, 32432400,
	36756720, 43243200, 61261200, 73513440, 110270160, 122522400, 147026880,
	183783600, 245044800, 294053760, 367567200, 551350800, 698377680, 735134400,
	1102701600, 1396755360, 2095133040, 2205403200, 2327925600, 2793510720,
	3491888400, 4655851200, 5587021440, 6983776800, 10475665200, 13967553600,
	20951330400, 27935107200, 41902660800, 48886437600, 64250746560,
	73329656400, 80313433200, 97772875200, 128501493120, 146659312800,
	160626866400, 240940299600, 293318625600, 321253732800, 481880599200,
	642507465600, 963761198400, 1124388064800, 1606268664000, 1686582097200,
	1927522396800, 2248776129600, 3212537328000, 3373164194400, 4497552259200,
	6746328388800, 8995104518400, 9316358251200, 13492656777600, 18632716502400,
	26985313555200, 27949074753600, 32607253879200, 46581791256000,
	48910880818800, 55898149507200, 65214507758400, 93163582512000,
	97821761637600, 130429015516800, 195643523275200, 260858031033600,
	288807105787200, 391287046550400, 577614211574400, 782574093100800,
	866421317361600, 1010824870255200, 1444035528936000, 1516237305382800,
	1732842634723200, 2021649740510400, 2888071057872000, 3032474610765600,
	4043299481020800, 6064949221531200, 8086598962041600, 10108248702552000,
	12129898443062400, 18194847664593600, 20216497405104000, 24259796886124800,
	30324746107656000, 36389695329187200, 48519593772249600, 60649492215312000,
	72779390658374400, 74801040398884800, 106858629141264000,
	112201560598327200, 149602080797769600, 224403121196654400,
	299204161595539200, 374005201994424000, 448806242393308800,
	673209363589963200, 748010403988848000, 897612484786617600,
	1122015605983272000, 1346418727179926400, 1795224969573235200,
	2244031211966544000, 2692837454359852800, 3066842656354276800,
	4381203794791824000, 4488062423933088000, 6133685312708553600,
	8976124847866176000, 9200527969062830400, 12267370625417107200ULL,
	15334213281771384000ULL, 18401055938125660800ULL}; /* clang-format on */
	return highly_composites_internal;
}

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
							 "math: number to factor must be positive");

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
							 "math: number to factor must be positive");

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
		"math: remainder must be positive and smaller than the mod");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::modular_inverse(5, 5),
		"math: remainder must be positive and smaller than the mod");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::modular_inverse(2, 6),
							 "math: remainder and mod must be coprime");
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

	EXPECT_THROW_TGEN_PREFIX(tgen::math::totient(0),
							 "math: totient(0) is undefined");

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
							 "math: there is no prime in range [2, 1]");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_prime(0, 1),
							 "math: there is no prime in range [0, 1]");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_prime(6787988999657777798, 6787988999657779306),
		"math: there is no prime in range [6787988999657777798, "
		"6787988999657779306]");
}

TEST(math_test, gen_prime) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		auto p = tgen::math::gen_prime(0, largest_number_64);
		EXPECT_TRUE(tgen::math::is_prime(p));
	}

	for (int i = 0; i < 100; ++i) {
		auto l = tgen::next<uint64_t>(0, largest_number_64 / 100);
		auto r =
			tgen::next<uint64_t>(largest_number_64 / 100, largest_number_64);

		auto p = tgen::math::gen_prime(l, r);
		EXPECT_TRUE(tgen::math::is_prime(p));
		EXPECT_TRUE(l <= p and p <= r);
	}
}

TEST(math_test, gen_prime_uniform) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	check_function_uniform(tgen::math::gen_prime, 25, 0, 100);
	check_function_uniform(tgen::math::gen_prime, 168, 0, 1000);
}

TEST(math_test, prime_from) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::math::prime_from(largest_number_64 - 57),
							 "math: invalid bound");
	EXPECT_EQ(tgen::math::prime_from(largest_number_64 - 58), largest_prime_64);

	auto p = tgen::math::gen_prime(0, largest_number_64 / 100);
	EXPECT_TRUE(tgen::math::is_prime(p));
	for (int i = 0; i < 100; ++i) {
		EXPECT_THROW_TGEN_PREFIX(
			tgen::math::gen_prime(p + 1, tgen::math::prime_from(p + 1) - 1),
			"math: there is no prime in range");
		p = tgen::math::prime_from(p + 1);
		EXPECT_TRUE(tgen::math::is_prime(p));
	}
}

TEST(math_test, prime_upto) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::math::prime_upto(1),
							 "math: there is no prime up to 1");

	auto p = tgen::math::gen_prime(largest_number_64 / 100, largest_number_64);
	EXPECT_TRUE(tgen::math::is_prime(p));
	for (int i = 0; i < 100; ++i) {
		EXPECT_THROW_TGEN_PREFIX(
			tgen::math::gen_prime(tgen::math::prime_upto(p - 1) + 1, p - 1),
			"math: there is no prime in range");
		p = tgen::math::prime_upto(p - 1);
		EXPECT_TRUE(tgen::math::is_prime(p));
	}
}

TEST(math_test, num_divisors) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::math::num_divisors(0),
							 "math: number to factor must be positive");
	EXPECT_EQ(tgen::math::num_divisors(1), 1);
	EXPECT_EQ(tgen::math::num_divisors(2), 2);
	EXPECT_EQ(tgen::math::num_divisors(12), 6);
	EXPECT_EQ(tgen::math::num_divisors(largest_prime_64), 2);
	EXPECT_EQ(tgen::math::num_divisors(largest_number_64), 128);
}

TEST(math_test, gen_divisor_count) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_divisor_count(1, 10, -1),
							 "math: divisor count must be prime");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_divisor_count(0, 0, 2),
							 "math: there is no prime in range [0, 0]");

	for (int i = 0; i < 100; ++i) {
		auto p = tgen::math::gen_prime(0, 30);
		auto x = tgen::math::gen_divisor_count(0, largest_number_64, p);
		EXPECT_TRUE(static_cast<uint64_t>(tgen::math::num_divisors(x)) == p);
	}

	for (int i = 0; i < 100; ++i) {
		auto l = tgen::next<uint64_t>(0, largest_number_64 / 100);
		auto r =
			tgen::next<uint64_t>(largest_number_64 / 100, largest_number_64);

		auto p = tgen::math::gen_prime(0, 10);
		auto x = tgen::math::gen_divisor_count(l, r, p);
		EXPECT_TRUE(static_cast<uint64_t>(tgen::math::num_divisors(x)) == p);
		EXPECT_TRUE(l <= x and x <= r);
	}
}

TEST(math_test, gen_divisor_count_uniform) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	check_function_uniform(tgen::math::gen_divisor_count, 25, 0, 100, 2);
	check_function_uniform(tgen::math::gen_divisor_count, 11, 0, 1000, 3);
	check_function_uniform(tgen::math::gen_divisor_count, 3, 0, 1000, 5);
}

TEST(math_test, prime_gaps) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::math::prime_gaps(), prime_gaps());
}

TEST(math_test, prime_gap_upto) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	using pair = std::pair<uint64_t, uint64_t>;

	EXPECT_THROW_TGEN_PREFIX(tgen::math::prime_gap_upto(3),
							 "math: there is no prime gap up to 3");
	EXPECT_EQ(tgen::math::prime_gap_upto(4), pair(4, 4));
	EXPECT_EQ(tgen::math::prime_gap_upto(5), pair(4, 4));
	EXPECT_EQ(tgen::math::prime_gap_upto(6), pair(4, 4));
	EXPECT_EQ(tgen::math::prime_gap_upto(7), pair(4, 4));
	EXPECT_EQ(tgen::math::prime_gap_upto(8), pair(4, 4));
	EXPECT_EQ(tgen::math::prime_gap_upto(9), pair(8, 9));
	EXPECT_EQ(tgen::math::prime_gap_upto(10), pair(8, 10));
	EXPECT_EQ(tgen::math::prime_gap_upto(11), pair(8, 10));
	EXPECT_EQ(tgen::math::prime_gap_upto(24), pair(8, 10));
	EXPECT_EQ(tgen::math::prime_gap_upto(25), pair(8, 10));
	EXPECT_EQ(tgen::math::prime_gap_upto(26), pair(8, 10));
	EXPECT_EQ(tgen::math::prime_gap_upto(27), pair(24, 27));

	for (int i = 0; i < 100; ++i) {
		auto r = tgen::next<uint64_t>(4, largest_number_64);
		auto [x, y] = tgen::math::prime_gap_upto(r);
		for (auto j = x; j <= y; ++j)
			EXPECT_FALSE(tgen::math::is_prime(j));
	}
}

TEST(math_test, highly_composites) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::math::highly_composites(), highly_composites());
}

TEST(math_test, highly_composite_upto) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::highly_composite_upto(0),
		"math: there is no highly composite number up to 0");
	EXPECT_EQ(tgen::math::highly_composite_upto(1), 1);
	EXPECT_EQ(tgen::math::highly_composite_upto(2), 2);
	EXPECT_EQ(tgen::math::highly_composite_upto(3), 2);
	EXPECT_EQ(tgen::math::highly_composite_upto(200), 180);
	EXPECT_EQ(tgen::math::highly_composite_upto(239), 180);
	EXPECT_EQ(tgen::math::highly_composite_upto(240), 240);
	EXPECT_EQ(tgen::math::highly_composite_upto(largest_number_64),
			  highly_composites().back());
}

TEST(math_test, gen_congruent_invalid) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_congruent(1, 0, 0, 2),
		"math: there is no congruent number in range [1, 0]");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_congruent(0, 10, {0, 1}, {2}),
		"math: number of remainders and mods must be the same");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_congruent(2, 5, {1, 1}, {2, 3}),
		"math: there is no congruent number in range [2, 5]");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_congruent(0, 0, {1, 1}, {2, 3}),
		"math: there is no congruent number in range [0, 0]");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_congruent(1, 5, {1, 1, 0}, {2, 3, 5}),
		"math: there is no congruent number in range [1, 5]");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_congruent(0, 10, {0, 1}, {2, 4}),
		"math: there is no congruent number in range [0, 10]");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_congruent(2, 4, 1, 4),
		"math: there is no congruent number in range [2, 4]");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_congruent(8, 12, {1, 1}, {2, 3}),
		"math: there is no congruent number in range [8, 12]");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_congruent(
			2, largest_number_64, {1, 1},
			{largest_prime_64, tgen::math::prime_upto(largest_prime_64 - 1)}),
		"math: there is no congruent number in range");
}

TEST(math_test, gen_congruent) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	using vec = std::vector<uint64_t>;

	auto test_congruent = [&](uint64_t l, uint64_t r, vec rems, vec mods) {
		auto val = tgen::math::gen_congruent(l, r, rems, mods);
		EXPECT_TRUE(l <= val and val <= r);
		for (int i = 0; i < static_cast<int>(rems.size()); ++i)
			EXPECT_TRUE(val % mods[i] == rems[i]);
	};

	EXPECT_TRUE(tgen::math::gen_even(0, largest_number_64) % 2 == 0);
	EXPECT_TRUE(tgen::math::gen_odd(0, largest_number_64) % 2 == 1);
	EXPECT_TRUE(tgen::math::gen_congruent(0, largest_number_64, 1, 3) % 3 == 1);
	EXPECT_EQ(tgen::math::gen_congruent(
				  1, largest_number_64, {1, 1},
				  {largest_prime_64, tgen::math::prime_upto(largest_prime_64)}),
			  1);

	for (int i = 0; i < 100; ++i) {
		vec primes, rems;
		for (int j = 0; j < 5; ++j)
			primes.push_back(tgen::math::gen_prime(0, 1000));
		std::sort(primes.begin(), primes.end());
		primes.erase(unique(primes.begin(), primes.end()), primes.end());
		for (int j = 0; j < static_cast<int>(primes.size()); ++j)
			rems.push_back(tgen::next<uint64_t>(0, primes[j] - 1));

		test_congruent(0, largest_number_64, rems, primes);
	}
}

TEST(math_test, gen_congruent_uniform) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	auto func = [](uint64_t l, uint64_t r, std::vector<uint64_t> rems,
				   std::vector<uint64_t> mods) {
		return tgen::math::gen_congruent(l, r, rems, mods);
	};

	check_function_uniform(func, 50, 0, 99, std::vector<uint64_t>{0},
						   std::vector<uint64_t>{2});
	check_function_uniform(func, 16, 0, 99, std::vector<uint64_t>{0, 1},
						   std::vector<uint64_t>{2, 3});
}

TEST(math_test, fft_mod) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::math::FFT_MOD, fft_mod);
}

TEST(math_test, fibonacci) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	auto fib = tgen::math::fibonacci();

	EXPECT_EQ(fib[0], 0);
	EXPECT_EQ(fib[1], 1);
	for (int i = 2; i < static_cast<int>(fib.size()); ++i)
		EXPECT_EQ(fib[i], fib[i - 1] + fib[i - 2]);
}

TEST(math_test, gen_partition_invalid) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_partition(0, 1, 2),
							 "math: invalid parameters to gen_partition");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_partition(1, 0, 1),
							 "math: invalid parameters to gen_partition");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_partition(1, 2, 3),
							 "math: no such partition");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_partition(100, 40, 45),
							 "math: no such partition");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_partition(100, 10, -10),
							 "math: no such partition");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_partition(65005, 1e4, 1e4 + 10),
							 "math: no such partition");

	for (int i = 0; i < 10; ++i) {
		EXPECT_THROW_TGEN_PREFIX(
			tgen::math::gen_partition(tgen::next<int>(1e4 + 1e3, 1e4 + 2e3),
									  1e4, 1e4 + 10),
			"math: no such partition");
	}
}

TEST(math_test, gen_partition) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	auto test_partition = [&](int n, int part_l = 1, int part_r = -1) {
		auto part = tgen::math::gen_partition(n, part_l, part_r);
		for (int i : part) {
			EXPECT_TRUE(part_l <= i);
			if (part_r != -1) {
				EXPECT_TRUE(i <= part_r);
			}
		}
		EXPECT_EQ(std::accumulate(part.begin(), part.end(), 0), n);
		return part;
	};

	EXPECT_EQ(test_partition(1e5, 5e4, 5e4 + 10),
			  std::vector<int>({50000, 50000}));

	for (int i = 0; i < 100; ++i) {
		test_partition(tgen::next<int>(1, 1e3));
		test_partition(tgen::next<int>(1e2, 1e3), 1e2);
		test_partition(tgen::next<int>(1e2, 1e3), 10, 1e2 + 10);
		test_partition(tgen::next<int>(1e2, 1e3), 10, 1e9);
	}
	for (int i = 0; i < 5; ++i) {
		test_partition(tgen::next<int>(1, 1e5));
		test_partition(tgen::next<int>(1e4, 1e5), 1e4);
		test_partition(tgen::next<int>(1e4, 1e5), 10, 1e4 + 10);
		test_partition(tgen::next<int>(1e4, 1e5), 10, 1e9);
	}
	test_partition(1e6);
}

TEST(math_test, gen_partition_uniform) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	check_function_uniform(tgen::math::gen_partition, 512, 10, 1, -1);
	check_function_uniform(tgen::math::gen_partition, 7, 10, 2, 3);
	check_function_uniform(tgen::math::gen_partition, 266, 100, 13, 15);
}

TEST(math_test, gen_partition_fixed_size_invalid) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_partition_fixed_size(1, 0),
		"math: invalid parameters to gen_partition_fixed_size");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_partition_fixed_size(1, 2),
		"math: invalid parameters to gen_partition_fixed_size");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::math::gen_partition_fixed_size(1, 1, -1, 1),
		"math: invalid parameters to gen_partition_fixed_size");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_partition_fixed_size(3, 3, 0, -10),
							 "math: no such partition");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_partition_fixed_size(3, 2, 2, 3),
							 "math: no such partition");
	EXPECT_THROW_TGEN_PREFIX(tgen::math::gen_partition_fixed_size(7, 2, 2, 3),
							 "math: no such partition");
}

TEST(math_test, gen_partition_fixed_size) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	auto test_partition_fixed_size = [&](int n, int k, int part_l = 0,
										 int part_r = -1) {
		auto part = tgen::math::gen_partition_fixed_size(n, k, part_l, part_r);
		for (int i : part) {
			EXPECT_TRUE(part_l <= i);
			if (part_r != -1) {
				EXPECT_TRUE(i <= part_r);
			}
		}
		EXPECT_EQ(part.size(), k);
		EXPECT_EQ(std::accumulate(part.begin(), part.end(), 0), n);
		return part;
	};

	for (int i = 0; i < 100; ++i) {
		test_partition_fixed_size(tgen::next<int>(10, 1e3), tgen::next(1, 10));
		test_partition_fixed_size(tgen::next<int>(1e2, 1e3),
								  tgen::next<int>(1, 1e2), 1);
		test_partition_fixed_size(tgen::next<int>(1e2, 1e3),
								  tgen::next<int>(1, 10), 10, 1e3 + 10);
		test_partition_fixed_size(tgen::next<int>(1e2, 1e3),
								  tgen::next<int>(1, 10), 10, 1e9);
	}
	for (int i = 0; i < 5; ++i) {
		test_partition_fixed_size(tgen::next<int>(10, 1e5), tgen::next(1, 10));
		test_partition_fixed_size(tgen::next<int>(1e4, 1e5),
								  tgen::next<int>(1, 1e4), 1);
		test_partition_fixed_size(tgen::next<int>(1e4, 1e5),
								  tgen::next<int>(1, 10), 10, 1e5 + 10);
		test_partition_fixed_size(tgen::next<int>(1e4, 1e5),
								  tgen::next<int>(1, 10), 10, 1e9);
	}
	test_partition_fixed_size(1e6, 10);
	test_partition_fixed_size(1e6, 5, 0);
	test_partition_fixed_size(1e6, 5, 0, 1e6);
}

TEST(math_test, gen_partition_fixed_size_uniform) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	check_function_uniform(tgen::math::gen_partition_fixed_size, 66, 10, 3, 0,
						   -1);
	check_function_uniform(tgen::math::gen_partition_fixed_size, 36, 10, 3, 1,
						   -1);
	check_function_uniform(tgen::math::gen_partition_fixed_size, 381, 100, 5,
						   18, 22);
}
