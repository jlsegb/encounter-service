#pragma once

#include <string>
#include <variant>
#include <vector>

#include "src/domain/errors.h"
#include "src/domain/encounter_models.h"
#include "src/storage/audit_repo.h"
#include "src/storage/encounter_repo.h"
#include "src/util/clock.h"
#include "src/util/id_generator.h"

namespace encounter_service::domain {

template <typename T>
using ServiceResult = std::variant<T, DomainError>;

struct CreateEncounterInput {
    std::string patientId;
    std::string providerId;
    std::chrono::system_clock::time_point encounterDate{};
    std::string encounterType;
    nlohmann::json clinicalData;
};

class EncounterService {
public:
    virtual ~EncounterService() = default;

    virtual ServiceResult<Encounter> CreateEncounter(const CreateEncounterInput& input, const std::string& actor) = 0;
    virtual ServiceResult<Encounter> GetEncounter(const std::string& id, const std::string& actor) = 0;
    virtual ServiceResult<std::vector<Encounter>> QueryEncounters(const storage::EncounterQueryFilters& filters) = 0;
};

class DefaultEncounterService final : public EncounterService {
public:
    DefaultEncounterService(storage::EncounterRepository& encounterRepository,
                            storage::AuditRepository& auditRepository,
                            util::Clock& clock,
                            util::IdGenerator& idGenerator);

    ServiceResult<Encounter> CreateEncounter(const CreateEncounterInput& input, const std::string& actor) override;
    ServiceResult<Encounter> GetEncounter(const std::string& id, const std::string& actor) override;
    ServiceResult<std::vector<Encounter>> QueryEncounters(const storage::EncounterQueryFilters& filters) override;

private:
    storage::EncounterRepository& encounterRepository_;
    storage::AuditRepository& auditRepository_;
    util::Clock& clock_;
    util::IdGenerator& idGenerator_;
};

}  // namespace encounter_service::domain
