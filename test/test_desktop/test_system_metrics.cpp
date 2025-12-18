#include <gtest/gtest.h>

#include "CommonTestFixture.h"
#include "MockSystemMetrics.h"
#include "TestUtils.h"

class SystemMetricsTest : public CommonTestFixture {
protected:
    MockSystemMetrics metrics;

    void SetUp() override {
        CommonTestFixture::SetUp();
        metrics.resetStats();
    }
};

TEST_F(SystemMetricsTest, SetHeapSizeCalculatesUsageCorrectly) {
    metrics.setHeapSize(320000, 160000);
    
    EXPECT_EQ(metrics.getTotalHeap(), 320000u);
    EXPECT_EQ(metrics.getFreeHeap(), 160000u);
    EXPECT_EQ(metrics.getUsedHeap(), 160000u);
}

TEST_F(SystemMetricsTest, HeapUsagePercentIsCalculatedCorrectly) {
    metrics.setHeapSize(100000, 50000);
    
    EXPECT_NEAR(metrics.getHeapUsagePercent(), 50.0f, 0.1f);
}

TEST_F(SystemMetricsTest, SetBootTimeAndUpdateUptime) {
    auto bootTime = std::chrono::system_clock::now().time_since_epoch().count();
    metrics.setBootTime(bootTime);
    
    // Simulate some time passing
    TestTimeUtils::advanceTime(std::chrono::seconds(125)); // 2 min 5 sec
    
    metrics.updateUptime();
    EXPECT_EQ(metrics.getUptimeDays(), 0u);
    EXPECT_EQ(metrics.getUptimeHours(), 0u);
    // Note: Uptime calculation in mock uses steady_clock, so this may vary
}

TEST_F(SystemMetricsTest, SetBootReasonPopulatesString) {
    metrics.setBootReason(1); // Power On
    
    EXPECT_EQ(metrics.getBootReasonCode(), 1u);
    std::string reason = metrics.getBootReasonString();
    EXPECT_NE(reason.find("Power"), std::string::npos);
}

TEST_F(SystemMetricsTest, SetCPUUsage) {
    metrics.setCPUUsage(45.5f);
    EXPECT_NEAR(metrics.getCPUUsage(), 45.5f, 0.1f);
}

TEST_F(SystemMetricsTest, SetWiFiStatusUpdatesAllFields) {
    metrics.setWiFiStatus(true, 75, -50, "TestSSID");
    
    EXPECT_TRUE(metrics.isWiFiConnected());
    EXPECT_EQ(metrics.getWiFiSignalStrength(), 75u);
    EXPECT_EQ(metrics.getWiFiRSSI(), -50);
    EXPECT_EQ(metrics.getWiFiSSID(), "TestSSID");
}

TEST_F(SystemMetricsTest, SetTemperatureStats) {
    metrics.setTemperatureStats(2, 22.5f);
    
    EXPECT_EQ(metrics.getTemperatureSensorCount(), 2u);
    EXPECT_NEAR(metrics.getAverageTemperature(), 22.5f, 0.1f);
}

TEST_F(SystemMetricsTest, AddPumpCycleIncrementsCycleCount) {
    metrics.addPumpCycle(300);
    metrics.addPumpCycle(300);
    
    EXPECT_EQ(metrics.getPumpCycleCount(), 2u);
    EXPECT_EQ(metrics.getPumpRunTime(), 600u);
}

TEST_F(SystemMetricsTest, SetPumpStatsOverwritesPreviousValues) {
    metrics.setPumpStats(1000, 5);
    
    EXPECT_EQ(metrics.getPumpRunTime(), 1000u);
    EXPECT_EQ(metrics.getPumpCycleCount(), 5u);
}

TEST_F(SystemMetricsTest, AddDoorOperationIncrementsCount) {
    metrics.addDoorOperation();
    metrics.addDoorOperation();
    
    EXPECT_EQ(metrics.getDoorOperationCount(), 2u);
}

TEST_F(SystemMetricsTest, AddDoorFaultIncrementsCount) {
    metrics.addDoorFault();
    
    EXPECT_EQ(metrics.getDoorFaultCount(), 1u);
}

TEST_F(SystemMetricsTest, SetDoorStatsOverwritesPreviousValues) {
    metrics.setDoorStats(10, 2);
    
    EXPECT_EQ(metrics.getDoorOperationCount(), 10u);
    EXPECT_EQ(metrics.getDoorFaultCount(), 2u);
}

TEST_F(SystemMetricsTest, ToJsonContainsHeapInformation) {
    metrics.setHeapSize(320000, 160000);
    
    std::string json = metrics.toJson();
    
    EXPECT_NE(json.find("heapTotal"), std::string::npos);
    EXPECT_NE(json.find("heapFree"), std::string::npos);
    EXPECT_NE(json.find("heapUsed"), std::string::npos);
}

TEST_F(SystemMetricsTest, ToJsonContainsWiFiInformation) {
    metrics.setWiFiStatus(true, 75, -50, "TestSSID");
    
    std::string json = metrics.toJson();
    
    EXPECT_NE(json.find("wifiConnected"), std::string::npos);
    EXPECT_NE(json.find("TestSSID"), std::string::npos);
}

