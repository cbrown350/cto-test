#include "MockSettingsManager.h"
#include <sstream>
#include <algorithm>
#include <fstream>

// Individual setting accessor implementations
bool MockSettingsManager::getSettingBool(const std::string& key, bool defaultValue) const {
    auto it = rawSettings_.find(key);
    if (it == rawSettings_.end()) {
        return defaultValue;
    }
    return (it->second == "true" || it->second == "1");
}

int MockSettingsManager::getSettingInt(const std::string& key, int defaultValue) const {
    auto it = rawSettings_.find(key);
    if (it == rawSettings_.end()) {
        return defaultValue;
    }
    std::stringstream ss(it->second);
    int value;
    ss >> value;
    return value;
}

unsigned int MockSettingsManager::getSettingUInt(const std::string& key, unsigned int defaultValue) const {
    auto it = rawSettings_.find(key);
    if (it == rawSettings_.end()) {
        return defaultValue;
    }
    std::stringstream ss(it->second);
    unsigned int value;
    ss >> value;
    return value;
}

float MockSettingsManager::getSettingFloat(const std::string& key, float defaultValue) const {
    auto it = rawSettings_.find(key);
    if (it == rawSettings_.end()) {
        return defaultValue;
    }
    std::stringstream ss(it->second);
    float value;
    ss >> value;
    return value;
}

std::string MockSettingsManager::getSettingString(const std::string& key, const std::string& defaultValue) const {
    auto it = rawSettings_.find(key);
    if (it == rawSettings_.end()) {
        return defaultValue;
    }
    return it->second;
}

bool MockSettingsManager::setSettingBool(const std::string& key, bool value) {
    std::string oldValue = getSettingRaw(key);
    setSettingRaw(key, value ? "true" : "false");
    notifySettingChange(key, oldValue, value ? "true" : "false");
    return true;
}

bool MockSettingsManager::setSettingInt(const std::string& key, int value) {
    std::string oldValue = getSettingRaw(key);
    std::string newValue = std::to_string(value);
    setSettingRaw(key, newValue);
    notifySettingChange(key, oldValue, newValue);
    return true;
}

bool MockSettingsManager::setSettingUInt(const std::string& key, unsigned int value) {
    std::string oldValue = getSettingRaw(key);
    std::string newValue = std::to_string(value);
    setSettingRaw(key, newValue);
    notifySettingChange(key, oldValue, newValue);
    return true;
}

bool MockSettingsManager::setSettingFloat(const std::string& key, float value) {
    std::string oldValue = getSettingRaw(key);
    std::string newValue = std::to_string(value);
    setSettingRaw(key, newValue);
    notifySettingChange(key, oldValue, newValue);
    return true;
}

bool MockSettingsManager::setSettingString(const std::string& key, const std::string& value) {
    std::string oldValue = getSettingRaw(key);
    setSettingRaw(key, value);
    notifySettingChange(key, oldValue, value);
    return true;
}

bool MockSettingsManager::loadSettings() {
    if (testMode_) {
        // In test mode, use in-memory storage
        return true;
    }
    
    // In real mode, would load from file system
    // For testing, simulate successful load
    return true;
}

bool MockSettingsManager::saveSettings() {
    if (testMode_) {
        // In test mode, just mark as saved
        unsavedChanges_ = false;
        return true;
    }
    
    // In real mode, would save to file system
    unsavedChanges_ = false;
    return true;
}

bool MockSettingsManager::resetToDefaults() {
    settings_ = Settings{};
    clearAllSettings();
    unsavedChanges_ = true;
    return true;
}

bool MockSettingsManager::settingsFileExists() const {
    if (testMode_) {
        return true; // Simulate file exists in test mode
    }
    
    // In real mode, would check file system
    return false;
}

void MockSettingsManager::setSettings(const Settings& settings) {
    settings_ = settings;
    unsavedChanges_ = true;
}

