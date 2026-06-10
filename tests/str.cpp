#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <string>

TEST(str_test, constructor_size_zero) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::str(0), "str: size must be positive");
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

	expect_generator_uniform(tgen::str(2, 'a', 'b'), 4);
	expect_generator_uniform(tgen::str(3, std::set<char>{'0', '1'}), 8);
	expect_generator_uniform(tgen::str(2, 'a', 'z').fix(0, 'x'), 26);

	expect_generator_uniform(tgen::str("[ab]"), 2);
	expect_generator_uniform(tgen::str("a|b|c"), 3);
	expect_generator_uniform(tgen::str("[ab][cd]"), 4);
	expect_generator_uniform(tgen::str("[0-9]"), 10);
	expect_generator_uniform(tgen::str("[ab]{2}"), 4);
	expect_generator_uniform(tgen::str("(a|b)(c|d)"), 4);
	expect_generator_uniform(tgen::str("0 | [1-9][0-9]{0, 2} | 1000"), 1001);
	expect_generator_uniform(tgen::str("0 | [1-9][0-9]{0, 1}"), 100);
}

TEST(str_test, add_restriction_after_regex_throws) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::str("a").fix(0, 'x'),
							 "str: cannot add restriction for regex");
	EXPECT_THROW_TGEN_PREFIX(tgen::str("a").all_different(),
							 "str: cannot add restriction for regex");
}

TEST(str_test, value_ops) {
	tgen::register_gen();

	tgen::str::value inst = tgen::str("hello").gen();
	EXPECT_EQ(inst.size(), 5);
	EXPECT_EQ(inst[0], 'h');
	EXPECT_EQ(inst.to_std(), "hello");

	inst.reverse();
	EXPECT_EQ(inst.to_std(), "olleh");
	inst.reverse();

	inst.sort();
	EXPECT_EQ(inst.to_std(), "ehllo");

	tgen::str::value a = tgen::str("ab").gen();
	tgen::str::value b = tgen::str("cd").gen();
	EXPECT_EQ((a + b).to_std(), "abcd");

	EXPECT_EQ((std::ostringstream() << tgen::str::value("xyz")).str(), "xyz");
}

TEST(str_test, value_shuffle_pick_choose) {
	tgen::register_gen();

	tgen::str::value inst("aabc");
	std::string sorted = inst.to_std();
	std::sort(sorted.begin(), sorted.end());

	for (int i = 0; i < 100; ++i) {
		inst.shuffle();
		std::string cur = inst.to_std();
		std::sort(cur.begin(), cur.end());
		EXPECT_EQ(cur, sorted);
	}

	for (int i = 0; i < 100; ++i) {
		char c = inst.pick();
		EXPECT_NE(inst.to_std().find(c), std::string::npos);
	}

	tgen::str::value dist_inst("abc");
	expect_distribution(
		[&] { return dist_inst.pick_by_distribution({1, 2, 3}) - 'a'; },
		{1, 2, 3});

	EXPECT_THROW_TGEN_PREFIX(inst.pick_by_distribution({1, 2}),
							 "value and distribution must have the same size");

	for (int i = 0; i < 100; ++i) {
		int k = tgen::next<int>(1, inst.size());
		auto sub = inst.choose(k);
		EXPECT_EQ(sub.size(), k);
		int idx = 0;
		for (int j = 0; j < inst.size(); ++j)
			if (idx < sub.size() and sub[idx] == inst[j])
				++idx;
		EXPECT_EQ(idx, sub.size());
	}

	EXPECT_THROW_TGEN_PREFIX(static_cast<void>(inst.choose(0)),
							 "number of elements to choose must be valid");
	EXPECT_THROW_TGEN_PREFIX(static_cast<void>(inst.choose(inst.size() + 1)),
							 "number of elements to choose must be valid");
}

TEST(str_test, value_index_out_of_bounds) {
	tgen::register_gen();

	tgen::str::value inst("ab");
	EXPECT_THROW_TGEN_PREFIX(inst[-1], "str: value: index out of bounds");
	EXPECT_THROW_TGEN_PREFIX(inst[2], "str: value: index out of bounds");
}
