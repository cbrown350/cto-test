#include "MockPushbuttonController.h"
#include <chrono>

MockPushbuttonController::MockPushbuttonController(uint32_t pin, uint32_t debounceMs)
    : pin_(pin), debounceMs_(debounceMs), pressStartTime_(std::chrono::steady_clock::now()) {
}

bool MockPushbuttonController::begin() {
    if (pin_ == 0) {
        return false;
    }
    initialized_ = true;
    state_ = ButtonState::IDLE;
    return true;
}

void MockPushbuttonController::simulatePress(uint32_t holdDurationMs) {
    if (!testMode_) {
        return;
    }

    // Simulate pressing
    state_ = ButtonState::PRESSED;
    pressStartTime_ = std::chrono::steady_clock::now();
    wasPressed_ = true;
    
    // Check if this is a long press
    if (holdDurationMs > longPressTimeMs_ && onLongPressCallback_) {
        state_ = ButtonState::HELD;
        onLongPressCallback_();
    }
    
    // Simulate release
    state_ = ButtonState::RELEASED;
    lastPressDurationMs_ = holdDurationMs;
    pressCount_++;
    
    triggerPumpCycle();
    
    // Record the press
    recordPress(holdDurationMs, ActionType::PUMP_CYCLE);
    
    state_ = ButtonState::IDLE;
}

void MockPushbuttonController::simulateRelease() {
    if (!testMode_ || state_ == ButtonState::IDLE) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto pressedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - pressStartTime_);
    
    state_ = ButtonState::RELEASED;
    lastPressDurationMs_ = pressedDuration.count();
    pressCount_++;
    
    triggerPumpCycle();
    recordPress(lastPressDurationMs_, ActionType::PUMP_CYCLE);
    
    state_ = ButtonState::IDLE;
}

void MockPushbuttonController::simulateLongPress(uint32_t holdDurationMs) {
    if (!testMode_) {
        return;
    }

    state_ = ButtonState::PRESSED;
    pressStartTime_ = std::chrono::steady_clock::now();
    wasPressed_ = true;
    
    // Trigger long press callback
    if (holdDurationMs >= longPressTimeMs_ && onLongPressCallback_) {
        state_ = ButtonState::HELD;
        onLongPressCallback_();
    }
    
    state_ = ButtonState::RELEASED;
    lastPressDurationMs_ = holdDurationMs;
    pressCount_++;
    
    recordPress(holdDurationMs, ActionType::PUMP_CYCLE);
    
    state_ = ButtonState::IDLE;
}

void MockPushbuttonController::simulateDoubleClick() {
    if (!testMode_) {
        return;
    }

    // First click
    simulatePress(100);
    
    // Second click (within 500ms)
    simulatePress(100);
}

void MockPushbuttonController::clearPressHistory() {
    pressHistory_.clear();
    pressCount_ = 0;
    lastPressDurationMs_ = 0;
}

void MockPushbuttonController::triggerPumpCycle() {
    pumpCycleCount_++;
    if (onPressCallback_) {
        onPressCallback_(ActionType::PUMP_CYCLE, lastPressDurationMs_);
    }
}

void MockPushbuttonController::triggerManualOverride() {
    manualOverrideCount_++;
    if (onPressCallback_) {
        onPressCallback_(ActionType::MANUAL_OVERRIDE, lastPressDurationMs_);
    }
}

void MockPushbuttonController::triggerConfigurationReset() {
    if (onPressCallback_) {
        onPressCallback_(ActionType::CONFIGURATION_RESET, lastPressDurationMs_);
    }
}

void MockPushbuttonController::recordPress(uint32_t durationMs, ActionType action) {
    ButtonPress press;
    press.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    press.pressedDurationMs = durationMs;
    press.action = action;
    press.processed = true;
    
    pressHistory_.push_back(press);
}

void MockPushbuttonController::triggerCallback(ActionType action, uint32_t durationMs) {
    if (onPressCallback_) {
        onPressCallback_(action, durationMs);
    }
}
