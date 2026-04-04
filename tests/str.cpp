#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <set>
#include <string>

TEST(str_test, constructor_size_zero) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::str(0), "sequence: size must be positive");
}

TEST(str_test, constructor_regex_empty) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::str(""), "str: regex must be non-empty");
}

TEST(str_test, gen_fixed_size_range) {
	tgen::register_gen();

	for (int i = 0; i < 50; ++i) {
		auto s = tgen::str(10, 'c', 'g').gen();
		EXPECT_EQ(s.size(), 10u);
		for (char c : s.to_std())
			EXPECT_TRUE('c' <= c and c <= 'g');
	}
	{
		auto s = tgen::str(5, '0', '9').gen();
		EXPECT_EQ(s.size(), 5u);
		for (char c : s.to_std())
			EXPECT_TRUE('0' <= c and c <= '9');
	}
}

TEST(str_test, gen_fixed_size_set) {
	tgen::register_gen();

	std::set<char> dna = {'A', 'C', 'G', 'T'};
	for (int i = 0; i < 50; ++i) {
		auto s = tgen::str(8, dna).gen();
		EXPECT_EQ(s.size(), 8u);
		for (char c : s.to_std())
			EXPECT_TRUE(dna.count(c) == 1);
	}
}

TEST(str_test, gen_regex_single_char) {
	tgen::register_gen();

	EXPECT_EQ(tgen::str("a").gen().to_std(), "a");
	EXPECT_EQ(tgen::str("Z").gen().to_std(), "Z");
}

TEST(str_test, gen_regex_char_class) {
	tgen::register_gen();

	std::set<std::string> expected = {"a", "b"};
	for (int i = 0; i < 50; ++i) {
		std::string s = tgen::str("[ab]").gen().to_std();
		EXPECT_EQ(s.size(), 1u);
		EXPECT_TRUE(expected.count(s) == 1);
	}
}

TEST(str_test, gen_regex_char_class_range) {
	tgen::register_gen();

	for (int i = 0; i < 50; ++i) {
		std::string s = tgen::str("[0-9]").gen().to_std();
		EXPECT_EQ(s.size(), 1u);
		EXPECT_TRUE('0' <= s[0] and s[0] <= '9');
	}
}

TEST(str_test, gen_regex_repetition_fixed) {
	tgen::register_gen();

	EXPECT_EQ(tgen::str("a{3}").gen().to_std(), "aaa");
	EXPECT_EQ(tgen::str("x{1}").gen().to_std(), "x");
	EXPECT_EQ(tgen::str("ax{0}").gen().to_std(), "a");
}

TEST(str_test, gen_regex_repetition_range) {
	tgen::register_gen();

	std::set<int> lengths = {2, 3, 4};
	for (int i = 0; i < 100; ++i) {
		std::string s = tgen::str("a{2,4}").gen().to_std();
		EXPECT_TRUE(lengths.count(static_cast<int>(s.size())) == 1);
		for (char c : s)
			EXPECT_EQ(c, 'a');
	}
}

TEST(str_test, gen_regex_alternation) {
	tgen::register_gen();

	std::set<std::string> expected = {"a", "b"};
	for (int i = 0; i < 50; ++i) {
		std::string s = tgen::str("a|b").gen().to_std();
		EXPECT_TRUE(expected.count(s) == 1);
	}
}

TEST(str_test, gen_regex_complex) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		std::string s = tgen::str("[1-9][0-9]{1,2}").gen().to_std();
		EXPECT_TRUE(s.size() >= 2 and s.size() <= 3);
		EXPECT_TRUE('1' <= s[0] and s[0] <= '9');
		for (size_t j = 1; j < s.size(); ++j)
			EXPECT_TRUE('0' <= s[j] and s[j] <= '9');
	}
}

TEST(str_test, gen_regex_with_grouping) {
	tgen::register_gen();

	EXPECT_EQ(tgen::str("(ab){2}").gen().to_std(), "abab");
}

