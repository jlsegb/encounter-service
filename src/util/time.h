#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace encounter_service::util {

// Parses UTC timestamps in `YYYY-MM-DD` or `YYYY-MM-DDTHH:MM:SSZ` format.
// Returns std::nullopt when `value` cannot be parsed.
std::optional<std::chrono::system_clock::time_point> ParseIso8601Utc(const std::string& value);
// Formats `value` as `YYYY-MM-DDTHH:MM:SSZ` in UTC.
std::string FormatIso8601Utc(std::chrono::system_clock::time_point value);

}  // namespace encounter_service::util
