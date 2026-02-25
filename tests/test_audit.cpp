#define ENCOUNTER_SERVICE_CATCH_COMPAT_MAIN
#include "tests/catch_compat.h"

#include <chrono>
#include <deque>
#include <variant>

#include "src/domain/encounter_service.h"
#include "src/storage/in_memory_encounter_repo.h"
#include "src/storage/in_memory_audit_repo.h"
#include "src/util/clock.h"
#include "src/util/id_generator.h"

namespace {

class FixedClock final : public encounter_service::util::Clock {
public:
    explicit FixedClock(TimePoint now)
        : now_(now) {}

    TimePoint Now() const override {
        return now_;
    }

private:
    TimePoint now_;
};

class FixedIdGenerator final : public encounter_service::util::IdGenerator {
public:
    explicit FixedIdGenerator(std::deque<std::string> ids)
        : ids_(std::move(ids)) {}

    std::string NextId() override {
        if (ids_.empty()) {
            return "test-id";
        }
        auto value = ids_.front();
        ids_.pop_front();
        return value;
    }

private:
    std::deque<std::string> ids_;
};

}  // namespace

TEST_CASE("Audit repository stores and queries entries") {
    encounter_service::storage::InMemoryAuditRepository repo;
    repo.Append(encounter_service::domain::AuditEntry{
        .timestamp = std::chrono::system_clock::time_point{},
        .actor = "tester",
        .action = encounter_service::domain::AuditAction::READ_ENCOUNTER,
        .encounterId = "enc-1"
    });

    encounter_service::storage::AuditDateRange range{};
    const auto results = repo.Query(range);
    REQUIRE(results.size() == 1);
}

TEST_CASE("CreateEncounter sets metadata and appends CREATE audit entry") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000000}});
    FixedIdGenerator idGenerator({"enc-123"});

    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    encounter_service::domain::CreateEncounterInput input{};
    input.patientId = "patient-1";
    input.providerId = "provider-9";
    input.encounterDate = system_clock::time_point{seconds{1700000100}};
    input.encounterType = "office-visit";
    input.clinicalData = nlohmann::json::object();

    const auto result = service.CreateEncounter(input, "clinician-a");
    REQUIRE(result.index() == 0);

    const auto encounter = std::get<encounter_service::domain::Encounter>(result);
    REQUIRE(encounter.encounterId == "enc-123");
    REQUIRE(encounter.metadata.createdAt == clock.Now());
    REQUIRE(encounter.metadata.updatedAt == clock.Now());
    REQUIRE(encounter.metadata.createdBy == "clinician-a");

    const auto audits = auditRepo.Query({});
    REQUIRE(audits.size() == 1);
    REQUIRE(audits[0].action == encounter_service::domain::AuditAction::CREATE_ENCOUNTER);
    REQUIRE(audits[0].actor == "clinician-a");
    REQUIRE(audits[0].encounterId == "enc-123");
    REQUIRE(audits[0].timestamp == clock.Now());
}

TEST_CASE("GetEncounter appends READ audit entry with actor and encounterId") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000200}});
    FixedIdGenerator idGenerator({"unused-id"});

    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    encounter_service::domain::Encounter seeded{};
    seeded.encounterId = "enc-999";
    seeded.patientId = "patient-2";
    seeded.providerId = "provider-2";
    seeded.encounterDate = system_clock::time_point{seconds{1700000001}};
    seeded.encounterType = "follow-up";
    seeded.clinicalData = nlohmann::json::object();
    seeded.metadata.createdAt = system_clock::time_point{seconds{1700000000}};
    seeded.metadata.updatedAt = system_clock::time_point{seconds{1700000000}};
    seeded.metadata.createdBy = "seed-user";
    encounterRepo.Create(seeded);

    const auto result = service.GetEncounter("enc-999", "auditor-a");
    REQUIRE(result.index() == 0);

    const auto fetched = std::get<encounter_service::domain::Encounter>(result);
    REQUIRE(fetched.encounterId == "enc-999");

    const auto audits = auditRepo.Query({});
    REQUIRE(audits.size() == 1);
    REQUIRE(audits[0].action == encounter_service::domain::AuditAction::READ_ENCOUNTER);
    REQUIRE(audits[0].actor == "auditor-a");
    REQUIRE(audits[0].encounterId == "enc-999");
    REQUIRE(audits[0].timestamp == clock.Now());
}
