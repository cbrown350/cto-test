#ifndef MOCK_LIGHT_CONTROLLER_H
#define MOCK_LIGHT_CONTROLLER_H

#include <vector>
#include <functional>
#include <chrono>
#include <memory>

class MockLightController {
public:
    struct LightState {
        bool isOn;
        uint8_t brightness; // 0-255
        bool isAutoMode;
        bool isDayTime;
        uint32_t onDuration; // seconds
        uint32_t offDuration; // seconds
        bool transitionActive;
        float transitionProgress; // 0.0-1.0
        std::chrono::steady_clock::time_point lastStateChange;
    };

    struct Config {
        bool enableLight = true;
        uint8_t maxBrightness = 255;
        uint8_t minBrightness = 0;
        uint32_t fadeInDuration = 300; // 5 minutes
        uint32_t fadeOutDuration = 300; // 5 minutes
        uint32_t dayStartHour = 6; // 6 AM
        uint32_t dayEndHour = 22; // 10 PM
        bool enableSunriseSunset = false;
        float latitude = 0.0f;
        float longitude = 0.0f;
        int timezoneOffset = 0; // minutes from UTC
    };

    enum class LightMode {
        AUTO,
        MANUAL_ON,
        MANUAL_OFF,
        DISABLED
    };

    MockLightController() = default;
    virtual ~MockLightController() = default;

    // Configuration
    void setConfig(const Config& config);
    Config getConfig() const { return config_; }
    
    // Mode control
    void setMode(LightMode mode);
    LightMode getMode() const { return mode_; }
    
    // Manual control
    void setManualBrightness(uint8_t brightness);
    uint8_t getManualBrightness() const { return manualBrightness_; }
    void setOn(bool on);
    bool isOn() const { return state_.isOn; }
    
    // Auto mode inputs
    void setCurrentTime(uint32_t hour, uint32_t minute);
    void setDayTime(bool isDayTime);
    void setSunriseTime(uint32_t hour, uint32_t minute);
    void setSunsetTime(uint32_t hour, uint32_t minute);
    
    // Status
    LightState getState() const { return state_; }
    uint8_t getBrightness() const { return state_.brightness; }
    bool isTransitionActive() const { return state_.transitionActive; }
    float getTransitionProgress() const { return state_.transitionProgress; }
    
    // Control methods
    void enable();
    void disable();
    void resetStatistics();
    
    // Time simulation
    void simulateTimeAdvance(std::chrono::seconds seconds);
    void processTick(); // Call this periodically
    
    // Brightness transitions
    void startTransition(uint8_t targetBrightness);
    void stopTransition();
    bool isTransitioning() const { return state_.transitionActive; }
    
    // Manual override
    void setManualOverride(bool override);
    bool getManualOverride() const { return manualOverride_; }
    
    // Sine wave simulation
    void startSineWaveTransition(uint32_t durationSeconds);
    void stopSineWaveTransition();
    bool isSineWaveActive() const { return sineWaveActive_; }

private:
    Config config_;
    LightMode mode_ = LightMode::AUTO;
    LightState state_;
    uint8_t manualBrightness_ = 128;
    bool manualOverride_ = false;
    
    // Time tracking (tick-based for deterministic tests)
    uint64_t simulatedSeconds_ = 0;
    uint32_t currentHour_ = 12;
    uint32_t currentMinute_ = 0;
    uint32_t sunriseHour_ = 6;
    uint32_t sunriseMinute_ = 30;
    uint32_t sunsetHour_ = 18;
    uint32_t sunsetMinute_ = 30;
    
    // Transition state
    uint8_t transitionStartBrightness_ = 0;
    uint8_t transitionTargetBrightness_ = 0;
    std::chrono::steady_clock::time_point transitionStartTime_;
    uint32_t transitionDuration_ = 300; // seconds
    
    // Sine wave state
    bool sineWaveActive_ = false;
    std::chrono::steady_clock::time_point sineWaveStartTime_;
    uint32_t sineWaveDuration_ = 300;
    
    // Statistics
    uint32_t accumulatedOnTime_{0};
    uint32_t accumulatedOffTime_{0};
    
    // Callbacks
    std::function<void(const LightState&)> stateChangeCallback_;
    std::function<void(uint8_t)> brightnessChangeCallback_;
    
    void updateLightState();
    void startBrightnessTransition(uint8_t targetBrightness);
    void updateTransition();
    void updateSineWave();
    void updateTiming();
    void notifyStateChange();
    void notifyBrightnessChange(uint8_t brightness);

    std::chrono::steady_clock::time_point now() const;
};

#endif // MOCK_LIGHT_CONTROLLER_H