#include "MockSensorManager.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {
float clampFloat(float v, float minV, float maxV) {
    return std::max(minV, std::min(maxV, v));
}
}

void MockSensorManager::setConfig(const Config& config) {
    config_ = config;
    initializeSensors();
}

void MockSensorManager::initializeSensors() {
    sensorData_.clear();
    pulseGenerationActive_.clear();
    currentPulseRate_.clear();
    pulseAccumulator_.clear();
    callbacks_.clear();
    lastPulseCount_.clear();
    lastPulseTime_.clear();

    currentTime_ = std::chrono::steady_clock::time_point(std::chrono::milliseconds(0));

    int sensorCount = (config_.enableFirstSensor ? 1 : 0) + (config_.enableSecondSensor ? 1 : 0);
    sensorData_.resize(sensorCount);
    pulseGenerationActive_.resize(sensorCount, false);
    currentPulseRate_.resize(sensorCount, 0);
    pulseAccumulator_.resize(sensorCount, 0.0f);
    callbacks_.resize(sensorCount);
    lastPulseCount_.resize(sensorCount, 0);
    lastPulseTime_.resize(sensorCount, currentTime_);

    // Initialize with default values
    for (size_t i = 0; i < sensorData_.size(); ++i) {
        sensorData_[i].temperature = 20.0f; // room temperature
        sensorData_[i].isValid = true;
        sensorData_[i].isWaterMeter = false;
        sensorData_[i].pulseCount = 0;
        sensorData_[i].flowRateGPM = 0.0f;
        sensorData_[i].totalGallons = 0.0f;
        sensorData_[i].lastUpdate = currentTime_;
    }
}

void MockSensorManager::setTemperature(float temperature, int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    sensorData_[sensorIndex].temperature = clampFloat(temperature, config_.minTemperature, config_.maxTemperature);
    sensorData_[sensorIndex].lastUpdate = currentTime_;

    if (callbacks_[sensorIndex]) {
        callbacks_[sensorIndex](sensorData_[sensorIndex], sensorIndex);
    }
}

void MockSensorManager::setRandomTemperature(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(config_.minTemperature, config_.maxTemperature);

    setTemperature(dist(gen), sensorIndex);
}

void MockSensorManager::setGradientTemperature(float startTemp, float endTemp, int steps, int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size()) || steps <= 0) return;

    float stepSize = (endTemp - startTemp) / steps;
    for (int i = 0; i <= steps; ++i) {
        setTemperature(startTemp + (stepSize * i), sensorIndex);
    }
}

void MockSensorManager::setPulseCount(uint32_t pulseCount, int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    sensorData_[sensorIndex].pulseCount = pulseCount;
    sensorData_[sensorIndex].lastUpdate = currentTime_;

    // Reset flow reference points.
    lastPulseCount_[sensorIndex] = pulseCount;
    lastPulseTime_[sensorIndex] = currentTime_;
    sensorData_[sensorIndex].flowRateGPM = 0.0f;

    if (callbacks_[sensorIndex]) {
        callbacks_[sensorIndex](sensorData_[sensorIndex], sensorIndex);
    }
}

void MockSensorManager::generatePulses(uint32_t pulseCount, int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    sensorData_[sensorIndex].pulseCount += pulseCount;
    sensorData_[sensorIndex].lastUpdate = currentTime_;

    updateFlowMetrics(sensorIndex);

    if (callbacks_[sensorIndex]) {
        callbacks_[sensorIndex](sensorData_[sensorIndex], sensorIndex);
    }
}

void MockSensorManager::startPulseGeneration(uint32_t pulsesPerSecond, int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    pulseGenerationActive_[sensorIndex] = true;
    currentPulseRate_[sensorIndex] = pulsesPerSecond;
}

void MockSensorManager::stopPulseGeneration(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    pulseGenerationActive_[sensorIndex] = false;
    currentPulseRate_[sensorIndex] = 0;
    pulseAccumulator_[sensorIndex] = 0.0f;
}

void MockSensorManager::setSensorType(bool isWaterMeter, int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    sensorData_[sensorIndex].isWaterMeter = isWaterMeter;
    sensorData_[sensorIndex].lastUpdate = currentTime_;
}

void MockSensorManager::simulateSensorFailure(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    sensorData_[sensorIndex].isValid = false;
    sensorData_[sensorIndex].lastUpdate = currentTime_;
}

void MockSensorManager::simulateSensorRecovery(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    sensorData_[sensorIndex].isValid = true;
    sensorData_[sensorIndex].lastUpdate = currentTime_;
}

bool MockSensorManager::isSensorValid(int sensorIndex) const {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return false;
    return sensorData_[sensorIndex].isValid;
}

