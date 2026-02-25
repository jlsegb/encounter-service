#include "src/util/time.h"

#include <cctype>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>

namespace encounter_service::util {

namespace {

std::optional<int> ParseIntPart(const std::string& s, std::size_t pos, std::size_t len) {
    if (pos + len > s.size()) {
        return std::nullopt;
    }
    int value = 0;
    for (std::size_t i = 0; i < len; ++i) {
        const unsigned char ch = static_cast<unsigned char>(s[pos + i]);
        if (!std::isdigit(ch)) {
            return std::nullopt;
        }
        value = (value * 10) + static_cast<int>(ch - '0');
    }
    return value;
}

std::optional<std::tm> ParseTmUtc(const std::string& value) {
    const auto year = ParseIntPart(value, 0, 4);
    const auto month = ParseIntPart(value, 5, 2);
    const auto day = ParseIntPart(value, 8, 2);
    if (!year || !month || !day || value.size() < 10 || value[4] != '-' || value[7] != '-') {
        return std::nullopt;
    }

    std::tm tm{};
    tm.tm_year = *year - 1900;
    tm.tm_mon = *month - 1;
    tm.tm_mday = *day;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;

    if (value.size() == 10) {
        return tm;
    }

    if (value.size() != 20 || value[10] != 'T' || value[13] != ':' || value[16] != ':' || value[19] != 'Z') {
        return std::nullopt;
    }

    const auto hour = ParseIntPart(value, 11, 2);
    const auto minute = ParseIntPart(value, 14, 2);
    const auto second = ParseIntPart(value, 17, 2);
    if (!hour || !minute || !second) {
        return std::nullopt;
    }

    tm.tm_hour = *hour;
    tm.tm_min = *minute;
    tm.tm_sec = *second;
    return tm;
}

std::optional<std::time_t> TimegmUtc(std::tm tm) {
#if defined(_WIN32)
    const auto t = _mkgmtime(&tm);
#else
    const auto t = timegm(&tm);
#endif
    if (t == static_cast<std::time_t>(-1)) {
        return std::nullopt;
    }
    return t;
}

}  // namespace

std::optional<std::chrono::system_clock::time_point> ParseIso8601Utc(const std::string& value) {
    const auto tm = ParseTmUtc(value);
    if (!tm) {
        return std::nullopt;
    }
    const auto epoch = TimegmUtc(*tm);
    if (!epoch) {
        return std::nullopt;
    }
    return std::chrono::system_clock::from_time_t(*epoch);
}

std::string FormatIso8601Utc(std::chrono::system_clock::time_point value) {
    const auto t = std::chrono::system_clock::to_time_t(value);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

}  // namespace encounter_service::util
