#include <gtest/gtest.h>

#include <chrono>

#include "CommonTestFixture.h"
#include "MockLightController.h"

class LightControllerTest : public CommonTestFixture {
protected:
    MockLightController light;

    void SetUp() override {
        CommonTestFixture::SetUp();

        MockLightController::Config cfg;
        cfg.enableLight = true;
        cfg.maxBrightness = 200;
        cfg.minBrightness = 10;
        cfg.fadeInDuration = 10;
        cfg.fadeOutDuration = 10;
        cfg.dayStartHour = 6;
        cfg.dayEndHour = 22;
        light.setConfig(cfg);
        light.setMode(MockLightController::LightMode::AUTO);
    }
};

TEST_F(LightControllerTest, ManualBrightnessClampsToMax) {
    light.setMode(MockLightController::LightMode::MANUAL_ON);
    light.setManualBrightness(255);
    EXPECT_EQ(light.getManualBrightness(), 200);
}

TEST_F(LightControllerTest, ManualBrightnessClampsToMin) {
    light.setMode(MockLightController::LightMode::MANUAL_ON);
    light.setManualBrightness(0);
    EXPECT_EQ(light.getManualBrightness(), 10);
}

TEST_F(LightControllerTest, SetOnTrueSwitchesToManualOn) {
    light.setOn(true);
    EXPECT_EQ(light.getMode(), MockLightController::LightMode::MANUAL_ON);
    EXPECT_TRUE(light.isOn());
}

TEST_F(LightControllerTest, SetOnFalseSwitchesToManualOffAndTransitionsToZero) {
    light.setOn(true);
    light.processTick();

    light.setOn(false);
    EXPECT_EQ(light.getMode(), MockLightController::LightMode::MANUAL_OFF);
    EXPECT_TRUE(light.isTransitioning());
}

TEST_F(LightControllerTest, AutoModeTurnsOnAtDayStartBoundary) {
    // Edge: dayStartHour boundary should be inclusive.
    light.setMode(MockLightController::LightMode::AUTO);
    light.setCurrentTime(6, 0);
    EXPECT_TRUE(light.isOn());
}

TEST_F(LightControllerTest, AutoModeTurnsOffOutsideDayHours) {
    light.setMode(MockLightController::LightMode::AUTO);
    light.setCurrentTime(2, 0);
    EXPECT_FALSE(light.isOn());
}

TEST_F(LightControllerTest, AutoModeUsesSunriseSunsetTimesWhenEnabled) {
    MockLightController::Config cfg = light.getConfig();
    cfg.enableSunriseSunset = true;
    cfg.maxBrightness = 150;
    light.setConfig(cfg);

    light.setMode(MockLightController::LightMode::AUTO);
    light.setSunriseTime(7, 0);
    light.setSunsetTime(19, 0);

    light.setCurrentTime(6, 59);
    EXPECT_FALSE(light.isOn());

    light.setCurrentTime(7, 0);
    EXPECT_TRUE(light.isOn());
    EXPECT_EQ(light.getBrightness(), 150);

    light.setCurrentTime(19, 1);
    EXPECT_FALSE(light.isOn());
}

TEST_F(LightControllerTest, SetCurrentTimeClampsHourAndMinute) {
    // Edge: hour>23 and minute>59 should clamp to 23:59.
    light.setCurrentTime(99, 99);
    EXPECT_FALSE(light.getState().isOn);
}

TEST_F(LightControllerTest, StartTransitionActivatesTransitionState) {
    light.setMode(MockLightController::LightMode::MANUAL_ON);
    light.setManualBrightness(200);
    light.processTick();

    light.startTransition(10);
    EXPECT_TRUE(light.isTransitionActive());
    EXPECT_NEAR(light.getTransitionProgress(), 0.0f, 0.001f);
}

TEST_F(LightControllerTest, TransitionCompletesAfterSimulatedTime) {
    light.setMode(MockLightController::LightMode::MANUAL_ON);
    light.setManualBrightness(200);
    light.processTick();

    light.startTransition(10);
    light.simulateTimeAdvance(std::chrono::seconds(20));

    EXPECT_FALSE(light.isTransitionActive());
    EXPECT_EQ(light.getBrightness(), 10);
}

