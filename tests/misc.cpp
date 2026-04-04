#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

TEST(misc_test, unordered_hack) {
	tgen::register_gen();

	int size = 1e6;
	std::vector<long long> hack = tgen::misc::unordered_hack(size);

	EXPECT_EQ(hack.size(), size);
}