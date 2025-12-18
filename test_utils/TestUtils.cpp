#include "TestUtils.h"
#include <algorithm>
#include <sstream>
#include <functional>
#include <thread>

// TestTimeUtils implementation
TestTimeUtils::SimulatedTime& TestTimeUtils::getSimulatedTime() {
    static SimulatedTime simulatedTime;
    return simulatedTime;
}

void TestTimeUtils::setCurrentTime(const std::chrono::steady_clock::time_point& time) {
    auto& simTime = getSimulatedTime();
    simTime.current = time;
    if (simTime.start.time_since_epoch().count() == 0) {
        simTime.start = time;
    }
}

std::chrono::steady_clock::time_point TestTimeUtils::getCurrentTime() {
    auto& simTime = getSimulatedTime();
    if (simTime.isPaused) {
        return simTime.current;
    }
    return std::chrono::steady_clock::now();
}

void TestTimeUtils::advanceTime(std::chrono::seconds seconds) {
    auto& simTime = getSimulatedTime();
    simTime.current += seconds;
}

void TestTimeUtils::advanceTime(std::chrono::milliseconds milliseconds) {
    auto& simTime = getSimulatedTime();
    simTime.current += milliseconds;
}

void TestTimeUtils::resetTime() {
    auto& simTime = getSimulatedTime();
    simTime.current = std::chrono::steady_clock::now();
    simTime.start = simTime.current;
    simTime.isPaused = false;
}

