#include <gtest/gtest.h>

#include <chrono>

#include "CommonTestFixture.h"
#include "SunriseSunset.h"
#include "TestConstants.h"

class SunriseSunsetTest : public CommonTestFixture {};

static double minutesDiff(int a, int b) {
    int d = std::abs(a - b);
    return std::min(d, 24 * 60 - d);
}

TEST_F(SunriseSunsetTest, InvalidLocationReturnsNoSunriseOrSunset) {
    SunriseSunset ss;
    ss.setLocation(1000.0, 0.0);

    auto r = ss.calculate(2025, 6, 21);
    EXPECT_FALSE(r.hasSunrise);
    EXPECT_FALSE(r.hasSunset);
}

TEST_F(SunriseSunsetTest, DayOfYearLeapYearFeb29) {
    // Edge: leap year calculation
    EXPECT_EQ(SunriseSunset::dayOfYear(2024, 2, 29), 60);
}

TEST_F(SunriseSunsetTest, WrapMinutesNormalizesNegativeAndOverflows) {
    EXPECT_EQ(SunriseSunset::wrapMinutes(-1), 1439);
    EXPECT_EQ(SunriseSunset::wrapMinutes(1440), 0);
    EXPECT_EQ(SunriseSunset::wrapMinutes(1441), 1);
}

TEST_F(SunriseSunsetTest, EquatorEquinoxIsApproximatelySixToEighteenUtc) {
    SunriseSunset ss;
    ss.setLocation(0.0, 0.0);
    ss.setTimezoneOffsetMinutes(0);

    auto r = ss.calculate(2025, 3, 20);
    ASSERT_TRUE(r.hasSunrise);
    ASSERT_TRUE(r.hasSunset);

    // Approximate check (algorithm errors and atmospheric conditions vary)
    EXPECT_LT(minutesDiff(r.sunriseUtc.toMinutes(), 6 * 60), 30.0);
    EXPECT_LT(minutesDiff(r.sunsetUtc.toMinutes(), 18 * 60), 30.0);
}

TEST_F(SunriseSunsetTest, NewYorkCityJuneSolsticeMatchesApproxLocalTimes) {
    SunriseSunset ss;
    ss.setLocation(TestConstants::kNYCLat, TestConstants::kNYCLon);
    ss.setTimezoneOffsetMinutes(-4 * 60); // EDT

    auto r = ss.calculate(2025, 6, 21);
    ASSERT_TRUE(r.hasSunrise);
    ASSERT_TRUE(r.hasSunset);

    int expectedSunrise = 5 * 60 + 25;
    int expectedSunset = 20 * 60 + 31;

    EXPECT_LT(minutesDiff(r.sunriseLocal.toMinutes(), expectedSunrise), TestConstants::kTimeToleranceMinutes);
    EXPECT_LT(minutesDiff(r.sunsetLocal.toMinutes(), expectedSunset), TestConstants::kTimeToleranceMinutes);
}

TEST_F(SunriseSunsetTest, LondonWinterSolsticeHasShortDay) {
    SunriseSunset ss;
    ss.setLocation(TestConstants::kLondonLat, TestConstants::kLondonLon);
    ss.setTimezoneOffsetMinutes(0);

    auto r = ss.calculate(2025, 12, 21);
    ASSERT_TRUE(r.hasSunrise);
    ASSERT_TRUE(r.hasSunset);

    // Rough expectations around 08:03 / 15:53 UTC.
    int expectedSunrise = 8 * 60 + 3;
    int expectedSunset = 15 * 60 + 53;

    EXPECT_LT(minutesDiff(r.sunriseUtc.toMinutes(), expectedSunrise), 30.0);
    EXPECT_LT(minutesDiff(r.sunsetUtc.toMinutes(), expectedSunset), 30.0);
    EXPECT_LT(r.sunsetUtc.toMinutes() - r.sunriseUtc.toMinutes(), 10 * 60);
}

TEST_F(SunriseSunsetTest, TimezoneConversionCanCrossMidnight) {
    SunriseSunset ss;
    ss.setLocation(0.0, 179.9);
    ss.setTimezoneOffsetMinutes(14 * 60); // UTC+14

    auto r = ss.calculate(2025, 3, 20);
    ASSERT_TRUE(r.hasSunrise);

    // Edge: local time should still be valid and wrapped.
    EXPECT_GE(r.sunriseLocal.hour, 0);
    EXPECT_LE(r.sunriseLocal.hour, 23);
}

TEST_F(SunriseSunsetTest, PolarDayProducesNoSunsetOrSunrise) {
    SunriseSunset ss;
    ss.setLocation(69.6492, 18.9553); // Tromso
    ss.setTimezoneOffsetMinutes(2 * 60);

    auto r = ss.calculate(2025, 6, 21);
    // Edge: sun can be above horizon all day.
    EXPECT_FALSE(r.hasSunrise);
    EXPECT_FALSE(r.hasSunset);
}

TEST_F(SunriseSunsetTest, PolarNightProducesNoSunriseOrSunset) {
    SunriseSunset ss;
    ss.setLocation(69.6492, 18.9553);
    ss.setTimezoneOffsetMinutes(1 * 60);

    auto r = ss.calculate(2025, 12, 21);
    EXPECT_FALSE(r.hasSunrise);
    EXPECT_FALSE(r.hasSunset);
}

TEST_F(SunriseSunsetTest, ForTypicalLocationsSunriseIsBeforeSunsetInLocalMinutes) {
    SunriseSunset ss;
    ss.setLocation(TestConstants::kNYCLat, TestConstants::kNYCLon);
    ss.setTimezoneOffsetMinutes(-5 * 60); // EST

    auto r = ss.calculate(2025, 1, 15);
    ASSERT_TRUE(r.hasSunrise);
    ASSERT_TRUE(r.hasSunset);

    EXPECT_LT(r.sunriseLocal.toMinutes(), r.sunsetLocal.toMinutes());
}

TEST_F(SunriseSunsetTest, PerformanceCalculatingManyDaysIsFast) {
    SunriseSunset ss;
    ss.setLocation(TestConstants::kNYCLat, TestConstants::kNYCLon);
    ss.setTimezoneOffsetMinutes(-5 * 60);

    auto start = std::chrono::steady_clock::now();
    for (int day = 1; day <= 28; ++day) {
        auto r = ss.calculate(2025, 2, day);
        (void)r;
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    EXPECT_LT(elapsed.count(), 1000);
}