TEST(str_test, gen_regex_uniform) {
	tgen::register_gen();

	check_generator_uniform(tgen::str(2, 'a', 'b'), 4);
	check_generator_uniform(tgen::str(3, std::set<char>{'0', '1'}), 8);
	check_generator_uniform(tgen::str(2, 'a', 'z').fix(0, 'x'), 26);

	check_generator_uniform(tgen::str("[ab]"), 2);
	check_generator_uniform(tgen::str("a|b|c"), 3);
	check_generator_uniform(tgen::str("[ab][cd]"), 4);
	check_generator_uniform(tgen::str("[0-9]"), 10);
	check_generator_uniform(tgen::str("[ab]{2}"), 4);
	check_generator_uniform(tgen::str("(a|b)(c|d)"), 4);
	check_generator_uniform(tgen::str("0 | [1-9][0-9]{0, 2} | 1000"), 1001);
	check_generator_uniform(tgen::str("0 | [1-9][0-9]{0, 1}"), 100);
}

TEST(str_test, add_restriction_after_regex_throws) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::str("a").fix(0, 'x'),
							 "str: cannot add restriction for regex");
	EXPECT_THROW_TGEN_PREFIX(tgen::str("a").distinct(),
							 "str: cannot add restriction for regex");
}

TEST(str_test, instance_ops) {
	tgen::register_gen();

	tgen::str::instance inst = tgen::str("hello").gen();
	EXPECT_EQ(inst.size(), 5);
	EXPECT_EQ(inst[0], 'h');
	EXPECT_EQ(inst.to_std(), "hello");

	inst.reverse();
	EXPECT_EQ(inst.to_std(), "olleh");
	inst.reverse();

	inst.sort();
	EXPECT_EQ(inst.to_std(), "ehllo");

	tgen::str::instance a = tgen::str("ab").gen();
	tgen::str::instance b = tgen::str("cd").gen();
	EXPECT_EQ((a + b).to_std(), "abcd");

	testing::internal::CaptureStdout();
	std::cout << tgen::str::instance("xyz");
	EXPECT_EQ(testing::internal::GetCapturedStdout(), "xyz");
}

TEST(str_test, instance_index_out_of_bounds) {
	tgen::register_gen();

	tgen::str::instance inst("ab");
	EXPECT_THROW_TGEN_PREFIX(inst[-1], "str: instane: index out of bounds");
	EXPECT_THROW_TGEN_PREFIX(inst[2], "str: instane: index out of bounds");
}

TEST(str_test, abacaba) {
	tgen::register_gen();

	EXPECT_EQ(tgen::str::abacaba(1).to_std(), "a");
	auto s = tgen::str::abacaba(19);
	EXPECT_EQ(s.size(), 19);
	EXPECT_EQ(s.to_std(), "abacabadabacabaeaba");
}

TEST(str_test, unsigned_hash_hack_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::str::polynomial_hash_hack(1, 31, 1e9 + 7),
							 "str: alphabet size must be greater than 1");
	EXPECT_THROW_TGEN_PREFIX(tgen::str::polynomial_hash_hack(2, 0, 1e9 + 7),
							 "str: base must be in (0, mod)");
	EXPECT_THROW_TGEN_PREFIX(tgen::str::polynomial_hash_hack(2, 31, 31),
							 "str: base must be in (0, mod)");
}

TEST(str_test, unsigned_hash_hack) {
	tgen::register_gen();

	auto [a, b] = tgen::str::unsigned_polynomial_hash_hack();
	uint64_t hash_a = 0, hash_b = 0, base = 127;
	for (int i = 0; i < a.size(); ++i) {
		hash_a = hash_a * base + a[i];
		hash_b = hash_b * base + b[i];
	}
	EXPECT_EQ(hash_a, hash_b);
	for (char c : a.to_std())
		EXPECT_TRUE(c == 'a' or c == 'b');
	for (char c : b.to_std())
		EXPECT_TRUE(c == 'a' or c == 'b');
}

