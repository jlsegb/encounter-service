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

class SequenceClock final : public encounter_service::util::Clock {
public:
    explicit SequenceClock(std::deque<TimePoint> values)
        : values_(std::move(values)) {}

    TimePoint Now() const override {
        if (values_.empty()) {
            return TimePoint{};
        }
        const auto value = values_.front();
        values_.pop_front();
        return value;
    }

private:
    mutable std::deque<TimePoint> values_;
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
    REQUIRE(encounter.metadata.createdAt == encounter.metadata.updatedAt);
    REQUIRE(encounter.metadata.createdBy == "clinician-a");

    const auto audits = auditRepo.Query({});
    REQUIRE(audits.size() == 1);
    REQUIRE(audits[0].action == encounter_service::domain::AuditAction::CREATE_ENCOUNTER);
    REQUIRE(audits[0].actor == "clinician-a");
    REQUIRE(audits[0].encounterId == "enc-123");
    REQUIRE(audits[0].timestamp == clock.Now());
}

TEST_CASE("CreateEncounter sets createdAt and updatedAt to same value") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000000}});
    FixedIdGenerator idGenerator({"enc-200"});
    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    encounter_service::domain::CreateEncounterInput input{};
    input.patientId = "patient-1";
    input.providerId = "provider-1";
    input.encounterDate = system_clock::time_point{seconds{1700000050}};
    input.encounterType = "visit";
    input.clinicalData = nlohmann::json::object();

    const auto result = service.CreateEncounter(input, "clinician-z");
    REQUIRE(result.index() == 0);

    const auto encounter = std::get<encounter_service::domain::Encounter>(result);
    REQUIRE(encounter.metadata.createdAt == encounter.metadata.updatedAt);
    REQUIRE(encounter.metadata.createdAt == clock.Now());
}

TEST_CASE("CreateEncounter sets createdBy to actor") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000000}});
    FixedIdGenerator idGenerator({"enc-201"});
    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    encounter_service::domain::CreateEncounterInput input{};
    input.patientId = "patient-1";
    input.providerId = "provider-1";
    input.encounterDate = system_clock::time_point{seconds{1700000050}};
    input.encounterType = "visit";
    input.clinicalData = nlohmann::json::object();

    const auto result = service.CreateEncounter(input, "actor-created-by");
    REQUIRE(result.index() == 0);

    const auto encounter = std::get<encounter_service::domain::Encounter>(result);
    REQUIRE(encounter.metadata.createdBy == "actor-created-by");
}

TEST_CASE("CreateEncounter duplicate submissions create distinct monotonic encounters") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    SequenceClock clock({
        system_clock::time_point{seconds{1700001000}},
        system_clock::time_point{seconds{1700001001}}
    });
    FixedIdGenerator idGenerator({"enc-300", "enc-301"});
    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    encounter_service::domain::CreateEncounterInput input{};
    input.patientId = "patient-dup";
    input.providerId = "provider-dup";
    input.encounterDate = system_clock::time_point{seconds{1700002000}};
    input.encounterType = "visit";
    input.clinicalData = nlohmann::json::object();

    const auto first = service.CreateEncounter(input, "creator-a");
    const auto second = service.CreateEncounter(input, "creator-a");
    REQUIRE(first.index() == 0);
    REQUIRE(second.index() == 0);

    const auto firstEncounter = std::get<encounter_service::domain::Encounter>(first);
    const auto secondEncounter = std::get<encounter_service::domain::Encounter>(second);

    REQUIRE(firstEncounter.encounterId == "enc-300");
    REQUIRE(secondEncounter.encounterId == "enc-301");
    REQUIRE(firstEncounter.encounterId != secondEncounter.encounterId);
    REQUIRE(firstEncounter.metadata.createdAt < secondEncounter.metadata.createdAt);
    REQUIRE(firstEncounter.metadata.updatedAt < secondEncounter.metadata.updatedAt);

    const auto audits = auditRepo.Query({});
    REQUIRE(audits.size() == 2);
    REQUIRE(audits[0].action == encounter_service::domain::AuditAction::CREATE_ENCOUNTER);
    REQUIRE(audits[1].action == encounter_service::domain::AuditAction::CREATE_ENCOUNTER);
    REQUIRE(audits[0].encounterId == "enc-300");
    REQUIRE(audits[1].encounterId == "enc-301");
    REQUIRE(audits[0].timestamp < audits[1].timestamp);
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

TEST_CASE("GetEncounter not found returns not found code") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000300}});
    FixedIdGenerator idGenerator({"unused-id"});

    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    const auto result = service.GetEncounter("missing-encounter", "auditor-b");
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.code == encounter_service::domain::DomainErrorCode::NotFound);
}

TEST_CASE("GetEncounter not found returns safe message") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000300}});
    FixedIdGenerator idGenerator({"unused-id"});

    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    const auto result = service.GetEncounter("missing-encounter", "auditor-b");
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.message == "Encounter not found");
}

TEST_CASE("GetEncounter not found does not append audit entry") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000300}});
    FixedIdGenerator idGenerator({"unused-id"});

    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    const auto result = service.GetEncounter("missing-encounter", "auditor-b");
    REQUIRE(result.index() == 1);

    const auto audits = auditRepo.Query({});
    REQUIRE(audits.empty());
}

