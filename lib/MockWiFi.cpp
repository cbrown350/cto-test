#include "MockWiFi.h"
#include <algorithm>
#include <random>

void MockWiFi::setNextBeginResult(bool success, uint32_t reason) {
    hasNextBeginResult_ = true;
    nextBeginResult_ = success;
    nextBeginFailureReason_ = reason;
}

bool MockWiFi::begin(const std::string& ssid, const std::string& password, int32_t channel) {
    if (ssid.empty()) {
        return false;
    }
    
    connectionAttempts_++;
    lastConnectionAttempt_ = std::chrono::steady_clock::now();
    status_ = ConnectionStatus::CONNECTING;
    
    // Simulate connection delay
    simulateConnectionDelay(std::chrono::milliseconds(100));

    if (hasNextBeginResult_) {
        bool ok = nextBeginResult_;
        uint32_t reason = nextBeginFailureReason_;
        hasNextBeginResult_ = false;

        state_.ssid = ssid;
        state_.password = password;

        if (!ok) {
            state_.connected = false;
            state_.disconnectReason = reason;
            status_ = ConnectionStatus::ERROR;
            notifyEvent("WIFI_CONNECTION_FAILED");
            notifyConnectionChange(false);
            return false;
        }
    }

    // In real scenario, this would fail for invalid credentials
    // For testing, simulate successful connection
    state_.ssid = ssid;
    state_.password = password;
    state_.connected = true;
    state_.localIP = generateMockIP(ssid);
    state_.gatewayIP = "192.168.1.1";
    state_.subnetMask = "255.255.255.0";
    state_.dnsIP = "8.8.8.8";
    state_.macAddress = "24:6F:28:AA:BB:CC";
    state_.rssi = -45; // Good signal strength
    state_.channel = (channel == 0) ? 6 : channel; // Default to channel 6
    state_.connectionTime = std::chrono::steady_clock::now();
    state_.disconnectReason = 0;
    
    connectionStartTime_ = state_.connectionTime;
    
    status_ = ConnectionStatus::CONNECTED;
    successfulConnections_++;
    
    notifyConnectionChange(true);
    notifyEvent("WIFI_CONNECTED");
    
    return true;
}

bool MockWiFi::beginWithWPS() {
    connectionAttempts_++;
    status_ = ConnectionStatus::CONNECTING;
    
    // Simulate WPS process
    simulateConnectionDelay(std::chrono::seconds(2));
    
    // For testing, simulate WPS failure (WPS is complex to mock)
    status_ = ConnectionStatus::ERROR;
    notifyEvent("WPS_FAILED");
    
    return false;
}

bool MockWiFi::disconnect(bool wifiOff) {
    if (state_.connected) {
        uint32_t reason = wifiOff ? 2 : 1; // WIFI_DISCONNECT_WIFI_OFF or WIFI_DISCONNECT_MANUAL
        simulateDisconnection(reason);
    }
    
    return true;
}

bool MockWiFi::reconnect() {
    if (state_.ssid.empty()) {
        return false;
    }
    
    return begin(state_.ssid, state_.password);
}

bool MockWiFi::softAP(const std::string& ssid, const std::string& password, int32_t channel, bool hidden) {
    apSSID_ = ssid;
    apPassword_ = password;
    apChannel_ = (channel == 0) ? 6 : channel;
    apHidden_ = hidden;
    
    apMode_ = AccessPointMode::ON;
    
    notifyEvent("WIFI_AP_STARTED");
    return true;
}

bool MockWiFi::softAPDisconnect(bool wifiOff) {
    (void)wifiOff;

    apMode_ = AccessPointMode::OFF;
    apConnectedClients_.clear();
    
    notifyEvent("WIFI_AP_STOPPED");
    return true;
}

void MockWiFi::setAutoReconnect(bool autoReconnect) {
    autoReconnect_ = autoReconnect;
}

void MockWiFi::setHostname(const std::string& hostname) {
    hostname_ = hostname;
}

void MockWiFi::setDNS(uint8_t dns_no, const std::string& dns1, const std::string& dns2) {
    (void)dns2;

    if (dns_no == 0) {
        state_.dnsIP = dns1;
    }
    // DNS2 would be stored separately in real implementation
}

bool MockWiFi::config(const std::string& localIP, const std::string& gatewayIP, const std::string& subnetMask, const std::string& dns1, const std::string& dns2) {
    (void)dns2;

    state_.localIP = localIP;
    state_.gatewayIP = gatewayIP;
    state_.subnetMask = subnetMask;
    if (!dns1.empty()) {
        state_.dnsIP = dns1;
    }
    
    notifyEvent("IP_CONFIG_UPDATED");
    return true;
}

bool MockWiFi::setIPAddress(const std::string& localIP, const std::string& gatewayIP, const std::string& subnetMask) {
    return config(localIP, gatewayIP, subnetMask);
}

void MockWiFi::enableSTA(bool enable) {
    staEnabled_ = enable;
    if (!enable && state_.connected) {
        disconnect();
    }
}

void MockWiFi::setSleepMode(int32_t sleepMode) {
    sleepMode_ = sleepMode;
}

std::string MockWiFi::getDNSIP(uint8_t dns_no) const {
    if (dns_no == 0) {
        return state_.dnsIP;
    }
    return std::string(); // DNS2 not implemented in mock
}

