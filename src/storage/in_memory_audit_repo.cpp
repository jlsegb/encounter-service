#include "src/storage/in_memory_audit_repo.h"

#include <algorithm>

namespace encounter_service::storage {

void InMemoryAuditRepository::Append(const domain::AuditEntry& entry) {
    entries_.push_back(entry);
}

std::vector<domain::AuditEntry> InMemoryAuditRepository::Query(const AuditDateRange& range) const {
    std::vector<domain::AuditEntry> out;
    out.reserve(entries_.size());

    for (const auto& entry : entries_) {
        if (range.from && entry.timestamp < *range.from) {
            continue;
        }
        if (range.to && entry.timestamp > *range.to) {
            continue;
        }
        out.push_back(entry);
    }

    std::sort(out.begin(), out.end(), [](const domain::AuditEntry& a, const domain::AuditEntry& b) {
        if (a.timestamp != b.timestamp) {
            return a.timestamp < b.timestamp;
        }
        if (a.encounterId != b.encounterId) {
            return a.encounterId < b.encounterId;
        }
        return a.actor < b.actor;
    });

    return out;
}

}  // namespace encounter_service::storage