TEST_CASE("QueryEncounters filters by patientId") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000400}});
    FixedIdGenerator idGenerator({"unused-id"});

    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    const auto t = system_clock::time_point{seconds{1700000000}};

    encounter_service::domain::Encounter e1{};
    e1.encounterId = "enc-1";
    e1.patientId = "patient-1";
    e1.providerId = "provider-1";
    e1.encounterDate = t;
    e1.encounterType = "visit";
    e1.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e1);

    encounter_service::domain::Encounter e2{};
    e2.encounterId = "enc-2";
    e2.patientId = "patient-2";
    e2.providerId = "provider-1";
    e2.encounterDate = t;
    e2.encounterType = "visit";
    e2.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e2);

    encounter_service::storage::EncounterQueryFilters filters{};
    filters.patientId = "patient-1";
    const auto result = service.QueryEncounters(filters);
    REQUIRE(result.index() == 0);

    const auto encounters = std::get<std::vector<encounter_service::domain::Encounter>>(result);
    REQUIRE(encounters.size() == 1);
    REQUIRE(encounters[0].encounterId == "enc-1");
}

TEST_CASE("QueryEncounters filters by providerId") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000400}});
    FixedIdGenerator idGenerator({"unused-id"});

    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    const auto t = system_clock::time_point{seconds{1700000000}};

    encounter_service::domain::Encounter e1{};
    e1.encounterId = "enc-1";
    e1.patientId = "patient-1";
    e1.providerId = "provider-1";
    e1.encounterDate = t;
    e1.encounterType = "visit";
    e1.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e1);

    encounter_service::domain::Encounter e2{};
    e2.encounterId = "enc-2";
    e2.patientId = "patient-1";
    e2.providerId = "provider-2";
    e2.encounterDate = t;
    e2.encounterType = "visit";
    e2.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e2);

    encounter_service::storage::EncounterQueryFilters filters{};
    filters.providerId = "provider-2";
    const auto result = service.QueryEncounters(filters);
    REQUIRE(result.index() == 0);

    const auto encounters = std::get<std::vector<encounter_service::domain::Encounter>>(result);
    REQUIRE(encounters.size() == 1);
    REQUIRE(encounters[0].encounterId == "enc-2");
}

TEST_CASE("QueryEncounters filters by from and to inclusive") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000400}});
    FixedIdGenerator idGenerator({"unused-id"});

    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    const auto t0 = system_clock::time_point{seconds{1700000000}};
    const auto t1 = system_clock::time_point{seconds{1700000100}};
    const auto t2 = system_clock::time_point{seconds{1700000200}};
    const auto t3 = system_clock::time_point{seconds{1700000300}};

    encounter_service::domain::Encounter e0{};
    e0.encounterId = "enc-0";
    e0.patientId = "patient-1";
    e0.providerId = "provider-1";
    e0.encounterDate = t0;
    e0.encounterType = "visit";
    e0.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e0);

    encounter_service::domain::Encounter e1{};
    e1.encounterId = "enc-1";
    e1.patientId = "patient-1";
    e1.providerId = "provider-1";
    e1.encounterDate = t1;
    e1.encounterType = "visit";
    e1.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e1);

    encounter_service::domain::Encounter e2{};
    e2.encounterId = "enc-2";
    e2.patientId = "patient-1";
    e2.providerId = "provider-1";
    e2.encounterDate = t2;
    e2.encounterType = "visit";
    e2.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e2);

    encounter_service::domain::Encounter e3{};
    e3.encounterId = "enc-3";
    e3.patientId = "patient-1";
    e3.providerId = "provider-1";
    e3.encounterDate = t3;
    e3.encounterType = "visit";
    e3.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e3);

    encounter_service::storage::EncounterQueryFilters filters{};
    filters.encounterDateFrom = t1;
    filters.encounterDateTo = t2;

    const auto result = service.QueryEncounters(filters);
    REQUIRE(result.index() == 0);

    const auto encounters = std::get<std::vector<encounter_service::domain::Encounter>>(result);
    REQUIRE(encounters.size() == 2);
    REQUIRE(encounters[0].encounterId == "enc-1");
    REQUIRE(encounters[1].encounterId == "enc-2");
}

TEST_CASE("QueryEncounters returns deterministic order by encounterDate then encounterId") {
    using namespace std::chrono;

    encounter_service::storage::InMemoryEncounterRepository encounterRepo;
    encounter_service::storage::InMemoryAuditRepository auditRepo;
    FixedClock clock(system_clock::time_point{seconds{1700000400}});
    FixedIdGenerator idGenerator({"unused-id"});

    encounter_service::domain::DefaultEncounterService service(encounterRepo, auditRepo, clock, idGenerator);

    const auto t1 = system_clock::time_point{seconds{1700000100}};
    const auto t2 = system_clock::time_point{seconds{1700000200}};

    encounter_service::domain::Encounter e1{};
    e1.encounterId = "enc-b";
    e1.patientId = "patient-1";
    e1.providerId = "provider-1";
    e1.encounterDate = t2;
    e1.encounterType = "visit";
    e1.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e1);

    encounter_service::domain::Encounter e2{};
    e2.encounterId = "enc-a";
    e2.patientId = "patient-1";
    e2.providerId = "provider-1";
    e2.encounterDate = t2;
    e2.encounterType = "visit";
    e2.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e2);

    encounter_service::domain::Encounter e3{};
    e3.encounterId = "enc-c";
    e3.patientId = "patient-1";
    e3.providerId = "provider-1";
    e3.encounterDate = t1;
    e3.encounterType = "visit";
    e3.clinicalData = nlohmann::json::object();
    encounterRepo.Create(e3);

    encounter_service::storage::EncounterQueryFilters filters{};
    const auto result = service.QueryEncounters(filters);
    REQUIRE(result.index() == 0);

    const auto encounters = std::get<std::vector<encounter_service::domain::Encounter>>(result);
    REQUIRE(encounters.size() == 3);
    REQUIRE(encounters[0].encounterId == "enc-c");
    REQUIRE(encounters[1].encounterId == "enc-a");
    REQUIRE(encounters[2].encounterId == "enc-b");
}
