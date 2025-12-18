#include "MockSystemMetrics.h"
#include <cmath>
#include <chrono>
#include <sstream>
#include <iomanip>

void MockSystemMetrics::setHeapSize(uint32_t totalHeap, uint32_t freeHeap) {
    stats_.totalHeapBytes = totalHeap;
    stats_.freeHeapBytes = freeHeap;
    stats_.usedHeapBytes = totalHeap - freeHeap;
    
    if (totalHeap > 0) {
        stats_.heapUsagePercent = (float)stats_.usedHeapBytes / (float)totalHeap * 100.0f;
    }
}

void MockSystemMetrics::setBootTime(uint64_t bootTimestamp) {
    stats_.bootTimestamp = bootTimestamp;
    bootTime_ = std::chrono::steady_clock::now();
}

void MockSystemMetrics::updateUptime() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - bootTime_);
    
    stats_.uptimeSeconds = elapsed.count();
    stats_.uptimeDays = stats_.uptimeSeconds / 86400;
    stats_.uptimeHours = (stats_.uptimeSeconds % 86400) / 3600;
    stats_.uptimeMinutes = (stats_.uptimeSeconds % 3600) / 60;
}

void MockSystemMetrics::setBootReason(uint32_t reasonCode) {
    stats_.bootReasonCode = reasonCode;
    stats_.bootReasonString = getBootReasonName(reasonCode);
}

void MockSystemMetrics::setWiFiStatus(bool connected, uint8_t signalStrength, 
                                       int8_t rssi, const std::string& ssid) {
    stats_.wifiConnected = connected;
    stats_.wifiSignalStrength = signalStrength;
    stats_.wifiRSSI = rssi;
    stats_.wifiSSID = ssid;
}

void MockSystemMetrics::setTemperatureStats(uint32_t sensorCount, float averageTemp) {
    stats_.temperatureSensors = sensorCount;
    stats_.averageTemperature = averageTemp;
}

void MockSystemMetrics::addPumpCycle(uint32_t runTimeSeconds) {
    stats_.pumpRunTimeSeconds += runTimeSeconds;
    stats_.pumpCycles++;
}

void MockSystemMetrics::setPumpStats(uint32_t totalRunSeconds, uint32_t cycleCount) {
    stats_.pumpRunTimeSeconds = totalRunSeconds;
    stats_.pumpCycles = cycleCount;
}

void MockSystemMetrics::addDoorOperation() {
    stats_.doorOperations++;
}

void MockSystemMetrics::addDoorFault() {
    stats_.doorFaults++;
}

void MockSystemMetrics::setDoorStats(uint32_t operations, uint32_t faults) {
    stats_.doorOperations = operations;
    stats_.doorFaults = faults;
}

std::string MockSystemMetrics::toJson() const {
    std::stringstream ss;
    ss << "{"
       << "\"heapTotal\":" << stats_.totalHeapBytes << ","
       << "\"heapFree\":" << stats_.freeHeapBytes << ","
       << "\"heapUsed\":" << stats_.usedHeapBytes << ","
       << "\"heapUsagePercent\":" << std::fixed << std::setprecision(2) << stats_.heapUsagePercent << ","
       << "\"uptimeSeconds\":" << stats_.uptimeSeconds << ","
       << "\"uptimeDays\":" << stats_.uptimeDays << ","
       << "\"uptimeHours\":" << stats_.uptimeHours << ","
       << "\"uptimeMinutes\":" << stats_.uptimeMinutes << ","
       << "\"cpuUsagePercent\":" << stats_.cpuUsagePercent << ","
       << "\"cpuSpeed\":" << stats_.cpuSpeed << ","
       << "\"coreCount\":" << stats_.coreCount << ","
       << "\"wifiConnected\":" << (stats_.wifiConnected ? "true" : "false") << ","
       << "\"wifiSignal\":" << (int)stats_.wifiSignalStrength << ","
       << "\"wifiRSSI\":" << (int)stats_.wifiRSSI << ","
       << "\"wifiSSID\":\"" << stats_.wifiSSID << "\","
       << "\"temperatureSensors\":" << stats_.temperatureSensors << ","
       << "\"averageTemperature\":" << std::fixed << std::setprecision(2) << stats_.averageTemperature << ","
       << "\"pumpRunTime\":" << stats_.pumpRunTimeSeconds << ","
       << "\"pumpCycles\":" << stats_.pumpCycles << ","
       << "\"doorOperations\":" << stats_.doorOperations << ","
       << "\"doorFaults\":" << stats_.doorFaults
       << "}";
    return ss.str();
}

void MockSystemMetrics::resetStats() {
    stats_.pumpRunTimeSeconds = 0;
    stats_.pumpCycles = 0;
    stats_.doorOperations = 0;
    stats_.doorFaults = 0;
}

void MockSystemMetrics::resetPumpStats() {
    stats_.pumpRunTimeSeconds = 0;
    stats_.pumpCycles = 0;
}

void MockSystemMetrics::resetDoorStats() {
    stats_.doorOperations = 0;
    stats_.doorFaults = 0;
}

std::string MockSystemMetrics::getFormattedReport() const {
    std::stringstream ss;
    ss << "=== SYSTEM METRICS REPORT ===\n";
    ss << "Heap Usage: " << stats_.usedHeapBytes << "/" << stats_.totalHeapBytes << " bytes ("
       << std::fixed << std::setprecision(1) << stats_.heapUsagePercent << "%)\n";
    ss << "Uptime: " << stats_.uptimeDays << "d " << stats_.uptimeHours << "h "
       << stats_.uptimeMinutes << "m\n";
    ss << "CPU Usage: " << std::fixed << std::setprecision(1) << stats_.cpuUsagePercent << "%\n";
    ss << "CPU Speed: " << stats_.cpuSpeed << " MHz (" << stats_.coreCount << " cores)\n";
    ss << "WiFi: " << (stats_.wifiConnected ? "Connected" : "Disconnected");
    if (stats_.wifiConnected) {
        ss << " (" << stats_.wifiSSID << ", Signal: " << (int)stats_.wifiSignalStrength << "%, "
           << (int)stats_.wifiRSSI << " dBm)";
    }
    ss << "\n";
    ss << "Temperature Sensors: " << stats_.temperatureSensors << " (Avg: "
       << std::fixed << std::setprecision(1) << stats_.averageTemperature << "°C)\n";
    ss << "Pump: " << stats_.pumpCycles << " cycles, " << stats_.pumpRunTimeSeconds
       << "s total runtime\n";
    ss << "Door: " << stats_.doorOperations << " operations, " << stats_.doorFaults << " faults\n";
    return ss.str();
}

std::string MockSystemMetrics::getBootReasonName(uint32_t reasonCode) {
    switch (reasonCode) {
        case 0: return "Unknown";
        case 1: return "Power On";
        case 2: return "External Reset";
        case 3: return "Software Reset";
        case 4: return "Watchdog Reset";
        case 5: return "Deep Sleep Reset";
        case 6: return "SOS Reset";
        case 7: return "OTA Reset";
        case 8: return "SDIO Reset";
        case 9: return "JTAG Reset";
        case 10: return "Brownout Reset";
        case 11: return "Main XTAL Reset";
        case 12: return "Flash Data Reset";
        case 13: return "Cache Enabled Reset";
        case 14: return "CRC Reset";
        case 15: return "Analog Comparator Reset";
        case 16: return "EFUSE CRC Reset";
        default: return "Unknown (" + std::to_string(reasonCode) + ")";
    }
}
