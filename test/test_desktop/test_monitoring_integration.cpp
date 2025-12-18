#include <gtest/gtest.h>

#include "CommonTestFixture.h"
#include "MockEmailManager.h"
#include "MockTelegramManager.h"
#include "MockSystemMetrics.h"
#include "MockSettingsManager.h"
#include "MockPushbuttonController.h"
#include "MockAPIRequestQueue.h"
#include "TestUtils.h"

class MonitoringIntegrationTest : public CommonTestFixture {
protected:
    MockEmailManager emailManager;
    MockTelegramManager telegramManager;
    MockSystemMetrics metrics;
    MockSettingsManager settingsManager;
    MockPushbuttonController pushbutton{34, 50};
    MockAPIRequestQueue requestQueue;

    void SetUp() override {
        CommonTestFixture::SetUp();
        
        // Configure all systems
        settingsManager.setTestMode(true);
        emailManager.setTestMode(true);
        telegramManager.setTestMode(true);
        pushbutton.setTestMode(true);
        requestQueue.setTestMode(true);
        
        // Configure email
        emailManager.configure("smtp.example.com", 587, true,
                                "test@example.com", "password",
                                "alerts@example.com");
        emailManager.addRecipient("user@example.com");
        
        // Configure Telegram
        telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
        
        // Initialize pushbutton
        pushbutton.begin();
    }
};

TEST_F(MonitoringIntegrationTest, SettingsManagerPersistsEmailConfiguration) {
    settingsManager.setSettingString("email.smtpServer", "smtp.gmail.com");
    settingsManager.setSettingInt("email.smtpPort", 587);
    settingsManager.setSettingString("email.fromAddress", "sender@gmail.com");
    
    EXPECT_EQ(settingsManager.getSettingString("email.smtpServer"), "smtp.gmail.com");
    EXPECT_EQ(settingsManager.getSettingInt("email.smtpPort"), 587);
    EXPECT_EQ(settingsManager.getSettingString("email.fromAddress"), "sender@gmail.com");
}

TEST_F(MonitoringIntegrationTest, SettingsManagerPersistsTelegramConfiguration) {
    settingsManager.setSettingString("telegram.botToken", "1234567890:ABCDefGHIJKLmnopqrstuvwxyz");
    settingsManager.setSettingString("telegram.chatId", "123456789");
    
    EXPECT_EQ(settingsManager.getSettingString("telegram.botToken"), "1234567890:ABCDefGHIJKLmnopqrstuvwxyz");
    EXPECT_EQ(settingsManager.getSettingString("telegram.chatId"), "123456789");
}

TEST_F(MonitoringIntegrationTest, SettingsManagerPersistsOpenWeatherConfiguration) {
    settingsManager.setSettingString("openweather.apiKey", "test-api-key-12345");
    settingsManager.setSettingFloat("openweather.latitude", 40.7128f);
    settingsManager.setSettingFloat("openweather.longitude", -74.0060f);
    
    EXPECT_EQ(settingsManager.getSettingString("openweather.apiKey"), "test-api-key-12345");
    EXPECT_NEAR(settingsManager.getSettingFloat("openweather.latitude"), 40.7128f, 0.001f);
    EXPECT_NEAR(settingsManager.getSettingFloat("openweather.longitude"), -74.0060f, 0.001f);
}

TEST_F(MonitoringIntegrationTest, SettingsManagerPersistsPushbuttonConfiguration) {
    settingsManager.setSettingInt("pushbutton.pin", 34);
    settingsManager.setSettingInt("pushbutton.debounceMs", 50);
    
    EXPECT_EQ(settingsManager.getSettingInt("pushbutton.pin"), 34);
    EXPECT_EQ(settingsManager.getSettingInt("pushbutton.debounceMs"), 50);
}

TEST_F(MonitoringIntegrationTest, SystemMetricsCanBeConvertedToJson) {
    metrics.setHeapSize(320000, 160000);
    metrics.setWiFiStatus(true, 75, -50, "TestSSID");
    metrics.setPumpStats(600, 2);
    
    std::string json = metrics.toJson();
    
    EXPECT_NE(json.find("heapTotal"), std::string::npos);
    EXPECT_NE(json.find("wifiConnected"), std::string::npos);
    EXPECT_NE(json.find("pumpRunTime"), std::string::npos);
}

TEST_F(MonitoringIntegrationTest, PushbuttonTriggersNotification) {
    bool notificationSent = false;
    
    pushbutton.setOnPressCallback([&](MockPushbuttonController::ActionType action, uint32_t duration) {
        if (action == MockPushbuttonController::ActionType::PUMP_CYCLE) {
            notificationSent = emailManager.sendAlert("Manual pump cycle triggered");
        }
    });
    
    pushbutton.simulatePress(100);
    
    EXPECT_TRUE(notificationSent);
    EXPECT_EQ(emailManager.getSentMessageCount(), 1u);
}

