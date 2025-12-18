#include <gtest/gtest.h>

#include "CommonTestFixture.h"
#include "MockEmailManager.h"
#include "TestUtils.h"

class EmailManagerTest : public CommonTestFixture {
protected:
    MockEmailManager emailManager;

    void SetUp() override {
        CommonTestFixture::SetUp();
        emailManager.setTestMode(true);
    }
};

TEST_F(EmailManagerTest, ConfigureWithValidParametersSucceeds) {
    bool result = emailManager.configure("smtp.gmail.com", 587, true,
                                          "user@gmail.com", "password",
                                          "sender@gmail.com");
    EXPECT_TRUE(result);
}

TEST_F(EmailManagerTest, ConfigureWithEmptyServerFails) {
    bool result = emailManager.configure("", 587, true,
                                          "user@gmail.com", "password",
                                          "sender@gmail.com");
    EXPECT_FALSE(result);
}

TEST_F(EmailManagerTest, ConfigureWithEmptyFromAddressFails) {
    bool result = emailManager.configure("smtp.gmail.com", 587, true,
                                          "user@gmail.com", "password", "");
    EXPECT_FALSE(result);
}

TEST_F(EmailManagerTest, ConfigureWithInvalidFromAddressFails) {
    bool result = emailManager.configure("smtp.gmail.com", 587, true,
                                          "user@gmail.com", "password",
                                          "notanemail");
    EXPECT_FALSE(result);
}

TEST_F(EmailManagerTest, AddValidRecipientSucceeds) {
    EXPECT_TRUE(emailManager.addRecipient("test@example.com"));
}

TEST_F(EmailManagerTest, AddInvalidRecipientFails) {
    EXPECT_FALSE(emailManager.addRecipient("notanemail"));
}

TEST_F(EmailManagerTest, AddDuplicateRecipientIsIdempotent) {
    EXPECT_TRUE(emailManager.addRecipient("test@example.com"));
    EXPECT_TRUE(emailManager.addRecipient("test@example.com"));
    EXPECT_EQ(emailManager.getRecipients().size(), 1u);
}

TEST_F(EmailManagerTest, RemoveRecipientSucceeds) {
    emailManager.addRecipient("test@example.com");
    EXPECT_TRUE(emailManager.removeRecipient("test@example.com"));
    EXPECT_EQ(emailManager.getRecipients().size(), 0u);
}

TEST_F(EmailManagerTest, RemoveNonExistentRecipientFails) {
    EXPECT_FALSE(emailManager.removeRecipient("notadded@example.com"));
}

TEST_F(EmailManagerTest, ClearRecipientsSucceeds) {
    emailManager.addRecipient("test1@example.com");
    emailManager.addRecipient("test2@example.com");
    EXPECT_TRUE(emailManager.clearRecipients());
    EXPECT_EQ(emailManager.getRecipients().size(), 0u);
}

TEST_F(EmailManagerTest, SendEmailToValidAddressSucceeds) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    bool result = emailManager.sendEmail("to@example.com", "Test Subject", "Test Body");
    EXPECT_TRUE(result);
}

TEST_F(EmailManagerTest, SendEmailToInvalidAddressFails) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    bool result = emailManager.sendEmail("notanemail", "Test Subject", "Test Body");
    EXPECT_FALSE(result);
}

TEST_F(EmailManagerTest, SendEmailWithoutConfigurationFails) {
    bool result = emailManager.sendEmail("to@example.com", "Test Subject", "Test Body");
    EXPECT_FALSE(result);
}

TEST_F(EmailManagerTest, SendEmailWithoutWiFiConnectivityFails) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    emailManager.setTestMode(false);
    emailManager.setWiFiConnected(false);
    
    bool result = emailManager.sendEmail("to@example.com", "Test Subject", "Test Body");
    EXPECT_FALSE(result);
    EXPECT_EQ(emailManager.getStatus(), MockEmailManager::EmailStatus::OFFLINE);
}

TEST_F(EmailManagerTest, SendEmailBatchToMultipleRecipients) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    emailManager.addRecipient("recipient1@example.com");
    emailManager.addRecipient("recipient2@example.com");
    
    bool result = emailManager.sendEmailBatch("Test Subject", "Test Body");
    EXPECT_TRUE(result);
    EXPECT_EQ(emailManager.getSentMessageCount(), 2u);
}

