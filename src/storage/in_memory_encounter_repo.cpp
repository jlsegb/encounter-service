#include "src/storage/in_memory_encounter_repo.h"

#include <algorithm>

namespace encounter_service::storage {

namespace {

bool Matches(const domain::Encounter& encounter, const EncounterQueryFilters& filters) {
    if (filters.patientId && encounter.patientId != *filters.patientId) {
        return false;
    }
    if (filters.providerId && encounter.providerId != *filters.providerId) {
        return false;
    }
    if (filters.encounterType && encounter.encounterType != *filters.encounterType) {
        return false;
    }
    if (filters.encounterDateFrom && encounter.encounterDate < *filters.encounterDateFrom) {
        return false;
    }
    if (filters.encounterDateTo && encounter.encounterDate > *filters.encounterDateTo) {
        return false;
    }
    return true;
}

}  // namespace

domain::Encounter InMemoryEncounterRepository::Create(const domain::Encounter& encounter) {
    encounters_[encounter.encounterId] = encounter;
    return encounter;
}

std::optional<domain::Encounter> InMemoryEncounterRepository::GetById(const std::string& encounterId) const {
    const auto it = encounters_.find(encounterId);
    if (it == encounters_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<domain::Encounter> InMemoryEncounterRepository::Query(const EncounterQueryFilters& filters) const {
    std::vector<domain::Encounter> matches;
    matches.reserve(encounters_.size());
    for (const auto& [unused_id, encounter] : encounters_) {
        (void)unused_id;
        if (Matches(encounter, filters)) {
            matches.push_back(encounter);
        }
    }

    std::sort(matches.begin(), matches.end(), [](const domain::Encounter& a, const domain::Encounter& b) {
        if (a.encounterDate != b.encounterDate) {
            return a.encounterDate < b.encounterDate;
        }
        return a.encounterId < b.encounterId;
    });

    const auto begin_index = std::min(filters.offset, matches.size());
    const auto end_index = std::min(begin_index + filters.limit, matches.size());

    return std::vector<domain::Encounter>(
        matches.begin() + static_cast<std::ptrdiff_t>(begin_index),
        matches.begin() + static_cast<std::ptrdiff_t>(end_index));
}

}  // namespace encounter_service::storage
