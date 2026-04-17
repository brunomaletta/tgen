#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

TEST(misc_test, parenthesis) {
	tgen::register_gen();

	for (int i = 0; i < 100; ++i) {
		std::string s = tgen::misc::gen_parenthesis(10);
		EXPECT_EQ(s.size(), 10);
		int sum = 0;
		for (char c : s) {
			sum += (c == '(' ? 1 : -1);
			EXPECT_TRUE(sum >= 0);
		}
	}

	check_function_uniform(tgen::misc::gen_parenthesis, 5, 6);
	check_function_uniform(tgen::misc::gen_parenthesis, 14, 8);
}