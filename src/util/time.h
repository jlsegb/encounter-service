#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace encounter_service::util {

std::optional<std::chrono::system_clock::time_point> ParseIso8601Utc(const std::string& value);
std::string FormatIso8601Utc(std::chrono::system_clock::time_point value);

}  // namespace encounter_service::util
