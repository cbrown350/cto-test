#ifndef LOGGER_H
#define LOGGER_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class Logger {
public:
    enum class Level {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };

    struct Entry {
        uint64_t timestampMs = 0;
        Level level = Level::INFO;
        std::string tag;
        std::string message;
    };

    using TimeProvider = std::function<uint64_t()>;

    explicit Logger(size_t capacity = 256);

    void setTimeProvider(TimeProvider provider);

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    void clear();

    size_t size() const { return count_; }
    size_t capacity() const { return capacity_; }
    bool empty() const { return count_ == 0; }

    void log(Level level, const std::string& message, const std::string& tag = "");
    void debug(const std::string& message, const std::string& tag = "") { log(Level::DEBUG, message, tag); }
    void info(const std::string& message, const std::string& tag = "") { log(Level::INFO, message, tag); }
    void warn(const std::string& message, const std::string& tag = "") { log(Level::WARN, message, tag); }
    void error(const std::string& message, const std::string& tag = "") { log(Level::ERROR, message, tag); }

    std::vector<Entry> getEntries(Level minLevel = Level::DEBUG) const;
    std::vector<Entry> getEntries(Level minLevel, const std::string& tagFilter) const;

    std::string exportToJson(Level minLevel = Level::DEBUG) const;

    static const char* levelToString(Level level);
    static bool tryParseLevel(const std::string& level, Level& out);

private:
    size_t capacity_;
    std::vector<Entry> buffer_;
    size_t writeIndex_ = 0;
    size_t count_ = 0;

    bool enabled_ = true;
    TimeProvider timeProvider_;

    uint64_t nowMs() const;
};

#endif // LOGGER_H
