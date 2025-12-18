#include "WifiController.h"

#include "MockWiFi.h"

WifiController::WifiController(MockWiFi& wifi) : wifi_(wifi) {
}

void WifiController::setConfig(const Config& config) {
    config_ = config;

    if (!config_.enabled) {
        disable();
    } else if (state_ == State::DISABLED) {
        enable();
    }
}

void WifiController::setCredentials(const std::string& ssid, const std::string& password) {
    ssid_ = ssid;
    password_ = password;
}

void WifiController::enable() {
    if (config_.enabled) {
        if (state_ == State::DISABLED) {
            state_ = State::DISCONNECTED;
        }
    }
}

void WifiController::disable() {
    state_ = State::DISABLED;
    wifi_.disconnect(true);
    wifi_.softAPDisconnect(true);
}

bool WifiController::connectNow() {
    if (!config_.enabled) {
        state_ = State::DISABLED;
        return false;
    }

    if (ssid_.empty()) {
        state_ = State::DISCONNECTED;
        return false;
    }

    state_ = State::CONNECTING;

    bool ok = wifi_.begin(ssid_, password_);
    if (ok) {
        state_ = State::CONNECTED;
        secondsSinceLastAttempt_ = 0;
        return true;
    }

    state_ = State::DISCONNECTED;
    return false;
}

void WifiController::disconnect() {
    wifi_.disconnect(true);
    state_ = State::DISCONNECTED;
}

bool WifiController::isConnected() const {
    return state_ == State::CONNECTED && wifi_.isConnected();
}

void WifiController::resetRetryCount() {
    retryCount_ = 0;
    secondsSinceLastAttempt_ = 0;
}

void WifiController::handleWiFiDisconnected(uint32_t reason) {
    (void)reason;

    if (state_ == State::DISABLED || state_ == State::AP_MODE) {
        return;
    }

    state_ = State::DISCONNECTED;
    secondsSinceLastAttempt_ = config_.retryIntervalSeconds;
}

void WifiController::processTick() {
    if (!config_.enabled) {
        state_ = State::DISABLED;
        return;
    }

    // If the underlying WiFi got disconnected externally, reflect it.
    if (state_ == State::CONNECTED && !wifi_.isConnected()) {
        handleWiFiDisconnected(wifi_.getState().disconnectReason);
    }

    if (state_ == State::AP_MODE) {
        return;
    }

    if (state_ == State::CONNECTED) {
        return;
    }

    // DISCONNECTED/CONNECTING state machine with retry/backoff.
    secondsSinceLastAttempt_++;

    if (ssid_.empty()) {
        return;
    }

    if (secondsSinceLastAttempt_ < config_.retryIntervalSeconds) {
        return;
    }

    secondsSinceLastAttempt_ = 0;

    if (retryCount_ >= config_.maxRetries) {
        if (config_.enableAPFallback) {
            startAccessPoint();
        }
        return;
    }

    retryCount_++;
    connectNow();

    if (state_ == State::CONNECTED) {
        retryCount_ = 0;
    }
}

void WifiController::startAccessPoint() {
    wifi_.softAP(config_.apSSID, config_.apPassword);
    state_ = State::AP_MODE;
}
