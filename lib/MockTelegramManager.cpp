#include "MockTelegramManager.h"
#include <algorithm>
#include <chrono>
#include <cctype>

bool MockTelegramManager::configure(const std::string& botToken, const std::string& chatId) {
    if (!validateBotToken(botToken)) {
        lastError_ = "Invalid bot token format";
        return false;
    }

    if (!validateChatId(chatId)) {
        lastError_ = "Invalid chat ID format";
        return false;
    }

    botToken_ = botToken;
    chatId_ = chatId;
    status_ = TelegramStatus::IDLE;
    lastError_ = "";
    return true;
}

bool MockTelegramManager::setBotToken(const std::string& token) {
    if (!validateBotToken(token)) {
        lastError_ = "Invalid bot token format";
        return false;
    }
    botToken_ = token;
    return true;
}

bool MockTelegramManager::setChatId(const std::string& chatId) {
    if (!validateChatId(chatId)) {
        lastError_ = "Invalid chat ID format";
        return false;
    }
    chatId_ = chatId;
    return true;
}

bool MockTelegramManager::sendMessage(const std::string& message) {
    return sendMessage(chatId_, message);
}

bool MockTelegramManager::sendMessage(const std::string& chatId, const std::string& message) {
    if (!validateConfiguration()) {
        lastError_ = "Telegram not configured";
        status_ = TelegramStatus::FAILED;
        return false;
    }

    if (!testMode_ && !wifiConnected_) {
        lastError_ = "WiFi not connected";
        status_ = TelegramStatus::OFFLINE;
        return false;
    }

    return performSend(message, chatId);
}

bool MockTelegramManager::sendAlert(const std::string& alertMessage) {
    std::string formattedAlert = "🚨 ALERT: " + alertMessage;
    return sendMessage(formattedAlert);
}

bool MockTelegramManager::sendStatusReport(const std::string& statusText) {
    std::string formattedStatus = "📊 STATUS REPORT:\n" + statusText;
    return sendMessage(formattedStatus);
}

bool MockTelegramManager::processCommand(const std::string& command, const std::string& chatId) {
    CommandType cmdType = parseCommand(command);
    
    TelegramMessage msg;
    msg.command = cmdType;
    msg.chatId = chatId.empty() ? chatId_ : chatId;
    msg.text = command;
    msg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    msg.processed = (cmdType != CommandType::UNKNOWN);
    
    if (commandHandler_) {
        msg.response = commandHandler_(cmdType);
    } else {
        msg.response = getCommandResponse(cmdType);
    }
    
    messageHistory_.push_back(msg);
    
    return msg.processed;
}

MockTelegramManager::CommandType MockTelegramManager::parseCommand(const std::string& commandStr) {
    std::string cmd = commandStr;
    // Convert to lowercase
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    
    if (cmd.find("/status") != std::string::npos || cmd.find("status") != std::string::npos) {
        return CommandType::STATUS;
    } else if (cmd.find("/pump_on") != std::string::npos || cmd.find("pump on") != std::string::npos) {
        return CommandType::PUMP_ON;
    } else if (cmd.find("/pump_off") != std::string::npos || cmd.find("pump off") != std::string::npos) {
        return CommandType::PUMP_OFF;
    } else if (cmd.find("/door_open") != std::string::npos || cmd.find("door open") != std::string::npos) {
        return CommandType::DOOR_OPEN;
    } else if (cmd.find("/door_close") != std::string::npos || cmd.find("door close") != std::string::npos) {
        return CommandType::DOOR_CLOSE;
    }
    
    return CommandType::UNKNOWN;
}

std::string MockTelegramManager::getCommandResponse(CommandType command) {
    switch (command) {
        case CommandType::STATUS:
            return "✅ System status: All systems operational\n"
                   "🌡️ Temp: 22.5°C\n"
                   "💧 Pump: OFF\n"
                   "💡 Light: ON";
        case CommandType::PUMP_ON:
            return "✅ Pump activated";
        case CommandType::PUMP_OFF:
            return "✅ Pump deactivated";
        case CommandType::DOOR_OPEN:
            return "✅ Door opened";
        case CommandType::DOOR_CLOSE:
            return "✅ Door closed";
        case CommandType::UNKNOWN:
            return "❌ Unknown command. Available commands:\n"
                   "/status - System status\n"
                   "/pump_on - Activate pump\n"
                   "/pump_off - Deactivate pump\n"
                   "/door_open - Open door\n"
                   "/door_close - Close door";
    }
    return "❌ Unknown command";
}

bool MockTelegramManager::validateConfiguration() const {
    return !botToken_.empty() && !chatId_.empty();
}

bool MockTelegramManager::validateBotToken(const std::string& token) {
    if (token.empty() || token.length() < 10) {
        return false;
    }
    // Telegram bot tokens are typically in format: NNNNNNNNNN:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    size_t colonPos = token.find(':');
    if (colonPos == std::string::npos || colonPos < 5) {
        return false;
    }
    return true;
}

bool MockTelegramManager::validateChatId(const std::string& chatId) {
    if (chatId.empty()) {
        return false;
    }
    // Chat IDs can be positive integers or negative (for groups)
    for (size_t i = 0; i < chatId.length(); ++i) {
        if (!isdigit(chatId[i]) && (i > 0 || chatId[i] != '-')) {
            return false;
        }
    }
    return true;
}

void MockTelegramManager::clearMessageHistory() {
    messageHistory_.clear();
}

bool MockTelegramManager::performSend(const std::string& message, const std::string& targetChatId) {
    status_ = TelegramStatus::CONNECTING;

    std::string actualChatId = targetChatId.empty() ? chatId_ : targetChatId;

    if (testMode_) {
        status_ = TelegramStatus::SUCCESS;
        lastError_ = "";
        
        TelegramMessage msg;
        msg.command = CommandType::UNKNOWN;
        msg.chatId = actualChatId;
        msg.text = message;
        msg.response = "";
        msg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        msg.processed = true;
        
        messageHistory_.push_back(msg);
        return true;
    }

    // Simulate connection failure in non-test mode without WiFi
    status_ = TelegramStatus::FAILED;
    lastError_ = "Simulated send failure (WiFi required)";
    return false;
}

std::string MockTelegramManager::formatMessage(const std::string& message) {
    // Add any message formatting if needed
    return message;
}
