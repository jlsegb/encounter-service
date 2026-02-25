#pragma once

#include <chrono>
#include <optional>
#include <vector>

#include "src/domain/audit_models.h"

namespace encounter_service::storage {

struct AuditDateRange {
    std::optional<std::chrono::system_clock::time_point> from;
    std::optional<std::chrono::system_clock::time_point> to;
};

class AuditRepository {
public:
    virtual ~AuditRepository() = default;

    virtual void Append(const domain::AuditEntry& entry) = 0;
    virtual std::vector<domain::AuditEntry> Query(const AuditDateRange& range) const = 0;
};

}  // namespace encounter_service::storage
