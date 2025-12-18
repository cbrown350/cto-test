#ifndef COMMON_TEST_FIXTURE_H
#define COMMON_TEST_FIXTURE_H

#include <gtest/gtest.h>

class CommonTestFixture : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
};

#endif // COMMON_TEST_FIXTURE_H
