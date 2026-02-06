#include <gtest/gtest.h>

#include "tgen.h"

TEST(opts_test, has_opt) {
	char arg0[] = "./executable";
	char arg1[] = "-n";
	char arg2[] = "10";
	char *argv[] = {arg0, arg1, arg2, nullptr};
	tgen::register_gen(3, argv);

	EXPECT_EQ(tgen::has_opt("n"), true);
}
