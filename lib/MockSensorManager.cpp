#include "MockSensorManager.h"
#include <algorithm>
#include <cmath>
#include <random>

void MockSensorManager::setConfig(const Config& config) {
    config_ = config;
    initializeSensors();
}

void MockSensorManager::initializeSensors() {
    sensorData_.clear();
    pulseGenerationActive_.clear();
    currentPulseRate_.clear();
    callbacks_.clear();
    
    int sensorCount = (config_.enableFirstSensor ? 1 : 0) + (config_.enableSecondSensor ? 1 : 0);
    sensorData_.resize(sensorCount);
    pulseGenerationActive_.resize(sensorCount, false);
    currentPulseRate_.resize(sensorCount, 0);
    callbacks_.resize(sensorCount);
    
    // Initialize with default values
    for (size_t i = 0; i < sensorData_.size(); ++i) {
        sensorData_[i].temperature = 20.0f; // Room temperature
        sensorData_[i].isValid = true;
        sensorData_[i].isWaterMeter = false;
        sensorData_[i].pulseCount = 0;
        sensorData_[i].lastUpdate = std::chrono::steady_clock::now();
    }
}

void MockSensorManager::setTemperature(float temperature, int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;
    
    temperature = std::max(config_.minTemperature, std::min(config_.maxTemperature, temperature));
    sensorData_[sensorIndex].temperature = temperature;
    sensorData_[sensorIndex].lastUpdate = std::chrono::steady_clock::now();
    
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
    sensorData_[sensorIndex].lastUpdate = std::chrono::steady_clock::now();
    
    if (callbacks_[sensorIndex]) {
        callbacks_[sensorIndex](sensorData_[sensorIndex], sensorIndex);
    }
}

void MockSensorManager::generatePulses(uint32_t pulseCount, int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;
    
    sensorData_[sensorIndex].pulseCount += pulseCount;
    sensorData_[sensorIndex].lastUpdate = std::chrono::steady_clock::now();
    
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
}

void MockSensorManager::setSensorType(bool isWaterMeter, int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;
    
    sensorData_[sensorIndex].isWaterMeter = isWaterMeter;
    sensorData_[sensorIndex].lastUpdate = std::chrono::steady_clock::now();
}

void MockSensorManager::simulateSensorFailure(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;
    
    sensorData_[sensorIndex].isValid = false;
    sensorData_[sensorIndex].lastUpdate = std::chrono::steady_clock::now();
}

void MockSensorManager::simulateSensorRecovery(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;
    
    sensorData_[sensorIndex].isValid = true;
    sensorData_[sensorIndex].lastUpdate = std::chrono::steady_clock::now();
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
        return SensorData{0.0f, false, false, 0, std::chrono::steady_clock::now()};
    }
    return sensorData_[sensorIndex];
}

std::vector<MockSensorManager::SensorData> MockSensorManager::getAllSensorData() const {
    return sensorData_;
}

void MockSensorManager::setDataCallback(DataCallback callback, int sensorIndex) {
    if (sensorIndex == -1) {
        // Set callback for all sensors
        for (auto& cb : callbacks_) {
            cb = callback;
        }
    } else if (sensorIndex >= 0 && sensorIndex < static_cast<int>(callbacks_.size())) {
        callbacks_[sensorIndex] = callback;
    }
}

void MockSensorManager::updateSensorData(int sensorIndex) {
    if (sensorIndex < 0 || sensorIndex >= static_cast<int>(sensorData_.size())) return;
    
    // Generate pulses if pulse generation is active
    if (pulseGenerationActive_[sensorIndex]) {
        // This would typically be called in a timer interrupt or regular interval
        // For testing, we can call this manually or simulate time progression
        uint32_t pulsesThisUpdate = currentPulseRate_[sensorIndex] / 10; // Assuming 10Hz update rate
        if (pulsesThisUpdate > 0) {
            generatePulses(pulsesThisUpdate, sensorIndex);
        }
    }
}