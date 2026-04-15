#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <cstdint>
#include <utility>

template <typename T> struct pair_test {
	T l1_, r1_, l2_, r2_;
	tgen::pair<T> p_;
	typename tgen::pair<T>::restriction_type type_ =
		tgen::pair<T>::restriction_type::unspecified;

	pair_test(T l1, T r1, T l2, T r2)
		: l1_(l1), r1_(r1), l2_(l2), r2_(r2), p_(l1, r1, l2, r2) {}

	pair_test &eq() {
		p_.eq();
		type_ = tgen::pair<T>::restriction_type::eq;
		return *this;
	}
	pair_test &neq() {
		p_.neq();
		type_ = tgen::pair<T>::restriction_type::neq;
		return *this;
	}
	pair_test &lt() {
		p_.lt();
		type_ = tgen::pair<T>::restriction_type::lt;
		return *this;
	}
	pair_test &gt() {
		p_.gt();
		type_ = tgen::pair<T>::restriction_type::gt;
		return *this;
	}
	pair_test &leq() {
		p_.leq();
		type_ = tgen::pair<T>::restriction_type::leq;
		return *this;
	}
	pair_test &geq() {
		p_.geq();
		type_ = tgen::pair<T>::restriction_type::geq;
		return *this;
	}

	typename tgen::pair<T>::instance check(int count = 1) {
		typename tgen::pair<T>::instance p(0, 0);
		for (int i = 0; i < count; ++i) {
			p = p_.gen();
			EXPECT_TRUE(l1_ <= p.first() and p.first() <= r1_);
			EXPECT_TRUE(l2_ <= p.second() and p.second() <= r2_);

			if (type_ == tgen::pair<T>::restriction_type::eq) {
				EXPECT_TRUE(p.first() == p.second());
			} else if (type_ == tgen::pair<T>::restriction_type::neq) {
				EXPECT_TRUE(p.first() != p.second());
			} else if (type_ == tgen::pair<T>::restriction_type::lt) {
				EXPECT_TRUE(p.first() < p.second());
			} else if (type_ == tgen::pair<T>::restriction_type::gt) {
				EXPECT_TRUE(p.first() > p.second());
			} else if (type_ == tgen::pair<T>::restriction_type::leq) {
				EXPECT_TRUE(p.first() <= p.second());
			} else if (type_ == tgen::pair<T>::restriction_type::geq) {
				EXPECT_TRUE(p.first() >= p.second());
			}
		}

		return p;
	}
};

TEST(pair_test, invalid_range) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::pair<int>(10, 1, 1, 10).gen(),
							 "pair: first range must be valid");
	EXPECT_THROW_TGEN_PREFIX(tgen::pair<int>(1, 10, 10, 1).gen(),
							 "pair: second range must be valid");
}

TEST(pair_test, instance_ops) {
	tgen::register_gen();

	tgen::pair<int>::instance inst = {2, 3};

	EXPECT_EQ(inst.first(), 2);
	EXPECT_EQ(inst.second(), 3);

	EXPECT_EQ(inst.to_std(), std::make_pair(2, 3));

	testing::internal::CaptureStdout();
	std::cout << inst;
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("2 3"));

	testing::internal::CaptureStdout();
	std::cout << inst.separator(',');
	EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string("2,3"));
}

TEST(pair_test, gen_eq) {
	tgen::register_gen();

	pair_test<int>(1, 10, 1, 10).eq().check();
	pair_test<int>(5, 5, 5, 5).eq().check();

	EXPECT_THROW_TGEN_PREFIX(tgen::pair<int>(1, 3, 5, 7).eq().gen(),
							 "pair: no valid values to generate");
	EXPECT_THROW_TGEN_PREFIX(tgen::pair<int>(1, 1, 2, 2).eq().gen(),
							 "pair: no valid values to generate");
}

TEST(pair_test, gen_neq) {
	tgen::register_gen();

	pair_test<int>(1, 10, 1, 10).neq().check();
	pair_test<int>(1, 3, 5, 7).neq().check();
	pair_test<int>(1, 100, 50, 60).neq().check(1000);
	pair_test<int>(50, 60, 1, 100).neq().check(1000);

	// Intersection is a single point
	pair_test<int>(1, 5, 5, 10).neq().check(1000);

	// One range fully inside another
	pair_test<int>(1, 10, 3, 7).neq().check(1000);

	EXPECT_THROW_TGEN_PREFIX(tgen::pair<int>(3, 3, 3, 3).neq().gen(),
							 "pair: no valid values to generate");
}

