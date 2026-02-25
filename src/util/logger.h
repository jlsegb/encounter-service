#pragma once

#include <string>
#include <string_view>

namespace encounter_service::util {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

class Logger {
public:
    virtual ~Logger() = default;
    virtual void Log(LogLevel level, std::string_view message) = 0;
};

class StdoutLogger final : public Logger {
public:
    void Log(LogLevel level, std::string_view message) override;
};

}  // namespace encounter_service::util
