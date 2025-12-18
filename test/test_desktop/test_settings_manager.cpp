#include <gtest/gtest.h>

#include "CommonTestFixture.h"
#include "MockSettingsManager.h"
#include "TestUtils.h"

class SettingsManagerTest : public CommonTestFixture {
protected:
    MockSettingsManager settings;

    void SetUp() override {
        CommonTestFixture::SetUp();
        settings.setTestMode(true);
        settings.resetToDefaults();
        settings.markSaved();
    }
};

TEST_F(SettingsManagerTest, DefaultsValidateTrue) {
    EXPECT_TRUE(settings.validateSettings());
}

TEST_F(SettingsManagerTest, ResetToDefaultsMarksUnsavedChanges) {
    settings.resetToDefaults();
    EXPECT_TRUE(settings.hasUnsavedChanges());
}

TEST_F(SettingsManagerTest, TestModeSettingsFileExistsIsTrue) {
    settings.setTestMode(true);
    EXPECT_TRUE(settings.settingsFileExists());
}

TEST_F(SettingsManagerTest, NonTestModeSettingsFileExistsIsFalse) {
    settings.setTestMode(false);
    EXPECT_FALSE(settings.settingsFileExists());
}

TEST_F(SettingsManagerTest, IsTestModeReflectsSetting) {
    settings.setTestMode(true);
    EXPECT_TRUE(settings.isTestMode());

    settings.setTestMode(false);
    EXPECT_FALSE(settings.isTestMode());
}

TEST_F(SettingsManagerTest, LoadAndSaveSettingsReturnTrue) {
    settings.setTestMode(true);

    EXPECT_TRUE(settings.loadSettings());

    settings.setSettingString("wifi.ssid", "SSID");
    ASSERT_TRUE(settings.hasUnsavedChanges());

    EXPECT_TRUE(settings.saveSettings());
    EXPECT_FALSE(settings.hasUnsavedChanges());
}

TEST_F(SettingsManagerTest, GetMissingBoolReturnsDefault) {
    EXPECT_TRUE(settings.getSettingBool("missing", true));
    EXPECT_FALSE(settings.getSettingBool("missing", false));
}

TEST_F(SettingsManagerTest, SetGetBoolWorks) {
    settings.setSettingBool("feature.enabled", true);
    EXPECT_TRUE(settings.getSettingBool("feature.enabled"));
}

TEST_F(SettingsManagerTest, SetGetIntWorks) {
    settings.setSettingInt("pump.onDuration", -42);
    EXPECT_EQ(settings.getSettingInt("pump.onDuration"), -42);
}

TEST_F(SettingsManagerTest, SetGetUIntWorks) {
    settings.setSettingUInt("pump.onDuration", 42u);
    EXPECT_EQ(settings.getSettingUInt("pump.onDuration"), 42u);
}

TEST_F(SettingsManagerTest, SetGetFloatWorks) {
    settings.setSettingFloat("pump.freezeThreshold", 1.25f);
    EXPECT_NEAR(settings.getSettingFloat("pump.freezeThreshold"), 1.25f, 1e-3f);
}

TEST_F(SettingsManagerTest, SetGetStringWorks) {
    settings.setSettingString("wifi.ssid", "TestSSID");
    EXPECT_EQ(settings.getSettingString("wifi.ssid"), "TestSSID");
}

TEST_F(SettingsManagerTest, ChangeCallbackInvokedOnValueChange) {
    std::string key;
    std::string oldValue;
    std::string newValue;

    settings.setSettingsChangeCallback([&](const std::string& k, const std::string& o, const std::string& n) {
        key = k;
        oldValue = o;
        newValue = n;
    });

    settings.setSettingString("wifi.ssid", "SSID1");
    settings.setSettingString("wifi.ssid", "SSID2");

    EXPECT_EQ(key, "wifi.ssid");
    EXPECT_EQ(oldValue, "SSID1");
    EXPECT_EQ(newValue, "SSID2");
}

TEST_F(SettingsManagerTest, SerializeToJsonContainsExpectedFields) {
    MockSettingsManager::Settings s = settings.getSettings();
    s.wifiSSID = "MySSID";
    s.wifiPassword = "MyPass";
    settings.setSettings(s);

    std::string json = settings.serializeToJson();

    EXPECT_NE(json.find("\"wifiSSID\": \"MySSID\""), std::string::npos);
    EXPECT_NE(json.find("\"wifiPassword\": \"MyPass\""), std::string::npos);
}

TEST_F(SettingsManagerTest, DeserializeFromJsonUpdatesValues) {
    // Happy path: parse a valid JSON payload.
    std::string json = TestStringUtils::generateValidSettingsJson();
    EXPECT_TRUE(settings.deserializeFromJson(json));
    EXPECT_TRUE(settings.hasUnsavedChanges());
}

TEST_F(SettingsManagerTest, ValidateSettingsFailsForOutOfRangeFreezeThreshold) {
    MockSettingsManager::Settings s = settings.getSettings();
    s.freezeThreshold = 200.0f; // Edge: out of DS18B20 range
    settings.setSettings(s);

    EXPECT_FALSE(settings.validateSettings());
}

TEST_F(SettingsManagerTest, ValidateSettingsFailsForZeroDurations) {
    MockSettingsManager::Settings s = settings.getSettings();
    s.pumpOnDuration = 0; // Edge: invalid boundary
    settings.setSettings(s);

    EXPECT_FALSE(settings.validateSettings());
}

TEST_F(SettingsManagerTest, ValidateSettingsFailsForBrightnessMinGreaterThanMax) {
    MockSettingsManager::Settings s = settings.getSettings();
    s.lightMaxBrightness = 10;
    s.lightMinBrightness = 50; // Edge: invalid order
    settings.setSettings(s);

    EXPECT_FALSE(settings.validateSettings());
}

TEST_F(SettingsManagerTest, GetValidationErrorsContainsExpectedMessages) {
    MockSettingsManager::Settings s = settings.getSettings();
    s.freezeThreshold = 200.0f;
    s.pumpOnDuration = 0;
    settings.setSettings(s);

    std::vector<std::string> errors = settings.getValidationErrors();
    EXPECT_GE(errors.size(), 2u);
}

TEST_F(SettingsManagerTest, BackupAndRestoreReturnTrueInTestMode) {
    settings.setTestMode(true);
    EXPECT_TRUE(settings.createBackup("backup.json"));
    EXPECT_TRUE(settings.restoreBackup("backup.json"));
}

TEST_F(SettingsManagerTest, MarkSavedClearsUnsavedFlag) {
    settings.setSettingString("wifi.ssid", "X");
    ASSERT_TRUE(settings.hasUnsavedChanges());

    settings.markSaved();
    EXPECT_FALSE(settings.hasUnsavedChanges());
}

TEST_F(SettingsManagerTest, ClearAllSettingsRemovesRawValues) {
    settings.setSettingString("wifi.ssid", "SSID");
    ASSERT_EQ(settings.getSettingString("wifi.ssid"), "SSID");

    settings.clearAllSettings();
    EXPECT_EQ(settings.getSettingString("wifi.ssid", "default"), "default");
}

TEST_F(SettingsManagerTest, RawSettingReadWriteRoundTrips) {
    settings.setSettingRaw("raw.key", "raw.value");
    EXPECT_EQ(settings.getSettingRaw("raw.key"), "raw.value");
}
