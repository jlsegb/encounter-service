#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <utility>

namespace encounter_service::util {

class IdGenerator {
public:
    virtual ~IdGenerator() = default;
    virtual std::string NextId() = 0;
};

class DefaultIdGenerator final : public IdGenerator {
public:
    explicit DefaultIdGenerator(std::string prefix = "id")
        : prefix_(std::move(prefix)) {}

    std::string NextId() override {
        const auto value = next_.fetch_add(1, std::memory_order_relaxed);
        return prefix_ + "-" + std::to_string(value);
    }

private:
    std::string prefix_;
    std::atomic<std::uint64_t> next_{1};
};

}  // namespace encounter_service::util