std::string MockSettingsManager::serializeToJson() const {
    // Simplified JSON serialization for testing
    std::ostringstream json;
    json << "{\n";
    json << "  \"pumpEnabled\": " << (settings_.pumpEnabled ? "true" : "false") << ",\n";
    json << "  \"pumpFreezeThreshold\": " << settings_.freezeThreshold << ",\n";
    json << "  \"pumpOnDuration\": " << settings_.pumpOnDuration << ",\n";
    json << "  \"pumpOffDuration\": " << settings_.pumpOffDuration << ",\n";
    json << "  \"lightEnabled\": " << (settings_.lightEnabled ? "true" : "false") << ",\n";
    json << "  \"lightMaxBrightness\": " << static_cast<int>(settings_.lightMaxBrightness) << ",\n";
    json << "  \"wifiSSID\": \"" << settings_.wifiSSID << "\",\n";
    json << "  \"wifiPassword\": \"" << settings_.wifiPassword << "\",\n";
    json << "  \"tempMeterPin\": " << settings_.tempMeterPin << ",\n";
    json << "  \"lightPin\": " << settings_.lightPin << ",\n";
    json << "  \"pulsesPerGallon\": " << settings_.pulsesPerGallon << "\n";
    json << "}";
    
    return json.str();
}

bool MockSettingsManager::deserializeFromJson(const std::string& json) {
    // Simplified JSON parsing for testing
    // In real implementation would use ArduinoJson
    if (json.find("\"pumpEnabled\"") != std::string::npos) {
        settings_.pumpEnabled = json.find("true") != std::string::npos;
    }
    if (json.find("\"lightEnabled\"") != std::string::npos) {
        settings_.lightEnabled = json.find("true") != std::string::npos;
    }
    // Parse other fields as needed...
    
    unsavedChanges_ = true;
    return true;
}

bool MockSettingsManager::validateSettings() const {
    // Basic validation for testing
    if (settings_.freezeThreshold < -55.0f || settings_.freezeThreshold > 125.0f) {
        return false;
    }
    if (settings_.pumpOnDuration == 0 || settings_.pumpOffDuration == 0) {
        return false;
    }
    if (settings_.lightMaxBrightness > 255 || settings_.lightMinBrightness > 255) {
        return false;
    }
    if (settings_.lightMaxBrightness < settings_.lightMinBrightness) {
        return false;
    }
    return true;
}

std::vector<std::string> MockSettingsManager::getValidationErrors() const {
    std::vector<std::string> errors;
    
    if (settings_.freezeThreshold < -55.0f || settings_.freezeThreshold > 125.0f) {
        errors.push_back("Freeze threshold out of DS18B20 sensor range");
    }
    if (settings_.pumpOnDuration == 0) {
        errors.push_back("Pump on duration cannot be zero");
    }
    if (settings_.pumpOffDuration == 0) {
        errors.push_back("Pump off duration cannot be zero");
    }
    if (settings_.lightMaxBrightness > 255) {
        errors.push_back("Light max brightness exceeds 255");
    }
    if (settings_.lightMinBrightness > 255) {
        errors.push_back("Light min brightness exceeds 255");
    }
    if (settings_.lightMaxBrightness < settings_.lightMinBrightness) {
        errors.push_back("Max brightness must be greater than or equal to min brightness");
    }
    
    return errors;
}

bool MockSettingsManager::createBackup(const std::string& filename) {
    (void)filename;

    if (testMode_) {
        // In test mode, just simulate backup creation
        return true;
    }
    
    // In real mode, would create backup file
    return true;
}

bool MockSettingsManager::restoreBackup(const std::string& filename) {
    (void)filename;

    if (testMode_) {
        // In test mode, simulate restore
        return true;
    }
    
    // In real mode, would restore from backup file
    return true;
}

void MockSettingsManager::setSettingsChangeCallback(SettingsChangeCallback callback) {
    changeCallback_ = callback;
}

void MockSettingsManager::clearAllSettings() {
    rawSettings_.clear();
    unsavedChanges_ = true;
}

void MockSettingsManager::setSettingRaw(const std::string& key, const std::string& value) {
    rawSettings_[key] = value;
    unsavedChanges_ = true;
}

std::string MockSettingsManager::getSettingRaw(const std::string& key) const {
    auto it = rawSettings_.find(key);
    if (it != rawSettings_.end()) {
        return it->second;
    }
    return std::string();
}

void MockSettingsManager::notifySettingChange(const std::string& key, const std::string& oldValue, const std::string& newValue) {
    if (changeCallback_) {
        changeCallback_(key, oldValue, newValue);
    }
}

std::string MockSettingsManager::getSettingKey(const std::string& group, const std::string& name) const {
    return group + "." + name;
}