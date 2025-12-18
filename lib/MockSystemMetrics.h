#ifndef MOCK_SYSTEM_METRICS_H
#define MOCK_SYSTEM_METRICS_H

#include <cstdint>
#include <string>
#include <chrono>

class MockSystemMetrics {
public:
    struct SystemStats {
        uint32_t totalHeapBytes = 0;
        uint32_t freeHeapBytes = 0;
        uint32_t usedHeapBytes = 0;
        float heapUsagePercent = 0.0f;
        
        uint32_t uptimeSeconds = 0;
        uint32_t uptimeDays = 0;
        uint32_t uptimeHours = 0;
        uint32_t uptimeMinutes = 0;
        
        uint64_t bootTimestamp = 0;
        uint32_t bootReasonCode = 0;
        std::string bootReasonString;
        
        float cpuUsagePercent = 0.0f;
        uint32_t cpuSpeed = 240; // MHz
        uint32_t coreCount = 2;
        
        bool wifiConnected = false;
        uint8_t wifiSignalStrength = 0; // 0-100
        int8_t wifiRSSI = -100; // dBm
        std::string wifiSSID;
        
        uint32_t temperatureSensors = 0;
        float averageTemperature = 0.0f;
        
        uint32_t pumpRunTimeSeconds = 0;
        uint32_t pumpCycles = 0;
        
        uint32_t doorOperations = 0;
        uint32_t doorFaults = 0;
    };

    MockSystemMetrics() = default;
    virtual ~MockSystemMetrics() = default;

    // Heap management
    void setHeapSize(uint32_t totalHeap, uint32_t freeHeap);
    uint32_t getTotalHeap() const { return stats_.totalHeapBytes; }
    uint32_t getFreeHeap() const { return stats_.freeHeapBytes; }
    uint32_t getUsedHeap() const { return stats_.usedHeapBytes; }
    float getHeapUsagePercent() const { return stats_.heapUsagePercent; }
    
    // Uptime tracking
    void setBootTime(uint64_t bootTimestamp);
    void updateUptime();
    uint32_t getUptimeSeconds() const { return stats_.uptimeSeconds; }
    uint32_t getUptimeDays() const { return stats_.uptimeDays; }
    uint32_t getUptimeHours() const { return stats_.uptimeHours; }
    uint32_t getUptimeMinutes() const { return stats_.uptimeMinutes; }
    
    // Boot reason
    void setBootReason(uint32_t reasonCode);
    uint32_t getBootReasonCode() const { return stats_.bootReasonCode; }
    std::string getBootReasonString() const { return stats_.bootReasonString; }
    
    // CPU usage
    void setCPUUsage(float usagePercent) { stats_.cpuUsagePercent = usagePercent; }
    float getCPUUsage() const { return stats_.cpuUsagePercent; }
    uint32_t getCPUSpeed() const { return stats_.cpuSpeed; }
    uint32_t getCoreCount() const { return stats_.coreCount; }
    
    // WiFi status
    void setWiFiStatus(bool connected, uint8_t signalStrength, int8_t rssi, 
                       const std::string& ssid);
    bool isWiFiConnected() const { return stats_.wifiConnected; }
    uint8_t getWiFiSignalStrength() const { return stats_.wifiSignalStrength; }
    int8_t getWiFiRSSI() const { return stats_.wifiRSSI; }
    std::string getWiFiSSID() const { return stats_.wifiSSID; }
    
    // Temperature sensors
    void setTemperatureStats(uint32_t sensorCount, float averageTemp);
    uint32_t getTemperatureSensorCount() const { return stats_.temperatureSensors; }
    float getAverageTemperature() const { return stats_.averageTemperature; }
    
    // Pump statistics
    void addPumpCycle(uint32_t runTimeSeconds);
    void setPumpStats(uint32_t totalRunSeconds, uint32_t cycleCount);
    uint32_t getPumpRunTime() const { return stats_.pumpRunTimeSeconds; }
    uint32_t getPumpCycleCount() const { return stats_.pumpCycles; }
    
    // Door statistics
    void addDoorOperation();
    void addDoorFault();
    void setDoorStats(uint32_t operations, uint32_t faults);
    uint32_t getDoorOperationCount() const { return stats_.doorOperations; }
    uint32_t getDoorFaultCount() const { return stats_.doorFaults; }
    
    // Get full stats
    const SystemStats& getStats() const { return stats_; }
    
    // JSON serialization
    std::string toJson() const;
    
    // Reset statistics
    void resetStats();
    void resetPumpStats();
    void resetDoorStats();
    
    // Logging
    std::string getFormattedReport() const;

private:
    SystemStats stats_;
    std::chrono::steady_clock::time_point bootTime_;
    
    std::string getBootReasonName(uint32_t reasonCode);
};

#endif // MOCK_SYSTEM_METRICS_H
