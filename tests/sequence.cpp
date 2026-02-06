#include <gtest/gtest.h>

#include "tgen.h"

inline void EXPECT_STARTS_WITH(const std::string &msg,
							   const std::string &prefix) {
	EXPECT_TRUE(msg.rfind(prefix, 0) == 0)
		<< "Expected prefix: \"" << prefix << "\"\n"
		<< "Actual message: \"" << msg << "\"";
}

TEST(general_test, sequence_constructor_invalid_range) {
	char arg0[] = "./executable";
	char *argv[] = {arg0, nullptr};
	tgen::register_gen(1, argv);

	try {
		auto s = tgen::sequence<int>(10, 2, 1);
		FAIL() << "Expected std::runtime_error, but no error ocurred";
	} catch (const std::runtime_error &e) {
		EXPECT_STARTS_WITH(e.what(), "tgen: value range must be valid");
	} catch (...) {
		FAIL() << "Expected std::runtime_error, but caught a different type";
	}
}
