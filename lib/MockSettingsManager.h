#ifndef MOCK_SETTINGS_MANAGER_H
#define MOCK_SETTINGS_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

class MockSettingsManager {
public:
    struct Settings {
        // Pump settings
        bool pumpEnabled = true;
        float freezeThreshold = 1.1f; // 34°F
        uint32_t pumpOnDuration = 300; // 5 minutes
        uint32_t pumpOffDuration = 600; // 10 minutes
        uint32_t pumpMaxOnTime = 1800; // 30 minutes
        uint32_t pumpFaultTimeout = 60; // seconds
        uint32_t pumpMinPulsesPerMinute = 10;
        
        // Light settings
        bool lightEnabled = true;
        uint8_t lightMaxBrightness = 255;
        uint8_t lightMinBrightness = 0;
        uint32_t lightFadeInDuration = 300; // 5 minutes
        uint32_t lightFadeOutDuration = 300; // 5 minutes
        uint32_t lightDayStartHour = 6;
        uint32_t lightDayEndHour = 22;
        bool lightEnableSunriseSunset = false;
        float lightLatitude = 0.0f;
        float lightLongitude = 0.0f;
        int lightTimezoneOffset = 0; // minutes from UTC
        
        // Network settings
        std::string wifiSSID = "";
        std::string wifiPassword = "";
        bool wifiEnabled = true;
        uint16_t webServerPort = 80;
        
        // Sensor settings
        uint32_t tempMeterPin = 32;
        uint32_t tempMeter2Pin = 33;
        uint32_t pumpPin = 26;
        uint32_t lightPin = 25;
        uint32_t pulsesPerGallon = 1000;
        
        // System settings
        bool syslogEnabled = false;
        std::string syslogServer = "";
        uint16_t syslogPort = 514;
        bool emailEnabled = false;
        std::string emailServer = "";
        uint16_t emailPort = 587;
        std::string emailUsername = "";
        std::string emailPassword = "";
        std::string emailRecipient = "";
        
        // Door settings (planned)
        bool doorEnabled = false;
        uint32_t doorOpenTime = 30; // seconds
        uint32_t doorCloseTime = 30; // seconds
        uint32_t doorRetryAttempts = 3;
    };

    MockSettingsManager() = default;
    virtual ~MockSettingsManager() = default;

    // Settings management
    bool loadSettings();
    bool saveSettings();
    bool resetToDefaults();
    bool settingsFileExists() const;
    
    // Settings access
    Settings getSettings() const { return settings_; }
    void setSettings(const Settings& settings);
    
    // Individual setting accessors
    bool getSettingBool(const std::string& key, bool defaultValue = false) const;
    int getSettingInt(const std::string& key, int defaultValue = 0) const;
    unsigned int getSettingUInt(const std::string& key, unsigned int defaultValue = 0u) const;
    float getSettingFloat(const std::string& key, float defaultValue = 0.0f) const;
    std::string getSettingString(const std::string& key, const std::string& defaultValue = "") const;
    
    bool setSettingBool(const std::string& key, bool value);
    bool setSettingInt(const std::string& key, int value);
    bool setSettingUInt(const std::string& key, unsigned int value);
    bool setSettingFloat(const std::string& key, float value);
    bool setSettingString(const std::string& key, const std::string& value);
    
    // Serialization
    std::string serializeToJson() const;
    bool deserializeFromJson(const std::string& json);
    
    // Validation
    bool validateSettings() const;
    std::vector<std::string> getValidationErrors() const;
    
    // Backup and restore
    bool createBackup(const std::string& filename);
    bool restoreBackup(const std::string& filename);
    
    // Change tracking
    bool hasUnsavedChanges() const { return unsavedChanges_; }
    void markSaved() { unsavedChanges_ = false; }
    
    // Callback registration
    using SettingsChangeCallback = std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)>;
    void setSettingsChangeCallback(SettingsChangeCallback callback);
    
    // Test utilities
    void setTestMode(bool testMode) { testMode_ = testMode; }
    bool isTestMode() const { return testMode_; }
    
    void clearAllSettings();
    void setSettingRaw(const std::string& key, const std::string& value);
    std::string getSettingRaw(const std::string& key) const;

private:
    Settings settings_;
    std::map<std::string, std::string> rawSettings_;
    bool unsavedChanges_ = false;
    bool testMode_ = false;
    std::string settingsFilePath_ = "/test/user_settings.json";
    SettingsChangeCallback changeCallback_;
    
    void notifySettingChange(const std::string& key, const std::string& oldValue, const std::string& newValue);
    std::string getSettingKey(const std::string& group, const std::string& name) const;
};

#endif // MOCK_SETTINGS_MANAGER_H