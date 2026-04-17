
#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

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