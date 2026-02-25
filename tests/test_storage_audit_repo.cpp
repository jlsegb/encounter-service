#include "tests/catch_compat.h"

#include <chrono>

#include "src/storage/in_memory_audit_repo.h"

namespace {

encounter_service::domain::AuditEntry MakeAudit(std::chrono::system_clock::time_point ts,
                                                std::string actor,
                                                std::string encounterId,
                                                encounter_service::domain::AuditAction action = encounter_service::domain::AuditAction::READ_ENCOUNTER) {
    return encounter_service::domain::AuditEntry{
        .timestamp = ts,
        .actor = std::move(actor),
        .action = action,
        .encounterId = std::move(encounterId)
    };
}

}  // namespace

TEST_CASE("InMemoryAuditRepository Query returns empty when no entries match") {
    using namespace std::chrono;
    encounter_service::storage::InMemoryAuditRepository repo;
    repo.Append(MakeAudit(system_clock::time_point{seconds{10}}, "a", "enc-1"));

    encounter_service::storage::AuditDateRange range{};
    range.from = system_clock::time_point{seconds{20}};
    const auto results = repo.Query(range);
    REQUIRE(results.empty());
}

TEST_CASE("InMemoryAuditRepository Query applies inclusive from and to bounds") {
    using namespace std::chrono;
    encounter_service::storage::InMemoryAuditRepository repo;
    const auto t1 = system_clock::time_point{seconds{10}};
    const auto t2 = system_clock::time_point{seconds{20}};
    const auto t3 = system_clock::time_point{seconds{30}};
    repo.Append(MakeAudit(t1, "a", "enc-1"));
    repo.Append(MakeAudit(t2, "a", "enc-2"));
    repo.Append(MakeAudit(t3, "a", "enc-3"));

    encounter_service::storage::AuditDateRange range{};
    range.from = t1;
    range.to = t2;
    const auto results = repo.Query(range);
    REQUIRE(results.size() == 2);
    REQUIRE(results[0].encounterId == "enc-1");
    REQUIRE(results[1].encounterId == "enc-2");
}

TEST_CASE("InMemoryAuditRepository Query sorts ties by encounterId then actor") {
    using namespace std::chrono;
    encounter_service::storage::InMemoryAuditRepository repo;
    const auto t = system_clock::time_point{seconds{10}};
    repo.Append(MakeAudit(t, "z-actor", "enc-b"));
    repo.Append(MakeAudit(t, "b-actor", "enc-a"));
    repo.Append(MakeAudit(t, "a-actor", "enc-a"));

    const auto results = repo.Query({});
    REQUIRE(results.size() == 3);
    REQUIRE(results[0].encounterId == "enc-a");
    REQUIRE(results[0].actor == "a-actor");
    REQUIRE(results[1].encounterId == "enc-a");
    REQUIRE(results[1].actor == "b-actor");
    REQUIRE(results[2].encounterId == "enc-b");
}
