#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <chrono>
#include <random>
#include <cmath>
#include <vector>
#include <string>
#include <functional>

class TestTimeUtils {
public:
    // Time simulation for testing
    struct SimulatedTime {
        std::chrono::steady_clock::time_point current;
        std::chrono::steady_clock::time_point start;
        bool isPaused = false;
    };

    // Time control methods
    static void setCurrentTime(const std::chrono::steady_clock::time_point& time);
    static std::chrono::steady_clock::time_point getCurrentTime();
    static void advanceTime(std::chrono::seconds seconds);
    static void advanceTime(std::chrono::milliseconds milliseconds);
    static void resetTime();
    
    // Time-based test helpers
    static bool waitForCondition(std::function<bool()> condition, std::chrono::seconds timeout);
    static bool waitForCondition(std::function<bool()> condition, std::chrono::milliseconds timeout);
    
    // Timestamp comparison
    static bool isAfter(const std::chrono::steady_clock::time_point& time1, const std::chrono::steady_clock::time_point& time2);
    static bool isBefore(const std::chrono::steady_clock::time_point& time1, const std::chrono::steady_clock::time_point& time2);
    static std::chrono::milliseconds getDifference(const std::chrono::steady_clock::time_point& time1, const std::chrono::steady_clock::time_point& time2);

private:
    static SimulatedTime& getSimulatedTime();
};

class TestTemperatureUtils {
public:
    // Temperature value generation
    static float generateRandomTemperature(float min = -55.0f, float max = 125.0f);
    static float generateRoomTemperature();
    static float generateFreezingTemperature();
    static float generateHotTemperature();
    
    // Temperature sequences
    static std::vector<float> generateTemperatureSequence(float startTemp, float endTemp, int steps);
    static std::vector<float> generateDailyTemperatureCycle(float baseTemp, float variation, int points);
    
    // Temperature conversion helpers
    static float celsiusToFahrenheit(float celsius);
    static float fahrenheitToCelsius(float fahrenheit);
    
    // Temperature validation
    static bool isValidTemperature(float temp);
    static bool isFreezingTemperature(float temp); // 32°F / 0°C
    static bool isDangerousTemperature(float temp); // extreme temps

private:
    static std::random_device& getRandomDevice();
};

class TestFlowRateUtils {
public:
    // Flow rate generation
    static float generateRandomFlowRate(float min = 0.0f, float max = 10.0f);
    static float generateNoFlow();
    static float generateNormalFlow();
    static float generateHighFlow();
    static float generateIntermittentFlow();
    
    // Pulse generation
    static uint32_t generatePulseCount(float flowRate, std::chrono::seconds duration, float pulsesPerGallon = 1000.0f);
    static uint32_t generateRandomPulseCount(uint32_t min = 0, uint32_t max = 10000);
    
    // Flow rate sequences
    static std::vector<float> generateFlowRateSequence(float startFlow, float endFlow, int steps);
    static std::vector<uint32_t> generatePulseSequence(uint32_t startPulses, uint32_t endPulses, int steps);
    
    // Flow validation
    static bool isValidFlowRate(float flowRate);
    static bool isNormalFlow(float flowRate);
    static bool isExcessiveFlow(float flowRate);

private:
    static std::random_device& getRandomDevice();
};

class TestAssertUtils {
public:
    // Custom assertions for embedded testing
    static void assertTrue(bool condition, const std::string& message = "");
    static void assertFalse(bool condition, const std::string& message = "");
    static void assertEqual(int expected, int actual, const std::string& message = "");
    static void assertEqual(float expected, float actual, float epsilon = 0.001f, const std::string& message = "");
    static void assertEqual(const std::string& expected, const std::string& actual, const std::string& message = "");
    static void assertNotEqual(int expected, int actual, const std::string& message = "");
    static void assertNotEqual(float expected, float actual, float epsilon = 0.001f, const std::string& message = "");
    static void assertGreaterThan(int expected, int actual, const std::string& message = "");
    static void assertLessThan(int expected, int actual, const std::string& message = "");
    static void assertGreaterThanOrEqual(int expected, int actual, const std::string& message = "");
    static void assertLessThanOrEqual(int expected, int actual, const std::string& message = "");
    
    // Pointer assertions
    static void assertNotNull(void* pointer, const std::string& message = "");
    static void assertNull(void* pointer, const std::string& message = "");
    
