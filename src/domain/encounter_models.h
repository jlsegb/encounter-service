#pragma once

#include <string>

#include "src/util/json_compat.h"

namespace encounter_service::domain {

struct EncounterMetadata {
    std::string createdAt;
    std::string updatedAt;
    std::string createdBy;
};

struct Encounter {
    std::string encounterId;
    std::string patientId;   // PHI
    std::string providerId;
    std::string encounterDate;
    std::string encounterType;
    nlohmann::json clinicalData;
    EncounterMetadata metadata;
};

}  // namespace encounter_service::domain
