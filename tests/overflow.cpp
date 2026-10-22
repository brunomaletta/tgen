#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <climits>

TEST(overflow_test, wnext_extreme_skew) {
	tgen::register_gen(0);
	for (int w : {INT_MIN, INT_MAX}) {
		for (int i = 0; i < 100; ++i) {
			int a = tgen::wnext(10, w);
			int b = tgen::wnext(-10, 10, w);
			EXPECT_TRUE(0 <= a and a < 10);
			EXPECT_TRUE(-10 <= b and b <= 10);
		}
	}
}

TEST(overflow_test, wnext_full_integer_ranges) {
	tgen::register_gen(0);
	for (int w : {-100, -6, 6, 100, INT_MAX}) {
		for (int i = 0; i < 100; ++i) {
			// The first span overflows signed 64 bits; the half-open case
			// also catches rounding to the unrepresentable upper endpoint.
			auto a = tgen::wnext(LLONG_MIN, LLONG_MAX, w);
			auto b = tgen::wnext(LLONG_MAX, w);
			auto c = tgen::wnext<uint64_t>(0, UINT64_MAX, w);
			EXPECT_GE(a, LLONG_MIN);
			EXPECT_LE(a, LLONG_MAX);
			EXPECT_GE(b, 0);
			EXPECT_LT(b, LLONG_MAX);
			EXPECT_LE(c, UINT64_MAX);
			auto d = tgen::wnext(INT_MIN, INT_MAX, w);
			EXPECT_GE(d, INT_MIN);
			EXPECT_LE(d, INT_MAX);
		}
	}
}

TEST(overflow_test, distinct_full_signed_range) {
	tgen::register_gen(0);
	tgen::distinct_range<long long> d(LLONG_MIN, LLONG_MAX);
	const auto full = tgen::detail::u128(1) << 64;
	EXPECT_TRUE(d.size() == full);
	std::set<long long> seen;
	for (int i = 0; i < 100; ++i)
		EXPECT_TRUE(seen.insert(d.gen()).second);
	EXPECT_TRUE(d.size() == full - 100);
	auto pts = tgen::distinct_range<long long>(LLONG_MIN, LLONG_MAX)
				   .gen_list(100)
				   .to_std();
	EXPECT_EQ(std::set<long long>(pts.begin(), pts.end()).size(), 100U);
}

TEST(overflow_test, distinct_full_unsigned_range) {
	tgen::register_gen(0);
	tgen::distinct_range<uint64_t> d(0, UINT64_MAX);
	const auto full = tgen::detail::u128(1) << 64;
	EXPECT_TRUE(d.size() == full);
	std::set<uint64_t> seen;
	for (int i = 0; i < 100; ++i)
		EXPECT_TRUE(seen.insert(d.gen()).second);
	EXPECT_TRUE(d.size() == full - 100);
	auto pts =
		tgen::distinct_range<uint64_t>(0, UINT64_MAX).gen_list(100).to_std();
	EXPECT_EQ(std::set<uint64_t>(pts.begin(), pts.end()).size(), 100U);
}

TEST(overflow_test, distinct_pool_at_maximum) {
	tgen::register_gen(0);
	auto pts = tgen::distinct_range<long long>(LLONG_MAX - 2, LLONG_MAX)
				   .gen_all()
				   .to_std();
	std::sort(pts.begin(), pts.end());
	EXPECT_EQ(
		pts, (std::vector<long long>{LLONG_MAX - 2, LLONG_MAX - 1, LLONG_MAX}));
}

TEST(overflow_test, distinct_complement_at_maximum) {
	tgen::register_gen(0);
	// Just above the pool threshold; exclude only one value so the
	// complement branch is exercised without a large exclusion hash set.
	constexpr int count = (1 << 23);
	auto pts = tgen::distinct_range<long long>(LLONG_MAX - count, LLONG_MAX)
				   .gen_list(count)
				   .to_std();
	ASSERT_EQ(pts.size(), static_cast<size_t>(count));
	EXPECT_GE(pts.front(), LLONG_MAX - count);
	EXPECT_LE(pts.back(), LLONG_MAX);
	EXPECT_TRUE(std::is_sorted(pts.begin(), pts.end()));
	EXPECT_EQ(std::adjacent_find(pts.begin(), pts.end()), pts.end());
}