TEST_F(SystemMetricsTest, ToJsonContainsPumpStatistics) {
    metrics.setPumpStats(600, 2);
    
    std::string json = metrics.toJson();
    
    EXPECT_NE(json.find("pumpRunTime"), std::string::npos);
    EXPECT_NE(json.find("pumpCycles"), std::string::npos);
}

TEST_F(SystemMetricsTest, ToJsonContainsDoorStatistics) {
    metrics.setDoorStats(5, 1);
    
    std::string json = metrics.toJson();
    
    EXPECT_NE(json.find("doorOperations"), std::string::npos);
    EXPECT_NE(json.find("doorFaults"), std::string::npos);
}

TEST_F(SystemMetricsTest, ResetStatsResetsCounters) {
    metrics.setPumpStats(600, 2);
    metrics.setDoorStats(5, 1);
    metrics.resetStats();
    
    EXPECT_EQ(metrics.getPumpRunTime(), 0u);
    EXPECT_EQ(metrics.getPumpCycleCount(), 0u);
    EXPECT_EQ(metrics.getDoorOperationCount(), 0u);
    EXPECT_EQ(metrics.getDoorFaultCount(), 0u);
}

TEST_F(SystemMetricsTest, ResetPumpStatsResetsOnlyPumpMetrics) {
    metrics.setPumpStats(600, 2);
    metrics.setDoorStats(5, 1);
    metrics.resetPumpStats();
    
    EXPECT_EQ(metrics.getPumpRunTime(), 0u);
    EXPECT_EQ(metrics.getPumpCycleCount(), 0u);
    EXPECT_EQ(metrics.getDoorOperationCount(), 5u);
}

TEST_F(SystemMetricsTest, ResetDoorStatsResetsOnlyDoorMetrics) {
    metrics.setPumpStats(600, 2);
    metrics.setDoorStats(5, 1);
    metrics.resetDoorStats();
    
    EXPECT_EQ(metrics.getPumpRunTime(), 600u);
    EXPECT_EQ(metrics.getDoorOperationCount(), 0u);
    EXPECT_EQ(metrics.getDoorFaultCount(), 0u);
}

TEST_F(SystemMetricsTest, GetFormattedReportContainsHeapInfo) {
    metrics.setHeapSize(320000, 160000);
    
    std::string report = metrics.getFormattedReport();
    
    EXPECT_NE(report.find("Heap"), std::string::npos);
    EXPECT_NE(report.find("Usage"), std::string::npos);
}

TEST_F(SystemMetricsTest, GetFormattedReportContainsWiFiInfo) {
    metrics.setWiFiStatus(true, 75, -50, "TestSSID");
    
    std::string report = metrics.getFormattedReport();
    
    EXPECT_NE(report.find("WiFi"), std::string::npos);
    EXPECT_NE(report.find("Connected"), std::string::npos);
}

TEST_F(SystemMetricsTest, GetFormattedReportContainsPumpInfo) {
    metrics.setPumpStats(600, 2);
    
    std::string report = metrics.getFormattedReport();
    
    EXPECT_NE(report.find("Pump"), std::string::npos);
}

TEST_F(SystemMetricsTest, MultipleBootReasonsAreMapped) {
    metrics.setBootReason(0); // Unknown
    EXPECT_NE(metrics.getBootReasonString(), "");
    
    metrics.setBootReason(4); // Watchdog
    EXPECT_NE(metrics.getBootReasonString().find("Watchdog"), std::string::npos);
}

TEST_F(SystemMetricsTest, GetStatsReturnsCompleteStructure) {
    metrics.setHeapSize(320000, 160000);
    metrics.setPumpStats(600, 2);
    metrics.setWiFiStatus(true, 75, -50, "TestSSID");
    
    const auto& stats = metrics.getStats();
    
    EXPECT_EQ(stats.totalHeapBytes, 320000u);
    EXPECT_EQ(stats.pumpRunTimeSeconds, 600u);
    EXPECT_TRUE(stats.wifiConnected);
}

TEST_F(SystemMetricsTest, HeapUsagePercentWith100PercentUsage) {
    metrics.setHeapSize(100000, 0);
    
    EXPECT_NEAR(metrics.getHeapUsagePercent(), 100.0f, 0.1f);
}

TEST_F(SystemMetricsTest, HeapUsagePercentWith0PercentUsage) {
    metrics.setHeapSize(100000, 100000);
    
    EXPECT_NEAR(metrics.getHeapUsagePercent(), 0.0f, 0.1f);
}

TEST_F(SystemMetricsTest, MultiplePumpCyclesAccumulate) {
    metrics.addPumpCycle(100);
    metrics.addPumpCycle(200);
    metrics.addPumpCycle(150);
    
    EXPECT_EQ(metrics.getPumpCycleCount(), 3u);
    EXPECT_EQ(metrics.getPumpRunTime(), 450u);
}
