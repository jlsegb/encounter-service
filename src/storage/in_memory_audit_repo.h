#pragma once

#include <vector>

#include "src/storage/audit_repo.h"

namespace encounter_service::storage {

class InMemoryAuditRepository final : public AuditRepository {
public:
    void Append(const domain::AuditEntry& entry) override;
    std::vector<domain::AuditEntry> Query(const AuditDateRange& range) const override;

private:
    // Not thread-safe. Production should use synchronization or a database-backed repository.
    std::vector<domain::AuditEntry> entries_;
};

}  // namespace encounter_service::storage
