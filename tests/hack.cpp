
#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>

TEST(hack_test, abacaba) {
	tgen::register_gen();

	EXPECT_EQ(tgen::hack::abacaba(1), "a");
	EXPECT_EQ(tgen::hack::abacaba(19), "abacabadabacabaeaba");
}

TEST(hack_test, unsigned_hash_hack_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::hack::polynomial_hash_hack(1, 31, 1e9 + 7),
							 "str: alphabet size must be greater than 1");
	EXPECT_THROW_TGEN_PREFIX(tgen::hack::polynomial_hash_hack(2, 0, 1e9 + 7),
							 "str: base must be in (0, mod)");
	EXPECT_THROW_TGEN_PREFIX(tgen::hack::polynomial_hash_hack(2, 31, 31),
							 "str: base must be in (0, mod)");
}

TEST(hack_test, unsigned_hash_hack) {
	tgen::register_gen();

	auto [a, b] = tgen::hack::unsigned_polynomial_hash_hack();
	uint64_t hash_a = 0, hash_b = 0, base = 127;
	for (int i = 0; i < static_cast<int>(a.size()); ++i) {
		hash_a = hash_a * base + a[i];
		hash_b = hash_b * base + b[i];
	}
	EXPECT_EQ(hash_a, hash_b);
	for (char c : a)
		EXPECT_TRUE(c == 'a' or c == 'b');
	for (char c : b)
		EXPECT_TRUE(c == 'a' or c == 'b');
}

TEST(hack_test, hash_hack_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::hack::polynomial_hash_hack(1, 31, 1e9 + 7),
							 "str: alphabet size must be greater than 1");
	EXPECT_THROW_TGEN_PREFIX(tgen::hack::polynomial_hash_hack(2, 0, 1e9 + 7),
							 "str: base must be in (0, mod)");
	EXPECT_THROW_TGEN_PREFIX(tgen::hack::polynomial_hash_hack(2, 31, 31),
							 "str: base must be in (0, mod)");

	EXPECT_THROW_TGEN_PREFIX(tgen::hack::polynomial_hash_hack(
								 2, std::vector<int>{}, std::vector<int>{}),
							 "str: must have at least one (base, mod) pair");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::hack::polynomial_hash_hack(
			2, {31}, {static_cast<int>(1e9 + 7), static_cast<int>(1e9 + 9)}),
		"str: bases and mods must have the same size");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::hack::polynomial_hash_hack(2, {31, 33, 37},
										 {static_cast<int>(1e9 + 7),
										  static_cast<int>(1e9 + 9),
										  static_cast<int>(1e9 + 9)}),
		"str: multi-hash hack only supported for up to 2 (base, mod) pairs");
}

TEST(hack_test, hash_hack) {
	tgen::register_gen();

	auto hash = [&](const std::string &s, int base, int mod) {
		uint64_t h = 0;
		for (char c : s) {
			h = (h * base + c - 'a' + 1) % mod;
		}
		return h;
	};

	auto [a, b] = tgen::hack::polynomial_hash_hack(2, 31, 1e9 + 7);
	EXPECT_EQ(hash(a, 31, 1e9 + 7), hash(b, 31, 1e9 + 7));
	for (char c : a)
		EXPECT_TRUE(c == 'a' or c == 'b');
	for (char c : b)
		EXPECT_TRUE(c == 'a' or c == 'b');
	std::tie(a, b) = tgen::hack::polynomial_hash_hack(4, 31, 1e9 + 7);
	EXPECT_EQ(hash(a, 31, 1e9 + 7), hash(b, 31, 1e9 + 7));
	for (char c : a)
		EXPECT_TRUE('a' <= c and c < 'a' + 4);
	for (char c : b)
		EXPECT_TRUE('a' <= c and c < 'a' + 4);
	std::tie(a, b) = tgen::hack::polynomial_hash_hack(26, 31, 1e9 + 7);
	EXPECT_EQ(hash(a, 31, 1e9 + 7), hash(b, 31, 1e9 + 7));
	for (char c : a)
		EXPECT_TRUE('a' <= c and c < 'a' + 26);
	for (char c : b)
		EXPECT_TRUE('a' <= c and c < 'a' + 26);

	std::tie(a, b) = tgen::hack::polynomial_hash_hack(
		2, {31, 33}, {static_cast<int>(1e9 + 7), static_cast<int>(1e9 + 9)});
	EXPECT_EQ(hash(a, 31, 1e9 + 7), hash(b, 31, 1e9 + 7));
	EXPECT_EQ(hash(a, 33, 1e9 + 9), hash(b, 33, 1e9 + 9));
	for (char c : a)
		EXPECT_TRUE(c == 'a' or c == 'b');
	for (char c : b)
		EXPECT_TRUE(c == 'a' or c == 'b');
	std::tie(a, b) = tgen::hack::polynomial_hash_hack(
		26, {31, 33}, {static_cast<int>(1e9 + 7), static_cast<int>(1e9 + 9)});
	EXPECT_EQ(hash(a, 31, 1e9 + 7), hash(b, 31, 1e9 + 7));
	EXPECT_EQ(hash(a, 33, 1e9 + 9), hash(b, 33, 1e9 + 9));
	for (char c : a)
		EXPECT_TRUE('a' <= c and c < 'a' + 26);
	for (char c : b)
		EXPECT_TRUE('a' <= c and c < 'a' + 26);
}

