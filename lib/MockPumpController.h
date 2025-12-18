#ifndef MOCK_PUMP_CONTROLLER_H
#define MOCK_PUMP_CONTROLLER_H

#include <vector>
#include <functional>
#include <chrono>
#include <memory>

class MockPumpController {
public:
    struct PumpState {
        bool isActive;
        bool isEnabled;
        uint32_t onTime; // seconds
        uint32_t offTime; // seconds
        uint32_t cycleCount;
        bool faultDetected;
        float flowRate; // gallons per minute
        uint32_t totalPulses;
        std::chrono::steady_clock::time_point lastStateChange;
    };

    struct Config {
        bool enablePump = true;
        float freezeThreshold = 1.1f; // 34°F
        uint32_t onDuration = 300; // 5 minutes in seconds
        uint32_t offDuration = 600; // 10 minutes in seconds
        uint32_t maxOnTime = 1800; // 30 minutes maximum
        uint32_t faultTimeout = 60; // seconds without flow = fault
        uint32_t minPulsesPerMinute = 10; // minimum expected pulses for valid flow
        bool autoMode = true;
    };

    enum class PumpMode {
        AUTO,
        MANUAL_ON,
        MANUAL_OFF,
        DISABLED
    };

    MockPumpController() = default;
    virtual ~MockPumpController() = default;

    // Configuration
    void setConfig(const Config& config);
    Config getConfig() const { return config_; }
    
    // Mode control
    void setMode(PumpMode mode);
    PumpMode getMode() const { return mode_; }
    
    // Manual control (for manual modes)
    void setManualState(bool state);
    bool getManualState() const { return manualState_; }
    
    // Simulation inputs (normally from sensors)
    void setTemperature(float temperature);
    void setFlowPulses(uint32_t pulseCount);
    
    // Control methods
    void enable();
    void disable();
    void resetStatistics();
    
    // Status queries
    PumpState getState() const { return state_; }
    bool isRunning() const { return state_.isActive; }
    bool isEnabled() const { return state_.isEnabled; }
    bool isInFault() const { return state_.faultDetected; }
    float getFlowRate() const { return state_.flowRate; }
    
    // Statistics
    uint32_t getTotalOnTime() const { return state_.onTime; }
    uint32_t getTotalOffTime() const { return state_.offTime; }
    uint32_t getCycleCount() const { return state_.cycleCount; }
    uint32_t getTotalPulses() const { return state_.totalPulses; }
    
    // Time simulation
    void simulateTimeAdvance(std::chrono::seconds seconds);
    void processTick(); // Call this periodically (e.g., every second)
    
    // Callback registration
    using StateChangeCallback = std::function<void(const PumpState&, bool)>;
    using FaultCallback = std::function<void(const std::string&)>;
    
    void setStateChangeCallback(StateChangeCallback callback);
    void setFaultCallback(FaultCallback callback);

private:
    Config config_;
    PumpMode mode_ = PumpMode::AUTO;
    bool manualState_ = false;
    PumpState state_;
    std::chrono::steady_clock::time_point lastTickTime_;
    
    // Statistics tracking
    std::chrono::seconds accumulatedOnTime_{0};
    std::chrono::seconds accumulatedOffTime_{0};
    uint32_t lastPulseCount_ = 0;
    std::chrono::steady_clock::time_point lastFlowTime_;
    
    // Callbacks
    StateChangeCallback stateChangeCallback_;
    FaultCallback faultCallback_;
    
    void updatePumpState();
    void checkForFaults();
    void resetFault();
    void notifyStateChange(bool oldActive);
    void notifyFault(const std::string& faultType);
};

#endif // MOCK_PUMP_CONTROLLER_H