TEST_F(LightControllerTest, StopTransitionStopsTransitionAndSineWave) {
    light.setMode(MockLightController::LightMode::MANUAL_ON);
    light.setManualBrightness(200);
    light.startTransition(10);
    light.startSineWaveTransition(10);

    light.stopTransition();

    EXPECT_FALSE(light.isTransitionActive());
    EXPECT_FALSE(light.isSineWaveActive());
}

TEST_F(LightControllerTest, StartSineWaveTransitionActivatesFlag) {
    light.setMode(MockLightController::LightMode::MANUAL_ON);
    light.startSineWaveTransition(10);
    EXPECT_TRUE(light.isSineWaveActive());
}

TEST_F(LightControllerTest, SineWaveTransitionChangesBrightnessOverTime) {
    light.setMode(MockLightController::LightMode::MANUAL_ON);
    light.startSineWaveTransition(10);

    uint8_t b0 = light.getBrightness();
    light.simulateTimeAdvance(std::chrono::seconds(2));
    uint8_t b1 = light.getBrightness();

    EXPECT_NE(b0, b1);
}

TEST_F(LightControllerTest, SineWaveTransitionStopsAfterDuration) {
    light.setMode(MockLightController::LightMode::MANUAL_ON);
    light.startSineWaveTransition(2);

    light.simulateTimeAdvance(std::chrono::seconds(3));
    EXPECT_FALSE(light.isSineWaveActive());
}

TEST_F(LightControllerTest, DisableForcesOffAndZeroBrightness) {
    light.setOn(true);
    light.disable();

    EXPECT_EQ(light.getMode(), MockLightController::LightMode::DISABLED);
    EXPECT_FALSE(light.isOn());
    EXPECT_EQ(light.getBrightness(), 0);
}

TEST_F(LightControllerTest, ResetStatisticsResetsDurations) {
    light.setOn(true);
    light.simulateTimeAdvance(std::chrono::seconds(3));

    ASSERT_GT(light.getState().onDuration, 0u);

    light.resetStatistics();
    EXPECT_EQ(light.getState().onDuration, 0u);
    EXPECT_EQ(light.getState().offDuration, 0u);
}

TEST_F(LightControllerTest, ProcessTickAccumulatesOnDurationWhenOn) {
    light.setOn(true);
    light.simulateTimeAdvance(std::chrono::seconds(5));
    EXPECT_GE(light.getState().onDuration, 5u);
}

TEST_F(LightControllerTest, ProcessTickAccumulatesOffDurationWhenOff) {
    light.setOn(false);
    light.simulateTimeAdvance(std::chrono::seconds(5));
    EXPECT_GE(light.getState().offDuration, 5u);
}

TEST_F(LightControllerTest, ManualOverrideFlagSetGet) {
    light.setManualOverride(true);
    EXPECT_TRUE(light.getManualOverride());

    light.setManualOverride(false);
    EXPECT_FALSE(light.getManualOverride());
}

TEST_F(LightControllerTest, StartTransitionToSameBrightnessDoesNotActivateTransition) {
    // Edge: starting a transition to the current brightness should be a no-op.
    light.setMode(MockLightController::LightMode::MANUAL_ON);
    light.setManualBrightness(100);
    light.processTick();

    light.startTransition(light.getBrightness());
    EXPECT_FALSE(light.isTransitionActive());
}

TEST_F(LightControllerTest, TransitionDurationIsNeverZeroForSmallDelta) {
    // Edge: brightnessDiff > 0 should still produce a duration >= 1, avoiding divide-by-zero.
    light.setMode(MockLightController::LightMode::MANUAL_ON);
    light.setManualBrightness(11);
    light.processTick();

    light.startTransition(10);
    EXPECT_TRUE(light.isTransitionActive());

    light.simulateTimeAdvance(std::chrono::seconds(2));
    EXPECT_FALSE(light.isTransitionActive());
    EXPECT_EQ(light.getBrightness(), 10);
}

TEST_F(LightControllerTest, PerformanceProcessTickTenThousandIterationsFast) {
    light.setOn(true);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 10000; ++i) {
        light.processTick();
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    EXPECT_LT(elapsed.count(), 2000);
}