TEST(str_test, hash_hack_invalid) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::str::polynomial_hash_hack(1, 31, 1e9 + 7),
							 "str: alphabet size must be greater than 1");
	EXPECT_THROW_TGEN_PREFIX(tgen::str::polynomial_hash_hack(2, 0, 1e9 + 7),
							 "str: base must be in (0, mod)");
	EXPECT_THROW_TGEN_PREFIX(tgen::str::polynomial_hash_hack(2, 31, 31),
							 "str: base must be in (0, mod)");

	EXPECT_THROW_TGEN_PREFIX(tgen::str::polynomial_hash_hack(
								 2, std::vector<int>{}, std::vector<int>{}),
							 "str: must have at least one (base, mod) pair");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::str::polynomial_hash_hack(
			2, {31}, {static_cast<int>(1e9 + 7), static_cast<int>(1e9 + 9)}),
		"str: bases and mods must have the same size");
	EXPECT_THROW_TGEN_PREFIX(
		tgen::str::polynomial_hash_hack(2, {31, 33, 37},
										{static_cast<int>(1e9 + 7),
										 static_cast<int>(1e9 + 9),
										 static_cast<int>(1e9 + 9)}),
		"str: multi-hash hack only supported for up to 2 (base, mod) pairs");
}

TEST(str_test, hash_hack) {
	tgen::register_gen();

	auto hash = [&](const std::string &s, int base, int mod) {
		uint64_t h = 0;
		for (char c : s) {
			h = (h * base + c - 'a' + 1) % mod;
		}
		return h;
	};

	auto [a, b] = tgen::str::polynomial_hash_hack(2, 31, 1e9 + 7);
	EXPECT_EQ(hash(a.to_std(), 31, 1e9 + 7), hash(b.to_std(), 31, 1e9 + 7));
	for (char c : a.to_std())
		EXPECT_TRUE(c == 'a' or c == 'b');
	for (char c : b.to_std())
		EXPECT_TRUE(c == 'a' or c == 'b');
	std::tie(a, b) = tgen::str::polynomial_hash_hack(4, 31, 1e9 + 7);
	EXPECT_EQ(hash(a.to_std(), 31, 1e9 + 7), hash(b.to_std(), 31, 1e9 + 7));
	for (char c : a.to_std())
		EXPECT_TRUE('a' <= c and c < 'a' + 4);
	for (char c : b.to_std())
		EXPECT_TRUE('a' <= c and c < 'a' + 4);
	std::tie(a, b) = tgen::str::polynomial_hash_hack(26, 31, 1e9 + 7);
	EXPECT_EQ(hash(a.to_std(), 31, 1e9 + 7), hash(b.to_std(), 31, 1e9 + 7));
	for (char c : a.to_std())
		EXPECT_TRUE('a' <= c and c < 'a' + 26);
	for (char c : b.to_std())
		EXPECT_TRUE('a' <= c and c < 'a' + 26);

	std::tie(a, b) = tgen::str::polynomial_hash_hack(
		2, {31, 33}, {static_cast<int>(1e9 + 7), static_cast<int>(1e9 + 9)});
	EXPECT_EQ(hash(a.to_std(), 31, 1e9 + 7), hash(b.to_std(), 31, 1e9 + 7));
	EXPECT_EQ(hash(a.to_std(), 33, 1e9 + 9), hash(b.to_std(), 33, 1e9 + 9));
	for (char c : a.to_std())
		EXPECT_TRUE(c == 'a' or c == 'b');
	for (char c : b.to_std())
		EXPECT_TRUE(c == 'a' or c == 'b');
	std::tie(a, b) = tgen::str::polynomial_hash_hack(
		26, {31, 33}, {static_cast<int>(1e9 + 7), static_cast<int>(1e9 + 9)});
	EXPECT_EQ(hash(a.to_std(), 31, 1e9 + 7), hash(b.to_std(), 31, 1e9 + 7));
	EXPECT_EQ(hash(a.to_std(), 33, 1e9 + 9), hash(b.to_std(), 33, 1e9 + 9));
	for (char c : a.to_std())
		EXPECT_TRUE('a' <= c and c < 'a' + 26);
	for (char c : b.to_std())
		EXPECT_TRUE('a' <= c and c < 'a' + 26);
}
