#include "src/domain/encounter_service.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace encounter_service::domain {

namespace {

std::string ToIso8601(util::Clock::TimePoint timePoint) {
    const auto timeT = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &timeT);
#else
    gmtime_r(&timeT, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

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
    const auto nowIso = ToIso8601(now);

    const Encounter encounter{
        .encounterId = idGenerator_.NextId(),
        .patientId = input.patientId,
        .providerId = input.providerId,
        .encounterDate = input.encounterDate,
        .encounterType = input.encounterType,
        .clinicalData = input.clinicalData,
        .metadata = EncounterMetadata{
            .createdAt = nowIso,
            .updatedAt = nowIso,
            .createdBy = actor
        }
    };

    auto persisted = encounterRepository_.Create(encounter);

    auditRepository_.Append(AuditEntry{
        .timestamp = nowIso,
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
        .timestamp = ToIso8601(clock_.Now()),
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