bool TestTimeUtils::waitForCondition(std::function<bool()> condition, std::chrono::seconds timeout) {
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + timeout;
    
    while (std::chrono::steady_clock::now() < endTime) {
        if (condition()) {
            return true;
        }
        // In real embedded testing, this would use delaysYield()
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return false;
}

bool TestTimeUtils::waitForCondition(std::function<bool()> condition, std::chrono::milliseconds timeout) {
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + timeout;
    
    while (std::chrono::steady_clock::now() < endTime) {
        if (condition()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    return false;
}

bool TestTimeUtils::isAfter(const std::chrono::steady_clock::time_point& time1, const std::chrono::steady_clock::time_point& time2) {
    return time1 > time2;
}

bool TestTimeUtils::isBefore(const std::chrono::steady_clock::time_point& time1, const std::chrono::steady_clock::time_point& time2) {
    return time1 < time2;
}

std::chrono::milliseconds TestTimeUtils::getDifference(const std::chrono::steady_clock::time_point& time1, const std::chrono::steady_clock::time_point& time2) {
    if (time1 > time2) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(time1 - time2);
    } else {
        return std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time1);
    }
}

// TestTemperatureUtils implementation
std::random_device& TestTemperatureUtils::getRandomDevice() {
    static std::random_device rd;
    return rd;
}

float TestTemperatureUtils::generateRandomTemperature(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

float TestTemperatureUtils::generateRoomTemperature() {
    return generateRandomTemperature(20.0f, 25.0f); // 68-77°F
}

float TestTemperatureUtils::generateFreezingTemperature() {
    return generateRandomTemperature(-5.0f, 2.0f); // 23-36°F
}

float TestTemperatureUtils::generateHotTemperature() {
    return generateRandomTemperature(35.0f, 50.0f); // 95-122°F
}

std::vector<float> TestTemperatureUtils::generateTemperatureSequence(float startTemp, float endTemp, int steps) {
    std::vector<float> sequence;
    if (steps <= 0) return sequence;
    
    float stepSize = (endTemp - startTemp) / steps;
    for (int i = 0; i <= steps; ++i) {
        sequence.push_back(startTemp + (stepSize * i));
    }
    
    return sequence;
}

std::vector<float> TestTemperatureUtils::generateDailyTemperatureCycle(float baseTemp, float variation, int points) {
    std::vector<float> cycle;
    if (points <= 0 || variation < 0) return cycle;
    
    for (int i = 0; i < points; ++i) {
        float progress = static_cast<float>(i) / points;
        // Sine wave temperature variation
        float tempVariation = variation * std::sin(2.0f * M_PI * progress);
        cycle.push_back(baseTemp + tempVariation);
    }
    
    return cycle;
}

float TestTemperatureUtils::celsiusToFahrenheit(float celsius) {
    return (celsius * 9.0f / 5.0f) + 32.0f;
}

float TestTemperatureUtils::fahrenheitToCelsius(float fahrenheit) {
    return (fahrenheit - 32.0f) * 5.0f / 9.0f;
}

bool TestTemperatureUtils::isValidTemperature(float temp) {
    return temp >= -55.0f && temp <= 125.0f; // DS18B20 range
}

bool TestTemperatureUtils::isFreezingTemperature(float temp) {
    return temp <= 0.0f; // 0°C / 32°F
}

bool TestTemperatureUtils::isDangerousTemperature(float temp) {
    return temp < -40.0f || temp > 85.0f; // Extreme temperatures
}

// TestFlowRateUtils implementation
std::random_device& TestFlowRateUtils::getRandomDevice() {
    static std::random_device rd;
    return rd;
}

float TestFlowRateUtils::generateRandomFlowRate(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

float TestFlowRateUtils::generateNoFlow() {
    return 0.0f;
}

float TestFlowRateUtils::generateNormalFlow() {
    return generateRandomFlowRate(0.5f, 3.0f);
}

float TestFlowRateUtils::generateHighFlow() {
    return generateRandomFlowRate(3.0f, 8.0f);
}

float TestFlowRateUtils::generateIntermittentFlow() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::bernoulli_distribution dist(0.3f); // 30% chance of flow
    return dist(gen) ? generateRandomFlowRate(0.1f, 1.0f) : 0.0f;
}

uint32_t TestFlowRateUtils::generatePulseCount(float flowRate, std::chrono::seconds duration, float pulsesPerGallon) {
    if (flowRate <= 0.0f || duration.count() <= 0) {
        return 0;
    }
    
    float flowRateGPM = flowRate;
    float totalGallons = flowRateGPM * (duration.count() / 60.0f);
    return static_cast<uint32_t>(totalGallons * pulsesPerGallon);
}

uint32_t TestFlowRateUtils::generateRandomPulseCount(uint32_t min, uint32_t max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(min, max);
    return dist(gen);
}

std::vector<float> TestFlowRateUtils::generateFlowRateSequence(float startFlow, float endFlow, int steps) {
    std::vector<float> sequence;
    if (steps <= 0) return sequence;
    
    float stepSize = (endFlow - startFlow) / steps;
    for (int i = 0; i <= steps; ++i) {
        sequence.push_back(startFlow + (stepSize * i));
    }
    
    return sequence;
}

std::vector<uint32_t> TestFlowRateUtils::generatePulseSequence(uint32_t startPulses, uint32_t endPulses, int steps) {
    std::vector<uint32_t> sequence;
    if (steps <= 0) return sequence;
    
    uint32_t stepSize = (endPulses - startPulses) / steps;
    for (int i = 0; i <= steps; ++i) {
        sequence.push_back(startPulses + (stepSize * i));
    }
    
    return sequence;
}

bool TestFlowRateUtils::isValidFlowRate(float flowRate) {
    return flowRate >= 0.0f && flowRate <= 20.0f; // Reasonable range
}

bool TestFlowRateUtils::isNormalFlow(float flowRate) {
    return flowRate >= 0.2f && flowRate <= 4.0f;
}

bool TestFlowRateUtils::isExcessiveFlow(float flowRate) {
    return flowRate > 10.0f;
}

// TestAssertUtils implementation
void TestAssertUtils::failTest(const std::string& message) {
    throw std::runtime_error("Test failed: " + message);
}

void TestAssertUtils::assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        failTest(message.empty() ? "Expected true but got false" : message);
    }
}

void TestAssertUtils::assertFalse(bool condition, const std::string& message) {
    if (condition) {
        failTest(message.empty() ? "Expected false but got true" : message);
    }
}

void TestAssertUtils::assertEqual(int expected, int actual, const std::string& message) {
    if (expected != actual) {
        std::ostringstream ss;
        ss << "Expected " << expected << " but got " << actual;
        failTest(message.empty() ? ss.str() : message);
    }
}

