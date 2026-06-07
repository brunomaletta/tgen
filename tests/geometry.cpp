#include <gtest/gtest.h>

#include <limits>
#include <set>
#include <sstream>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

namespace {

bool collinear(const tgen::geometry::point<long long> &a,
			   const tgen::geometry::point<long long> &b,
			   const tgen::geometry::point<long long> &c) {
	__int128 dx1 = b.x() - a.x(), dy1 = b.y() - a.y();
	__int128 dx2 = c.x() - a.x(), dy2 = c.y() - a.y();
	return dx1 * dy2 == dy1 * dx2;
}

void expect_in_range(const std::vector<tgen::geometry::point<long long>> &pts,
					 long long min_coord, long long max_coord) {
	for (const tgen::geometry::point<long long> &p : pts) {
		EXPECT_GE(p.x(), min_coord);
		EXPECT_LE(p.x(), max_coord);
		EXPECT_GE(p.y(), min_coord);
		EXPECT_LE(p.y(), max_coord);
	}
}

void expect_distinct(const std::vector<tgen::geometry::point<long long>> &pts) {
	std::set<tgen::geometry::point<long long>> seen(pts.begin(), pts.end());
	EXPECT_EQ(seen.size(), pts.size());
}

void expect_touches_min_on_both_axes(
	const std::vector<tgen::geometry::point<long long>> &pts,
	long long min_coord) {
	bool min_x = false, min_y = false;
	for (const tgen::geometry::point<long long> &p : pts) {
		if (p.x() == min_coord)
			min_x = true;
		if (p.y() == min_coord)
			min_y = true;
	}
	EXPECT_TRUE(min_x);
	EXPECT_TRUE(min_y);
}

void expect_touches_max_on_both_axes(
	const std::vector<tgen::geometry::point<long long>> &pts,
	long long max_coord) {
	bool max_x = false, max_y = false;
	for (const tgen::geometry::point<long long> &p : pts) {
		if (p.x() == max_coord)
			max_x = true;
		if (p.y() == max_coord)
			max_y = true;
	}
	EXPECT_TRUE(max_x);
	EXPECT_TRUE(max_y);
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

bool verify_random_convex_polygon(
	const std::vector<tgen::geometry::point<long long>> &pts, int n,
	long long min_coord, long long max_coord) {
	if (static_cast<int>(pts.size()) != n)
		return false;
	for (const tgen::geometry::point<long long> &p : pts) {
		if (p.x() < min_coord or p.x() > max_coord or p.y() < min_coord or
			p.y() > max_coord)
			return false;
	}
	if (n < 3)
		return false;

	auto turn = [](const tgen::geometry::point<long long> &a,
				   const tgen::geometry::point<long long> &b,
				   const tgen::geometry::point<long long> &c) -> __int128 {
		return (b - a) ^ (c - b);
	};

	__int128 s0 = turn(pts[n - 1], pts[0], pts[1]);
	if (s0 == 0)
		return false;
	for (int i = 1; i < n; ++i) {
		__int128 s = turn(pts[i - 1], pts[i], pts[(i + 1) % n]);
		if (s == 0 or (s > 0) != (s0 > 0))
			return false;
	}

	__int128 area2 = 0;
	for (int i = 0; i < n; ++i) {
		int j = (i + 1) % n;
		area2 += static_cast<__int128>(pts[i].x()) * pts[j].y() -
				 static_cast<__int128>(pts[j].x()) * pts[i].y();
	}
	if (area2 == 0)
		return false;

	std::vector<tgen::geometry::point<long long>> sorted = pts;
	std::sort(sorted.begin(), sorted.end());
	std::vector<tgen::geometry::point<long long>> hull(2 * n);
	int k = 0;
	for (int i = 0; i < n; ++i) {
		while (k >= 2 and
			   ((hull[k - 1] - hull[k - 2]) ^ (sorted[i] - hull[k - 1])) <= 0)
			k--;
		hull[k++] = sorted[i];
	}
	for (int i = n - 2, t = k + 1; i >= 0; --i) {
		while (k >= t and
			   ((hull[k - 1] - hull[k - 2]) ^ (sorted[i] - hull[k - 1])) <= 0)
			k--;
		hull[k++] = sorted[i];
	}
	return k - 1 == n;
}

bool segments_properly_intersect(const tgen::geometry::point<long long> &a,
								 const tgen::geometry::point<long long> &b,
								 const tgen::geometry::point<long long> &c,
								 const tgen::geometry::point<long long> &d) {
	auto turn = [](const tgen::geometry::point<long long> &p,
				   const tgen::geometry::point<long long> &q,
				   const tgen::geometry::point<long long> &r) -> __int128 {
		return (q - p) ^ (r - p);
	};
	__int128 o1 = turn(a, b, c), o2 = turn(a, b, d);
	__int128 o3 = turn(c, d, a), o4 = turn(c, d, b);
	return ((o1 > 0 and o2 < 0) or (o1 < 0 and o2 > 0)) and
		   ((o3 > 0 and o4 < 0) or (o3 < 0 and o4 > 0));
}

bool verify_simple_polygon(
	const std::vector<tgen::geometry::point<long long>> &poly, int n,
	long long min_coord, long long max_coord) {
	if (static_cast<int>(poly.size()) != n)
		return false;
	std::set<tgen::geometry::point<long long>> seen(poly.begin(), poly.end());
	if (static_cast<int>(seen.size()) != n)
		return false;
	for (const tgen::geometry::point<long long> &p : poly) {
		if (p.x() < min_coord or p.x() > max_coord or p.y() < min_coord or
			p.y() > max_coord)
			return false;
	}
	for (int i = 0; i < n; ++i) {
		int ni = (i + 1) % n;
		for (int j = i + 1; j < n; ++j) {
			int nj = (j + 1) % n;
			if (j == i + 1 or (i == 0 and j == n - 1))
				continue;
			if (segments_properly_intersect(poly[i], poly[ni], poly[j],
											poly[nj]))
				return false;
		}
	}
	return true;
}

bool contains_all_points(
	const std::vector<tgen::geometry::point<long long>> &poly,
	const std::vector<tgen::geometry::point<long long>> &pts) {
	std::set<tgen::geometry::point<long long>> seen(poly.begin(), poly.end());
	for (const tgen::geometry::point<long long> &p : pts) {
		if (!seen.count(p))
			return false;
	}
	return true;
}

std::vector<tgen::geometry::point<long long>> grid_points(int rows, int cols) {
	std::vector<tgen::geometry::point<long long>> pts;
	pts.reserve(static_cast<size_t>(rows) * cols);
	for (int y = 0; y < rows; ++y)
		for (int x = 0; x < cols; ++x)
			pts.emplace_back(x, y);
	return pts;
}

void expect_simple_polygon_through_points(
	const std::vector<tgen::geometry::point<long long>> &pts,
	long long min_coord, long long max_coord) {
	const int n = static_cast<int>(pts.size());
	auto poly = tgen::geometry::random_simple_polygon_through_points(pts);
	EXPECT_EQ(poly.size(), static_cast<size_t>(n));
	EXPECT_TRUE(contains_all_points(poly, pts));
	EXPECT_TRUE(verify_simple_polygon(poly, n, min_coord, max_coord));
}

} // namespace

TEST(geometry_test, default_constructor) {
	tgen::geometry::point<int> p;

	EXPECT_EQ(p.x(), 0);
	EXPECT_EQ(p.y(), 0);
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

TEST(geometry_test, random_points_general_position_basic) {
	tgen::register_gen(42);

	auto pts = tgen::geometry::random_points_general_position(10, 0, 100);

	EXPECT_EQ(pts.size(), 10u);
	expect_in_range(pts, 0, 100);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, random_points_general_position_invalid_n) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_points_general_position(0, 0, 100),
		"geometry: random_points_general_position: n must be positive");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_points_general_position(-1, 0, 100),
		"geometry: random_points_general_position: n must be positive");
}

