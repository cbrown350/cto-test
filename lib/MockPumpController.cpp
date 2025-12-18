#include "MockPumpController.h"
#include <algorithm>
#include <cmath>
#include <string>

void MockPumpController::setConfig(const Config& config) {
    config_ = config;
    resetFault();
}

void MockPumpController::setMode(PumpMode mode) {
    PumpMode oldMode = mode_;
    mode_ = mode;
    
    if (oldMode != mode_) {
        // Reset accumulated times when mode changes
        accumulatedOnTime_ = std::chrono::seconds(state_.onTime);
        accumulatedOffTime_ = std::chrono::seconds(state_.offTime);
    }
    
    updatePumpState();
}

void MockPumpController::setManualState(bool state) {
    manualState_ = state;
    if (mode_ == PumpMode::MANUAL_ON || mode_ == PumpMode::MANUAL_OFF) {
        updatePumpState();
    }
}

void MockPumpController::setTemperature(float temperature) {
    // Store temperature for logic, but pump state updates through processTick()
    if (mode_ == PumpMode::AUTO && config_.autoMode) {
        // Logic will be evaluated in processTick()
    }
}

void MockPumpController::setFlowPulses(uint32_t pulseCount) {
    state_.totalPulses = pulseCount;
    lastFlowTime_ = std::chrono::steady_clock::now();
}

void MockPumpController::enable() {
    state_.isEnabled = true;
    updatePumpState();
}

void MockPumpController::disable() {
    state_.isEnabled = false;
    state_.isActive = false;
    updatePumpState();
}

void MockPumpController::resetStatistics() {
    state_.onTime = 0;
    state_.offTime = 0;
    state_.cycleCount = 0;
    state_.totalPulses = 0;
    accumulatedOnTime_ = std::chrono::seconds{0};
    accumulatedOffTime_ = std::chrono::seconds{0};
    lastPulseCount_ = 0;
}

void MockPumpController::simulateTimeAdvance(std::chrono::seconds seconds) {
    auto now = std::chrono::steady_clock::now();
    auto timeDiff = now - lastTickTime_;
    lastTickTime_ = now;
    
    // Simulate multiple ticks
    auto tickInterval = std::chrono::seconds{1};
    auto tickCount = std::chrono::duration_cast<std::chrono::seconds>(timeDiff).count();
    
    for (int i = 0; i < tickCount; ++i) {
        processTick();
    }
}

void MockPumpController::processTick() {
    auto now = std::chrono::steady_clock::now();
    
    // Initialize timing on first call
    if (lastTickTime_.time_since_epoch().count() == 0) {
        lastTickTime_ = now;
        lastFlowTime_ = now;
        return;
    }
    
    updatePumpState();
    checkForFaults();
    
    // Update timing
    if (state_.isActive) {
        accumulatedOnTime_ += std::chrono::seconds{1};
    } else {
        accumulatedOffTime_ += std::chrono::seconds{1};
    }
    
    state_.onTime = std::chrono::duration_cast<std::chrono::seconds>(accumulatedOnTime_).count();
    state_.offTime = std::chrono::duration_cast<std::chrono::seconds>(accumulatedOffTime_).count();
    state_.lastStateChange = now;
    
    lastTickTime_ = now;
}

void MockPumpController::setStateChangeCallback(StateChangeCallback callback) {
    stateChangeCallback_ = callback;
}

void MockPumpController::setFaultCallback(FaultCallback callback) {
    faultCallback_ = callback;
}

void MockPumpController::updatePumpState() {
    bool oldActive = state_.isActive;
    
    switch (mode_) {
        case PumpMode::DISABLED:
            state_.isActive = false;
            break;
            
        case PumpMode::MANUAL_ON:
            state_.isActive = state_.isEnabled && manualState_;
            break;
            
        case PumpMode::MANUAL_OFF:
            state_.isActive = false;
            break;
            
        case PumpMode::AUTO:
            if (!state_.isEnabled || !config_.autoMode) {
                state_.isActive = false;
            } else {
                // Simple freeze protection logic
                // This would normally use temperature from sensors
                state_.isActive = true; // Simplified for mock
                
                // Check timing constraints
                if (state_.onTime >= config_.maxOnTime) {
                    state_.isActive = false; // Force off after max time
                }
            }
            break;
    }
    
    // Notify state changes
    if (oldActive != state_.isActive) {
        if (state_.isActive) {
            state_.cycleCount++;
        }
        notifyStateChange(oldActive);
    }
}

void MockPumpController::checkForFaults() {
    auto now = std::chrono::steady_clock::now();
    
    // Check for flow fault (no pulses for fault timeout)
    auto timeSinceFlow = now - lastFlowTime_;
    if (std::chrono::duration_cast<std::chrono::seconds>(timeSinceFlow).count() > config_.faultTimeout) {
        if (!state_.faultDetected) {
            state_.faultDetected = true;
            notifyFault(std::string("No flow detected"));
        }
    }
    
    // Check for excessive runtime
    if (state_.onTime > config_.maxOnTime && state_.isActive) {
        if (!state_.faultDetected) {
            state_.faultDetected = true;
            notifyFault(std::string("Excessive runtime"));
        }
    }
}

void MockPumpController::resetFault() {
    state_.faultDetected = false;
}

void MockPumpController::notifyStateChange(bool oldActive) {
    if (stateChangeCallback_) {
        stateChangeCallback_(state_, oldActive);
    }
}

void MockPumpController::notifyFault(const std::string& faultType) {
    if (faultCallback_) {
        faultCallback_(faultType);
    }
}