std::vector<MockWiFi::WiFiNetwork> MockWiFi::scanNetworks(bool async) {
    (void)async;

    if (scanInProgress_) {
        return std::vector<WiFiNetwork>();
    }
    
    scanInProgress_ = true;
    
    // Simulate scan delay
    simulateConnectionDelay(std::chrono::milliseconds(100));
    
    // Generate mock networks
    availableNetworks_.clear();
    
    static const char* mockSSIDs[] = {
        "HomeNetwork", "NeighborWiFi", "CoffeeShop", "OfficeWiFi", 
        "MobileHotspot", "GuestNetwork", "TestNetwork", "ESP32-AP"
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> ssidDist(0, 7);
    std::uniform_int_distribution<> rssiDist(-80, -30);
    std::uniform_int_distribution<> channelDist(1, 11);
    std::uniform_int_distribution<> encDist(0, 3);
    
    uint8_t networkCount = ssidDist(gen) + 1; // 1-8 networks
    
    for (int i = 0; i < networkCount; ++i) {
        WiFiNetwork network;
        network.ssid = mockSSIDs[ssidDist(gen)];
        network.bssid = "AA:BB:CC:DD:EE:FF";
        network.rssi = rssiDist(gen);
        network.channel = channelDist(gen);
        network.encryptionType = encDist(gen);
        network.hidden = false;
        
        availableNetworks_.push_back(network);
    }
    
    scanInProgress_ = false;
    notifyScanComplete();
    
    return availableNetworks_;
}

uint8_t MockWiFi::scanNetworksCount() {
    scanNetworks(false);
    return static_cast<uint8_t>(availableNetworks_.size());
}

void MockWiFi::simulateConnection(const std::string& ssid, const std::string& ip) {
    state_.ssid = ssid;
    state_.localIP = ip;
    state_.gatewayIP = "192.168.1.1";
    state_.subnetMask = "255.255.255.0";
    state_.connected = true;
    state_.rssi = -50;
    state_.connectionTime = std::chrono::steady_clock::now();
    
    status_ = ConnectionStatus::CONNECTED;
    successfulConnections_++;
    notifyConnectionChange(true);
}

void MockWiFi::simulateDisconnection(uint32_t reason) {
    if (state_.connected) {
        state_.connected = false;
        state_.disconnectReason = reason;
        status_ = ConnectionStatus::DISCONNECTED;
        disconnections_++;
        
        notifyConnectionChange(false);
        notifyEvent("WIFI_DISCONNECTED");
    }
}

void MockWiFi::simulateConnectionFailure(const std::string& ssid, uint32_t reason) {
    (void)ssid;
    (void)reason;

    status_ = ConnectionStatus::ERROR;
    notifyEvent("WIFI_CONNECTION_FAILED");
}

void MockWiFi::simulateNetworkFound(const WiFiNetwork& network) {
    availableNetworks_.push_back(network);
}

void MockWiFi::simulateScanComplete() {
    scanInProgress_ = false;
    notifyScanComplete();
}

void MockWiFi::simulateAPConnection(const std::string& clientIP) {
    apConnectedClients_.push_back(clientIP);
    apMode_ = AccessPointMode::ON_WITH_CLIENTS;
    notifyEvent("WIFI_AP_CLIENT_CONNECTED");
}

void MockWiFi::simulateAPDisconnection(const std::string& clientIP) {
    auto it = std::find(apConnectedClients_.begin(), apConnectedClients_.end(), clientIP);
    if (it != apConnectedClients_.end()) {
        apConnectedClients_.erase(it);
    }
    
    if (apConnectedClients_.empty()) {
        apMode_ = AccessPointMode::ON;
    }
    
    notifyEvent("WIFI_AP_CLIENT_DISCONNECTED");
}

std::chrono::milliseconds MockWiFi::getConnectionDuration() const {
    if (!state_.connected) {
        return std::chrono::milliseconds{0};
    }
    
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - connectionStartTime_);
}

void MockWiFi::setEventCallback(WiFiEventCallback callback) {
    eventCallback_ = callback;
}

void MockWiFi::setConnectionCallback(ConnectionCallback callback) {
    connectionCallback_ = callback;
}

void MockWiFi::setScanCallback(ScanCallback callback) {
    scanCallback_ = callback;
}

void MockWiFi::updateConnectionState(bool connected, const std::string& reason) {
    if (state_.connected != connected) {
        state_.connected = connected;
        notifyConnectionChange(connected);
        
        if (!connected) {
            notifyEvent("WIFI_DISCONNECTED: " + reason);
        }
    }
}

void MockWiFi::notifyConnectionChange(bool connected) {
    if (connectionCallback_) {
        connectionCallback_(connected);
    }
}

void MockWiFi::notifyEvent(const std::string& event) {
    if (eventCallback_) {
        eventCallback_(event);
    }
}

void MockWiFi::notifyScanComplete() {
    if (scanCallback_) {
        scanCallback_(availableNetworks_);
    }
}

std::string MockWiFi::generateMockIP(const std::string& ssid) {
    // Generate consistent IP based on SSID for testing
    uint32_t hash = 0;
    for (char c : ssid) {
        hash += static_cast<uint32_t>(c);
    }
    
    uint8_t octet4 = (hash % 254) + 1; // 1-254
    return "192.168.1." + std::to_string(octet4);
}

void MockWiFi::simulateConnectionDelay(const std::chrono::milliseconds& delay) {
    (void)delay;
    // In real embedded testing, this would not block.
}