TEST(geometry_test, random_points_general_position_invalid_range) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_points_general_position(10, 5, 4),
		"geometry: random_points_general_position: min_coord must be at most "
		"max_coord");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_points_general_position(10, 0, -1),
		"geometry: random_points_general_position: min_coord must be at most "
		"max_coord");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_points_general_position(
			10, std::numeric_limits<long long>::max(),
			std::numeric_limits<long long>::max() - 1),
		"geometry: random_points_general_position: min_coord must be at most "
		"max_coord");
}

TEST(geometry_test, random_points_general_position_range_too_small) {
	tgen::register_gen();

	int n = 10;
	long long width = min_width_for(n) - 1;

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_points_general_position(n, 0, width),
		"geometry: random_points_general_position: coordinate range too small "
		"for n");
}

TEST(geometry_test, random_points_general_position_minimum_range) {
	tgen::register_gen();

	int n = 10;
	long long width = min_width_for(n);
	auto pts = tgen::geometry::random_points_general_position(n, 0, width);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	expect_in_range(pts, 0, width);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, random_points_general_position_large_n) {
	tgen::register_gen();

	auto pts = tgen::geometry::random_points_general_position(1000, 0, 10000);

	EXPECT_EQ(pts.size(), 1000u);
	expect_in_range(pts, 0, 10000);
	expect_distinct(pts);
	expect_no_three_collinear_sampled(pts, 10000);
}

