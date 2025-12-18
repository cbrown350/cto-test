#include <gtest/gtest.h>

#include "CommonTestFixture.h"
#include "MockPushbuttonController.h"
#include "TestUtils.h"

class PushbuttonControllerTest : public CommonTestFixture {
protected:
    MockPushbuttonController button{34, 50};

    void SetUp() override {
        CommonTestFixture::SetUp();
        button.setTestMode(true);
    }
};

TEST_F(PushbuttonControllerTest, InitializeWithValidPin) {
    EXPECT_TRUE(button.begin());
    EXPECT_TRUE(button.isInitialized());
}

TEST_F(PushbuttonControllerTest, InitializeWithZeroPinFails) {
    MockPushbuttonController btn(0, 50);
    EXPECT_FALSE(btn.begin());
}

TEST_F(PushbuttonControllerTest, SetAndGetPin) {
    button.setPin(35);
    EXPECT_EQ(button.getPin(), 35u);
}

TEST_F(PushbuttonControllerTest, SetAndGetDebounceMs) {
    button.setDebounceMs(100);
    EXPECT_EQ(button.getDebounceMs(), 100u);
}

TEST_F(PushbuttonControllerTest, SetAndGetHoldTimeMs) {
    button.setHoldTimeMs(3000);
    EXPECT_EQ(button.getHoldTimeMs(), 3000u);
}

TEST_F(PushbuttonControllerTest, SetAndGetLongPressTimeMs) {
    button.setLongPressTimeMs(5000);
    EXPECT_EQ(button.getLongPressTimeMs(), 5000u);
}

TEST_F(PushbuttonControllerTest, SimulatePressIncrementsCount) {
    button.begin();
    button.simulatePress(100);
    
    EXPECT_EQ(button.getPressCount(), 1u);
}

TEST_F(PushbuttonControllerTest, SimulatePressRecordsDuration) {
    button.begin();
    button.simulatePress(250);
    
    EXPECT_EQ(button.getLastPressDurationMs(), 250u);
}

TEST_F(PushbuttonControllerTest, SimulatePressTriggersCallback) {
    button.begin();
    
    bool callbackInvoked = false;
    MockPushbuttonController::ActionType receivedAction;
    
    button.setOnPressCallback([&](MockPushbuttonController::ActionType action, uint32_t duration) {
        callbackInvoked = true;
        receivedAction = action;
    });
    
    button.simulatePress(100);
    
    EXPECT_TRUE(callbackInvoked);
    EXPECT_EQ(receivedAction, MockPushbuttonController::ActionType::PUMP_CYCLE);
}

TEST_F(PushbuttonControllerTest, SimulateLongPressTriggersLongPressCallback) {
    button.begin();
    button.setLongPressTimeMs(1000);
    
    bool longPressInvoked = false;
    button.setOnLongPressCallback([&]() {
        longPressInvoked = true;
    });
    
    button.simulateLongPress(2000);
    
    EXPECT_TRUE(longPressInvoked);
}

TEST_F(PushbuttonControllerTest, SimulateDoubleClickIncrementsCountTwice) {
    button.begin();
    button.simulateDoubleClick();
    
    EXPECT_EQ(button.getPressCount(), 2u);
}

TEST_F(PushbuttonControllerTest, ClearPressHistoryRemovesRecords) {
    button.begin();
    button.simulatePress(100);
    button.simulatePress(200);
    
    EXPECT_EQ(button.getPressHistory().size(), 2u);
    
    button.clearPressHistory();
    
    EXPECT_EQ(button.getPressHistory().size(), 0u);
    EXPECT_EQ(button.getPressCount(), 0u);
}

TEST_F(PushbuttonControllerTest, GetPressCountReturnsCorrectValue) {
    button.begin();
    button.simulatePress(100);
    button.simulatePress(100);
    button.simulatePress(100);
    
    EXPECT_EQ(button.getTotalPressCount(), 3u);
}

TEST_F(PushbuttonControllerTest, TriggerPumpCycleIncrementsCounter) {
    button.begin();
    button.triggerPumpCycle();
    button.triggerPumpCycle();
    
    EXPECT_EQ(button.getPumpCycleCount(), 2u);
}

TEST_F(PushbuttonControllerTest, TriggerManualOverrideIncrementsCounter) {
    button.begin();
    button.triggerManualOverride();
    
    EXPECT_EQ(button.getManualOverrideCount(), 1u);
}

