#include <gtest/gtest.h>

#include <chrono>

#include "CommonTestFixture.h"
#include "MockSensorManager.h"
#include "MockPumpController.h"
#include "TestConstants.h"

class SensorManagerTest : public CommonTestFixture {
protected:
    MockSensorManager sensors;

    void SetUp() override {
        CommonTestFixture::SetUp();

        MockSensorManager::Config cfg;
        cfg.enableFirstSensor = true;
        cfg.enableSecondSensor = true;
        cfg.pulsesPerGallon = 1000;
        sensors.setConfig(cfg);
    }
};

TEST_F(SensorManagerTest, SetConfigCreatesTwoSensors) {
    EXPECT_EQ(sensors.getAllSensorData().size(), 2u);
}

TEST_F(SensorManagerTest, SetConfigWithNoSensorsCreatesEmptyList) {
    MockSensorManager::Config cfg;
    cfg.enableFirstSensor = false;
    cfg.enableSecondSensor = false;
    sensors.setConfig(cfg);

    EXPECT_TRUE(sensors.getAllSensorData().empty());
}

TEST_F(SensorManagerTest, SetTemperatureClampsToMinimum) {
    // Edge: below DS18B20 minimum should clamp to -55C.
    sensors.setTemperature(-1000.0f, 0);
    EXPECT_FLOAT_EQ(sensors.getSensorData(0).temperature, sensors.getConfig().minTemperature);
}

TEST_F(SensorManagerTest, SetTemperatureClampsToMaximum) {
    // Edge: above DS18B20 maximum should clamp to 125C.
    sensors.setTemperature(1000.0f, 0);
    EXPECT_FLOAT_EQ(sensors.getSensorData(0).temperature, sensors.getConfig().maxTemperature);
}

TEST_F(SensorManagerTest, SetRandomTemperatureStaysInRange) {
    sensors.setRandomTemperature(0);
    float t = sensors.getSensorData(0).temperature;
    EXPECT_GE(t, sensors.getConfig().minTemperature);
    EXPECT_LE(t, sensors.getConfig().maxTemperature);
}

TEST_F(SensorManagerTest, SetGradientTemperatureEndsAtExpectedValue) {
    sensors.setGradientTemperature(0.0f, 10.0f, 5, 0);
    EXPECT_NEAR(sensors.getSensorData(0).temperature, 10.0f, TestConstants::kFloatEpsilon);
}

TEST_F(SensorManagerTest, SetGradientTemperatureWithInvalidStepsDoesNothing) {
    // Edge: steps <= 0 should be ignored.
    sensors.setTemperature(5.0f, 0);
    sensors.setGradientTemperature(0.0f, 10.0f, 0, 0);
    EXPECT_FLOAT_EQ(sensors.getSensorData(0).temperature, 5.0f);
}

TEST_F(SensorManagerTest, SetPulseCountUpdatesCountAndResetsFlowRate) {
    sensors.setSensorType(true, 0);
    sensors.setPulseCount(123, 0);

    auto d = sensors.getSensorData(0);
    EXPECT_EQ(d.pulseCount, 123u);
    EXPECT_FLOAT_EQ(d.flowRateGPM, 0.0f);
}

TEST_F(SensorManagerTest, GeneratePulsesIncrementsCount) {
    sensors.setPulseCount(10, 0);
    sensors.generatePulses(5, 0);
    EXPECT_EQ(sensors.getSensorData(0).pulseCount, 15u);
}

TEST_F(SensorManagerTest, SetSensorTypeWaterMeterDetection) {
    sensors.setSensorType(true, 1);
    EXPECT_TRUE(sensors.isWaterMeterDetected(1));
}

TEST_F(SensorManagerTest, SimulateSensorFailureMarksInvalid) {
    sensors.simulateSensorFailure(0);
    EXPECT_FALSE(sensors.isSensorValid(0));
}

TEST_F(SensorManagerTest, SimulateSensorRecoveryMarksValid) {
    sensors.simulateSensorFailure(0);
    sensors.simulateSensorRecovery(0);
    EXPECT_TRUE(sensors.isSensorValid(0));
}

TEST_F(SensorManagerTest, InvalidIndexReturnsSafeDefaults) {
    EXPECT_FALSE(sensors.isSensorValid(99));
    EXPECT_FALSE(sensors.isWaterMeterDetected(99));

    auto d = sensors.getSensorData(99);
    EXPECT_FALSE(d.isValid);
    EXPECT_EQ(d.pulseCount, 0u);
}

TEST_F(SensorManagerTest, CallbackInvokedForSingleSensor) {
    int calls = 0;
    sensors.setDataCallback([&](const MockSensorManager::SensorData&, int idx) {
        if (idx == 0) {
            calls++;
        }
    }, 0);

    sensors.setTemperature(12.0f, 0);
    sensors.setTemperature(13.0f, 1);

    EXPECT_EQ(calls, 1);
}