TEST(pair_test, gen_neq_negative) {
	tgen::register_gen();

	pair_test<int>(-1, 0, -1, 0).neq().check(100);
	pair_test<int>(-5, 5, -5, 5).neq().check(100);
	pair_test<int>(-10, -1, -20, -5).neq().check(100);
	pair_test<int>(-10, 10, -3, 3).neq().check(100);
	pair_test<int>(-10, -5, -5, 0).neq().check(100);
	pair_test<int>(
		std::numeric_limits<int>::min(), std::numeric_limits<int>::min() + 10,
		std::numeric_limits<int>::min(), std::numeric_limits<int>::min() + 10)
		.neq()
		.check(1000);
}

TEST(pair_test, gen_lt) {
	tgen::register_gen();

	pair_test<int>(1, 2, 1, 2).lt().check(20);

	EXPECT_THROW_TGEN_PREFIX(tgen::pair<int>(5, 10, 1, 4).lt().gen(),
							 "pair: no valid values to generate");
}

TEST(pair_test, gen_gt) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::pair<int>(1, 4, 5, 10).gt().gen(),
							 "pair: no valid values to generate");
}

TEST(pair_test, gen_leq) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::pair<int>(5, 10, 1, 4).leq().gen(),
							 "pair: no valid values to generate");
}

TEST(pair_test, gen_geq) {
	tgen::register_gen();

	pair_test<int>(1, 2, 1, 2).leq().check(1000);

	EXPECT_THROW_TGEN_PREFIX(tgen::pair<int>(1, 4, 5, 10).geq().gen(),
							 "pair: no valid values to generate");
}

TEST(pair_test, gen_exhaustive_small) {
	tgen::register_gen();

	int MAX = 2;
	for (int L1 = -MAX; L1 <= MAX; L1++) {
		for (int R1 = L1; R1 <= MAX; R1++) {
			for (int L2 = -MAX; L2 <= MAX; L2++) {
				for (int R2 = L2; R2 <= MAX; R2++) {

					// Try all constraints
					for (auto type : {tgen::pair<int>::restriction_type::eq,
									  tgen::pair<int>::restriction_type::neq,
									  tgen::pair<int>::restriction_type::lt,
									  tgen::pair<int>::restriction_type::gt,
									  tgen::pair<int>::restriction_type::leq,
									  tgen::pair<int>::restriction_type::geq}) {
						tgen::pair<int> p(L1, R1, L2, R2);

						switch (type) {
						case tgen::pair<int>::restriction_type::eq:
							p.eq();
							break;
						case tgen::pair<int>::restriction_type::neq:
							p.neq();
							break;
						case tgen::pair<int>::restriction_type::lt:
							p.lt();
							break;
						case tgen::pair<int>::restriction_type::gt:
							p.gt();
							break;
						case tgen::pair<int>::restriction_type::leq:
							p.leq();
							break;
						case tgen::pair<int>::restriction_type::geq:
							p.geq();
							break;
						default:
							break;
						}

						bool exists = false;
						for (int a = L1; a <= R1; a++) {
							for (int b = L2; b <= R2; b++) {
								bool ok = false;
								switch (type) {
								case tgen::pair<int>::restriction_type::eq:
									ok = (a == b);
									break;
								case tgen::pair<int>::restriction_type::neq:
									ok = (a != b);
									break;
								case tgen::pair<int>::restriction_type::lt:
									ok = (a < b);
									break;
								case tgen::pair<int>::restriction_type::gt:
									ok = (a > b);
									break;
								case tgen::pair<int>::restriction_type::leq:
									ok = (a <= b);
									break;
								case tgen::pair<int>::restriction_type::geq:
									ok = (a >= b);
									break;
								default:
									break;
								}
								if (ok)
									exists = true;
							}
						}

						if (!exists) {
							EXPECT_THROW_TGEN_PREFIX(
								p.gen(), "pair: no valid values to generate");
						} else {
							auto inst = p.gen();
							EXPECT_TRUE(L1 <= inst.first() and
										inst.first() <= R1);
							EXPECT_TRUE(L2 <= inst.second() and
										inst.second() <= R2);
						}
					}
				}
			}
		}
	}
}

TEST(pair_test, gen_full_range_all_modes) {
	tgen::register_gen();

	uint64_t L = 0;
	uint64_t R = std::numeric_limits<uint64_t>::max();

	pair_test<uint64_t>(L, R, L, R).eq().check(1000);
	pair_test<uint64_t>(L, R, L, R).neq().check(1000);
	pair_test<uint64_t>(L, R, L, R).lt().check(1000);
	pair_test<uint64_t>(L, R, L, R).gt().check(1000);
	pair_test<uint64_t>(L, R, L, R).leq().check(1000);
	pair_test<uint64_t>(L, R, L, R).geq().check(1000);
}

