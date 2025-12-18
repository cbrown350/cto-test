#ifndef MOCK_PUMP_CONTROLLER_H
#define MOCK_PUMP_CONTROLLER_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

class MockPumpController {
public:
    struct PumpState {
        bool isActive = false;
        bool isEnabled = true;
        uint32_t onTime = 0; // seconds
        uint32_t offTime = 0; // seconds
        uint32_t cycleCount = 0;
        bool faultDetected = false;
        float flowRate = 0.0f; // gallons per minute
        uint32_t totalPulses = 0;
        float currentTemperature = 20.0f;
        std::chrono::steady_clock::time_point lastStateChange{};
    };

    struct Config {
        bool enablePump = true;
        float freezeThreshold = 1.1f; // 34°F
        float freezeHysteresis = 0.5f;

        uint32_t onDuration = 300; // seconds
        uint32_t offDuration = 600; // seconds
        uint32_t maxOnTime = 1800; // seconds

        uint32_t faultTimeout = 60; // seconds without flow = fault
        uint32_t minPulsesPerMinute = 10; // minimum expected pulses for valid flow
        uint32_t pulsesPerGallon = 1000;

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
    void clearFault();
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
    void processTick(); // 1 second tick

    // Callback registration
    using StateChangeCallback = std::function<void(const PumpState&, bool oldActive)>;
    using FaultCallback = std::function<void(const std::string&)>;

    void setStateChangeCallback(StateChangeCallback callback);
    void setFaultCallback(FaultCallback callback);

private:
    enum class AutoPhase {
        OFF,
        ON
    };

    Config config_;
    PumpMode mode_ = PumpMode::AUTO;
    bool manualState_ = false;
    PumpState state_;

    AutoPhase autoPhase_ = AutoPhase::OFF;
    bool freezeActive_ = false;
    uint32_t phaseElapsedSeconds_ = 0;
    uint32_t continuousOnSeconds_ = 0;

    uint32_t secondsSinceLastPulse_ = 0;
    uint32_t pulsesThisMinute_ = 0;
    uint32_t secondsInMinute_ = 0;
    uint32_t lastPulseCount_ = 0;

    uint64_t simulatedSeconds_ = 0;

    // Callbacks
    StateChangeCallback stateChangeCallback_;
    FaultCallback faultCallback_;

    void updatePumpState();
    void updateFlowState();
    void checkForFaults();

    void notifyStateChange(bool oldActive);
    void notifyFault(const std::string& faultType);

    std::chrono::steady_clock::time_point now() const;
};

#endif // MOCK_PUMP_CONTROLLER_H
