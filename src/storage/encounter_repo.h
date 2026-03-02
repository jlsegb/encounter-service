#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "src/domain/encounter_models.h"

namespace encounter_service::storage {

struct EncounterQueryFilters {
    std::optional<std::string> patientId;
    std::optional<std::string> providerId;
    // Inclusive UTC lower bound applied to `Encounter.encounterDate`.
    std::optional<std::chrono::system_clock::time_point> encounterDateFrom;
    // Inclusive UTC upper bound applied to `Encounter.encounterDate`.
    std::optional<std::chrono::system_clock::time_point> encounterDateTo;
    std::optional<std::string> encounterType;
    // Maximum number of results to return.
    std::size_t limit{100};
    // Number of matching rows to skip before collecting results.
    std::size_t offset{0};
};

class EncounterRepository {
public:
    virtual ~EncounterRepository() = default;

    // Stores `encounter` and returns the persisted value.
    virtual domain::Encounter Create(const domain::Encounter& encounter) = 0;
    // Returns the encounter for `encounterId`, or std::nullopt when not found.
    virtual std::optional<domain::Encounter> GetById(const std::string& encounterId) const = 0;
    // Returns encounters that match `filters`.
    virtual std::vector<domain::Encounter> Query(const EncounterQueryFilters& filters) const = 0;
};

}  // namespace encounter_service::storage
