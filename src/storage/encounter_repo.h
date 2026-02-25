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
    std::optional<std::chrono::system_clock::time_point> encounterDateFrom;
    std::optional<std::chrono::system_clock::time_point> encounterDateTo;
    std::optional<std::string> encounterType;
    std::size_t limit{100};
    std::size_t offset{0};
};

class EncounterRepository {
public:
    virtual ~EncounterRepository() = default;

    virtual domain::Encounter Create(const domain::Encounter& encounter) = 0;
    virtual std::optional<domain::Encounter> GetById(const std::string& encounterId) const = 0;
    virtual std::vector<domain::Encounter> Query(const EncounterQueryFilters& filters) const = 0;
};

}  // namespace encounter_service::storage