void TestAssertUtils::assertEqual(float expected, float actual, float epsilon, const std::string& message) {
    if (std::abs(expected - actual) > epsilon) {
        std::ostringstream ss;
        ss << "Expected " << expected << " but got " << actual << " (epsilon: " << epsilon << ")";
        failTest(message.empty() ? ss.str() : message);
    }
}

void TestAssertUtils::assertEqual(const std::string& expected, const std::string& actual, const std::string& message) {
    if (expected != actual) {
        std::ostringstream ss;
        ss << "Expected '" << expected << "' but got '" << actual << "'";
        failTest(message.empty() ? ss.str() : message);
    }
}

void TestAssertUtils::assertNotEqual(int expected, int actual, const std::string& message) {
    if (expected == actual) {
        std::ostringstream ss;
        ss << "Expected value different from " << expected << " but got same value";
        failTest(message.empty() ? ss.str() : message);
    }
}

void TestAssertUtils::assertNotEqual(float expected, float actual, float epsilon, const std::string& message) {
    if (std::abs(expected - actual) <= epsilon) {
        std::ostringstream ss;
        ss << "Expected value different from " << expected << " but got " << actual;
        failTest(message.empty() ? ss.str() : message);
    }
}

void TestAssertUtils::assertGreaterThan(int expected, int actual, const std::string& message) {
    if (actual <= expected) {
        std::ostringstream ss;
        ss << "Expected " << actual << " > " << expected;
        failTest(message.empty() ? ss.str() : message);
    }
}

void TestAssertUtils::assertLessThan(int expected, int actual, const std::string& message) {
    if (actual >= expected) {
        std::ostringstream ss;
        ss << "Expected " << actual << " < " << expected;
        failTest(message.empty() ? ss.str() : message);
    }
}

void TestAssertUtils::assertGreaterThanOrEqual(int expected, int actual, const std::string& message) {
    if (actual < expected) {
        std::ostringstream ss;
        ss << "Expected " << actual << " >= " << expected;
        failTest(message.empty() ? ss.str() : message);
    }
}

void TestAssertUtils::assertLessThanOrEqual(int expected, int actual, const std::string& message) {
    if (actual > expected) {
        std::ostringstream ss;
        ss << "Expected " << actual << " <= " << expected;
        failTest(message.empty() ? ss.str() : message);
    }
}

void TestAssertUtils::assertNotNull(void* pointer, const std::string& message) {
    if (pointer == nullptr) {
        failTest(message.empty() ? "Expected non-null pointer" : message);
    }
}

void TestAssertUtils::assertNull(void* pointer, const std::string& message) {
    if (pointer != nullptr) {
        failTest(message.empty() ? "Expected null pointer" : message);
    }
}

void TestAssertUtils::assertEmpty(const std::vector<int>& vector, const std::string& message) {
    if (!vector.empty()) {
        std::ostringstream ss;
        ss << "Expected empty vector but got size " << vector.size();
        failTest(message.empty() ? ss.str() : message);
    }
}

void TestAssertUtils::assertNotEmpty(const std::vector<int>& vector, const std::string& message) {
    if (vector.empty()) {
        failTest(message.empty() ? "Expected non-empty vector" : message);
    }
}

void TestAssertUtils::assertSize(const std::vector<int>& vector, size_t expectedSize, const std::string& message) {
    if (vector.size() != expectedSize) {
        std::ostringstream ss;
        ss << "Expected vector size " << expectedSize << " but got " << vector.size();
        failTest(message.empty() ? ss.str() : message);
    }
}

// TestMemoryUtils implementation
void* TestMemoryUtils::testHeapStart = nullptr;
size_t TestMemoryUtils::testHeapSize = 0;
size_t TestMemoryUtils::currentAllocation = 0;

void TestMemoryUtils::initializeTestHeap(size_t heapSize) {
    testHeapStart = malloc(heapSize);
    testHeapSize = heapSize;
    currentAllocation = 0;
}

void TestMemoryUtils::resetTestHeap() {
    if (testHeapStart) {
        free(testHeapStart);
        testHeapStart = nullptr;
    }
    testHeapSize = 0;
    currentAllocation = 0;
}

size_t TestMemoryUtils::getAvailableHeap() {
    return testHeapSize - currentAllocation;
}

size_t TestMemoryUtils::getUsedHeap() {
    return currentAllocation;
}

