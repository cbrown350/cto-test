#ifndef MOCK_PUSHBUTTON_CONTROLLER_H
#define MOCK_PUSHBUTTON_CONTROLLER_H

#include <cstdint>
#include <functional>
#include <vector>
#include <chrono>

class MockPushbuttonController {
public:
    enum class ButtonState {
        IDLE,
        PRESSED,
        RELEASED,
        HELD
    };

    enum class ActionType {
        PUMP_CYCLE,
        MANUAL_OVERRIDE,
        CONFIGURATION_RESET
    };

    struct ButtonPress {
        uint64_t timestamp = 0;
        uint32_t pressedDurationMs = 0;
        ActionType action = ActionType::PUMP_CYCLE;
        bool processed = false;
    };

    MockPushbuttonController(uint32_t pin = 34, uint32_t debounceMs = 50);
    virtual ~MockPushbuttonController() = default;

    // Configuration
    void setPin(uint32_t pin) { pin_ = pin; }
    uint32_t getPin() const { return pin_; }
    
    void setDebounceMs(uint32_t ms) { debounceMs_ = ms; }
    uint32_t getDebounceMs() const { return debounceMs_; }
    
    void setHoldTimeMs(uint32_t ms) { holdTimeMs_ = ms; }
    uint32_t getHoldTimeMs() const { return holdTimeMs_; }
    
    void setLongPressTimeMs(uint32_t ms) { longPressTimeMs_ = ms; }
    uint32_t getLongPressTimeMs() const { return longPressTimeMs_; }
    
    // Initialization
    bool begin();
    bool isInitialized() const { return initialized_; }
    
    // State management
    ButtonState getState() const { return state_; }
    bool isPressed() const { return state_ == ButtonState::PRESSED; }
    bool isHeld() const { return state_ == ButtonState::HELD; }
    
    // Button simulation (for testing)
    void simulatePress(uint32_t holdDurationMs = 100);
    void simulateRelease();
    void simulateLongPress(uint32_t holdDurationMs = 3000);
    void simulateDoubleClick();
    
    // Press detection
    uint32_t getPressCount() const { return pressCount_; }
    uint32_t getLastPressDurationMs() const { return lastPressDurationMs_; }
    const std::vector<ButtonPress>& getPressHistory() const { return pressHistory_; }
    void clearPressHistory();
    
    // Action triggers
    void triggerPumpCycle();
    void triggerManualOverride();
    void triggerConfigurationReset();
    
    // Callback registration
    using PressCallback = std::function<void(ActionType, uint32_t)>;
    void setOnPressCallback(PressCallback callback) { onPressCallback_ = callback; }
    
    using LongPressCallback = std::function<void()>;
    void setOnLongPressCallback(LongPressCallback callback) { onLongPressCallback_ = callback; }
    
    // Statistics
    uint32_t getTotalPressCount() const { return pressCount_; }
    uint32_t getPumpCycleCount() const { return pumpCycleCount_; }
    uint32_t getManualOverrideCount() const { return manualOverrideCount_; }
    
    // Feedback
    bool isAudioFeedbackEnabled() const { return audioFeedbackEnabled_; }
    void setAudioFeedbackEnabled(bool enabled) { audioFeedbackEnabled_ = enabled; }
    
    bool isVisualFeedbackEnabled() const { return visualFeedbackEnabled_; }
    void setVisualFeedbackEnabled(bool enabled) { visualFeedbackEnabled_ = enabled; }
    
    // Test mode
    void setTestMode(bool enabled) { testMode_ = enabled; }
    bool isTestMode() const { return testMode_; }

private:
    uint32_t pin_;
    uint32_t debounceMs_;
    uint32_t holdTimeMs_ = 2000;
    uint32_t longPressTimeMs_ = 5000;
    
    ButtonState state_ = ButtonState::IDLE;
    bool initialized_ = false;
    
    uint32_t pressCount_ = 0;
    uint32_t lastPressDurationMs_ = 0;
    uint32_t pumpCycleCount_ = 0;
    uint32_t manualOverrideCount_ = 0;
    
    std::vector<ButtonPress> pressHistory_;
    std::chrono::steady_clock::time_point pressStartTime_;
    bool wasPressed_ = false;
    
    bool audioFeedbackEnabled_ = true;
    bool visualFeedbackEnabled_ = true;
    bool testMode_ = false;
    
    PressCallback onPressCallback_;
    LongPressCallback onLongPressCallback_;
    
    void recordPress(uint32_t durationMs, ActionType action);
    void triggerCallback(ActionType action, uint32_t durationMs);
};

#endif // MOCK_PUSHBUTTON_CONTROLLER_H