TEST(overflow_test, distinct_gen_all_checks_count_before_narrowing) {
	tgen::register_gen(0);
	tgen::distinct_range<long long> d(0, 1LL << 40);
	EXPECT_THROW(d.gen_all(), std::runtime_error);
	tgen::distinct_container<int> empty(std::vector<int>{});
	EXPECT_EQ(empty.size(), 0U);
	EXPECT_TRUE(empty.gen_all().to_std().empty());
}

TEST(overflow_test, ordered_pairs_full_signed_range) {
	tgen::register_gen(0);
	auto lt = tgen::pair<long long>(LLONG_MIN, LLONG_MAX).lt();
	auto gt = tgen::pair<long long>(LLONG_MIN, LLONG_MAX).gt();
	for (int i = 0; i < 100; ++i) {
		auto a = lt.gen();
		auto b = gt.gen();
		EXPECT_LT(a.first(), a.second());
		EXPECT_GT(b.first(), b.second());
	}
}

TEST(overflow_test, polygon_grid_size_rejected_without_overflow) {
	tgen::register_gen(0);
	EXPECT_THROW_TGEN_PREFIX(
		tgen::geometry::random_simple_polygon(3, 0, LLONG_MAX),
		"geometry: random_simple_polygon: coordinate range too large");
}

TEST(overflow_test, graph_edge_count_at_int_max) {
	const long long n = INT_MAX;
	EXPECT_EQ(tgen::detail::max_graph_edges(INT_MAX, false, true),
			  n * (n + 1) / 2);
}

TEST(overflow_test, graph_edge_decoders_at_int_max) {
	const long long n = INT_MAX;
	EXPECT_EQ(tgen::detail::decode_undirected_simple_edge(INT_MAX,
														  n * (n - 1) / 2 - 1),
			  std::make_pair(INT_MAX - 2, INT_MAX - 1));
	EXPECT_EQ(tgen::detail::decode_undirected_loops_edge(INT_MAX,
														 n * (n + 1) / 2 - 1),
			  std::make_pair(INT_MAX - 1, INT_MAX - 1));
}

TEST(overflow_test, complete_graph_checks_edge_count_before_narrowing) {
	tgen::register_gen(0);
	// n*(n-1) overflows int. The correct edge count also exceeds the
	// public graph API limit, so rejection must precede allocation.
	EXPECT_THROW(tgen::K(65537), std::runtime_error);
	EXPECT_EQ(tgen::detail::max_graph_edges(65536, false, false), 2147450880LL);
}

TEST(overflow_test, partition_delimiter_count_exceeds_int) {
	tgen::register_gen(0);
	for (int k : {2, 3, 10}) {
		auto parts = tgen::math::gen_partition_fixed_size(INT_MAX, k);
		ASSERT_EQ(parts.size(), static_cast<size_t>(k));
		EXPECT_EQ(std::accumulate(parts.begin(), parts.end(), 0LL), INT_MAX);
		for (int p : parts)
			EXPECT_GE(p, 0);
	}
}

TEST(overflow_test, dot_product_rejects_unrepresentable_result) {
	using point = tgen::geometry::point<long long>;
	point p(LLONG_MIN, LLONG_MIN);
	EXPECT_THROW((void)(p * p), std::runtime_error);
	EXPECT_TRUE(p * point(LLONG_MIN, LLONG_MAX) ==
				(tgen::detail::i128(1) << 63));
	EXPECT_TRUE(point(LLONG_MAX, LLONG_MAX) * point(LLONG_MAX, LLONG_MAX) ==
				2 * tgen::detail::i128(LLONG_MAX) * LLONG_MAX);
}
