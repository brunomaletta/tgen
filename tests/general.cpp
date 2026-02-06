#include <gtest/gtest.h>

#include "tgen.h"

TEST(general_test, next) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	for (int i = 0; i < 100; ++i) {
		int next_int = tgen::next<int>(1, 100);
		EXPECT_TRUE(1 <= next_int and next_int <= 100);
	}
}
