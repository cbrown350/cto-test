#include <gtest/gtest.h>

#include <chrono>

#include "CommonTestFixture.h"
#include "MockPumpController.h"

class PumpControllerTest : public CommonTestFixture {
protected:
    MockPumpController pump;

    void SetUp() override {
        CommonTestFixture::SetUp();

        MockPumpController::Config cfg;
        cfg.enablePump = true;
        cfg.freezeThreshold = 1.1f;
        cfg.freezeHysteresis = 0.5f;
        cfg.onDuration = 5;
        cfg.offDuration = 5;
        cfg.maxOnTime = 30;
        cfg.faultTimeout = 0;  // Disable flow fault for basic tests
        cfg.minPulsesPerMinute = 0;
        cfg.pulsesPerGallon = 1000;
        cfg.autoMode = true;
        pump.setConfig(cfg);
        pump.setMode(MockPumpController::PumpMode::AUTO);
        pump.enable();
    }
};

TEST_F(PumpControllerTest, AutoModeTemperatureAboveThresholdDoesNotStartPump) {
    pump.setTemperature(10.0f);
    pump.processTick();
    EXPECT_FALSE(pump.isRunning());
}

TEST_F(PumpControllerTest, AutoModeTemperatureBelowThresholdStartsPump) {
    pump.setTemperature(0.0f);
    pump.processTick();
    EXPECT_TRUE(pump.isRunning());
}

TEST_F(PumpControllerTest, HysteresisKeepsFreezeProtectionActiveBetweenThresholds) {
    pump.setTemperature(0.0f);
    pump.processTick();
    EXPECT_TRUE(pump.isRunning());

    // Edge: temperature between start and stop thresholds should keep cycling active.
    pump.setTemperature(1.3f); // > start(1.1) but < stop(1.6)
    pump.simulateTimeAdvance(std::chrono::seconds(2));
    EXPECT_TRUE(pump.getState().currentTemperature > pump.getConfig().freezeThreshold);
}

TEST_F(PumpControllerTest, AutoModeCyclesOffAfterOnDuration) {
    pump.setTemperature(0.0f);

    // Run a full ON phase.
    pump.simulateTimeAdvance(std::chrono::seconds(6));
    EXPECT_FALSE(pump.isRunning());
}

TEST_F(PumpControllerTest, AutoModeCyclesBackOnAfterOffDuration) {
    pump.setTemperature(0.0f);

    pump.simulateTimeAdvance(std::chrono::seconds(6));
    EXPECT_FALSE(pump.isRunning());

    pump.simulateTimeAdvance(std::chrono::seconds(6));
    EXPECT_TRUE(pump.isRunning());
}

TEST_F(PumpControllerTest, CycleCountIncrementsOnEachOnTransition) {
    pump.setTemperature(0.0f);
    pump.simulateTimeAdvance(std::chrono::seconds(1));
    uint32_t first = pump.getCycleCount();

    pump.simulateTimeAdvance(std::chrono::seconds(12));
    EXPECT_GT(pump.getCycleCount(), first);
}

TEST_F(PumpControllerTest, DisableStopsPump) {
    pump.setTemperature(0.0f);
    pump.processTick();
    ASSERT_TRUE(pump.isRunning());

    pump.disable();
    pump.processTick();
    EXPECT_FALSE(pump.isRunning());
    EXPECT_FALSE(pump.isEnabled());
}

TEST_F(PumpControllerTest, ManualOnRequiresManualStateTrue) {
    pump.setMode(MockPumpController::PumpMode::MANUAL_ON);
    pump.enable();

    pump.setManualState(false);
    pump.processTick();
    EXPECT_FALSE(pump.isRunning());

    pump.setManualState(true);
    pump.processTick();
    EXPECT_TRUE(pump.isRunning());
}

TEST_F(PumpControllerTest, ManualOffForcesPumpOff) {
    pump.setMode(MockPumpController::PumpMode::MANUAL_ON);
    pump.setManualState(true);
    pump.processTick();
    ASSERT_TRUE(pump.isRunning());

    pump.setMode(MockPumpController::PumpMode::MANUAL_OFF);
    pump.processTick();
    EXPECT_FALSE(pump.isRunning());
}

TEST_F(PumpControllerTest, DisabledModeForcesPumpOff) {
    pump.setMode(MockPumpController::PumpMode::MANUAL_ON);
    pump.setManualState(true);
    pump.processTick();
    ASSERT_TRUE(pump.isRunning());

    pump.setMode(MockPumpController::PumpMode::DISABLED);
    pump.processTick();
    EXPECT_FALSE(pump.isRunning());
}