TEST(geometry_test, random_points_general_position_nonzero_min) {
	tgen::register_gen();

	long long min_coord = 50, max_coord = 100;
	auto pts = tgen::geometry::random_points_general_position(10, min_coord,
															  max_coord);

	EXPECT_EQ(pts.size(), 10u);
	expect_in_range(pts, min_coord, max_coord);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, random_points_general_position_offset_box) {
	tgen::register_gen();

	int n = 20;
	long long min_coord = 10000;
	long long max_coord = min_coord + min_width_for(n);
	auto pts =
		tgen::geometry::random_points_general_position(n, min_coord, max_coord);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	expect_in_range(pts, min_coord, max_coord);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, random_points_general_position_many_seeds_in_offset_box) {
	tgen::register_gen();

	int n = 15;
	long long min_coord = 500, max_coord = 800;

	for (int seed = 0; seed < 100; ++seed) {
		auto pts = tgen::geometry::random_points_general_position(n, min_coord,
																  max_coord);
		EXPECT_EQ(pts.size(), static_cast<size_t>(n));
		expect_in_range(pts, min_coord, max_coord);
	}
}

TEST(geometry_test, random_points_general_position_large_offset_tight_box) {
	tgen::register_gen();

	int n = 50;
	long long min_coord = 1'000'000'000'000LL;
	long long max_coord = min_coord + min_width_for(n);
	auto pts =
		tgen::geometry::random_points_general_position(n, min_coord, max_coord);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	expect_in_range(pts, min_coord, max_coord);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, random_points_general_position_large_coordinates) {
	tgen::register_gen();

	long long min_coord = -1'000'000'000'000LL;
	long long max_coord = min_coord + min_width_for(50);
	auto pts = tgen::geometry::random_points_general_position(50, min_coord,
															  max_coord);

	EXPECT_EQ(pts.size(), 50u);
	expect_in_range(pts, min_coord, max_coord);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, random_points_general_position_near_llong_limits) {
	tgen::register_gen(0);

	const int n = 50;
	const long long width = 500;

	for (int i = 0; i < 500; ++i) {
		long long max_coord = std::numeric_limits<long long>::max();
		long long min_coord = max_coord - width;
		auto pts = tgen::geometry::random_points_general_position(n, min_coord,
																  max_coord);
		EXPECT_EQ(pts.size(), static_cast<size_t>(n));
		expect_in_range(pts, min_coord, max_coord);
		expect_distinct(pts);
		if (i == 0)
			expect_no_three_collinear(pts);
	}

	for (int i = 0; i < 500; ++i) {
		long long min_coord = std::numeric_limits<long long>::min();
		long long max_coord = min_coord + width;
		auto pts = tgen::geometry::random_points_general_position(n, min_coord,
																  max_coord);
		EXPECT_EQ(pts.size(), static_cast<size_t>(n));
		expect_in_range(pts, min_coord, max_coord);
		expect_distinct(pts);
	}
}