TEST_F(MonitoringIntegrationTest, SystemMetricsAndSettingsIntegration) {
    // Set metrics
    metrics.setHeapSize(320000, 160000);
    metrics.setPumpStats(600, 2);
    
    // Store in settings
    settingsManager.setSettingUInt("metrics.heapTotal", metrics.getTotalHeap());
    settingsManager.setSettingUInt("metrics.heapFree", metrics.getFreeHeap());
    
    // Retrieve and verify
    uint32_t savedHeapTotal = settingsManager.getSettingUInt("metrics.heapTotal");
    EXPECT_EQ(savedHeapTotal, 320000u);
}

TEST_F(MonitoringIntegrationTest, EmailAndTelegramBothSendSameAlert) {
    std::string alertMessage = "Critical: System temperature high!";
    
    bool emailSent = emailManager.sendAlert("Critical: System temperature high!");
    bool telegramSent = telegramManager.sendAlert("Critical: System temperature high!");
    
    EXPECT_TRUE(emailSent);
    EXPECT_TRUE(telegramSent);
    EXPECT_EQ(emailManager.getSentMessageCount(), 1u);
    EXPECT_GT(telegramManager.getMessageCount(), 0u);
}

TEST_F(MonitoringIntegrationTest, APIQueueHandlesOfflineGracefully) {
    requestQueue.setWiFiConnected(false);
    
    bool enqueued = requestQueue.enqueueRequest("/weather", "{}", 
                                                 MockAPIRequestQueue::APIType::OPENWEATHER);
    EXPECT_TRUE(enqueued);
    
    bool processed = requestQueue.processQueue(false);
    EXPECT_FALSE(processed);
    
    // Queue should still have the request
    EXPECT_EQ(requestQueue.getQueueSize(), 1u);
}