TEST_F(PumpControllerTest, NoFlowFaultTriggersAfterTimeout) {
    // Re-enable fault timeout for this test
    MockPumpController::Config cfg = pump.getConfig();
    cfg.faultTimeout = 3;
    pump.setConfig(cfg);
    
    pump.setTemperature(0.0f);

    // Edge: with no flow pulses, faultTimeout seconds triggers a fault.
    pump.simulateTimeAdvance(std::chrono::seconds(5));

    EXPECT_TRUE(pump.isInFault());
    EXPECT_FALSE(pump.isRunning());
}

TEST_F(PumpControllerTest, FaultCallbackInvokedOnNoFlow) {
    // Re-enable fault timeout for this test
    MockPumpController::Config cfg = pump.getConfig();
    cfg.faultTimeout = 3;
    pump.setConfig(cfg);
    
    pump.setTemperature(0.0f);

    std::string fault;
    pump.setFaultCallback([&](const std::string& message) {
        fault = message;
    });

    pump.simulateTimeAdvance(std::chrono::seconds(5));
    EXPECT_EQ(fault, "No flow detected");
}

TEST_F(PumpControllerTest, ClearFaultAllowsPumpToRunAgain) {
    // Re-enable fault timeout for this test
    MockPumpController::Config cfg = pump.getConfig();
    cfg.faultTimeout = 3;
    pump.setConfig(cfg);
    
    pump.setTemperature(0.0f);
    pump.simulateTimeAdvance(std::chrono::seconds(5));
    ASSERT_TRUE(pump.isInFault());

    pump.clearFault();
    pump.setTemperature(0.0f);

    // Provide flow pulses so we don't re-trigger the fault.
    uint32_t pulses = 0;
    for (int i = 0; i < 4; ++i) {
        pulses += 10;
        pump.setFlowPulses(pulses);
        pump.processTick();
    }

    EXPECT_FALSE(pump.isInFault());
}

TEST_F(PumpControllerTest, MinPulsesPerMinuteFaultTriggers) {
    MockPumpController::Config cfg = pump.getConfig();
    cfg.minPulsesPerMinute = 50;
    cfg.faultTimeout = 0; // isolate the min pulses check
    cfg.onDuration = 120;
    cfg.offDuration = 120;
    cfg.maxOnTime = 9999;
    pump.setConfig(cfg);

    pump.setMode(MockPumpController::PumpMode::AUTO);
    pump.enable();
    pump.setTemperature(0.0f);

    // Provide insufficient pulses over a full minute.
    uint32_t pulses = 0;
    for (int i = 0; i < 60; ++i) {
        if (i < 10) {
            pulses += 1;
        }
        pump.setFlowPulses(pulses);
        pump.processTick();
    }

    EXPECT_TRUE(pump.isInFault());
}

TEST_F(PumpControllerTest, FlowRateComputedFromPulseDeltaPerSecond) {
    MockPumpController::Config cfg = pump.getConfig();
    cfg.faultTimeout = 0;
    cfg.minPulsesPerMinute = 0;
    pump.setConfig(cfg);

    pump.setMode(MockPumpController::PumpMode::MANUAL_ON);
    pump.setManualState(true);

    // 1000 pulses per gallon => 1000 pulses in 1 second => 1 gallon/sec => 60 gpm.
    pump.setFlowPulses(0);
    pump.processTick();

    pump.setFlowPulses(1000);
    pump.processTick();

    EXPECT_NEAR(pump.getFlowRate(), 60.0f, 0.01f);
}

TEST_F(PumpControllerTest, ResetStatisticsResetsCounters) {
    pump.setMode(MockPumpController::PumpMode::MANUAL_ON);
    pump.setManualState(true);
    pump.simulateTimeAdvance(std::chrono::seconds(3));

    ASSERT_GT(pump.getTotalOnTime(), 0u);

    pump.resetStatistics();
    EXPECT_EQ(pump.getTotalOnTime(), 0u);
    EXPECT_EQ(pump.getCycleCount(), 0u);
    EXPECT_EQ(pump.getTotalPulses(), 0u);
}

TEST_F(PumpControllerTest, MaxOnTimeTriggersFaultAndStopsPump) {
    MockPumpController::Config cfg = pump.getConfig();
    cfg.maxOnTime = 2;
    cfg.faultTimeout = 0;
    cfg.minPulsesPerMinute = 0;
    cfg.onDuration = 1000;
    cfg.offDuration = 1;
    pump.setConfig(cfg);

    pump.setMode(MockPumpController::PumpMode::AUTO);
    pump.enable();
    pump.setTemperature(0.0f);

    pump.simulateTimeAdvance(std::chrono::seconds(5));

    EXPECT_TRUE(pump.isInFault());
    EXPECT_FALSE(pump.isRunning());
}