TEST(geometry_test, random_points_general_position_hits_min_on_both_axes) {
	tgen::register_gen(0);

	const int n = 50;
	const long long min_coord = std::numeric_limits<long long>::min();
	const long long max_coord = min_coord + 500;

	auto pts =
		tgen::geometry::random_points_general_position(n, min_coord, max_coord);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	expect_in_range(pts, min_coord, max_coord);
	expect_distinct(pts);
	expect_no_three_collinear(pts);
}

TEST(geometry_test, random_points_general_position_full_long_long_range) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_points_general_position(
			50, std::numeric_limits<long long>::min(),
			std::numeric_limits<long long>::max()),
		"geometry: random_points_general_position: coordinate range too large");
}

TEST(geometry_test, random_convex_polygon_basic) {
	tgen::register_gen(42);

	auto pts = tgen::geometry::random_convex_polygon(10, 0, 100, true);

	EXPECT_EQ(pts.size(), 10u);
	EXPECT_TRUE(verify_random_convex_polygon(pts, 10, 0, 100));
}

TEST(geometry_test, random_convex_polygon_invalid_n) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_convex_polygon(2, 0, 100),
		"geometry: random_convex_polygon: n must be at least 3");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_convex_polygon(0, 0, 100),
		"geometry: random_convex_polygon: n must be at least 3");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_convex_polygon(1, 0, 100),
		"geometry: random_convex_polygon: n must be at least 3");
}

TEST(geometry_test, random_convex_polygon_invalid_range) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_convex_polygon(10, 5, 4),
		"geometry: random_convex_polygon: min_coord must be at most max_coord");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_convex_polygon(10, 0, -1),
		"geometry: random_convex_polygon: min_coord must be at most max_coord");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_convex_polygon(
			10, std::numeric_limits<long long>::max(),
			std::numeric_limits<long long>::max() - 1),
		"geometry: random_convex_polygon: min_coord must be at most max_coord");
}

TEST(geometry_test, random_convex_polygon_range_too_small) {
	tgen::register_gen();

	int n = 10;
	long long width = n - 2;

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_convex_polygon(n, 0, width),
		"geometry: random_convex_polygon: coordinate range too small for n");
}

TEST(geometry_test, random_convex_polygon_minimum_range) {
	tgen::register_gen(10);

	int n = 10;
	long long width = n - 1;
	auto pts = tgen::geometry::random_convex_polygon(n, 0, width, true);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	EXPECT_TRUE(verify_random_convex_polygon(pts, n, 0, width));
}

TEST(geometry_test, random_convex_polygon_large_n) {
	tgen::register_gen();

	int n = 500;
	long long max_coord = 1'000'000;
	auto pts = tgen::geometry::random_convex_polygon(n, 0, max_coord, true);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	EXPECT_TRUE(verify_random_convex_polygon(pts, n, 0, max_coord));
}

TEST(geometry_test, random_convex_polygon_nonzero_min) {
	tgen::register_gen();

	long long min_coord = 50, max_coord = 100;
	int n = 10;
	auto pts =
		tgen::geometry::random_convex_polygon(n, min_coord, max_coord, true);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	EXPECT_TRUE(verify_random_convex_polygon(pts, n, min_coord, max_coord));
}

TEST(geometry_test, random_convex_polygon_many_seeds) {
	tgen::register_gen();

	int n = 20;
	long long min_coord = 0, max_coord = 500;

	for (int seed = 0; seed < 100; ++seed) {
		auto pts = tgen::geometry::random_convex_polygon(n, min_coord,
														 max_coord, true);
		EXPECT_EQ(pts.size(), static_cast<size_t>(n));
		EXPECT_TRUE(verify_random_convex_polygon(pts, n, min_coord, max_coord));
	}
}

