#include "MockPumpController.h"

#include <algorithm>

void MockPumpController::setConfig(const Config& config) {
    config_ = config;

    state_.isEnabled = config_.enablePump;
    clearFault();
}

void MockPumpController::setMode(PumpMode mode) {
    PumpMode oldMode = mode_;
    mode_ = mode;

    if (oldMode != mode_) {
        // Reset cycle phase on mode changes for deterministic testing.
        autoPhase_ = AutoPhase::OFF;
        freezeActive_ = false;
        phaseElapsedSeconds_ = 0;
        continuousOnSeconds_ = 0;
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
    state_.currentTemperature = temperature;
}

void MockPumpController::setFlowPulses(uint32_t pulseCount) {
    state_.totalPulses = pulseCount;
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

void MockPumpController::clearFault() {
    state_.faultDetected = false;
    secondsSinceLastPulse_ = 0;
    pulsesThisMinute_ = 0;
    secondsInMinute_ = 0;
    continuousOnSeconds_ = 0;
}

void MockPumpController::resetStatistics() {
    state_.onTime = 0;
    state_.offTime = 0;
    state_.cycleCount = 0;
    state_.totalPulses = 0;
    state_.flowRate = 0.0f;

    phaseElapsedSeconds_ = 0;
    continuousOnSeconds_ = 0;

    secondsSinceLastPulse_ = 0;
    pulsesThisMinute_ = 0;
    secondsInMinute_ = 0;
    lastPulseCount_ = 0;
}

void MockPumpController::simulateTimeAdvance(std::chrono::seconds seconds) {
    for (int i = 0; i < static_cast<int>(seconds.count()); ++i) {
        processTick();
    }
}

std::chrono::steady_clock::time_point MockPumpController::now() const {
    return std::chrono::steady_clock::time_point(std::chrono::seconds(simulatedSeconds_));
}

void MockPumpController::processTick() {
    simulatedSeconds_++;

    updateFlowState();
    updatePumpState();
    checkForFaults();

    // Update timing statistics
    if (state_.isActive) {
        state_.onTime++;
        continuousOnSeconds_++;
    } else {
        state_.offTime++;
        continuousOnSeconds_ = 0;
    }

    state_.lastStateChange = now();
}

void MockPumpController::setStateChangeCallback(StateChangeCallback callback) {
    stateChangeCallback_ = callback;
}

void MockPumpController::setFaultCallback(FaultCallback callback) {
    faultCallback_ = callback;
}

void MockPumpController::updateFlowState() {
    // Update flow info based on pulse deltas per second.
    uint32_t currentPulses = state_.totalPulses;
    uint32_t deltaPulses = currentPulses - lastPulseCount_;

    if (deltaPulses > 0) {
        secondsSinceLastPulse_ = 0;
        pulsesThisMinute_ += deltaPulses;

        if (config_.pulsesPerGallon > 0) {
            float gallonsThisSecond = static_cast<float>(deltaPulses) / static_cast<float>(config_.pulsesPerGallon);
            state_.flowRate = gallonsThisSecond * 60.0f;
        }
    } else {
        secondsSinceLastPulse_++;
        state_.flowRate = 0.0f;
    }

    lastPulseCount_ = currentPulses;

    secondsInMinute_++;
    if (secondsInMinute_ >= 60) {
        // Evaluate minimum pulse expectations every minute.
        if (state_.isActive && config_.minPulsesPerMinute > 0 && pulsesThisMinute_ < config_.minPulsesPerMinute) {
            if (!state_.faultDetected) {
                state_.faultDetected = true;
                notifyFault("Insufficient flow");
            }
        }

        secondsInMinute_ = 0;
        pulsesThisMinute_ = 0;
    }
}

void MockPumpController::updatePumpState() {
    bool oldActive = state_.isActive;

    if (state_.faultDetected) {
        state_.isActive = false;
        autoPhase_ = AutoPhase::OFF;
        freezeActive_ = false;
        phaseElapsedSeconds_ = 0;
    } else {
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

            case PumpMode::AUTO: {
                if (!state_.isEnabled || !config_.autoMode) {
                    state_.isActive = false;
                    autoPhase_ = AutoPhase::OFF;
                    freezeActive_ = false;
                    phaseElapsedSeconds_ = 0;
                    break;
                }

                // Freeze detection with hysteresis.
                const float startTemp = config_.freezeThreshold;
                const float stopTemp = config_.freezeThreshold + config_.freezeHysteresis;

                if (!freezeActive_ && state_.currentTemperature <= startTemp) {
                    freezeActive_ = true;
                    autoPhase_ = AutoPhase::ON;
                    phaseElapsedSeconds_ = 0;
                } else if (freezeActive_ && state_.currentTemperature > stopTemp) {
                    freezeActive_ = false;
                    autoPhase_ = AutoPhase::OFF;
                    phaseElapsedSeconds_ = 0;
                }

                if (!freezeActive_) {
                    state_.isActive = false;
                    break;
                }

                // Freeze protection cycling state machine.
                phaseElapsedSeconds_++;

                if (autoPhase_ == AutoPhase::ON) {
                    state_.isActive = true;
                    if (phaseElapsedSeconds_ >= std::max<uint32_t>(1, config_.onDuration)) {
                        autoPhase_ = AutoPhase::OFF;
                        phaseElapsedSeconds_ = 0;
                    }
                } else {
                    state_.isActive = false;
                    if (phaseElapsedSeconds_ >= std::max<uint32_t>(1, config_.offDuration)) {
                        autoPhase_ = AutoPhase::ON;
                        phaseElapsedSeconds_ = 0;
                    }
                }
                break;
            }
        }

        // Hard safety cutoff
        if (state_.isActive && continuousOnSeconds_ >= config_.maxOnTime) {
            state_.isActive = false;
            if (!state_.faultDetected) {
                state_.faultDetected = true;
                notifyFault("Excessive runtime");
            }
        }
    }

    if (oldActive != state_.isActive) {
        if (state_.isActive) {
            state_.cycleCount++;
        }
        notifyStateChange(oldActive);
    }
}

void MockPumpController::checkForFaults() {
    if (!state_.isActive) {
        return;
    }

    if (config_.faultTimeout > 0 && secondsSinceLastPulse_ >= config_.faultTimeout) {
        if (!state_.faultDetected) {
            state_.faultDetected = true;
            state_.isActive = false;
            notifyFault("No flow detected");
        }
    }
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