    // Collection assertions
    static void assertEmpty(const std::vector<int>& vector, const std::string& message = "");
    static void assertNotEmpty(const std::vector<int>& vector, const std::string& message = "");
    static void assertSize(const std::vector<int>& vector, size_t expectedSize, const std::string& message = "");

private:
    static void failTest(const std::string& message);
};

class TestMemoryUtils {
public:
    // Memory management for testing
    static void initializeTestHeap(size_t heapSize);
    static void resetTestHeap();
    static size_t getAvailableHeap();
    static size_t getUsedHeap();
    
    // Memory leak detection
    static void resetMemoryTracking();
    static size_t getMemoryAllocated();
    static bool hasMemoryLeaks();
    
    // Buffer management
    static std::vector<uint8_t> createTestBuffer(size_t size, uint8_t fillValue = 0x00);
    static void fillBuffer(std::vector<uint8_t>& buffer, uint8_t fillValue = 0x00);
    static bool compareBuffers(const std::vector<uint8_t>& buffer1, const std::vector<uint8_t>& buffer2);

private:
    static void* testHeapStart;
    static size_t testHeapSize;
    static size_t currentAllocation;
};

class TestStringUtils {
public:
    // String generation for testing
    static std::string generateRandomString(size_t length);
    static std::string generateTestSSID();
    static std::string generateTestPassword();
    static std::string generateTestEmail();
    
    // JSON test data
    static std::string generateValidSettingsJson();
    static std::string generateInvalidSettingsJson();
    static std::string generateStatusResponseJson();
    
    // URL and path generation
    static std::string generateTestURL();
    static std::string generateTestPath(size_t depth = 1);

private:
    static std::random_device& getRandomDevice();
};

// Utility macro for test assertions
#ifdef UNIT_TEST
    #define TEST_ASSERT_TRUE(condition) TestAssertUtils::assertTrue(condition, "Assertion failed: " #condition)
    #define TEST_ASSERT_FALSE(condition) TestAssertUtils::assertFalse(condition, "Assertion failed: " #condition)
    #define TEST_ASSERT_EQUAL(expected, actual) TestAssertUtils::assertEqual(expected, actual, "Assertion failed: " #expected " == " #actual)
    #define TEST_ASSERT_EQUAL_FLOAT(expected, actual, epsilon) TestAssertUtils::assertEqual(expected, actual, epsilon, "Assertion failed: " #expected " == " #actual)
    #define TEST_ASSERT_EQUAL_STRING(expected, actual) TestAssertUtils::assertEqual(expected, actual, "Assertion failed: '" + expected + "' == '" + actual + "'")
    #define TEST_ASSERT_NOT_EQUAL(expected, actual) TestAssertUtils::assertNotEqual(expected, actual, "Assertion failed: " #expected " != " #actual)
    #define TEST_ASSERT_GREATER_THAN(expected, actual) TestAssertUtils::assertGreaterThan(expected, actual, "Assertion failed: " #actual " > " #expected)
    #define TEST_ASSERT_LESS_THAN(expected, actual) TestAssertUtils::assertLessThan(expected, actual, "Assertion failed: " #actual " < " #expected)
    #define TEST_ASSERT_NOT_NULL(pointer) TestAssertUtils::assertNotNull(pointer, "Assertion failed: " #pointer " != nullptr")
    #define TEST_ASSERT_NULL(pointer) TestAssertUtils::assertNull(pointer, "Assertion failed: " #pointer " == nullptr")
#else
    // For Google Test, use the built-in assertions
    #define TEST_ASSERT_TRUE(condition) ASSERT_TRUE(condition)
    #define TEST_ASSERT_FALSE(condition) ASSERT_FALSE(condition)
    #define TEST_ASSERT_EQUAL(expected, actual) ASSERT_EQ(expected, actual)
    #define TEST_ASSERT_EQUAL_FLOAT(expected, actual, epsilon) ASSERT_NEAR(expected, actual, epsilon)
    #define TEST_ASSERT_EQUAL_STRING(expected, actual) ASSERT_STREQ(expected.c_str(), actual.c_str())
    #define TEST_ASSERT_NOT_EQUAL(expected, actual) ASSERT_NE(expected, actual)
    #define TEST_ASSERT_GREATER_THAN(expected, actual) ASSERT_GT(actual, expected)
    #define TEST_ASSERT_LESS_THAN(expected, actual) ASSERT_LT(actual, expected)
    #define TEST_ASSERT_NOT_NULL(pointer) ASSERT_NE(pointer, nullptr)
    #define TEST_ASSERT_NULL(pointer) ASSERT_EQ(pointer, nullptr)
#endif

#endif // TEST_UTILS_H