TEST(geometry_test, random_convex_polygon_non_strict) {
	tgen::register_gen(0);

	int n = 20;
	long long min_coord = 0, max_coord = 500;
	auto pts = tgen::geometry::random_convex_polygon(n, min_coord, max_coord);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	expect_in_range(pts, min_coord, max_coord);
	expect_distinct(pts);
}

TEST(geometry_test, random_convex_polygon_strict_per_seed) {
	int n = 20;
	long long min_coord = 0, max_coord = 500;

	for (int seed = 0; seed < 1000; ++seed) {
		tgen::register_gen(seed);
		auto pts = tgen::geometry::random_convex_polygon(n, min_coord,
														 max_coord, true);
		EXPECT_EQ(pts.size(), static_cast<size_t>(n));
		EXPECT_TRUE(verify_random_convex_polygon(pts, n, min_coord, max_coord));
	}
}

TEST(geometry_test, random_convex_polygon_large_coordinates) {
	tgen::register_gen(0);

	long long min_coord = -1'000'000'000'000LL;
	long long max_coord = min_coord + 500;
	int n = 50;
	auto pts =
		tgen::geometry::random_convex_polygon(n, min_coord, max_coord, true);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	EXPECT_TRUE(verify_random_convex_polygon(pts, n, min_coord, max_coord));
}

TEST(geometry_test, random_convex_polygon_near_llong_limits) {
	tgen::register_gen(0);

	const int n = 50;
	const long long width = 500;

	for (int i = 0; i < 500; ++i) {
		long long max_coord = std::numeric_limits<long long>::max();
		long long min_coord = max_coord - width;
		auto pts =
			tgen::geometry::random_convex_polygon(n, min_coord, max_coord);
		EXPECT_EQ(pts.size(), static_cast<size_t>(n));
		expect_in_range(pts, min_coord, max_coord);
		expect_distinct(pts);
	}

	for (int i = 0; i < 500; ++i) {
		long long min_coord = std::numeric_limits<long long>::min();
		long long max_coord = min_coord + width;
		auto pts =
			tgen::geometry::random_convex_polygon(n, min_coord, max_coord);
		EXPECT_EQ(pts.size(), static_cast<size_t>(n));
		expect_in_range(pts, min_coord, max_coord);
		expect_distinct(pts);
	}
}

TEST(geometry_test, random_convex_polygon_hits_min_on_both_axes) {
	tgen::register_gen(0);

	const int n = 50;
	const long long min_coord = std::numeric_limits<long long>::min();
	const long long max_coord = min_coord + 500;

	auto pts =
		tgen::geometry::random_convex_polygon(n, min_coord, max_coord, true);

	EXPECT_EQ(pts.size(), static_cast<size_t>(n));
	expect_in_range(pts, min_coord, max_coord);
	expect_touches_min_on_both_axes(pts, min_coord);
	expect_touches_max_on_both_axes(pts, max_coord);
	EXPECT_TRUE(verify_random_convex_polygon(pts, n, min_coord, max_coord));
}

TEST(geometry_test, random_convex_polygon_strict_range_too_small) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_convex_polygon(100, 0, 99, true),
		"geometry: random_convex_polygon: generation failed: coordinate range "
		"too small for n");
}

TEST(geometry_test, random_convex_polygon_full_long_long_range) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_convex_polygon(
			50, std::numeric_limits<long long>::min(),
			std::numeric_limits<long long>::max()),
		"geometry: random_convex_polygon: coordinate range too large");
}

TEST(geometry_test, random_convex_polygon_many_n) {
	tgen::register_gen(0);

	for (int n = 3; n <= 200; ++n) {
		auto pts = tgen::geometry::random_convex_polygon(n, 0, 1'000'000, true);
		EXPECT_EQ(pts.size(), static_cast<size_t>(n));
		EXPECT_TRUE(verify_random_convex_polygon(pts, n, 0, 1'000'000));
	}
}