TEST_F(MonitoringIntegrationTest, APIQueueProcessesWhenWiFiConnects) {
    requestQueue.setWiFiConnected(false);
    requestQueue.enqueueRequest("/weather", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    
    // Initially offline
    bool processed = requestQueue.processQueue(false);
    EXPECT_FALSE(processed);
    EXPECT_EQ(requestQueue.getQueueSize(), 1u);
    
    // Now connect WiFi
    requestQueue.setWiFiConnected(true);
    processed = requestQueue.processQueue(true);
    EXPECT_TRUE(processed);
    EXPECT_EQ(requestQueue.getQueueSize(), 0u);
}

TEST_F(MonitoringIntegrationTest, TelegramCommandsExecuteCorrectly) {
    telegramManager.processCommand("/status", "123456789");
    telegramManager.processCommand("/pump_on", "123456789");
    telegramManager.processCommand("/pump_off", "123456789");
    
    EXPECT_EQ(telegramManager.getMessageCount(), 3u);
    
    const auto& history = telegramManager.getMessageHistory();
    EXPECT_EQ(history[0].command, MockTelegramManager::CommandType::STATUS);
    EXPECT_EQ(history[1].command, MockTelegramManager::CommandType::PUMP_ON);
    EXPECT_EQ(history[2].command, MockTelegramManager::CommandType::PUMP_OFF);
}

TEST_F(MonitoringIntegrationTest, SettingsValidateEmailAndTelegramConfig) {
    settingsManager.setSettingBool("email.enabled", true);
    settingsManager.setSettingString("email.smtpServer", "smtp.example.com");
    settingsManager.setSettingString("email.fromAddress", "alerts@example.com");
    
    settingsManager.setSettingBool("telegram.enabled", true);
    settingsManager.setSettingString("telegram.botToken", "1234567890:ABCDefGHIJKLmnopqrstuvwxyz");
    settingsManager.setSettingString("telegram.chatId", "123456789");
    
    EXPECT_TRUE(settingsManager.getSettingBool("email.enabled"));
    EXPECT_TRUE(settingsManager.getSettingBool("telegram.enabled"));
}

TEST_F(MonitoringIntegrationTest, MetricsReportIncludesAllSystems) {
    metrics.setHeapSize(320000, 160000);
    metrics.setCPUUsage(45.5f);
    metrics.setWiFiStatus(true, 75, -50, "TestSSID");
    metrics.setTemperatureStats(2, 22.5f);
    metrics.setPumpStats(600, 2);
    metrics.setDoorStats(5, 1);
    
    std::string report = metrics.getFormattedReport();
    
    EXPECT_NE(report.find("Heap"), std::string::npos);
    EXPECT_NE(report.find("CPU"), std::string::npos);
    EXPECT_NE(report.find("WiFi"), std::string::npos);
    EXPECT_NE(report.find("Temperature"), std::string::npos);
    EXPECT_NE(report.find("Pump"), std::string::npos);
    EXPECT_NE(report.find("Door"), std::string::npos);
}

TEST_F(MonitoringIntegrationTest, PushbuttonAndSettingsTogether) {
    // Configure pushbutton settings
    settingsManager.setSettingBool("pushbutton.enabled", true);
    settingsManager.setSettingInt("pushbutton.pin", 34);
    
    // Simulate press and record stats
    pushbutton.simulatePress(100);
    pushbutton.simulatePress(150);
    
    // Update metrics from pushbutton
    metrics.addPumpCycle(300);
    metrics.addPumpCycle(300);
    
    EXPECT_EQ(pushbutton.getTotalPressCount(), 2u);
    EXPECT_EQ(metrics.getPumpCycleCount(), 2u);
}

TEST_F(MonitoringIntegrationTest, MultipleNotificationChannels) {
    emailManager.sendEmail("user@example.com", "Subject", "Body");
    telegramManager.sendMessage("Message text");
    
    EXPECT_EQ(emailManager.getSentMessageCount(), 1u);
    EXPECT_GT(telegramManager.getMessageCount(), 0u);
}

TEST_F(MonitoringIntegrationTest, RequestQueueWithMultipleAPITypes) {
    requestQueue.enqueueRequest("/weather", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    requestQueue.enqueueRequest("/mail", "{}", MockAPIRequestQueue::APIType::EMAIL);
    requestQueue.enqueueRequest("/telegram", "{}", MockAPIRequestQueue::APIType::TELEGRAM);
    
    EXPECT_EQ(requestQueue.getQueueSize(), 3u);
    
    requestQueue.processQueue(true);
    
    EXPECT_EQ(requestQueue.getProcessedCount(), 3u);
}

TEST_F(MonitoringIntegrationTest, SystemMetricsAccumulateProperly) {
    // Start with empty stats
    EXPECT_EQ(metrics.getPumpCycleCount(), 0u);
    EXPECT_EQ(metrics.getDoorOperationCount(), 0u);
    
    // Add operations
    metrics.addPumpCycle(300);
    metrics.addPumpCycle(300);
    metrics.addDoorOperation();
    metrics.addDoorOperation();
    metrics.addDoorFault();
    
    // Verify accumulation
    EXPECT_EQ(metrics.getPumpCycleCount(), 2u);
    EXPECT_EQ(metrics.getPumpRunTime(), 600u);
    EXPECT_EQ(metrics.getDoorOperationCount(), 2u);
    EXPECT_EQ(metrics.getDoorFaultCount(), 1u);
}

TEST_F(MonitoringIntegrationTest, SettingsManageLargeEmailRecipientLists) {
    std::string recipientList = "user1@example.com,user2@example.com,user3@example.com";
    settingsManager.setSettingString("email.recipients", recipientList);
    
    std::string retrieved = settingsManager.getSettingString("email.recipients");
    EXPECT_EQ(retrieved, recipientList);
}

TEST_F(MonitoringIntegrationTest, PushbuttonDetectsLongPress) {
    // Test that long press can be simulated and recorded
    pushbutton.setLongPressTimeMs(2000);
    pushbutton.simulateLongPress(3000);
    
    const auto& history = pushbutton.getPressHistory();
    EXPECT_GT(history.size(), 0u);
    EXPECT_EQ(pushbutton.getLastPressDurationMs(), 3000u);
}

TEST_F(MonitoringIntegrationTest, TelegramMessageHistoryPersists) {
    telegramManager.processCommand("/status");
    telegramManager.processCommand("/pump_on");
    
    const auto& history = telegramManager.getMessageHistory();
    EXPECT_EQ(history.size(), 2u);
    
    // Clear and verify empty
    telegramManager.clearMessageHistory();
    EXPECT_EQ(telegramManager.getMessageCount(), 0u);
}

TEST_F(MonitoringIntegrationTest, EmailValidationWorks) {
    EXPECT_TRUE(MockEmailManager::validateEmailAddress("test@example.com"));
    EXPECT_TRUE(MockEmailManager::validateEmailAddress("user.name+tag@example.co.uk"));
    EXPECT_FALSE(MockEmailManager::validateEmailAddress("invalid"));
    EXPECT_FALSE(MockEmailManager::validateEmailAddress("@example.com"));
}
