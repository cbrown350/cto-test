#include "MockAPIRequestQueue.h"
#include <chrono>
#include <sstream>
#include <iomanip>

MockAPIRequestQueue::MockAPIRequestQueue() 
    : lastRetryTime_(std::chrono::steady_clock::now()) {
}

bool MockAPIRequestQueue::enqueueRequest(const std::string& endpoint, const std::string& payload,
                                          APIType apiType, uint32_t maxRetries) {
    if (requests_.size() >= maxQueueSize_) {
        return false; // Queue is full
    }

    APIRequest request;
    request.apiType = apiType;
    request.endpoint = endpoint;
    request.payload = payload;
    request.status = RequestStatus::QUEUED;
    request.createdTime = std::chrono::system_clock::now().time_since_epoch().count();
    request.maxRetries = maxRetries;
    request.retryCount = 0;

    requests_.push(request);
    return true;
}

bool MockAPIRequestQueue::processQueue(bool wifiConnected) {
    setWiFiConnected(wifiConnected);

    if (isQueueEmpty()) {
        return true;
    }

    if (!wifiConnected) {
        return false; // Cannot process without WiFi
    }

    bool allProcessed = true;
    
    while (!requests_.empty()) {
        if (!processSingleRequest(wifiConnected)) {
            allProcessed = false;
            break;
        }
    }

    return allProcessed;
}

MockAPIRequestQueue::APIRequest* MockAPIRequestQueue::peekNextRequest() {
    if (requests_.empty()) {
        return nullptr;
    }

    // Can't modify the queue from peek, so we'll just return nullptr
    // In a real implementation, this would return a pointer to the front
    return nullptr;
}

MockAPIRequestQueue::APIRequest* MockAPIRequestQueue::dequeueRequest() {
    if (requests_.empty()) {
        return nullptr;
    }

    APIRequest request = requests_.front();
    requests_.pop();
    
    return nullptr; // In real implementation, would return pointer
}

void MockAPIRequestQueue::clearHistory() {
    while (!requests_.empty()) {
        requests_.pop();
    }
    while (!failedRequests_.empty()) {
        failedRequests_.pop();
    }
    processedCount_ = 0;
    failedCount_ = 0;
    abandonedCount_ = 0;
}

bool MockAPIRequestQueue::processSingleRequest(bool wifiConnected) {
    if (requests_.empty()) {
        return true;
    }

    APIRequest request = requests_.front();
    requests_.pop();

    if (!wifiConnected && !testMode_) {
        // Put it back in the queue
        requests_.push(request);
        return false;
    }

    request.status = RequestStatus::RETRYING;
    request.retryCount++;
    request.sentTime = std::chrono::system_clock::now().time_since_epoch().count();

    // Try to send using callback if available
    bool success = false;
    if (sendCallback_) {
        success = sendCallback_(request);
    } else {
        // Default mock behavior
        success = testMode_;
    }

    if (success) {
        request.status = RequestStatus::SENT;
        processedCount_++;
    } else {
        // Check if we should retry
        if (request.retryCount < request.maxRetries) {
            request.status = RequestStatus::QUEUED;
            requests_.push(request); // Re-queue for retry
        } else {
            request.status = RequestStatus::FAILED;
            failedCount_++;
            failedRequests_.push(request);
            
            if (failureCallback_) {
                failureCallback_(request, "Max retries exceeded");
            }
        }
    }

    return success;
}

bool MockAPIRequestQueue::shouldRetry(const MockAPIRequestQueue::APIRequest& request) {
    // Check if enough time has passed since last retry
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastRetry = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastRetryTime_);
    
    return timeSinceLastRetry.count() >= static_cast<int32_t>(retryDelayMs_);
}

std::string MockAPIRequestQueue::apiTypeToString(APIType type) const {
    switch (type) {
        case APIType::OPENWEATHER:
            return "OpenWeather";
        case APIType::EMAIL:
            return "Email";
        case APIType::TELEGRAM:
            return "Telegram";
        default:
            return "Unknown";
    }
}

std::string MockAPIRequestQueue::getStats() const {
    std::stringstream ss;
    ss << "Queue Stats:\n";
    ss << "  Queued: " << requests_.size() << "\n";
    ss << "  Processed: " << processedCount_ << "\n";
    ss << "  Failed: " << failedCount_ << "\n";
    ss << "  Abandoned: " << abandonedCount_ << "\n";
    ss << "  Failed Queue: " << failedRequests_.size() << "\n";
    ss << "  WiFi Connected: " << (wifiConnected_ ? "Yes" : "No") << "\n";
    return ss.str();
}
