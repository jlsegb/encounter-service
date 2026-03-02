#pragma once

#include <chrono>
#include <string>

#include "src/util/json_compat.h"

namespace encounter_service::domain {

struct EncounterMetadata {
    std::chrono::system_clock::time_point createdAt{};
    std::chrono::system_clock::time_point updatedAt{};
    std::string createdBy;
};

struct Encounter {
    std::string encounterId;
    std::string patientId;   // PHI
    std::string providerId;
    std::chrono::system_clock::time_point encounterDate{};
    std::string encounterType;
    // User-provided clinical payload; treat as potentially PHI-bearing.
    nlohmann::json clinicalData;
    EncounterMetadata metadata;
};

}  // namespace encounter_service::domain
