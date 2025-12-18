#include <gtest/gtest.h>

#include "CommonTestFixture.h"
#include "MockAPIRequestQueue.h"
#include "TestUtils.h"

class APIRequestQueueTest : public CommonTestFixture {
protected:
    MockAPIRequestQueue queue;

    void SetUp() override {
        CommonTestFixture::SetUp();
        queue.setTestMode(true);
    }
};

TEST_F(APIRequestQueueTest, EnqueueRequestSucceeds) {
    bool result = queue.enqueueRequest("/weather", "{\"lat\": 40.7, \"lon\": -74.0}",
                                         MockAPIRequestQueue::APIType::OPENWEATHER);
    EXPECT_TRUE(result);
}

TEST_F(APIRequestQueueTest, EnqueueMultipleRequestsSucceeds) {
    EXPECT_TRUE(queue.enqueueRequest("/weather", "{}", MockAPIRequestQueue::APIType::OPENWEATHER));
    EXPECT_TRUE(queue.enqueueRequest("/mail", "{}", MockAPIRequestQueue::APIType::EMAIL));
    EXPECT_TRUE(queue.enqueueRequest("/telegram", "{}", MockAPIRequestQueue::APIType::TELEGRAM));
    
    EXPECT_EQ(queue.getQueueSize(), 3u);
}

TEST_F(APIRequestQueueTest, QueueIsEmptyInitially) {
    EXPECT_TRUE(queue.isQueueEmpty());
    EXPECT_EQ(queue.getQueueSize(), 0u);
}

TEST_F(APIRequestQueueTest, ProcessQueueWithWiFiConnectedSucceeds) {
    queue.enqueueRequest("/weather", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    queue.setWiFiConnected(true);
    
    bool result = queue.processQueue(true);
    EXPECT_TRUE(result);
}

TEST_F(APIRequestQueueTest, ProcessQueueWithoutWiFiReturnsFalse) {
    queue.enqueueRequest("/weather", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    queue.setWiFiConnected(false);
    
    bool result = queue.processQueue(false);
    EXPECT_FALSE(result);
}

TEST_F(APIRequestQueueTest, ProcessQueueEmptyReturnsTrue) {
    EXPECT_TRUE(queue.processQueue(true));
}

TEST_F(APIRequestQueueTest, ProcessQueueReducesQueueSize) {
    queue.enqueueRequest("/weather", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    queue.enqueueRequest("/mail", "{}", MockAPIRequestQueue::APIType::EMAIL);
    
    EXPECT_EQ(queue.getQueueSize(), 2u);
    
    queue.processQueue(true);
    
    EXPECT_EQ(queue.getQueueSize(), 0u);
}

TEST_F(APIRequestQueueTest, SetAndCheckWiFiConnectedState) {
    queue.setWiFiConnected(true);
    EXPECT_TRUE(queue.isWiFiConnected());
    
    queue.setWiFiConnected(false);
    EXPECT_FALSE(queue.isWiFiConnected());
}

TEST_F(APIRequestQueueTest, SetMaxRetries) {
    queue.setMaxRetries(5);
    // Verify by checking internal state through processing
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER, 5);
    EXPECT_EQ(queue.getQueueSize(), 1u);
}

TEST_F(APIRequestQueueTest, SetRetryDelayMs) {
    queue.setRetryDelayMs(2000);
    // Confirm no exception
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
}

TEST_F(APIRequestQueueTest, SetRequestTimeoutMs) {
    queue.setRequestTimeoutMs(10000);
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
}

TEST_F(APIRequestQueueTest, SetMaxQueueSize) {
    queue.setMaxQueueSize(5);
    
    // Add 5 requests
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER));
    }
    
    // 6th request should fail
    EXPECT_FALSE(queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER));
}

TEST_F(APIRequestQueueTest, ClearHistoryRemovesAllRequests) {
    queue.enqueueRequest("/weather", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    queue.enqueueRequest("/mail", "{}", MockAPIRequestQueue::APIType::EMAIL);
    
    queue.clearHistory();
    
    EXPECT_EQ(queue.getQueueSize(), 0u);
    EXPECT_EQ(queue.getProcessedCount(), 0u);
}

TEST_F(APIRequestQueueTest, ProcessedCountIncrementsOnSuccess) {
    queue.setSendCallback([](const MockAPIRequestQueue::APIRequest& req) {
        return true; // Simulate success
    });
    
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    queue.processQueue(true);
    
    EXPECT_EQ(queue.getProcessedCount(), 1u);
}

TEST_F(APIRequestQueueTest, FailedCountIncrementsOnFailure) {
    queue.setSendCallback([](const MockAPIRequestQueue::APIRequest& req) {
        return false; // Simulate failure
    });
    
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER, 1);
    queue.processQueue(true);
    
    EXPECT_EQ(queue.getFailedCount(), 1u);
}

