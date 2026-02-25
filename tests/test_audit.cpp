#define ENCOUNTER_SERVICE_CATCH_COMPAT_MAIN
#include "tests/catch_compat.h"

#include <chrono>

#include "src/storage/in_memory_audit_repo.h"

TEST_CASE("Audit repository skeleton compiles") {
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
