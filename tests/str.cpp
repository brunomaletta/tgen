#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <set>
#include <string>

TEST(str_test, constructor_size_zero) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::str(0), "sequence: size must be positive");
}

TEST(str_test, constructor_regex_empty) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::str(""), "str: regex must be non-empty");
}

TEST(str_test, gen_fixed_size_range) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

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
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::set<char> dna = {'A', 'C', 'G', 'T'};
	for (int i = 0; i < 50; ++i) {
		auto s = tgen::str(8, dna).gen();
		EXPECT_EQ(s.size(), 8u);
		for (char c : s.to_std())
			EXPECT_TRUE(dna.count(c) == 1);
	}
}

TEST(str_test, gen_regex_single_char) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::str("a").gen().to_std(), "a");
	EXPECT_EQ(tgen::str("Z").gen().to_std(), "Z");
}

TEST(str_test, gen_regex_char_class) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::set<std::string> expected = {"a", "b"};
	for (int i = 0; i < 50; ++i) {
		std::string s = tgen::str("[ab]").gen().to_std();
		EXPECT_EQ(s.size(), 1u);
		EXPECT_TRUE(expected.count(s) == 1);
	}
}

TEST(str_test, gen_regex_char_class_range) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 50; ++i) {
		std::string s = tgen::str("[0-9]").gen().to_std();
		EXPECT_EQ(s.size(), 1u);
		EXPECT_TRUE('0' <= s[0] and s[0] <= '9');
	}
}

TEST(str_test, gen_regex_repetition_fixed) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::str("a{3}").gen().to_std(), "aaa");
	EXPECT_EQ(tgen::str("x{1}").gen().to_std(), "x");
	EXPECT_EQ(tgen::str("ax{0}").gen().to_std(), "a");
}

TEST(str_test, gen_regex_repetition_range) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::set<int> lengths = {2, 3, 4};
	for (int i = 0; i < 100; ++i) {
		std::string s = tgen::str("a{2,4}").gen().to_std();
		EXPECT_TRUE(lengths.count(static_cast<int>(s.size())) == 1);
		for (char c : s)
			EXPECT_EQ(c, 'a');
	}
}

TEST(str_test, gen_regex_alternation) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	std::set<std::string> expected = {"a", "b"};
	for (int i = 0; i < 50; ++i) {
		std::string s = tgen::str("a|b").gen().to_std();
		EXPECT_TRUE(expected.count(s) == 1);
	}
}

TEST(str_test, gen_regex_complex) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	for (int i = 0; i < 100; ++i) {
		std::string s = tgen::str("[1-9][0-9]{1,2}").gen().to_std();
		EXPECT_TRUE(s.size() >= 2 and s.size() <= 3);
		EXPECT_TRUE('1' <= s[0] and s[0] <= '9');
		for (size_t j = 1; j < s.size(); ++j)
			EXPECT_TRUE('0' <= s[j] and s[j] <= '9');
	}
}

TEST(str_test, gen_regex_with_grouping) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::str("(ab){2}").gen().to_std(), "abab");
}

TEST(str_test, gen_regex_uniform) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

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
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_THROW_TGEN_PREFIX(tgen::str("a").fix(0, 'x'),
							 "str: cannot add restriction for regex");
	EXPECT_THROW_TGEN_PREFIX(tgen::str("a").distinct(),
							 "str: cannot add restriction for regex");
}

TEST(str_test, instance_ops) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

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
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	tgen::str::instance inst("ab");
	EXPECT_THROW_TGEN_PREFIX(inst[-1], "str: instane: index out of bounds");
	EXPECT_THROW_TGEN_PREFIX(inst[2], "str: instane: index out of bounds");
}

TEST(str_test, abacaba) {
	auto argv = get_argv({"./executable"});
	tgen::register_gen(argv.size() - 1, argv.data());

	EXPECT_EQ(tgen::str::abacaba(1).to_std(), "a");
	auto s = tgen::str::abacaba(19);
	EXPECT_EQ(s.size(), 19);
	EXPECT_EQ(s.to_std(), "abacabadabacabaeaba");
}
