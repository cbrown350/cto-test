#ifndef MOCK_API_REQUEST_QUEUE_H
#define MOCK_API_REQUEST_QUEUE_H

#include <string>
#include <queue>
#include <memory>
#include <functional>
#include <chrono>
#include <cstdint>

class MockAPIRequestQueue {
public:
    enum class APIType {
        OPENWEATHER,
        EMAIL,
        TELEGRAM,
        UNKNOWN
    };

    enum class RequestStatus {
        QUEUED,
        RETRYING,
        SENT,
        FAILED,
        ABANDONED
    };

    struct APIRequest {
        APIType apiType;
        std::string endpoint;
        std::string payload;
        RequestStatus status = RequestStatus::QUEUED;
        uint64_t createdTime = 0;
        uint64_t sentTime = 0;
        uint32_t retryCount = 0;
        uint32_t maxRetries = 3;
        std::string error;
    };

    MockAPIRequestQueue();
    virtual ~MockAPIRequestQueue() = default;

    // Queue management
    bool enqueueRequest(const std::string& endpoint, const std::string& payload, 
                        APIType apiType, uint32_t maxRetries = 3);
    
    bool processQueue(bool wifiConnected);
    
    APIRequest* peekNextRequest();
    APIRequest* dequeueRequest();
    
    uint32_t getQueueSize() const { return requests_.size(); }
    bool isQueueEmpty() const { return requests_.empty(); }
    
    // WiFi connection state
    void setWiFiConnected(bool connected) { wifiConnected_ = connected; }
    bool isWiFiConnected() const { return wifiConnected_; }
    
    // Retry configuration
    void setMaxRetries(uint32_t maxRetries) { maxRetries_ = maxRetries; }
    void setRetryDelayMs(uint32_t delayMs) { retryDelayMs_ = delayMs; }
    void setRequestTimeoutMs(uint32_t timeoutMs) { requestTimeoutMs_ = timeoutMs; }
    void setMaxQueueSize(uint32_t maxSize) { maxQueueSize_ = maxSize; }
    
    // Request history
    void clearHistory();
    uint32_t getProcessedCount() const { return processedCount_; }
    uint32_t getFailedCount() const { return failedCount_; }
    uint32_t getAbandonedCount() const { return abandonedCount_; }
    
    // Callbacks
    using SendCallback = std::function<bool(const APIRequest&)>;
    void setSendCallback(SendCallback callback) { sendCallback_ = callback; }
    
    using FailureCallback = std::function<void(const APIRequest&, const std::string&)>;
    void setFailureCallback(FailureCallback callback) { failureCallback_ = callback; }
    
    // Test utilities
    void setTestMode(bool enabled) { testMode_ = enabled; }
    bool isTestMode() const { return testMode_; }
    
    // Statistics
    std::string getStats() const;
    
    // Manual processing for testing
    bool processSingleRequest(bool wifiConnected);

private:
    std::queue<APIRequest> requests_;
    std::queue<APIRequest> failedRequests_;
    
    bool wifiConnected_ = false;
    uint32_t maxRetries_ = 3;
    uint32_t retryDelayMs_ = 1000;
    uint32_t requestTimeoutMs_ = 5000;
    uint32_t maxQueueSize_ = 100;
    
    uint32_t processedCount_ = 0;
    uint32_t failedCount_ = 0;
    uint32_t abandonedCount_ = 0;
    
    bool testMode_ = false;
    
    SendCallback sendCallback_;
    FailureCallback failureCallback_;
    
    std::chrono::steady_clock::time_point lastRetryTime_;
    
    bool shouldRetry(const APIRequest& request);
    std::string apiTypeToString(APIType type) const;
};

#endif // MOCK_API_REQUEST_QUEUE_H
