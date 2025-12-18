#include "Logger.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <sstream>

namespace {
uint64_t defaultNowMs() {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

std::string escapeJson(const std::string& s) {
    std::ostringstream out;
    for (char c : s) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out << "\\u";
                    out << std::hex << std::uppercase;
                    out.width(4);
                    out.fill('0');
                    out << static_cast<int>(static_cast<unsigned char>(c));
                    out << std::dec;
                } else {
                    out << c;
                }
        }
    }
    return out.str();
}
} // namespace

Logger::Logger(size_t capacity) : capacity_(capacity), buffer_(capacity) {
    if (capacity_ == 0) {
        capacity_ = 1;
        buffer_.resize(capacity_);
    }

    timeProvider_ = defaultNowMs;
}

void Logger::setTimeProvider(TimeProvider provider) {
    timeProvider_ = provider ? provider : defaultNowMs;
}

void Logger::setEnabled(bool enabled) {
    enabled_ = enabled;
}

void Logger::clear() {
    writeIndex_ = 0;
    count_ = 0;
}

uint64_t Logger::nowMs() const {
    return timeProvider_ ? timeProvider_() : defaultNowMs();
}

void Logger::log(Level level, const std::string& message, const std::string& tag) {
    if (!enabled_) {
        return;
    }

    Entry entry;
    entry.timestampMs = nowMs();
    entry.level = level;
    entry.tag = tag;
    entry.message = message;

    buffer_[writeIndex_] = entry;
    writeIndex_ = (writeIndex_ + 1) % capacity_;
    count_ = std::min(capacity_, count_ + 1);
}

std::vector<Logger::Entry> Logger::getEntries(Level minLevel) const {
    return getEntries(minLevel, std::string());
}

std::vector<Logger::Entry> Logger::getEntries(Level minLevel, const std::string& tagFilter) const {
    std::vector<Entry> out;
    out.reserve(count_);

    const bool filterByTag = !tagFilter.empty();

    // Oldest entry index in circular buffer
    size_t start = (count_ == capacity_) ? writeIndex_ : 0;

    for (size_t i = 0; i < count_; ++i) {
        const size_t idx = (start + i) % capacity_;
        const Entry& entry = buffer_[idx];

        if (static_cast<int>(entry.level) < static_cast<int>(minLevel)) {
            continue;
        }

        if (filterByTag && entry.tag != tagFilter) {
            continue;
        }

        out.push_back(entry);
    }

    return out;
}

std::string Logger::exportToJson(Level minLevel) const {
    std::ostringstream json;
    json << "[";

    std::vector<Entry> entries = getEntries(minLevel);
    for (size_t i = 0; i < entries.size(); ++i) {
        const Entry& e = entries[i];
        if (i != 0) {
            json << ',';
        }
        json << '{';
        json << "\"ts\":" << e.timestampMs << ',';
        json << "\"level\":\"" << levelToString(e.level) << "\",";
        json << "\"tag\":\"" << escapeJson(e.tag) << "\",";
        json << "\"msg\":\"" << escapeJson(e.message) << "\"";
        json << '}';
    }

    json << "]";
    return json.str();
}

const char* Logger::levelToString(Level level) {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARN: return "WARN";
        case Level::ERROR: return "ERROR";
    }
    return "INFO";
}

bool Logger::tryParseLevel(const std::string& level, Level& out) {
    std::string s = toUpper(level);
    if (s == "DEBUG") {
        out = Level::DEBUG;
        return true;
    }
    if (s == "INFO") {
        out = Level::INFO;
        return true;
    }
    if (s == "WARN" || s == "WARNING") {
        out = Level::WARN;
        return true;
    }
    if (s == "ERROR") {
        out = Level::ERROR;
        return true;
    }
    return false;
}