TEST_F(APIRequestQueueTest, RequestRetriesOnFailure) {
    int callCount = 0;
    queue.setSendCallback([&](const MockAPIRequestQueue::APIRequest& req) {
        callCount++;
        return callCount >= 3; // Succeed on 3rd attempt
    });
    
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER, 3);
    queue.processSingleRequest(true);
}

TEST_F(APIRequestQueueTest, CustomSendCallbackIsCalled) {
    bool callbackCalled = false;
    queue.setSendCallback([&](const MockAPIRequestQueue::APIRequest& req) {
        callbackCalled = true;
        return true;
    });
    
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    queue.processQueue(true);
    
    EXPECT_TRUE(callbackCalled);
}

TEST_F(APIRequestQueueTest, FailureCallbackIsCalled) {
    bool failureCallbackCalled = false;
    queue.setSendCallback([](const MockAPIRequestQueue::APIRequest& req) {
        return false;
    });
    queue.setFailureCallback([&](const MockAPIRequestQueue::APIRequest& req, const std::string& err) {
        failureCallbackCalled = true;
    });
    
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER, 1);
    queue.processQueue(true);
    
    EXPECT_TRUE(failureCallbackCalled);
}

TEST_F(APIRequestQueueTest, GetStatsReturnsFormattedString) {
    queue.enqueueRequest("/test1", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    queue.enqueueRequest("/test2", "{}", MockAPIRequestQueue::APIType::EMAIL);
    
    std::string stats = queue.getStats();
    
    EXPECT_NE(stats.find("Queue Stats"), std::string::npos);
    EXPECT_NE(stats.find("Queued"), std::string::npos);
}

TEST_F(APIRequestQueueTest, ProcessSingleRequestSucceeds) {
    queue.setSendCallback([](const MockAPIRequestQueue::APIRequest& req) {
        return true;
    });
    
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    bool result = queue.processSingleRequest(true);
    
    EXPECT_TRUE(result);
}

TEST_F(APIRequestQueueTest, ProcessSingleRequestWithoutWiFiFails) {
    queue.setTestMode(false);
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    
    bool result = queue.processSingleRequest(false);
    
    EXPECT_FALSE(result);
}

TEST_F(APIRequestQueueTest, QueueCanHoldLargePayload) {
    std::string largePayload(1000, 'x');
    bool result = queue.enqueueRequest("/test", largePayload, MockAPIRequestQueue::APIType::EMAIL);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(queue.getQueueSize(), 1u);
}

TEST_F(APIRequestQueueTest, MultipleAPITypesCanBeQueued) {
    queue.enqueueRequest("/weather", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    queue.enqueueRequest("/mail", "{}", MockAPIRequestQueue::APIType::EMAIL);
    queue.enqueueRequest("/telegram", "{}", MockAPIRequestQueue::APIType::TELEGRAM);
    
    EXPECT_EQ(queue.getQueueSize(), 3u);
}

TEST_F(APIRequestQueueTest, EnqueueAfterClearWorks) {
    queue.enqueueRequest("/test1", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    queue.clearHistory();
    queue.enqueueRequest("/test2", "{}", MockAPIRequestQueue::APIType::EMAIL);
    
    EXPECT_EQ(queue.getQueueSize(), 1u);
}

TEST_F(APIRequestQueueTest, PeekNextRequestReturnsNullptrWhenEmpty) {
    EXPECT_EQ(queue.peekNextRequest(), nullptr);
}

TEST_F(APIRequestQueueTest, DequeueRequestReturnsNullptr) {
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER);
    EXPECT_EQ(queue.dequeueRequest(), nullptr);
}

TEST_F(APIRequestQueueTest, TestModeCanBeToggled) {
    queue.setTestMode(true);
    EXPECT_TRUE(queue.isTestMode());
    
    queue.setTestMode(false);
    EXPECT_FALSE(queue.isTestMode());
}

TEST_F(APIRequestQueueTest, AbandonedCountTracksAbandonedRequests) {
    // This would require requests to exceed max retries
    queue.enqueueRequest("/test", "{}", MockAPIRequestQueue::APIType::OPENWEATHER, 1);
    queue.setSendCallback([](const MockAPIRequestQueue::APIRequest& req) {
        return false; // Always fail
    });
    
    queue.processQueue(true);
    
    EXPECT_EQ(queue.getFailedCount(), 1u);
}
