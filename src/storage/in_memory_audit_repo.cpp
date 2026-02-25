#include "src/storage/in_memory_audit_repo.h"

#include <mutex>

namespace encounter_service::storage {

void InMemoryAuditRepository::Append(const domain::AuditEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back(entry);
}

std::vector<domain::AuditEntry> InMemoryAuditRepository::Query(const AuditDateRange& range) const {
    std::lock_guard<std::mutex> lock(mutex_);
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

    return out;
}

}  // namespace encounter_service::storage
