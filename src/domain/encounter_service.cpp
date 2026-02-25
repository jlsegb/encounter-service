#include "src/domain/encounter_service.h"

namespace encounter_service::domain {

namespace {

DomainError MakeError(DomainErrorCode code, std::string message) {
    return DomainError{
        .code = code,
        .message = std::move(message),
        .details = std::nullopt
    };
}

}  // namespace

DefaultEncounterService::DefaultEncounterService(storage::EncounterRepository& encounterRepository,
                                                 storage::AuditRepository& auditRepository,
                                                 util::Clock& clock,
                                                 util::IdGenerator& idGenerator)
    : encounterRepository_(encounterRepository),
      auditRepository_(auditRepository),
      clock_(clock),
      idGenerator_(idGenerator) {}

ServiceResult<Encounter> DefaultEncounterService::CreateEncounter(const CreateEncounterInput& input, const std::string& actor) {
    if (actor.empty()) {
        return MakeError(DomainErrorCode::Unauthorized, "Unauthorized");
    }

    const auto now = clock_.Now();

    const Encounter encounter{
        .encounterId = idGenerator_.NextId(),
        .patientId = input.patientId,
        .providerId = input.providerId,
        .encounterDate = input.encounterDate,
        .encounterType = input.encounterType,
        .clinicalData = input.clinicalData,
        .metadata = EncounterMetadata{
            .createdAt = now,
            .updatedAt = now,
            .createdBy = actor
        }
    };

    auto persisted = encounterRepository_.Create(encounter);

    auditRepository_.Append(AuditEntry{
        .timestamp = now,
        .actor = actor,
        .action = AuditAction::CREATE_ENCOUNTER,
        .encounterId = persisted.encounterId
    });

    return persisted;
}

ServiceResult<Encounter> DefaultEncounterService::GetEncounter(const std::string& id, const std::string& actor) {
    if (actor.empty()) {
        return MakeError(DomainErrorCode::Unauthorized, "Unauthorized");
    }

    auto found = encounterRepository_.GetById(id);
    if (!found) {
        return MakeError(DomainErrorCode::NotFound, "Encounter not found");
    }

    auditRepository_.Append(AuditEntry{
        .timestamp = clock_.Now(),
        .actor = actor,
        .action = AuditAction::READ_ENCOUNTER,
        .encounterId = found->encounterId
    });

    return *found;
}

ServiceResult<std::vector<Encounter>> DefaultEncounterService::QueryEncounters(const storage::EncounterQueryFilters& filters) {
    return encounterRepository_.Query(filters);
}

}  // namespace encounter_service::domain
