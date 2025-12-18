#include "MockLightController.h"

#include <algorithm>
#include <cmath>

std::chrono::steady_clock::time_point MockLightController::now() const {
    return std::chrono::steady_clock::time_point(std::chrono::seconds(simulatedSeconds_));
}

void MockLightController::setConfig(const Config& config) {
    config_ = config;
}

void MockLightController::setMode(LightMode mode) {
    mode_ = mode;
    updateLightState();
}

void MockLightController::setManualBrightness(uint8_t brightness) {
    brightness = std::max(config_.minBrightness, std::min(config_.maxBrightness, brightness));
    manualBrightness_ = brightness;
    if (mode_ == LightMode::MANUAL_ON) {
        startBrightnessTransition(brightness);
    }
}

void MockLightController::setOn(bool on) {
    if (on) {
        setManualBrightness(manualBrightness_);
        if (mode_ != LightMode::MANUAL_ON) {
            setMode(LightMode::MANUAL_ON);
        }
    } else {
        if (mode_ != LightMode::MANUAL_OFF) {
            setMode(LightMode::MANUAL_OFF);
        }
        startBrightnessTransition(0);
    }
}

void MockLightController::setCurrentTime(uint32_t hour, uint32_t minute) {
    currentHour_ = std::max(0u, std::min(23u, hour));
    currentMinute_ = std::max(0u, std::min(59u, minute));
    updateLightState();
}

void MockLightController::setDayTime(bool isDayTime) {
    state_.isDayTime = isDayTime;
    updateLightState();
}

void MockLightController::setSunriseTime(uint32_t hour, uint32_t minute) {
    sunriseHour_ = std::max(0u, std::min(23u, hour));
    sunriseMinute_ = std::max(0u, std::min(59u, minute));
}

void MockLightController::setSunsetTime(uint32_t hour, uint32_t minute) {
    sunsetHour_ = std::max(0u, std::min(23u, hour));
    sunsetMinute_ = std::max(0u, std::min(59u, minute));
}

void MockLightController::enable() {
    state_.isOn = true;
    updateLightState();
}

void MockLightController::disable() {
    state_.isOn = false;
    state_.brightness = 0;
    state_.transitionActive = false;
    sineWaveActive_ = false;
    mode_ = LightMode::DISABLED;
}

void MockLightController::resetStatistics() {
    accumulatedOnTime_ = 0;
    accumulatedOffTime_ = 0;
    state_.onDuration = 0;
    state_.offDuration = 0;
}

void MockLightController::simulateTimeAdvance(std::chrono::seconds seconds) {
    for (int i = 0; i < static_cast<int>(seconds.count()); ++i) {
        processTick();
    }
}

void MockLightController::processTick() {
    simulatedSeconds_++;

    updateTransition();
    updateSineWave();
    updateTiming();

    state_.lastStateChange = now();
}

void MockLightController::startTransition(uint8_t targetBrightness) {
    startBrightnessTransition(targetBrightness);
}

void MockLightController::stopTransition() {
    state_.transitionActive = false;
    sineWaveActive_ = false;
}

void MockLightController::setManualOverride(bool override) {
    manualOverride_ = override;
}

void MockLightController::startSineWaveTransition(uint32_t durationSeconds) {
    if (!config_.enableLight) return;

    sineWaveActive_ = true;
    sineWaveStartTime_ = now();
    sineWaveDuration_ = durationSeconds;
}

void MockLightController::stopSineWaveTransition() {
    sineWaveActive_ = false;
}

