#include <gtest/gtest.h>

int add(int a, int b) {
    return a + b;
}

TEST(AddTest, WorksForPositiveNumbers) {
    EXPECT_EQ(add(2, 3), 5);
}

TEST(AddTest, WorksForZero) {
    EXPECT_EQ(add(0, 0), 0);
}