TEST(geometry_test, random_simple_polygon_through_points_basic) {
	tgen::register_gen(42);

	const int n = 10;
	auto pts = tgen::geometry::random_points_general_position(n, 0, 100);
	auto poly = tgen::geometry::random_simple_polygon_through_points(pts);

	EXPECT_EQ(poly.size(), static_cast<size_t>(n));
	EXPECT_TRUE(contains_all_points(poly, pts));
	EXPECT_TRUE(verify_simple_polygon(poly, n, 0, 100));
	expect_no_three_collinear(poly);
}

TEST(geometry_test, random_simple_polygon_through_points_invalid_n) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_simple_polygon_through_points({}),
		"geometry: random_simple_polygon_through_points: need at least 3 "
		"points");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_simple_polygon_through_points(
			{tgen::geometry::point<long long>(0, 0),
			 tgen::geometry::point<long long>(1, 1)}),
		"geometry: random_simple_polygon_through_points: need at least 3 "
		"points");
}

TEST(geometry_test, random_simple_polygon_through_points_fixed_set) {
	tgen::register_gen(0);

	std::vector<tgen::geometry::point<long long>> pts = {
		{0, 0}, {10, 0}, {10, 10}, {0, 10}, {5, 3}};
	auto poly = tgen::geometry::random_simple_polygon_through_points(pts);

	EXPECT_EQ(poly.size(), pts.size());
	EXPECT_TRUE(contains_all_points(poly, pts));
	EXPECT_TRUE(
		verify_simple_polygon(poly, static_cast<int>(pts.size()), 0, 10));
}

TEST(geometry_test, random_simple_polygon_through_points_many_n) {
	tgen::register_gen(7);

	for (int n = 3; n <= 80; ++n) {
		auto pts = tgen::geometry::random_points_general_position(n, 0, 10'000);
		auto poly = tgen::geometry::random_simple_polygon_through_points(pts);
		EXPECT_EQ(poly.size(), static_cast<size_t>(n));
		EXPECT_TRUE(contains_all_points(poly, pts));
		EXPECT_TRUE(verify_simple_polygon(poly, n, 0, 10'000));
	}
}

TEST(geometry_test, random_simple_polygon_through_points_collinear_grids) {
	tgen::register_gen();

	const int sizes[] = {3, 10, 30};
	for (int i = 0; i < 3; ++i) {
		expect_simple_polygon_through_points(grid_points(sizes[i], sizes[i]), 0,
											 sizes[i] - 1);
	}
}

TEST(geometry_test, random_simple_polygon_through_points_all_collinear) {
	tgen::register_gen();

	std::vector<tgen::geometry::point<long long>> pts;
	for (int i = 0; i < 5; ++i)
		pts.emplace_back(i, 0);

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_simple_polygon_through_points(pts),
		"geometry: random_simple_polygon_through_points: all points are "
		"collinear; no simple polygon exists");
}

TEST(geometry_test, random_simple_polygon_through_points_duplicate_points) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_simple_polygon_through_points(
			{{0, 0}, {1, 0}, {0, 0}}),
		"geometry: random_simple_polygon_through_points: points must be "
		"distinct");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_simple_polygon_through_points(
			{{0, 0}, {1, 1}, {2, 2}, {1, 1}}),
		"geometry: random_simple_polygon_through_points: points must be "
		"distinct");
}

TEST(geometry_test, random_simple_polygon_basic) {
	tgen::register_gen(42);

	const int n = 10;
	auto poly = tgen::geometry::random_simple_polygon(n, 0, 100);

	EXPECT_EQ(poly.size(), static_cast<size_t>(n));
	EXPECT_TRUE(verify_simple_polygon(poly, n, 0, 100));
	expect_no_three_collinear(poly);
}