bool MockSensorManager::isWaterMeterDetected(int sensorIndex) const {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return false;
    return sensorData_[sensorIndex].isWaterMeter;
}

MockSensorManager::SensorData MockSensorManager::getSensorData(int sensorIndex) const {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) {
        SensorData invalid;
        invalid.temperature = 0.0f;
        invalid.isValid = false;
        invalid.isWaterMeter = false;
        invalid.pulseCount = 0;
        invalid.flowRateGPM = 0.0f;
        invalid.totalGallons = 0.0f;
        invalid.lastUpdate = currentTime_;
        return invalid;
    }
    return sensorData_[sensorIndex];
}

std::vector<MockSensorManager::SensorData> MockSensorManager::getAllSensorData() const {
    return sensorData_;
}

float MockSensorManager::getFlowRateGPM(int sensorIndex) const {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return 0.0f;
    return sensorData_[sensorIndex].flowRateGPM;
}

float MockSensorManager::getTotalGallons(int sensorIndex) const {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return 0.0f;
    return sensorData_[sensorIndex].totalGallons;
}

void MockSensorManager::resetFlowStatistics(int sensorIndex) {
    if (sensorIndex == -1) {
        for (size_t i = 0; i < sensorData_.size(); ++i) {
            resetFlowStatistics(static_cast<int>(i));
        }
        return;
    }

    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    sensorData_[sensorIndex].flowRateGPM = 0.0f;
    sensorData_[sensorIndex].totalGallons = 0.0f;
    lastPulseCount_[sensorIndex] = sensorData_[sensorIndex].pulseCount;
    lastPulseTime_[sensorIndex] = currentTime_;
}

void MockSensorManager::processTick(std::chrono::milliseconds delta) {
    if (delta.count() <= 0) {
        return;
    }

    currentTime_ += delta;

    for (int i = 0; i < static_cast<int>(sensorData_.size()); ++i) {
        updateSensorData(i, delta);
    }
}

void MockSensorManager::simulateTimeAdvance(std::chrono::milliseconds total, std::chrono::milliseconds step) {
    if (total.count() <= 0) {
        return;
    }
    if (step.count() <= 0) {
        step = std::chrono::milliseconds(100);
    }

    std::chrono::milliseconds remaining = total;
    while (remaining.count() > 0) {
        std::chrono::milliseconds thisStep = (remaining < step) ? remaining : step;
        processTick(thisStep);
        remaining -= thisStep;
    }
}

void MockSensorManager::setDataCallback(DataCallback callback, int sensorIndex) {
    if (sensorIndex == -1) {
        for (auto& cb : callbacks_) {
            cb = callback;
        }
    } else if (sensorIndex >= 0 && sensorIndex < static_cast<int>(callbacks_.size())) {
        callbacks_[sensorIndex] = callback;
    }
}

void MockSensorManager::updateSensorData(int sensorIndex, std::chrono::milliseconds delta) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;

    if (pulseGenerationActive_[sensorIndex]) {
        float deltaSeconds = static_cast<float>(delta.count()) / 1000.0f;
        float pulsesToAdd = static_cast<float>(currentPulseRate_[sensorIndex]) * deltaSeconds;
        pulseAccumulator_[sensorIndex] += pulsesToAdd;

        uint32_t wholePulses = static_cast<uint32_t>(pulseAccumulator_[sensorIndex]);
        if (wholePulses > 0) {
            pulseAccumulator_[sensorIndex] -= static_cast<float>(wholePulses);
            generatePulses(wholePulses, sensorIndex);
        }
    }
}

void MockSensorManager::updateFlowMetrics(int sensorIndex) {
    if (!sensorData_[sensorIndex].isWaterMeter) {
        return;
    }

    if (config_.pulsesPerGallon == 0) {
        sensorData_[sensorIndex].flowRateGPM = 0.0f;
        return;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime_ - lastPulseTime_[sensorIndex]);
    if (elapsed.count() <= 0) {
        return;
    }

    uint32_t currentPulses = sensorData_[sensorIndex].pulseCount;
    uint32_t deltaPulses = currentPulses - lastPulseCount_[sensorIndex];

    float gallons = static_cast<float>(deltaPulses) / static_cast<float>(config_.pulsesPerGallon);
    sensorData_[sensorIndex].totalGallons += gallons;

    float elapsedMinutes = (static_cast<float>(elapsed.count()) / 1000.0f) / 60.0f;
    if (elapsedMinutes > 0.0f) {
        sensorData_[sensorIndex].flowRateGPM = gallons / elapsedMinutes;
    }

    lastPulseCount_[sensorIndex] = currentPulses;
    lastPulseTime_[sensorIndex] = currentTime_;
}