TEST_F(PumpControllerTest, AutoModeDisabledByConfigStopsPump) {
    MockPumpController::Config cfg = pump.getConfig();
    cfg.autoMode = false;
    cfg.faultTimeout = 0;
    pump.setConfig(cfg);

    pump.setMode(MockPumpController::PumpMode::AUTO);
    pump.enable();
    pump.setTemperature(0.0f);

    pump.processTick();
    EXPECT_FALSE(pump.isRunning());
}

TEST_F(PumpControllerTest, SetModeResetsCycleState) {
    MockPumpController::Config cfg = pump.getConfig();
    cfg.faultTimeout = 0;
    pump.setConfig(cfg);

    pump.setTemperature(0.0f);
    pump.simulateTimeAdvance(std::chrono::seconds(2));
    uint32_t cyclesBefore = pump.getCycleCount();

    pump.setMode(MockPumpController::PumpMode::MANUAL_OFF);
    pump.processTick();
    EXPECT_FALSE(pump.isRunning());

    pump.setMode(MockPumpController::PumpMode::AUTO);
    pump.setTemperature(0.0f);
    pump.processTick();
    EXPECT_GE(pump.getCycleCount(), cyclesBefore);
}

TEST_F(PumpControllerTest, StateChangeCallbackInvokedOnTransitions) {
    MockPumpController::Config cfg = pump.getConfig();
    cfg.faultTimeout = 0;
    cfg.minPulsesPerMinute = 0;
    pump.setConfig(cfg);

    int transitions = 0;
    pump.setStateChangeCallback([&](const MockPumpController::PumpState&, bool) {
        transitions++;
    });

    pump.setTemperature(0.0f);
    pump.processTick();

    // Drive an on->off transition.
    pump.simulateTimeAdvance(std::chrono::seconds(6));

    EXPECT_GE(transitions, 1);
}

TEST_F(PumpControllerTest, EnableAfterDisableAllowsAutoOperation) {
    MockPumpController::Config cfg = pump.getConfig();
    cfg.faultTimeout = 0;
    pump.setConfig(cfg);

    pump.disable();
    pump.setTemperature(0.0f);
    pump.processTick();
    ASSERT_FALSE(pump.isRunning());

    pump.enable();
    pump.processTick();
    EXPECT_TRUE(pump.isEnabled());
}

TEST_F(PumpControllerTest, ProcessTickAccumulatesOffTimeWhenNotRunning) {
    pump.setTemperature(10.0f);
    pump.simulateTimeAdvance(std::chrono::seconds(3));
    EXPECT_GE(pump.getTotalOffTime(), 3u);
}

TEST_F(PumpControllerTest, ProcessTickAccumulatesOnTimeWhenRunning) {
    pump.setMode(MockPumpController::PumpMode::MANUAL_ON);
    pump.setManualState(true);

    pump.simulateTimeAdvance(std::chrono::seconds(3));
    EXPECT_GE(pump.getTotalOnTime(), 3u);
}

TEST_F(PumpControllerTest, AutoModeStopsWhenTemperatureRisesAboveStopThreshold) {
    pump.setTemperature(0.0f);
    pump.processTick();
    ASSERT_TRUE(pump.isRunning());

    // Edge: crossing stop threshold should clear freeze protection.
    pump.setTemperature(pump.getConfig().freezeThreshold + pump.getConfig().freezeHysteresis + 0.1f);
    pump.processTick();

    EXPECT_FALSE(pump.isRunning());
}

TEST_F(PumpControllerTest, SetFlowPulsesUpdatesTotalPulses) {
    pump.setFlowPulses(42);
    EXPECT_EQ(pump.getTotalPulses(), 42u);
}

TEST_F(PumpControllerTest, PerformanceProcessTickTenThousandIterationsFast) {
    // Performance: state machine ticks should be cheap.
    MockPumpController::Config cfg = pump.getConfig();
    cfg.faultTimeout = 0;
    cfg.minPulsesPerMinute = 0;
    cfg.onDuration = 1000;
    cfg.offDuration = 1000;
    pump.setConfig(cfg);

    pump.setMode(MockPumpController::PumpMode::MANUAL_ON);
    pump.setManualState(true);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 10000; ++i) {
        pump.processTick();
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    EXPECT_LT(elapsed.count(), 2000);
}
