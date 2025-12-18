#include "MockEmailManager.h"
#include <algorithm>
#include <chrono>
#include <cstring>

bool MockEmailManager::configure(const std::string& smtpServer, uint16_t port, bool useTLS,
                                   const std::string& username, const std::string& password,
                                   const std::string& fromAddress) {
    if (smtpServer.empty() || fromAddress.empty()) {
        lastError_ = "SMTP server and from address are required";
        return false;
    }

    if (!validateEmailAddress(fromAddress)) {
        lastError_ = "Invalid from address format";
        return false;
    }

    smtpServer_ = smtpServer;
    smtpPort_ = port;
    useTLS_ = useTLS;
    username_ = username;
    password_ = password;
    fromAddress_ = fromAddress;

    status_ = EmailStatus::IDLE;
    lastError_ = "";
    return true;
}

bool MockEmailManager::addRecipient(const std::string& email) {
    if (!validateEmailAddress(email)) {
        lastError_ = "Invalid email address: " + email;
        return false;
    }

    auto it = std::find(recipients_.begin(), recipients_.end(), email);
    if (it != recipients_.end()) {
        return true; // Already added
    }

    recipients_.push_back(email);
    return true;
}

bool MockEmailManager::removeRecipient(const std::string& email) {
    auto it = std::find(recipients_.begin(), recipients_.end(), email);
    if (it != recipients_.end()) {
        recipients_.erase(it);
        return true;
    }
    return false;
}

bool MockEmailManager::clearRecipients() {
    recipients_.clear();
    return true;
}

bool MockEmailManager::sendEmail(const std::string& toAddress, const std::string& subject, 
                                  const std::string& body) {
    if (!validateEmailAddress(toAddress)) {
        lastError_ = "Invalid recipient email address";
        status_ = EmailStatus::FAILED;
        return false;
    }

    if (!validateConfiguration()) {
        lastError_ = "Email manager not configured";
        status_ = EmailStatus::FAILED;
        return false;
    }

    if (!testMode_ && !wifiConnected_) {
        lastError_ = "WiFi not connected";
        status_ = EmailStatus::OFFLINE;
        return false;
    }

    EmailMessage message;
    message.toAddress = toAddress;
    message.subject = subject;
    message.body = body;
    message.isHtml = false;
    message.sentTimestamp = std::chrono::system_clock::now().time_since_epoch().count();

    return performSend(message);
}

bool MockEmailManager::sendEmailBatch(const std::string& subject, const std::string& body) {
    if (recipients_.empty()) {
        lastError_ = "No recipients configured";
        status_ = EmailStatus::FAILED;
        return false;
    }

    bool allSuccess = true;
    for (const auto& recipient : recipients_) {
        if (!sendEmail(recipient, subject, body)) {
            allSuccess = false;
        }
    }

    return allSuccess;
}

bool MockEmailManager::sendHtmlEmail(const std::string& toAddress, const std::string& subject, 
                                      const std::string& htmlBody) {
    if (!validateEmailAddress(toAddress)) {
        lastError_ = "Invalid recipient email address";
        status_ = EmailStatus::FAILED;
        return false;
    }

    if (!validateConfiguration()) {
        lastError_ = "Email manager not configured";
        status_ = EmailStatus::FAILED;
        return false;
    }

    if (!testMode_ && !wifiConnected_) {
        lastError_ = "WiFi not connected";
        status_ = EmailStatus::OFFLINE;
        return false;
    }

    EmailMessage message;
    message.toAddress = toAddress;
    message.subject = subject;
    message.htmlBody = htmlBody;
    message.isHtml = true;
    message.sentTimestamp = std::chrono::system_clock::now().time_since_epoch().count();

    return performSend(message);
}

bool MockEmailManager::sendAlert(const std::string& alertMessage) {
    if (recipients_.empty()) {
        return sendEmailBatch("ALERT: System Alert", alertMessage);
    }
    return sendEmailBatch("ALERT: System Alert", alertMessage);
}

bool MockEmailManager::sendStatusReport(const std::string& statusText) {
    if (recipients_.empty()) {
        return sendEmailBatch("STATUS REPORT", statusText);
    }
    return sendEmailBatch("STATUS REPORT", statusText);
}

bool MockEmailManager::validateEmailAddress(const std::string& email) {
    if (email.empty() || email.length() > 254) {
        return false;
    }

    size_t atPos = email.find('@');
    if (atPos == std::string::npos || atPos == 0 || atPos == email.length() - 1) {
        return false;
    }

    size_t dotPos = email.find('.', atPos);
    if (dotPos == std::string::npos || dotPos <= atPos + 1 || dotPos == email.length() - 1) {
        return false;
    }

    // Check for valid characters
    for (char c : email) {
        if (!isalnum(c) && c != '@' && c != '.' && c != '-' && c != '_' && c != '+') {
            return false;
        }
    }

    return true;
}

bool MockEmailManager::validateConfiguration() const {
    return !smtpServer_.empty() && !fromAddress_.empty() && 
           smtpPort_ > 0 && !username_.empty() && !password_.empty();
}

void MockEmailManager::clearMessageHistory() {
    sentMessages_.clear();
}

bool MockEmailManager::performSend(const EmailMessage& message) {
    status_ = EmailStatus::CONNECTING;

    // If a custom callback is set, use it
    if (sendCallback_) {
        bool result = sendCallback_(message);
        if (result) {
            status_ = EmailStatus::SUCCESS;
            lastError_ = "";
            sentMessages_.push_back(message);
            return true;
        } else {
            status_ = EmailStatus::FAILED;
            return false;
        }
    }

    // Default mock behavior
    if (testMode_) {
        status_ = EmailStatus::SUCCESS;
        lastError_ = "";
        sentMessages_.push_back(message);
        return true;
    }

    // Simulate connection failure in non-test mode without WiFi
    status_ = EmailStatus::FAILED;
    lastError_ = "Simulated email send failure (WiFi required)";
    return false;
}

std::string MockEmailManager::formatEmailBody(const std::string& body) {
    // Add basic formatting/sanitization
    return body;
}
