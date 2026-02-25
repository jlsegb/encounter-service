#include "src/util/logger.h"

#include <iostream>

namespace encounter_service::util {

namespace {

const char* ToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
    }
    return "UNKNOWN";
}

}  // namespace

void StdoutLogger::Log(LogLevel level, std::string_view message) {
    std::cout << "[" << ToString(level) << "] " << message << '\n';
}

}  // namespace encounter_service::util