void MockLightController::updateLightState() {
    bool oldOnState = state_.isOn;
    uint8_t oldBrightness = state_.brightness;

    switch (mode_) {
        case LightMode::DISABLED:
            state_.isOn = false;
            state_.brightness = 0;
            break;

        case LightMode::MANUAL_ON:
            state_.isOn = true;
            if (!state_.transitionActive) {
                state_.brightness = manualBrightness_;
            }
            break;

        case LightMode::MANUAL_OFF:
            state_.isOn = false;
            state_.brightness = 0;
            break;

        case LightMode::AUTO:
            if (config_.enableSunriseSunset) {
                uint32_t currentTimeMinutes = currentHour_ * 60 + currentMinute_;
                uint32_t sunriseMinutes = sunriseHour_ * 60 + sunriseMinute_;
                uint32_t sunsetMinutes = sunsetHour_ * 60 + sunsetMinute_;

                state_.isOn = (currentTimeMinutes >= sunriseMinutes && currentTimeMinutes <= sunsetMinutes);
                state_.brightness = state_.isOn ? config_.maxBrightness : 0;
            } else {
                uint32_t currentTimeMinutes = currentHour_ * 60 + currentMinute_;
                uint32_t dayStartMinutes = config_.dayStartHour * 60;
                uint32_t dayEndMinutes = config_.dayEndHour * 60;

                state_.isOn = (currentTimeMinutes >= dayStartMinutes && currentTimeMinutes <= dayEndMinutes);
                state_.brightness = state_.isOn ? config_.maxBrightness : 0;
            }
            break;
    }

    if (oldOnState != state_.isOn || oldBrightness != state_.brightness) {
        notifyStateChange();
        if (oldBrightness != state_.brightness) {
            notifyBrightnessChange(state_.brightness);
        }
    }
}

void MockLightController::startBrightnessTransition(uint8_t targetBrightness) {
    if (!config_.enableLight) {
        return;
    }

    if (targetBrightness > config_.maxBrightness) targetBrightness = config_.maxBrightness;
    if (targetBrightness != 0 && targetBrightness < config_.minBrightness) targetBrightness = config_.minBrightness;

    if (targetBrightness == state_.brightness) {
        state_.transitionActive = false;
        state_.transitionProgress = 1.0f;
        return;
    }

    state_.transitionActive = true;
    state_.transitionProgress = 0.0f;
    transitionStartBrightness_ = state_.brightness;
    transitionTargetBrightness_ = targetBrightness;
    transitionStartTime_ = now();

    uint8_t brightnessDiff = static_cast<uint8_t>(std::abs(static_cast<int>(targetBrightness) - static_cast<int>(state_.brightness)));
    uint32_t baseDuration = (targetBrightness >= state_.brightness) ? config_.fadeInDuration : config_.fadeOutDuration;

    transitionDuration_ = std::max<uint32_t>(1, (brightnessDiff * baseDuration) / 255);
}

void MockLightController::updateTransition() {
    if (!state_.transitionActive) return;

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now() - transitionStartTime_);

    float progress = static_cast<float>(elapsed.count()) / static_cast<float>(transitionDuration_);
    progress = std::max(0.0f, std::min(1.0f, progress));

    state_.transitionProgress = progress;

    uint8_t newBrightness = static_cast<uint8_t>(
        transitionStartBrightness_ +
        (transitionTargetBrightness_ - transitionStartBrightness_) * progress
    );

    if (newBrightness != state_.brightness) {
        state_.brightness = newBrightness;
        notifyBrightnessChange(newBrightness);
    }

    if (progress >= 1.0f) {
        state_.transitionActive = false;
        state_.transitionProgress = 1.0f;
        state_.brightness = transitionTargetBrightness_;
    }
}

void MockLightController::updateSineWave() {
    if (!sineWaveActive_) return;

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now() - sineWaveStartTime_);

    if (elapsed.count() >= static_cast<long>(sineWaveDuration_)) {
        sineWaveActive_ = false;
        return;
    }

    float progress = static_cast<float>(elapsed.count()) / static_cast<float>(std::max<uint32_t>(1, sineWaveDuration_));

    float sineValue = 0.5f * (1.0f + std::sin(2.0f * M_PI * progress));
    uint8_t newBrightness = static_cast<uint8_t>(sineValue * config_.maxBrightness);

    if (newBrightness != state_.brightness) {
        state_.brightness = newBrightness;
        state_.isOn = (newBrightness > 0);
        notifyBrightnessChange(newBrightness);
    }
}

void MockLightController::updateTiming() {
    if (state_.isOn) {
        accumulatedOnTime_++;
        state_.onDuration = accumulatedOnTime_;
    } else {
        accumulatedOffTime_++;
        state_.offDuration = accumulatedOffTime_;
    }
}

void MockLightController::notifyStateChange() {
    if (stateChangeCallback_) {
        stateChangeCallback_(state_);
    }
}

void MockLightController::notifyBrightnessChange(uint8_t brightness) {
    if (brightnessChangeCallback_) {
        brightnessChangeCallback_(brightness);
    }
}