TEST(hack_test, std_unordered) {
	tgen::register_gen();

	int size = 1e5;
	std::vector<long long> hack = tgen::hack::std_unordered(size);

	EXPECT_EQ(hack.size(), size);
}

TEST(hack_test, mo) {
	tgen::register_gen();

	int size = 1e5;
	std::vector<std::pair<int, int>> hack = tgen::hack::mo(size, size);

	for (auto [l, r] : hack)
		EXPECT_TRUE(0 <= l and l <= r and r < size);
}

TEST(hack_test, string_set) {
	tgen::register_gen();

	int size = 1e5;
	std::vector<std::string> hack = tgen::hack::string_set(size);

	int sum = 0;
	for (std::string &s : hack)
		sum += s.size();
	EXPECT_TRUE(sum == size);
}

TEST(hack_test, non_strict_relaxation_dijkstra_bug_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::hack::non_strict_relaxation_dijkstra_bug(2),
		"hack: non_strict_relaxation_dijkstra_bug: needs at least 3 vertices");
}

TEST(hack_test, non_strict_relaxation_dijkstra_bug) {
	tgen::register_gen();

	for (int n : {3, 4, 10, 100}) {
		auto g = tgen::hack::non_strict_relaxation_dijkstra_bug(n);
		EXPECT_TRUE(graph_gen_result_valid(g, n, 2 * (n - 2), true, false));
		ASSERT_TRUE(g.edge_weights().has_value());
		for (int i = 0; i < g.m(); ++i)
			EXPECT_EQ((*g.edge_weights())[i], 1);
	}
}

TEST(hack_test, stale_heap_dijkstra_bug_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::hack::stale_heap_dijkstra_bug(3),
		"hack: stale_heap_dijkstra_bug: needs at least 4 vertices");
}

TEST(hack_test, stale_heap_dijkstra_bug) {
	tgen::register_gen();

	for (int n : {4, 5, 10, 100}) {
		auto g = tgen::hack::stale_heap_dijkstra_bug(n);
		int mid = n / 2;
		int edges = n + mid - 3;
		EXPECT_TRUE(graph_gen_result_valid(g, n, edges, true, false));
		ASSERT_TRUE(g.edge_weights().has_value());
		ASSERT_EQ(static_cast<int>(g.edge_weights()->size()), edges);
	}
}

TEST(hack_test, naive_rotating_calipers_max_dist_bug) {
	tgen::register_gen();

	std::vector<tgen::geometry::point<double>> poly =
		tgen::hack::naive_rotating_calipers_max_dist_bug();

	EXPECT_EQ(poly.size(), 6u);
}

TEST(hack_test, mt19937_xor_hash_hack) {
	tgen::register_gen();

	const std::vector<bool> mask = tgen::hack::mt19937_xor_hash_hack<int>();

	EXPECT_EQ(mask.size(), 19938u);
	EXPECT_TRUE(mask.back());

	auto xor_hash = [&](uint32_t seed) {
		std::mt19937 rng(seed);
		uint32_t hash = 0;
		for (bool use : mask) {
			uint32_t h = rng();
			if (use)
				hash ^= h;
		}
		return hash;
	};

	for (uint32_t seed : std::vector<uint32_t>{
			 0, 1, 42, 123456789, std::numeric_limits<uint32_t>::max()})
		EXPECT_EQ(xor_hash(seed), 0U);
}

TEST(hack_test, mt19937_64_xor_hash_hack) {
	tgen::register_gen();

	const std::vector<bool> mask =
		tgen::hack::mt19937_xor_hash_hack<long long>();

	EXPECT_EQ(mask.size(), 19938u);
	EXPECT_TRUE(mask.back());

	auto xor_hash = [&](uint64_t seed) {
		std::mt19937_64 rng(seed);
		uint64_t hash = 0;
		for (bool use : mask) {
			uint64_t h = rng();
			if (use)
				hash ^= h;
		}
		return hash;
	};

	for (uint64_t seed : std::vector<uint64_t>{
			 0, 1, 42, 123456789, std::numeric_limits<uint64_t>::max()})
		EXPECT_EQ(xor_hash(seed), 0ULL);
}