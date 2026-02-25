#pragma once

#include <chrono>

namespace encounter_service::util {

class Clock {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    virtual ~Clock() = default;
    virtual TimePoint Now() const = 0;
};

class SystemClock final : public Clock {
public:
    TimePoint Now() const override {
        return std::chrono::system_clock::now();
    }
};

}  // namespace encounter_service::util
