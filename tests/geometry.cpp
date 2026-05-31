#include <gtest/gtest.h>

#include <set>
#include <sstream>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

namespace {

bool collinear(const tgen::geometry::point<long long> &a,
			   const tgen::geometry::point<long long> &b,
			   const tgen::geometry::point<long long> &c) {
	__int128 dx1 = b.x_ - a.x_, dy1 = b.y_ - a.y_;
	__int128 dx2 = c.x_ - a.x_, dy2 = c.y_ - a.y_;
	return dx1 * dy2 == dy1 * dx2;
}

void expect_in_range(const std::vector<tgen::geometry::point<long long>> &pts,
					 long long min_coord, long long max_coord) {
	for (const tgen::geometry::point<long long> &p : pts) {
		EXPECT_GE(p.x_, min_coord);
		EXPECT_LE(p.x_, max_coord);
		EXPECT_GE(p.y_, min_coord);
		EXPECT_LE(p.y_, max_coord);
	}
}

void expect_distinct(const std::vector<tgen::geometry::point<long long>> &pts) {
	std::set<tgen::geometry::point<long long>> seen(pts.begin(), pts.end());
	EXPECT_EQ(seen.size(), pts.size());
}

void expect_no_three_collinear(
	const std::vector<tgen::geometry::point<long long>> &pts) {
	int n = static_cast<int>(pts.size());
	for (int i = 0; i < n; ++i)
		for (int j = i + 1; j < n; ++j)
			for (int k = j + 1; k < n; ++k)
				EXPECT_FALSE(collinear(pts[i], pts[j], pts[k]));
}

void expect_no_three_collinear_sampled(
	const std::vector<tgen::geometry::point<long long>> &pts, int samples) {
	int n = static_cast<int>(pts.size());
	if (n < 3)
		return;
	for (int s = 0; s < samples; ++s) {
		int i = tgen::next(n);
		int j = tgen::next(n);
		while (j == i)
			j = tgen::next(n);
		int k = tgen::next(n);
		while (k == i or k == j)
			k = tgen::next(n);
		EXPECT_FALSE(collinear(pts[i], pts[j], pts[k]));
	}
}

long long min_width_for(int n) {
	uint64_t p = tgen::math::prime_from(static_cast<uint64_t>(n) * 2);
	return static_cast<long long>(p - 1);
}

} // namespace

TEST(geometry_test, default_constructor) {
	tgen::geometry::point<int> p;

	EXPECT_EQ(p.x_, 0);
	EXPECT_EQ(p.y_, 0);
}

TEST(geometry_test, arithmetic_int) {
	tgen::geometry::point<int> a(1, 2), b(3, 4);

	EXPECT_EQ(a + b, tgen::geometry::point<int>(4, 6));
	EXPECT_EQ(a - b, tgen::geometry::point<int>(-2, -2));
	EXPECT_EQ(a * 2, tgen::geometry::point<int>(2, 4));
	EXPECT_EQ(a * b, 11);
	EXPECT_EQ(a ^ b, -2);
}

TEST(geometry_test, comparison_int) {
	tgen::geometry::point<int> a(1, 2), b(1, 3), c(2, 0);

	EXPECT_TRUE(a < b);
	EXPECT_TRUE(a < c);
	EXPECT_TRUE(a == tgen::geometry::point<int>(1, 2));
	EXPECT_FALSE(a == b);
	EXPECT_FALSE(b < a);
}

TEST(geometry_test, comparison_double_epsilon) {
	tgen::geometry::point<double> a(1.0, 2.0), b(1.0 + 1e-10, 2.0),
		c(1.0 + 1e-8, 2.0);

	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a == c);
	EXPECT_TRUE(a < c);
	EXPECT_FALSE(c < a);
	EXPECT_FALSE(a < b);
	EXPECT_FALSE(b < a);
}