void TestMemoryUtils::resetMemoryTracking() {
    currentAllocation = 0;
}

size_t TestMemoryUtils::getMemoryAllocated() {
    return currentAllocation;
}

bool TestMemoryUtils::hasMemoryLeaks() {
    return currentAllocation > 0;
}

std::vector<uint8_t> TestMemoryUtils::createTestBuffer(size_t size, uint8_t fillValue) {
    std::vector<uint8_t> buffer(size, fillValue);
    return buffer;
}

void TestMemoryUtils::fillBuffer(std::vector<uint8_t>& buffer, uint8_t fillValue) {
    std::fill(buffer.begin(), buffer.end(), fillValue);
}

bool TestMemoryUtils::compareBuffers(const std::vector<uint8_t>& buffer1, const std::vector<uint8_t>& buffer2) {
    return buffer1 == buffer2;
}

// TestStringUtils implementation
std::random_device& TestStringUtils::getRandomDevice() {
    static std::random_device rd;
    return rd;
}

std::string TestStringUtils::generateRandomString(size_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(gen)];
    }
    return result;
}

std::string TestStringUtils::generateTestSSID() {
    static const char* prefixes[] = {"Test", "Demo", "Mock", "TestNet", "WiFi"};
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> prefixDist(0, 4);
    std::uniform_int_distribution<> suffixDist(100, 999);
    
    return std::string(prefixes[prefixDist(gen)]) + "-" + std::to_string(suffixDist(gen));
}

std::string TestStringUtils::generateTestPassword() {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    std::uniform_int_distribution<> lengthDist(8, 16);
    
    size_t length = lengthDist(gen);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(gen)];
    }
    return result;
}

std::string TestStringUtils::generateTestEmail() {
    static const char* domains[] = {"test.com", "example.org", "demo.net", "mock.edu"};
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> domainDist(0, 3);
    std::string username = generateRandomString(8);
    
    return username + "@" + domains[domainDist(gen)];
}

std::string TestStringUtils::generateValidSettingsJson() {
    return "{"
        "\"pumpEnabled\": true,"
        "\"pumpFreezeThreshold\": 1.1,"
        "\"pumpOnDuration\": 300,"
        "\"pumpOffDuration\": 600,"
        "\"lightEnabled\": true,"
        "\"lightMaxBrightness\": 255,"
        "\"wifiSSID\": \"TestNetwork\","
        "\"wifiPassword\": \"TestPassword123\","
        "\"tempMeterPin\": 32,"
        "\"lightPin\": 25,"
        "\"pulsesPerGallon\": 1000"
        "}";
}

std::string TestStringUtils::generateInvalidSettingsJson() {
    return "{"
        "\"pumpEnabled\": true,"
        "\"pumpFreezeThreshold\": 999,"  // Invalid temperature
        "\"pumpOnDuration\": 0,"         // Invalid duration
        "\"lightMaxBrightness\": 999"    // Invalid brightness
        "}";
}

std::string TestStringUtils::generateStatusResponseJson() {
    return "{"
        "\"timestamp\": 1234567890,"
        "\"temperature\": 21.5,"
        "\"pumpActive\": false,"
        "\"lightActive\": true,"
        "\"wifiConnected\": true,"
        "\"uptime\": 3600"
        "}";
}

std::string TestStringUtils::generateTestURL() {
    static const char* hosts[] = {"localhost", "192.168.1.100", "esp32.local", "test.example.com"};
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> hostDist(0, 3);
    std::uniform_int_distribution<> portDist(80, 8080);
    
    return "http://" + std::string(hosts[hostDist(gen)]) + ":" + std::to_string(portDist(gen));
}

std::string TestStringUtils::generateTestPath(size_t depth) {
    static const char* segments[] = {"api", "status", "settings", "control", "data", "test", "mock"};
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> segmentDist(0, 6);
    std::uniform_int_distribution<> depthDist(1, 3);
    
    size_t actualDepth = (depth == 1) ? depthDist(gen) : depth;
    
    std::string path;
    for (size_t i = 0; i < actualDepth; ++i) {
        if (i > 0) path += "/";
        path += segments[segmentDist(gen)];
    }
    
    return "/" + path;
}