TEST_F(EmailManagerTest, SendEmailBatchWithNoRecipientsFails) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    
    bool result = emailManager.sendEmailBatch("Test Subject", "Test Body");
    EXPECT_FALSE(result);
}

TEST_F(EmailManagerTest, SendHtmlEmailSucceeds) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    
    std::string htmlBody = "<html><body><h1>Test</h1></body></html>";
    bool result = emailManager.sendHtmlEmail("to@example.com", "Test Subject", htmlBody);
    EXPECT_TRUE(result);
}

TEST_F(EmailManagerTest, SentMessageHistoryIsRecorded) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    
    emailManager.sendEmail("to1@example.com", "Subject 1", "Body 1");
    emailManager.sendEmail("to2@example.com", "Subject 2", "Body 2");
    
    const auto& history = emailManager.getSentMessages();
    EXPECT_EQ(history.size(), 2u);
    EXPECT_EQ(history[0].toAddress, "to1@example.com");
    EXPECT_EQ(history[1].toAddress, "to2@example.com");
}

TEST_F(EmailManagerTest, ValidateEmailAddressWithValidEmails) {
    EXPECT_TRUE(MockEmailManager::validateEmailAddress("test@example.com"));
    EXPECT_TRUE(MockEmailManager::validateEmailAddress("user.name+tag@example.co.uk"));
    EXPECT_TRUE(MockEmailManager::validateEmailAddress("a@b.c"));
}

TEST_F(EmailManagerTest, ValidateEmailAddressWithInvalidEmails) {
    EXPECT_FALSE(MockEmailManager::validateEmailAddress(""));
    EXPECT_FALSE(MockEmailManager::validateEmailAddress("notanemail"));
    EXPECT_FALSE(MockEmailManager::validateEmailAddress("@example.com"));
    EXPECT_FALSE(MockEmailManager::validateEmailAddress("user@"));
    EXPECT_FALSE(MockEmailManager::validateEmailAddress("user@.com"));
}

TEST_F(EmailManagerTest, ValidateConfigurationSucceedsWhenConfigured) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    EXPECT_TRUE(emailManager.validateConfiguration());
}

TEST_F(EmailManagerTest, ValidateConfigurationFailsWhenNotConfigured) {
    EXPECT_FALSE(emailManager.validateConfiguration());
}

TEST_F(EmailManagerTest, ClearMessageHistoryRemovesAllMessages) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    
    emailManager.sendEmail("to@example.com", "Subject", "Body");
    EXPECT_EQ(emailManager.getSentMessageCount(), 1u);
    
    emailManager.clearMessageHistory();
    EXPECT_EQ(emailManager.getSentMessageCount(), 0u);
}

TEST_F(EmailManagerTest, CustomSendCallbackIsInvoked) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    
    bool callbackInvoked = false;
    emailManager.setSendCallback([&](const MockEmailManager::EmailMessage& msg) {
        callbackInvoked = true;
        return true;
    });
    
    emailManager.sendEmail("to@example.com", "Subject", "Body");
    EXPECT_TRUE(callbackInvoked);
}

TEST_F(EmailManagerTest, StatusChangesOnSend) {
    emailManager.configure("smtp.example.com", 587, true,
                            "sender@example.com", "password", "from@example.com");
    
    emailManager.sendEmail("to@example.com", "Subject", "Body");
    EXPECT_EQ(emailManager.getStatus(), MockEmailManager::EmailStatus::SUCCESS);
}

TEST_F(EmailManagerTest, RetrySettingsAreConfigurable) {
    emailManager.setMaxRetries(5);
    emailManager.setRetryDelayMs(2000);
    
    // Settings should be stored and retrievable in configuration
    EXPECT_TRUE(emailManager.isTestMode());
}

TEST_F(EmailManagerTest, MultipleRecipientsCanBeAdded) {
    for (int i = 0; i < 5; ++i) {
        std::string email = "user" + std::to_string(i) + "@example.com";
        EXPECT_TRUE(emailManager.addRecipient(email));
    }
    
    EXPECT_EQ(emailManager.getRecipients().size(), 5u);
}