TEST(geometry_test, arithmetic_double) {
	tgen::geometry::point<double> a(1.5, 2.5), b(3.0, 1.0);

	EXPECT_DOUBLE_EQ(a * b, 7.0);
	EXPECT_DOUBLE_EQ(a ^ b, -6.0);
	EXPECT_EQ(a + b, tgen::geometry::point<double>(4.5, 3.5));
	EXPECT_EQ(a - b, tgen::geometry::point<double>(-1.5, 1.5));
	EXPECT_EQ(a * 2.0, tgen::geometry::point<double>(3.0, 5.0));
}

TEST(geometry_test, print) {
	tgen::geometry::point<int> a(3, -4);
	tgen::geometry::point<double> p(1.5, 2.5);

	EXPECT_EQ((std::ostringstream() << a).str(), "3 -4");
	EXPECT_EQ((std::ostringstream() << p).str(), "1.5 2.5");
}

TEST(geometry_test, general_position_basic) {
	tgen::register_gen(42);

	auto pts = tgen::geometry::general_position(10, 0, 100);

	EXPECT_EQ(pts.size(), 10u);
	expect_in_range(pts, 0, 100);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, general_position_invalid_n) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::geometry::general_position(0, 0, 100),
							 "geometry: general_position: n must be positive");
	EXPECT_THROW_TGEN_PREFIX(tgen::geometry::general_position(-1, 0, 100),
							 "geometry: general_position: n must be positive");
}

TEST(geometry_test, general_position_invalid_range) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::geometry::general_position(10, 5, 4),
							 "geometry: general_position: invalid coordinate "
							 "range");
}

TEST(geometry_test, general_position_range_too_small) {
	tgen::register_gen();

	int n = 10;
	long long width = min_width_for(n) - 1;

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::general_position(n, 0, width),
		"geometry: general_position: coordinate range too small for n");
}

TEST(geometry_test, general_position_minimum_range) {
	tgen::register_gen();

	int n = 10;
	long long width = min_width_for(n);
	auto pts = tgen::geometry::general_position(n, 0, width);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	expect_in_range(pts, 0, width);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, general_position_large_n) {
	tgen::register_gen();

	auto pts = tgen::geometry::general_position(1000, 0, 10000);

	EXPECT_EQ(pts.size(), 1000u);
	expect_in_range(pts, 0, 10000);
	expect_distinct(pts);
	expect_no_three_collinear_sampled(pts, 10000);
}

TEST(geometry_test, general_position_nonzero_min) {
	tgen::register_gen();

	long long min_coord = 50, max_coord = 100;
	auto pts = tgen::geometry::general_position(10, min_coord, max_coord);

	EXPECT_EQ(pts.size(), 10u);
	expect_in_range(pts, min_coord, max_coord);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, general_position_offset_box) {
	tgen::register_gen();

	int n = 20;
	long long min_coord = 10000;
	long long max_coord = min_coord + min_width_for(n);
	auto pts = tgen::geometry::general_position(n, min_coord, max_coord);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	expect_in_range(pts, min_coord, max_coord);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, general_position_many_seeds_in_offset_box) {
	tgen::register_gen();

	int n = 15;
	long long min_coord = 500, max_coord = 800;

	for (int seed = 0; seed < 100; ++seed) {
		auto pts = tgen::geometry::general_position(n, min_coord, max_coord);
		EXPECT_EQ(pts.size(), static_cast<size_t>(n));
		expect_in_range(pts, min_coord, max_coord);
	}
}

TEST(geometry_test, general_position_large_offset_tight_box) {
	tgen::register_gen();

	int n = 50;
	long long min_coord = 1'000'000'000'000LL;
	long long max_coord = min_coord + min_width_for(n);
	auto pts = tgen::geometry::general_position(n, min_coord, max_coord);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	expect_in_range(pts, min_coord, max_coord);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, general_position_large_coordinates) {
	tgen::register_gen();

	long long min_coord = -1'000'000'000'000LL;
	long long max_coord = min_coord + min_width_for(50);
	auto pts = tgen::geometry::general_position(50, min_coord, max_coord);

	EXPECT_EQ(pts.size(), 50u);
	expect_in_range(pts, min_coord, max_coord);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}
