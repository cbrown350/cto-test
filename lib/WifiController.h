#ifndef WIFI_CONTROLLER_H
#define WIFI_CONTROLLER_H

#include <cstdint>
#include <string>

class MockWiFi;

class WifiController {
public:
    struct Config {
        bool enabled = true;
        bool enableAPFallback = true;
        uint32_t maxRetries = 5;
        uint32_t retryIntervalSeconds = 5;

        std::string apSSID = "CoopController-Setup";
        std::string apPassword;
    };

    enum class State {
        DISABLED,
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        AP_MODE
    };

    explicit WifiController(MockWiFi& wifi);

    void setConfig(const Config& config);
    Config getConfig() const { return config_; }

    void setCredentials(const std::string& ssid, const std::string& password);
    std::string getSSID() const { return ssid_; }

    void enable();
    void disable();

    bool connectNow();
    void disconnect();

    void processTick(); // 1 second tick

    State getState() const { return state_; }
    bool isConnected() const;
    bool isAPMode() const { return state_ == State::AP_MODE; }

    uint32_t getRetryCount() const { return retryCount_; }
    void resetRetryCount();

    // Reconnection handling
    void handleWiFiDisconnected(uint32_t reason);

private:
    MockWiFi& wifi_;
    Config config_;

    State state_ = State::DISCONNECTED;
    std::string ssid_;
    std::string password_;

    uint32_t retryCount_ = 0;
    uint32_t secondsSinceLastAttempt_ = 0;

    void startAccessPoint();
};

#endif // WIFI_CONTROLLER_H
