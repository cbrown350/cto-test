#ifndef MOCK_TELEGRAM_MANAGER_H
#define MOCK_TELEGRAM_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <cstdint>

class MockTelegramManager {
public:
    enum class CommandType {
        STATUS,
        PUMP_ON,
        PUMP_OFF,
        DOOR_OPEN,
        DOOR_CLOSE,
        UNKNOWN
    };

    struct TelegramMessage {
        CommandType command;
        std::string chatId;
        std::string text;
        std::string response;
        uint64_t timestamp = 0;
        bool processed = false;
    };

    enum class TelegramStatus {
        IDLE,
        CONNECTING,
        CONNECTED,
        SENDING,
        SUCCESS,
        FAILED,
        OFFLINE
    };

    MockTelegramManager() = default;
    virtual ~MockTelegramManager() = default;

    // Configuration
    bool configure(const std::string& botToken, const std::string& chatId);
    bool setBotToken(const std::string& token);
    bool setChatId(const std::string& chatId);
    
    std::string getBotToken() const { return botToken_; }
    std::string getChatId() const { return chatId_; }
    
    // Message sending
    bool sendMessage(const std::string& message);
    bool sendMessage(const std::string& chatId, const std::string& message);
    bool sendAlert(const std::string& alertMessage);
    bool sendStatusReport(const std::string& statusText);
    
    // Command processing
    bool processCommand(const std::string& command, const std::string& chatId = "");
    CommandType parseCommand(const std::string& commandStr);
    std::string getCommandResponse(CommandType command);
    
    // Validation
    bool validateConfiguration() const;
    static bool validateBotToken(const std::string& token);
    static bool validateChatId(const std::string& chatId);
    
    // Status
    TelegramStatus getStatus() const { return status_; }
    std::string getLastError() const { return lastError_; }
    
    // Message history
    const std::vector<TelegramMessage>& getMessageHistory() const { return messageHistory_; }
    void clearMessageHistory();
    uint32_t getMessageCount() const { return messageHistory_.size(); }
    
    // WiFi connection state
    void setWiFiConnected(bool connected) { wifiConnected_ = connected; }
    bool isWiFiConnected() const { return wifiConnected_; }
    
    // Connection settings
    void setMaxRetries(uint32_t maxRetries) { maxRetries_ = maxRetries; }
    void setRetryDelayMs(uint32_t delayMs) { retryDelayMs_ = delayMs; }
    void setRequestTimeoutMs(uint32_t timeoutMs) { requestTimeoutMs_ = timeoutMs; }
    
    // Test mode
    void setTestMode(bool enabled) { testMode_ = enabled; }
    bool isTestMode() const { return testMode_; }
    
    // Command handlers (for testing)
    using CommandHandler = std::function<std::string(CommandType)>;
    void setCommandHandler(CommandHandler handler) { commandHandler_ = handler; }

private:
    std::string botToken_;
    std::string chatId_;
    std::vector<TelegramMessage> messageHistory_;
    
    TelegramStatus status_ = TelegramStatus::IDLE;
    std::string lastError_;
    
    bool wifiConnected_ = false;
    uint32_t maxRetries_ = 3;
    uint32_t retryDelayMs_ = 1000;
    uint32_t requestTimeoutMs_ = 5000;
    bool testMode_ = false;
    
    CommandHandler commandHandler_;
    
    bool performSend(const std::string& message, const std::string& targetChatId = "");
    std::string formatMessage(const std::string& message);
};

#endif // MOCK_TELEGRAM_MANAGER_H
