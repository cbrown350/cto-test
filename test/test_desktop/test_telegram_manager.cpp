#include <gtest/gtest.h>

#include "CommonTestFixture.h"
#include "MockTelegramManager.h"
#include "TestUtils.h"

class TelegramManagerTest : public CommonTestFixture {
protected:
    MockTelegramManager telegramManager;

    void SetUp() override {
        CommonTestFixture::SetUp();
        telegramManager.setTestMode(true);
    }
};

TEST_F(TelegramManagerTest, ConfigureWithValidParametersSucceeds) {
    bool result = telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    EXPECT_TRUE(result);
}

TEST_F(TelegramManagerTest, ConfigureWithInvalidBotTokenFails) {
    bool result = telegramManager.configure("invalid", "123456789");
    EXPECT_FALSE(result);
}

TEST_F(TelegramManagerTest, ConfigureWithInvalidChatIdFails) {
    bool result = telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "");
    EXPECT_FALSE(result);
}

TEST_F(TelegramManagerTest, SetBotTokenWithValidTokenSucceeds) {
    bool result = telegramManager.setBotToken("1234567890:ABCDefGHIJKLmnopqrstuvwxyz");
    EXPECT_TRUE(result);
}

TEST_F(TelegramManagerTest, SetBotTokenWithInvalidTokenFails) {
    bool result = telegramManager.setBotToken("invalid");
    EXPECT_FALSE(result);
}

TEST_F(TelegramManagerTest, SetChatIdWithValidIdSucceeds) {
    bool result = telegramManager.setChatId("123456789");
    EXPECT_TRUE(result);
}

TEST_F(TelegramManagerTest, SetChatIdWithInvalidIdFails) {
    bool result = telegramManager.setChatId("");
    EXPECT_FALSE(result);
}

TEST_F(TelegramManagerTest, SendMessageWithoutConfigurationFails) {
    bool result = telegramManager.sendMessage("Hello, world!");
    EXPECT_FALSE(result);
}

TEST_F(TelegramManagerTest, SendMessageWithoutWiFiConnectivityFails) {
    telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    telegramManager.setTestMode(false);
    telegramManager.setWiFiConnected(false);
    
    bool result = telegramManager.sendMessage("Hello, world!");
    EXPECT_FALSE(result);
    EXPECT_EQ(telegramManager.getStatus(), MockTelegramManager::TelegramStatus::OFFLINE);
}

TEST_F(TelegramManagerTest, SendMessageToSpecificChatSucceeds) {
    telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    
    bool result = telegramManager.sendMessage("987654321", "Hello, specific chat!");
    EXPECT_TRUE(result);
}

TEST_F(TelegramManagerTest, SendAlertMessageSucceeds) {
    telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    
    bool result = telegramManager.sendAlert("System alert!");
    EXPECT_TRUE(result);
}

TEST_F(TelegramManagerTest, SendStatusReportSucceeds) {
    telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    
    bool result = telegramManager.sendStatusReport("System is operational");
    EXPECT_TRUE(result);
}

TEST_F(TelegramManagerTest, ParseStatusCommand) {
    auto cmdType = telegramManager.parseCommand("/status");
    EXPECT_EQ(cmdType, MockTelegramManager::CommandType::STATUS);
}

TEST_F(TelegramManagerTest, ParsePumpOnCommand) {
    auto cmdType = telegramManager.parseCommand("/pump_on");
    EXPECT_EQ(cmdType, MockTelegramManager::CommandType::PUMP_ON);
}

TEST_F(TelegramManagerTest, ParsePumpOffCommand) {
    auto cmdType = telegramManager.parseCommand("/pump_off");
    EXPECT_EQ(cmdType, MockTelegramManager::CommandType::PUMP_OFF);
}

TEST_F(TelegramManagerTest, ParseDoorOpenCommand) {
    auto cmdType = telegramManager.parseCommand("/door_open");
    EXPECT_EQ(cmdType, MockTelegramManager::CommandType::DOOR_OPEN);
}

TEST_F(TelegramManagerTest, ParseDoorCloseCommand) {
    auto cmdType = telegramManager.parseCommand("/door_close");
    EXPECT_EQ(cmdType, MockTelegramManager::CommandType::DOOR_CLOSE);
}

TEST_F(TelegramManagerTest, ParseUnknownCommandReturnsUnknown) {
    auto cmdType = telegramManager.parseCommand("/unknown");
    EXPECT_EQ(cmdType, MockTelegramManager::CommandType::UNKNOWN);
}