TEST_F(SensorManagerTest, CallbackInvokedForAllSensors) {
    int calls = 0;
    sensors.setDataCallback([&](const MockSensorManager::SensorData&, int) {
        calls++;
    });

    sensors.setTemperature(12.0f, 0);
    sensors.setTemperature(13.0f, 1);

    EXPECT_EQ(calls, 2);
}

TEST_F(SensorManagerTest, FlowRateCalculatedFromPulsesAndTime) {
    // Edge: compute flow using a controlled time delta.
    sensors.setSensorType(true, 0);
    sensors.setPulseCount(0, 0);

    sensors.simulateTimeAdvance(std::chrono::milliseconds(30000)); // 30s
    sensors.generatePulses(500, 0); // 0.5 gallons over 0.5 minutes => 1 GPM

    EXPECT_NEAR(sensors.getFlowRateGPM(0), 1.0f, 0.05f);
    EXPECT_NEAR(sensors.getTotalGallons(0), 0.5f, 0.001f);
}

TEST_F(SensorManagerTest, TotalGallonsAccumulatesAcrossUpdates) {
    sensors.setSensorType(true, 0);
    sensors.setPulseCount(0, 0);

    sensors.simulateTimeAdvance(std::chrono::milliseconds(60000));
    sensors.generatePulses(1000, 0);

    sensors.simulateTimeAdvance(std::chrono::milliseconds(60000));
    sensors.generatePulses(500, 0);

    EXPECT_NEAR(sensors.getTotalGallons(0), 1.5f, 0.001f);
}

TEST_F(SensorManagerTest, PulseGenerationAddsPulsesOverSimulatedTime) {
    sensors.setSensorType(true, 0);
    sensors.setPulseCount(0, 0);

    sensors.startPulseGeneration(100, 0); // 100 pulses/sec
    sensors.simulateTimeAdvance(std::chrono::milliseconds(1000));

    EXPECT_EQ(sensors.getSensorData(0).pulseCount, 100u);
}

TEST_F(SensorManagerTest, StopPulseGenerationStopsAddingPulses) {
    sensors.setSensorType(true, 0);
    sensors.setPulseCount(0, 0);

    sensors.startPulseGeneration(100, 0);
    sensors.simulateTimeAdvance(std::chrono::milliseconds(1000));
    sensors.stopPulseGeneration(0);
    sensors.simulateTimeAdvance(std::chrono::milliseconds(1000));

    EXPECT_EQ(sensors.getSensorData(0).pulseCount, 100u);
}

TEST_F(SensorManagerTest, ResetFlowStatisticsResetsGallonsAndFlowRate) {
    sensors.setSensorType(true, 0);
    sensors.setPulseCount(0, 0);

    sensors.simulateTimeAdvance(std::chrono::milliseconds(60000));
    sensors.generatePulses(1000, 0);

    sensors.resetFlowStatistics(0);

    EXPECT_FLOAT_EQ(sensors.getTotalGallons(0), 0.0f);
    EXPECT_FLOAT_EQ(sensors.getFlowRateGPM(0), 0.0f);
}

TEST_F(SensorManagerTest, ProcessTickWithZeroDeltaDoesNotChangeTimeOrPulses) {
    sensors.setSensorType(true, 0);
    sensors.setPulseCount(0, 0);
    sensors.startPulseGeneration(100, 0);

    sensors.processTick(std::chrono::milliseconds(0));
    EXPECT_EQ(sensors.getSensorData(0).pulseCount, 0u);
}

TEST_F(SensorManagerTest, IntegrationSensorPulsesCanFeedPumpFlowCounter) {
    // Integration: water meter pulses drive pump flow input.
    MockPumpController pump;
    MockPumpController::Config cfg;
    cfg.onDuration = 5;
    cfg.offDuration = 5;
    cfg.faultTimeout = 3;
    cfg.maxOnTime = 30;
    cfg.minPulsesPerMinute = 1;
    cfg.pulsesPerGallon = 1000;
    pump.setConfig(cfg);
    pump.setMode(MockPumpController::PumpMode::AUTO);
    pump.enable();
    pump.setTemperature(0.0f);

    sensors.setSensorType(true, 0);
    sensors.setPulseCount(0, 0);

    uint32_t pulses = 0;
    for (int i = 0; i < 5; ++i) {
        sensors.simulateTimeAdvance(std::chrono::milliseconds(1000));
        sensors.generatePulses(10, 0);
        pulses = sensors.getSensorData(0).pulseCount;

        pump.setFlowPulses(pulses);
        pump.processTick();
    }

    EXPECT_FALSE(pump.isInFault());
}