TEST_F(PushbuttonControllerTest, AudioFeedbackCanBeEnabled) {
    button.setAudioFeedbackEnabled(true);
    EXPECT_TRUE(button.isAudioFeedbackEnabled());
}

TEST_F(PushbuttonControllerTest, AudioFeedbackCanBeDisabled) {
    button.setAudioFeedbackEnabled(false);
    EXPECT_FALSE(button.isAudioFeedbackEnabled());
}

TEST_F(PushbuttonControllerTest, VisualFeedbackCanBeEnabled) {
    button.setVisualFeedbackEnabled(true);
    EXPECT_TRUE(button.isVisualFeedbackEnabled());
}

TEST_F(PushbuttonControllerTest, VisualFeedbackCanBeDisabled) {
    button.setVisualFeedbackEnabled(false);
    EXPECT_FALSE(button.isVisualFeedbackEnabled());
}

TEST_F(PushbuttonControllerTest, StateStartsAsIdle) {
    EXPECT_EQ(button.getState(), MockPushbuttonController::ButtonState::IDLE);
}

TEST_F(PushbuttonControllerTest, IsNotPressedInitially) {
    EXPECT_FALSE(button.isPressed());
}

TEST_F(PushbuttonControllerTest, IsNotHeldInitially) {
    EXPECT_FALSE(button.isHeld());
}

TEST_F(PushbuttonControllerTest, TestModeCanBeEnabled) {
    button.setTestMode(true);
    EXPECT_TRUE(button.isTestMode());
}

TEST_F(PushbuttonControllerTest, TestModeCanBeDisabled) {
    button.setTestMode(false);
    EXPECT_FALSE(button.isTestMode());
}

TEST_F(PushbuttonControllerTest, PressHistoryContainsRecordedPresses) {
    button.begin();
    button.simulatePress(100);
    button.simulatePress(200);
    
    const auto& history = button.getPressHistory();
    EXPECT_EQ(history.size(), 2u);
    EXPECT_EQ(history[0].pressedDurationMs, 100u);
    EXPECT_EQ(history[1].pressedDurationMs, 200u);
}

TEST_F(PushbuttonControllerTest, PressHistoryMarksPressesAsProcessed) {
    button.begin();
    button.simulatePress(100);
    
    const auto& history = button.getPressHistory();
    EXPECT_TRUE(history[0].processed);
}

TEST_F(PushbuttonControllerTest, CallbackReceivesCorrectActionType) {
    button.begin();
    
    MockPushbuttonController::ActionType receivedAction;
    button.setOnPressCallback([&](MockPushbuttonController::ActionType action, uint32_t duration) {
        receivedAction = action;
    });
    
    button.simulatePress(100);
    
    EXPECT_EQ(receivedAction, MockPushbuttonController::ActionType::PUMP_CYCLE);
}

TEST_F(PushbuttonControllerTest, CallbackReceivesDuration) {
    button.begin();
    
    uint32_t receivedDuration = 0;
    button.setOnPressCallback([&](MockPushbuttonController::ActionType action, uint32_t duration) {
        receivedDuration = duration;
    });
    
    button.simulatePress(250);
    
    EXPECT_EQ(receivedDuration, 250u);
}

TEST_F(PushbuttonControllerTest, MultiplePressesCumulateStats) {
    button.begin();
    
    for (int i = 0; i < 10; ++i) {
        button.simulatePress(100);
    }
    
    EXPECT_EQ(button.getTotalPressCount(), 10u);
    EXPECT_EQ(button.getPumpCycleCount(), 10u);
}

TEST_F(PushbuttonControllerTest, SimulateReleaseWithoutPressDoesNothing) {
    button.begin();
    button.simulateRelease();
    
    EXPECT_EQ(button.getPressCount(), 0u);
}

TEST_F(PushbuttonControllerTest, LongPressDurationIsRecorded) {
    button.begin();
    button.simulateLongPress(3000);
    
    EXPECT_EQ(button.getLastPressDurationMs(), 3000u);
}

TEST_F(PushbuttonControllerTest, DoubleClickRecordsInHistory) {
    button.begin();
    button.simulateDoubleClick();
    
    const auto& history = button.getPressHistory();
    EXPECT_EQ(history.size(), 2u);
}