TEST(pair_test, gen_near_overflow_range_size) {
	tgen::register_gen();

	uint64_t L = std::numeric_limits<uint64_t>::max() - 10;
	uint64_t R = std::numeric_limits<uint64_t>::max();

	pair_test<uint64_t>(L, R, L, R).neq().check(1000);
	pair_test<uint64_t>(L, R, L, R).lt().check(1000);
	pair_test<uint64_t>(L, R, L, R).leq().check(1000);
}

TEST(pair_test, gen_intersection_at_max) {
	tgen::register_gen();

	uint64_t max = std::numeric_limits<uint64_t>::max();

	// Intersection is exactly one point: max.
	pair_test<uint64_t>(max - 100, max, max, max).neq().check(1000);
	pair_test<uint64_t>(max - 100, max, max, max).lt().check(1000);
	pair_test<uint64_t>(max - 100, max, max, max).leq().check(1000);
}

TEST(pair_test, gen_disjoint_extreme) {
	tgen::register_gen();

	uint64_t max = std::numeric_limits<uint64_t>::max();

	pair_test<uint64_t>(0, max / 2, max / 2 + 1, max).neq().check(1000);
	pair_test<uint64_t>(0, max / 2, max / 2 + 1, max).lt().check(1000);
	pair_test<uint64_t>(0, max / 2, max / 2 + 1, max).leq().check(1000);
}

TEST(pair_test, gen_huge_asymmetric) {
	tgen::register_gen();

	uint64_t max = std::numeric_limits<uint64_t>::max();

	pair_test<uint64_t>(0, max, max - 1000, max).neq().check(1000);
	pair_test<uint64_t>(max - 1000, max, 0, max).neq().check(1000);
	pair_test<uint64_t>(0, max, max - 1000, max).lt().check(1000);
	pair_test<uint64_t>(max - 1000, max, 0, max).lt().check(1000);
	pair_test<uint64_t>(0, max, max - 1000, max).leq().check(1000);
	pair_test<uint64_t>(max - 1000, max, 0, max).leq().check(1000);
}

TEST(pair_test, gen_tiny_window_at_max) {
	tgen::register_gen();

	uint64_t max = std::numeric_limits<uint64_t>::max();

	pair_test<uint64_t>(max - 2, max, max - 2, max).neq().check(1000);
	pair_test<uint64_t>(max - 2, max, max - 2, max).lt().check(1000);
	pair_test<uint64_t>(max - 2, max, max - 2, max).leq().check(1000);
}

TEST(pair_test, uniform_all) {
	tgen::register_gen();

	// Symmetric [0,4] x [0,4].
	{
		// Total pairs = 5*5 = 25.

		// eq = 5.
		check_generator_uniform(tgen::pair<int>(0, 4, 0, 4).eq(), 5);
		// neq = 25 - 5 = 20.
		check_generator_uniform(tgen::pair<int>(0, 4, 0, 4).neq(), 20);
		// lt = n(n-1)/2 = 10.
		check_generator_uniform(tgen::pair<int>(0, 4, 0, 4).lt(), 10);
		// gt = same as lt.
		check_generator_uniform(tgen::pair<int>(0, 4, 0, 4).gt(), 10);
		// leq = lt + eq = 10 + 5 = 15.
		check_generator_uniform(tgen::pair<int>(0, 4, 0, 4).leq(), 15);
		// geq = gt + eq = 10 + 5 = 15.
		check_generator_uniform(tgen::pair<int>(0, 4, 0, 4).geq(), 15);
	}

	// Asymmetric [0,3] x [2,6].
	{
		// eq = 2  (values 2 and 3).
		// lt = 17.
		// gt = 1  (only (3, 2)).

		check_generator_uniform(tgen::pair<int>(0, 3, 2, 6).lt(), 17);
		check_generator_uniform(tgen::pair<int>(0, 3, 2, 6).gt(), 1);
		// 17+ 2.
		check_generator_uniform(tgen::pair<int>(0, 3, 2, 6).leq(), 19);
		// 2 + 1.
		check_generator_uniform(tgen::pair<int>(0, 3, 2, 6).geq(), 3);
	}

	// Edge: single-point intersection [1,5] x [5,10].
	{
		// eq = 1 (only 5).
		// lt = 29.

		check_generator_uniform(tgen::pair<int>(1, 5, 5, 10).lt(), 29);
		// 29 + 1.
		check_generator_uniform(tgen::pair<int>(1, 5, 5, 10).leq(), 30);
	}

	// Edge: no intersection [0,2] x [5,7]
	{
		// total = 3 * 3 = 9.

		check_generator_uniform(tgen::pair<int>(0, 2, 5, 7).neq(), 9);
		check_generator_uniform(tgen::pair<int>(0, 2, 5, 7).leq(), 9);
	}
}