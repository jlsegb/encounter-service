#pragma once

#include <chrono>
#include <optional>
#include <vector>

#include "src/domain/audit_models.h"

namespace encounter_service::storage {

struct AuditDateRange {
    // Inclusive UTC lower bound applied to `AuditEntry.timestamp`.
    std::optional<std::chrono::system_clock::time_point> from;
    // Inclusive UTC upper bound applied to `AuditEntry.timestamp`.
    std::optional<std::chrono::system_clock::time_point> to;
};

class AuditRepository {
public:
    virtual ~AuditRepository() = default;

    // Appends `entry` to the audit trail.
    virtual void Append(const domain::AuditEntry& entry) = 0;
    // Returns audit entries that match `range`.
    virtual std::vector<domain::AuditEntry> Query(const AuditDateRange& range) const = 0;
};

}  // namespace encounter_service::storage
