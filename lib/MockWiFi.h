#ifndef MOCK_WIFI_H
#define MOCK_WIFI_H

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <chrono>

class MockWiFi {
public:
    struct WiFiNetwork {
        std::string ssid;
        std::string bssid;
        int32_t rssi; // signal strength in dBm
        uint8_t channel;
        uint8_t encryptionType;
        bool hidden;
    };

    struct ConnectionState {
        bool connected;
        std::string ssid;
        std::string password;
        std::string localIP;
        std::string gatewayIP;
        std::string subnetMask;
        std::string dnsIP;
        std::string macAddress;
        int32_t rssi;
        uint8_t channel;
        std::chrono::steady_clock::time_point connectionTime;
        uint32_t disconnectReason;
    };

    enum class ConnectionStatus {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        RECONNECTING,
        ERROR
    };

    enum class AccessPointMode {
        OFF,
        ON,
        ON_WITH_CLIENTS
    };

    MockWiFi() = default;
    virtual ~MockWiFi() = default;

    // Connection management
    bool begin(const std::string& ssid, const std::string& password = "", int32_t channel = 0);
    bool beginWithWPS();
    bool disconnect(bool wifiOff = false);
    bool reconnect();
    bool isConnected() const { return state_.connected; }
    
    // Access Point mode
    bool softAP(const std::string& ssid, const std::string& password = "", int32_t channel = 0, bool hidden = false);
    bool softAPDisconnect(bool wifiOff = false);
    bool softAPEnabled() const { return apMode_ != AccessPointMode::OFF; }
    
    // Configuration
    void setAutoReconnect(bool autoReconnect);
    bool getAutoReconnect() const { return autoReconnect_; }
    
    void setHostname(const std::string& hostname);
    std::string getHostname() const { return hostname_; }
    
    void setDNS(uint8_t dns_no, const std::string& dns1, const std::string& dns2 = "");
    
    // Status information
    ConnectionStatus status() const { return status_; }
    ConnectionState getState() const { return state_; }
    std::string getLocalIP() const { return state_.localIP; }
    std::string getGatewayIP() const { return state_.gatewayIP; }
    std::string getSubnetMask() const { return state_.subnetMask; }
    std::string getDNSIP(uint8_t dns_no = 0) const;
    std::string getMacAddress() const { return state_.macAddress; }
    int32_t getRSSI() const { return state_.rssi; }
    std::string getSSID() const { return state_.ssid; }
    
    // Network scanning
    std::vector<WiFiNetwork> scanNetworks(bool async = false);
    uint8_t scanNetworksCount();
    bool scanComplete() const { return scanInProgress_ == false; }
    
    // IP configuration
    bool config(const std::string& localIP, const std::string& gatewayIP, const std::string& subnetMask, const std::string& dns1 = "", const std::string& dns2 = "");
    bool setIPAddress(const std::string& localIP, const std::string& gatewayIP = "", const std::string& subnetMask = "");
    
    // Security
    void enableSTA(bool enable);
    bool STAEnabled() const { return staEnabled_; }
    
    // Power management
    void setSleepMode(int32_t sleepMode);
    int32_t getSleepMode() const { return sleepMode_; }
    
    // Simulation methods for testing
    void simulateConnection(const std::string& ssid, const std::string& ip = "192.168.1.100");
    void simulateDisconnection(uint32_t reason = 1);
    void simulateConnectionFailure(const std::string& ssid, uint32_t reason = 1);
    void simulateNetworkFound(const WiFiNetwork& network);
    void simulateScanComplete();
    void simulateAPConnection(const std::string& clientIP);
    void simulateAPDisconnection(const std::string& clientIP);
    
    // Statistics and monitoring
    uint32_t getConnectionAttempts() const { return connectionAttempts_; }
    uint32_t getSuccessfulConnections() const { return successfulConnections_; }
    uint32_t getDisconnections() const { return disconnections_; }
    std::chrono::milliseconds getConnectionDuration() const;
    
    // Callback registration
    using WiFiEventCallback = std::function<void(const std::string& event)>;
    using ConnectionCallback = std::function<void(bool connected)>;
    using ScanCallback = std::function<void(const std::vector<WiFiNetwork>&)>;
    
    void setEventCallback(WiFiEventCallback callback);
    void setConnectionCallback(ConnectionCallback callback);
    void setScanCallback(ScanCallback callback);

private:
    ConnectionState state_;
    ConnectionStatus status_ = ConnectionStatus::DISCONNECTED;
    AccessPointMode apMode_ = AccessPointMode::OFF;
    
    // Configuration
    std::string hostname_ = "esp32-coop";
    bool autoReconnect_ = true;
    bool staEnabled_ = true;
    int32_t sleepMode_ = 0; // WIFI_NONE_SLEEP
    
    // Network scanning
    bool scanInProgress_ = false;
    std::vector<WiFiNetwork> availableNetworks_;
    uint32_t lastScanTime_ = 0;
    
    // Access Point state
    std::string apSSID_;
    std::string apPassword_;
    int32_t apChannel_ = 1;
    bool apHidden_ = false;
    std::vector<std::string> apConnectedClients_;
    
    // Statistics
    uint32_t connectionAttempts_ = 0;
    uint32_t successfulConnections_ = 0;
    uint32_t disconnections_ = 0;
    
    // Time tracking
    std::chrono::steady_clock::time_point lastConnectionAttempt_;
    std::chrono::steady_clock::time_point connectionStartTime_;
    
    // Callbacks
    WiFiEventCallback eventCallback_;
    ConnectionCallback connectionCallback_;
    ScanCallback scanCallback_;
    
    // Internal methods
    void updateConnectionState(bool connected, const std::string& reason = "");
    void notifyConnectionChange(bool connected);
    void notifyEvent(const std::string& event);
    void notifyScanComplete();
    std::string generateMockIP(const std::string& ssid = "");
    void simulateConnectionDelay(const std::chrono::milliseconds& delay);
};

#endif // MOCK_WIFI_H