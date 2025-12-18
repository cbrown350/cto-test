#ifndef MOCK_EMAIL_MANAGER_H
#define MOCK_EMAIL_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

class MockEmailManager {
public:
    struct EmailMessage {
        std::string toAddress;
        std::string subject;
        std::string body;
        std::string htmlBody;
        bool isHtml = false;
        uint64_t sentTimestamp = 0;
    };

    enum class EmailStatus {
        IDLE,
        CONNECTING,
        SENDING,
        SUCCESS,
        FAILED,
        OFFLINE
    };

    MockEmailManager() = default;
    virtual ~MockEmailManager() = default;

    // Configuration
    bool configure(const std::string& smtpServer, uint16_t port, bool useTLS,
                   const std::string& username, const std::string& password,
                   const std::string& fromAddress);
    
    bool addRecipient(const std::string& email);
    bool removeRecipient(const std::string& email);
    std::vector<std::string> getRecipients() const { return recipients_; }
    bool clearRecipients();
    
    // Email sending
    bool sendEmail(const std::string& toAddress, const std::string& subject, const std::string& body);
    bool sendEmailBatch(const std::string& subject, const std::string& body);
    bool sendHtmlEmail(const std::string& toAddress, const std::string& subject, 
                       const std::string& htmlBody);
    bool sendAlert(const std::string& alertMessage);
    bool sendStatusReport(const std::string& statusText);
    
    // Validation
    static bool validateEmailAddress(const std::string& email);
    bool validateConfiguration() const;
    
    // Status
    EmailStatus getStatus() const { return status_; }
    std::string getLastError() const { return lastError_; }
    
    // Message history
    const std::vector<EmailMessage>& getSentMessages() const { return sentMessages_; }
    void clearMessageHistory();
    uint32_t getSentMessageCount() const { return sentMessages_.size(); }
    
    // WiFi connection state
    void setWiFiConnected(bool connected) { wifiConnected_ = connected; }
    bool isWiFiConnected() const { return wifiConnected_; }
    
    // Connection retry settings
    void setMaxRetries(uint32_t maxRetries) { maxRetries_ = maxRetries; }
    void setRetryDelayMs(uint32_t delayMs) { retryDelayMs_ = delayMs; }
    
    // Test mode
    void setTestMode(bool enabled) { testMode_ = enabled; }
    bool isTestMode() const { return testMode_; }
    
    // Callback for testing
    using SendCallback = std::function<bool(const EmailMessage&)>;
    void setSendCallback(SendCallback callback) { sendCallback_ = callback; }

private:
    std::string smtpServer_;
    uint16_t smtpPort_ = 587;
    bool useTLS_ = true;
    std::string username_;
    std::string password_;
    std::string fromAddress_;
    
    std::vector<std::string> recipients_;
    std::vector<EmailMessage> sentMessages_;
    
    EmailStatus status_ = EmailStatus::IDLE;
    std::string lastError_;
    
    bool wifiConnected_ = false;
    uint32_t maxRetries_ = 3;
    uint32_t retryDelayMs_ = 1000;
    bool testMode_ = false;
    
    SendCallback sendCallback_;
    
    bool performSend(const EmailMessage& message);
    std::string formatEmailBody(const std::string& body);
};

#endif // MOCK_EMAIL_MANAGER_H
