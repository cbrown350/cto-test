#ifndef MOCK_SENSOR_MANAGER_H
#define MOCK_SENSOR_MANAGER_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class MockSensorManager {
public:
    struct SensorData {
        float temperature;
        bool isValid;
        bool isWaterMeter;
        uint32_t pulseCount;

        // Derived metrics for water meters
        float flowRateGPM;
        float totalGallons;

        std::chrono::steady_clock::time_point lastUpdate;
    };

    struct Config {
        bool enableFirstSensor = true;
        bool enableSecondSensor = true;
        float minTemperature = -55.0f; // DS18B20 range
        float maxTemperature = 125.0f;
        uint32_t pulsesPerGallon = 1000; // Typical YF-S201 value
    };

    MockSensorManager() = default;
    virtual ~MockSensorManager() = default;

    // Configuration
    void setConfig(const Config& config);
    Config getConfig() const { return config_; }

    // Temperature simulation
    void setTemperature(float temperature, int sensorIndex = 0);
    void setRandomTemperature(int sensorIndex = 0);
    void setGradientTemperature(float startTemp, float endTemp, int steps, int sensorIndex = 0);

    // Water meter simulation
    void setPulseCount(uint32_t pulseCount, int sensorIndex = 0);
    void generatePulses(uint32_t pulseCount, int sensorIndex = 0);
    void startPulseGeneration(uint32_t pulsesPerSecond, int sensorIndex = 0);
    void stopPulseGeneration(int sensorIndex = 0);

    // Sensor type simulation (Dallas temp vs water meter)
    void setSensorType(bool isWaterMeter, int sensorIndex = 0);
    void simulateSensorFailure(int sensorIndex = 0);
    void simulateSensorRecovery(int sensorIndex = 0);

    // Status
    bool isSensorValid(int sensorIndex = 0) const;
    bool isWaterMeterDetected(int sensorIndex = 0) const;
    SensorData getSensorData(int sensorIndex = 0) const;
    std::vector<SensorData> getAllSensorData() const;

    // Flow helpers
    float getFlowRateGPM(int sensorIndex = 0) const;
    float getTotalGallons(int sensorIndex = 0) const;
    void resetFlowStatistics(int sensorIndex = -1);

    // Time simulation
    void processTick(std::chrono::milliseconds delta = std::chrono::milliseconds(100));
    void simulateTimeAdvance(std::chrono::milliseconds total, std::chrono::milliseconds step = std::chrono::milliseconds(100));

    // Callback registration for real-time updates
    using DataCallback = std::function<void(const SensorData&, int)>;
    void setDataCallback(DataCallback callback, int sensorIndex = -1);

private:
    Config config_;
    std::vector<SensorData> sensorData_;
    std::vector<bool> pulseGenerationActive_;
    std::vector<uint32_t> currentPulseRate_;
    std::vector<float> pulseAccumulator_;
    std::vector<DataCallback> callbacks_;

    std::vector<uint32_t> lastPulseCount_;
    std::vector<std::chrono::steady_clock::time_point> lastPulseTime_;

    std::chrono::steady_clock::time_point currentTime_ = std::chrono::steady_clock::time_point{};

    void initializeSensors();
    void updateSensorData(int sensorIndex, std::chrono::milliseconds delta);
    void updateFlowMetrics(int sensorIndex);
};

#endif // MOCK_SENSOR_MANAGER_H