TEST(geometry_test, random_simple_polygon_invalid_n) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::geometry::random_simple_polygon(2, 0, 100),
							 "geometry: random_simple_polygon: n must be at "
							 "least 3");
	EXPECT_THROW_TGEN_PREFIX(tgen::geometry::random_simple_polygon(0, 0, 100),
							 "geometry: random_simple_polygon: n must be at "
							 "least 3");
	EXPECT_THROW_TGEN_PREFIX(tgen::geometry::random_simple_polygon(1, 0, 100),
							 "geometry: random_simple_polygon: n must be at "
							 "least 3");
}

TEST(geometry_test, random_simple_polygon_invalid_range) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_simple_polygon(10, 5, 4),
		"geometry: random_points_general_position: min_coord must be at most "
		"max_coord");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_simple_polygon(10, 0, -1),
		"geometry: random_points_general_position: min_coord must be at most "
		"max_coord");
}

TEST(geometry_test, random_simple_polygon_range_too_small) {
	tgen::register_gen();

	const int n = 10;
	const long long width = min_width_for(n) - 1;
	EXPECT_THROW_TGEN_PREFIX(tgen::geometry::random_simple_polygon(n, 0, width),
							 "geometry: random_points_general_position: "
							 "coordinate range too small for n");
}

TEST(geometry_test, random_simple_polygon_minimum_range) {
	tgen::register_gen(7);

	const int n = 10;
	const long long width = min_width_for(n);
	auto poly = tgen::geometry::random_simple_polygon(n, 0, width);

	EXPECT_EQ(poly.size(), static_cast<size_t>(n));
	EXPECT_TRUE(verify_simple_polygon(poly, n, 0, width));
}

TEST(geometry_test, random_simple_polygon_many_n) {
	tgen::register_gen(0);

	for (int n = 3; n <= 80; ++n) {
		auto poly = tgen::geometry::random_simple_polygon(n, 0, 10'000);
		EXPECT_EQ(poly.size(), static_cast<size_t>(n));
		EXPECT_TRUE(verify_simple_polygon(poly, n, 0, 10'000));
	}
}

TEST(geometry_test, random_simple_polygon_nonzero_min) {
	tgen::register_gen(11);

	const int n = 12;
	const long long min_coord = 1'000, max_coord = 2'000;
	auto poly = tgen::geometry::random_simple_polygon(n, min_coord, max_coord);

	EXPECT_EQ(poly.size(), static_cast<size_t>(n));
	EXPECT_TRUE(verify_simple_polygon(poly, n, min_coord, max_coord));
}

TEST(geometry_test, random_simple_polygon_near_llong_limits) {
	tgen::register_gen(0);

	const int n = 50;
	const long long width = min_width_for(n);

	for (int i = 0; i < 100; ++i) {
		long long max_coord = std::numeric_limits<long long>::max();
		long long min_coord = max_coord - width;
		auto poly =
			tgen::geometry::random_simple_polygon(n, min_coord, max_coord);
		EXPECT_EQ(poly.size(), static_cast<size_t>(n));
		expect_in_range(poly, min_coord, max_coord);
		expect_distinct(poly);
		if (i == 0) {
			EXPECT_TRUE(verify_simple_polygon(poly, n, min_coord, max_coord));
			expect_no_three_collinear(poly);
		}
	}

	for (int i = 0; i < 100; ++i) {
		long long min_coord = std::numeric_limits<long long>::min();
		long long max_coord = min_coord + width;
		auto poly =
			tgen::geometry::random_simple_polygon(n, min_coord, max_coord);
		EXPECT_EQ(poly.size(), static_cast<size_t>(n));
		expect_in_range(poly, min_coord, max_coord);
		expect_distinct(poly);
		if (i == 0) {
			EXPECT_TRUE(verify_simple_polygon(poly, n, min_coord, max_coord));
		}
	}
}

TEST(geometry_test, random_simple_polygon_full_long_long_range) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_simple_polygon(
			50, std::numeric_limits<long long>::min(),
			std::numeric_limits<long long>::max()),
		"geometry: random_points_general_position: coordinate range too large");
}
