#include <gtest/gtest.h>

#include <chrono>

#include "CommonTestFixture.h"
#include "MockSettingsManager.h"
#include "MockWiFi.h"
#include "WifiController.h"

class WifiControllerTest : public CommonTestFixture {
protected:
    MockWiFi wifi;
    WifiController controller;

    WifiControllerTest() : controller(wifi) {}

    void SetUp() override {
        CommonTestFixture::SetUp();

        WifiController::Config cfg;
        cfg.enabled = true;
        cfg.enableAPFallback = true;
        cfg.maxRetries = 3;
        cfg.retryIntervalSeconds = 2;
        cfg.apSSID = "SetupAP";
        controller.setConfig(cfg);
        controller.setCredentials("TestSSID", "TestPassword");
    }
};

TEST_F(WifiControllerTest, DisabledConfigForcesDisabledState) {
    WifiController::Config cfg = controller.getConfig();
    cfg.enabled = false;
    controller.setConfig(cfg);

    controller.processTick();
    EXPECT_EQ(controller.getState(), WifiController::State::DISABLED);
}

TEST_F(WifiControllerTest, ConnectNowFailsWhenSSIDEmpty) {
    controller.setCredentials("", "");
    EXPECT_FALSE(controller.connectNow());
    EXPECT_EQ(controller.getState(), WifiController::State::DISCONNECTED);
}

TEST_F(WifiControllerTest, ConnectNowSucceedsAndEntersConnectedState) {
    EXPECT_TRUE(controller.connectNow());
    EXPECT_TRUE(controller.isConnected());
    EXPECT_EQ(controller.getState(), WifiController::State::CONNECTED);
}

TEST_F(WifiControllerTest, ConnectNowFailureReturnsDisconnectedState) {
    wifi.setNextBeginResult(false);
    EXPECT_FALSE(controller.connectNow());
    EXPECT_EQ(controller.getState(), WifiController::State::DISCONNECTED);
}

TEST_F(WifiControllerTest, RetryLogicAttemptsConnectionAfterInterval) {
    // Edge: begin failures should trigger retries.
    wifi.setNextBeginResult(false);
    controller.disconnect();

    controller.processTick(); // 1s
    EXPECT_EQ(controller.getRetryCount(), 0u);

    controller.processTick(); // 2s -> attempt #1
    EXPECT_EQ(controller.getRetryCount(), 1u);
    EXPECT_EQ(controller.getState(), WifiController::State::DISCONNECTED);
}

TEST_F(WifiControllerTest, RetryCountResetsAfterSuccessfulConnection) {
    wifi.setNextBeginResult(false);
    controller.disconnect();

    controller.processTick();
    controller.processTick();
    ASSERT_EQ(controller.getRetryCount(), 1u);

    // Next attempt succeeds
    wifi.setNextBeginResult(true);
    controller.processTick();
    controller.processTick();

    EXPECT_TRUE(controller.isConnected());
    EXPECT_EQ(controller.getRetryCount(), 0u);
}

TEST_F(WifiControllerTest, StartsAccessPointAfterMaxRetries) {
    wifi.setNextBeginResult(false);
    controller.disconnect();

    // Drive 3 failed attempts + one interval to trigger AP fallback.
    for (int i = 0; i < 8; ++i) {
        controller.processTick();
    }

    EXPECT_TRUE(controller.isAPMode());
    EXPECT_TRUE(wifi.softAPEnabled());
}

TEST_F(WifiControllerTest, APFallbackDisabledDoesNotStartAP) {
    WifiController::Config cfg = controller.getConfig();
    cfg.enableAPFallback = false;
    controller.setConfig(cfg);

    wifi.setNextBeginResult(false);
    controller.disconnect();

    for (int i = 0; i < 10; ++i) {
        controller.processTick();
    }

    EXPECT_FALSE(controller.isAPMode());
}

TEST_F(WifiControllerTest, HandleWiFiDisconnectedTriggersReconnectScheduling) {
    controller.connectNow();
    ASSERT_TRUE(controller.isConnected());

    wifi.simulateDisconnection(42);

    // First reconnect attempt fails so we can observe the transition.
    wifi.setNextBeginResult(false);
    controller.processTick();

    EXPECT_EQ(controller.getState(), WifiController::State::DISCONNECTED);

    // Next attempt succeeds.
    wifi.setNextBeginResult(true);
    controller.processTick();
    controller.processTick();

    EXPECT_TRUE(controller.isConnected());
}

TEST_F(WifiControllerTest, ProcessTickDoesNothingWhenConnected) {
    controller.connectNow();
    uint32_t retries = controller.getRetryCount();

    controller.processTick();
    controller.processTick();

    EXPECT_EQ(controller.getRetryCount(), retries);
    EXPECT_EQ(controller.getState(), WifiController::State::CONNECTED);
}

TEST_F(WifiControllerTest, ProcessTickDoesNothingWhenInAPMode) {
    wifi.setNextBeginResult(false);
    controller.disconnect();
    for (int i = 0; i < 8; ++i) {
        controller.processTick();
    }
    ASSERT_TRUE(controller.isAPMode());

    controller.processTick();
    EXPECT_TRUE(controller.isAPMode());
}

TEST_F(WifiControllerTest, DisconnectSetsStateDisconnected) {
    controller.connectNow();
    ASSERT_TRUE(controller.isConnected());

    controller.disconnect();
    EXPECT_EQ(controller.getState(), WifiController::State::DISCONNECTED);
}

TEST_F(WifiControllerTest, ResetRetryCountResetsInternalCounters) {
    wifi.setNextBeginResult(false);
    controller.disconnect();
    controller.processTick();
    controller.processTick();

    ASSERT_GT(controller.getRetryCount(), 0u);

    controller.resetRetryCount();
    EXPECT_EQ(controller.getRetryCount(), 0u);
}

TEST_F(WifiControllerTest, EnableAfterDisableAllowsConnection) {
    WifiController::Config cfg = controller.getConfig();
    cfg.enabled = false;
    controller.setConfig(cfg);
    ASSERT_EQ(controller.getState(), WifiController::State::DISABLED);

    cfg.enabled = true;
    controller.setConfig(cfg);

    EXPECT_TRUE(controller.connectNow());
    EXPECT_TRUE(controller.isConnected());
}

TEST_F(WifiControllerTest, IntegrationWithSettingsManagerCredentials) {
    MockSettingsManager settings;
    settings.setTestMode(true);

    MockSettingsManager::Settings s = settings.getSettings();
    s.wifiSSID = "FromSettings";
    s.wifiPassword = "Secret";
    settings.setSettings(s);

    controller.setCredentials(settings.getSettings().wifiSSID, settings.getSettings().wifiPassword);
    EXPECT_TRUE(controller.connectNow());
    EXPECT_EQ(wifi.getSSID(), "FromSettings");
}

TEST_F(WifiControllerTest, PerformanceTickLoopIsFast) {
    wifi.setNextBeginResult(false);
    controller.disconnect();

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 50000; ++i) {
        controller.processTick();
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    EXPECT_LT(elapsed.count(), 2000);
}
