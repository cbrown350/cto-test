#include <gtest/gtest.h>

#include <chrono>

#include "CommonTestFixture.h"
#include "Logger.h"
#include "TestUtils.h"

class LoggerTest : public CommonTestFixture {};

TEST_F(LoggerTest, StartsEmptyWithCapacity) {
    Logger logger(10);
    EXPECT_TRUE(logger.empty());
    EXPECT_EQ(logger.capacity(), 10u);
}

TEST_F(LoggerTest, CapacityZeroBecomesOne) {
    Logger logger(0);
    EXPECT_EQ(logger.capacity(), 1u);
}

TEST_F(LoggerTest, LogAddsEntry) {
    Logger logger(10);
    logger.info("hello", "test");

    auto entries = logger.getEntries();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].message, "hello");
    EXPECT_EQ(entries[0].tag, "test");
    EXPECT_EQ(entries[0].level, Logger::Level::INFO);
}

TEST_F(LoggerTest, DisabledLoggerDoesNotRecord) {
    Logger logger(10);
    logger.setEnabled(false);
    logger.info("hello");
    EXPECT_TRUE(logger.empty());
}

TEST_F(LoggerTest, CircularBufferOverwritesOldest) {
    Logger logger(3);

    logger.info("1");
    logger.info("2");
    logger.info("3");
    logger.info("4");

    auto entries = logger.getEntries();
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].message, "2");
    EXPECT_EQ(entries[2].message, "4");
}

TEST_F(LoggerTest, ClearEmptiesBuffer) {
    Logger logger(10);
    logger.info("hello");
    logger.clear();
    EXPECT_TRUE(logger.empty());
}

TEST_F(LoggerTest, GetEntriesFiltersByMinLevel) {
    Logger logger(10);
    logger.debug("d");
    logger.info("i");
    logger.warn("w");

    auto filtered = logger.getEntries(Logger::Level::WARN);
    ASSERT_EQ(filtered.size(), 1u);
    EXPECT_EQ(filtered[0].message, "w");
}

TEST_F(LoggerTest, GetEntriesFiltersByTag) {
    Logger logger(10);
    logger.info("a", "A");
    logger.info("b", "B");

    auto filtered = logger.getEntries(Logger::Level::DEBUG, "B");
    ASSERT_EQ(filtered.size(), 1u);
    EXPECT_EQ(filtered[0].message, "b");
}

TEST_F(LoggerTest, ExportToJsonEmptyIsArray) {
    Logger logger(10);
    EXPECT_EQ(logger.exportToJson(), "[]");
}

TEST_F(LoggerTest, ExportToJsonEscapesQuotes) {
    Logger logger(10);
    logger.info("a\"b");

    std::string json = logger.exportToJson();
    EXPECT_NE(json.find("a\\\"b"), std::string::npos);
}

TEST_F(LoggerTest, ExportToJsonFiltersByMinLevel) {
    Logger logger(10);
    logger.debug("debug");
    logger.error("error");

    std::string json = logger.exportToJson(Logger::Level::ERROR);

    EXPECT_EQ(json.find("debug"), std::string::npos);
    EXPECT_NE(json.find("error"), std::string::npos);
}

TEST_F(LoggerTest, TryParseLevelIsCaseInsensitive) {
    Logger::Level lvl = Logger::Level::INFO;

    EXPECT_TRUE(Logger::tryParseLevel("debug", lvl));
    EXPECT_EQ(lvl, Logger::Level::DEBUG);

    EXPECT_TRUE(Logger::tryParseLevel("WARNING", lvl));
    EXPECT_EQ(lvl, Logger::Level::WARN);

    EXPECT_FALSE(Logger::tryParseLevel("notalevel", lvl));
}

TEST_F(LoggerTest, LevelToStringReturnsExpectedValues) {
    EXPECT_STREQ(Logger::levelToString(Logger::Level::DEBUG), "DEBUG");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::INFO), "INFO");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::WARN), "WARN");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::ERROR), "ERROR");
}

TEST_F(LoggerTest, TimeProviderIsUsedForTimestamps) {
    Logger logger(10);
    uint64_t timeMs = 123;
    logger.setTimeProvider([&]() {
        return timeMs;
    });

    logger.info("a");
    timeMs = 456;
    logger.info("b");

    auto entries = logger.getEntries();
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].timestampMs, 123u);
    EXPECT_EQ(entries[1].timestampMs, 456u);
}

TEST_F(LoggerTest, ExplicitMemoryTrackingDetectsLeaksUnlessDeallocated) {
    void* ptr = TestMemoryUtils::allocate(128);
    ASSERT_NE(ptr, nullptr);

    // Edge: after allocation we should see leaks reported.
    // (Note: leak tracking is explicit, so tests must call deallocate.)
    EXPECT_TRUE(TestMemoryUtils::hasMemoryLeaks());
    EXPECT_GT(TestMemoryUtils::getMemoryAllocated(), 0u);

    TestMemoryUtils::deallocate(ptr);
    EXPECT_FALSE(TestMemoryUtils::hasMemoryLeaks());
}

TEST_F(LoggerTest, PerformanceLoggingTenThousandEntriesFast) {
    Logger logger(256);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 10000; ++i) {
        logger.debug("x");
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    EXPECT_LT(elapsed.count(), 2000);
}