TEST_F(TelegramManagerTest, ParseCommandIsCaseInsensitive) {
    auto cmdType1 = telegramManager.parseCommand("/STATUS");
    auto cmdType2 = telegramManager.parseCommand("status");
    EXPECT_EQ(cmdType1, MockTelegramManager::CommandType::STATUS);
    EXPECT_EQ(cmdType2, MockTelegramManager::CommandType::STATUS);
}

TEST_F(TelegramManagerTest, GetCommandResponseForStatusCommand) {
    std::string response = telegramManager.getCommandResponse(
        MockTelegramManager::CommandType::STATUS);
    EXPECT_NE(response.find("operational"), std::string::npos);
}

TEST_F(TelegramManagerTest, GetCommandResponseForPumpOnCommand) {
    std::string response = telegramManager.getCommandResponse(
        MockTelegramManager::CommandType::PUMP_ON);
    EXPECT_NE(response.find("activated"), std::string::npos);
}

TEST_F(TelegramManagerTest, GetCommandResponseForUnknownCommandListsAvailableCommands) {
    std::string response = telegramManager.getCommandResponse(
        MockTelegramManager::CommandType::UNKNOWN);
    EXPECT_NE(response.find("status"), std::string::npos);
    EXPECT_NE(response.find("pump_on"), std::string::npos);
}

TEST_F(TelegramManagerTest, ProcessCommandAddsToHistory) {
    telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    
    telegramManager.processCommand("/status", "123456789");
    
    const auto& history = telegramManager.getMessageHistory();
    EXPECT_EQ(history.size(), 1u);
    EXPECT_EQ(history[0].command, MockTelegramManager::CommandType::STATUS);
}

TEST_F(TelegramManagerTest, ValidateBotTokenWithValidToken) {
    bool valid = MockTelegramManager::validateBotToken("1234567890:ABCDefGHIJKLmnopqrstuvwxyz");
    EXPECT_TRUE(valid);
}

TEST_F(TelegramManagerTest, ValidateBotTokenWithInvalidToken) {
    EXPECT_FALSE(MockTelegramManager::validateBotToken(""));
    EXPECT_FALSE(MockTelegramManager::validateBotToken("short"));
    EXPECT_FALSE(MockTelegramManager::validateBotToken("no_colon_here"));
}

TEST_F(TelegramManagerTest, ValidateChatIdWithValidId) {
    EXPECT_TRUE(MockTelegramManager::validateChatId("123456789"));
    EXPECT_TRUE(MockTelegramManager::validateChatId("-123456789")); // Group ID
}

TEST_F(TelegramManagerTest, ValidateChatIdWithInvalidId) {
    EXPECT_FALSE(MockTelegramManager::validateChatId(""));
    EXPECT_FALSE(MockTelegramManager::validateChatId("not_a_number"));
    EXPECT_FALSE(MockTelegramManager::validateChatId("123abc"));
}

TEST_F(TelegramManagerTest, ValidateConfigurationSucceedsWhenConfigured) {
    telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    EXPECT_TRUE(telegramManager.validateConfiguration());
}

TEST_F(TelegramManagerTest, ValidateConfigurationFailsWhenNotConfigured) {
    EXPECT_FALSE(telegramManager.validateConfiguration());
}

TEST_F(TelegramManagerTest, ClearMessageHistoryRemovesAllMessages) {
    telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    
    telegramManager.processCommand("/status");
    telegramManager.processCommand("/pump_on");
    EXPECT_EQ(telegramManager.getMessageCount(), 2u);
    
    telegramManager.clearMessageHistory();
    EXPECT_EQ(telegramManager.getMessageCount(), 0u);
}

TEST_F(TelegramManagerTest, CustomCommandHandlerIsInvoked) {
    telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    
    bool handlerInvoked = false;
    telegramManager.setCommandHandler([&](MockTelegramManager::CommandType cmd) {
        handlerInvoked = true;
        return "Custom response";
    });
    
    telegramManager.processCommand("/status");
    EXPECT_TRUE(handlerInvoked);
}

TEST_F(TelegramManagerTest, MultipleCommandsCanBeProcessed) {
    telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    
    telegramManager.processCommand("/status");
    telegramManager.processCommand("/pump_on");
    telegramManager.processCommand("/pump_off");
    
    EXPECT_EQ(telegramManager.getMessageCount(), 3u);
}

TEST_F(TelegramManagerTest, StatusChangesOnSend) {
    telegramManager.configure("1234567890:ABCDefGHIJKLmnopqrstuvwxyz", "123456789");
    
    telegramManager.sendMessage("Test message");
    EXPECT_EQ(telegramManager.getStatus(), MockTelegramManager::TelegramStatus::SUCCESS);
}
