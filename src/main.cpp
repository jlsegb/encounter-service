#include "src/domain/encounter_service.h"
#include "src/http/routes.h"
#include "src/storage/in_memory_audit_repo.h"
#include "src/storage/in_memory_encounter_repo.h"
#include "src/util/clock.h"
#include "src/util/id_generator.h"
#include "src/util/logger.h"
#include "src/util/redaction.h"

int main() {
    encounter_service::storage::InMemoryEncounterRepository encounter_repo;
    encounter_service::storage::InMemoryAuditRepository audit_repo;
    encounter_service::util::SystemClock clock;
    encounter_service::util::DefaultIdGenerator id_generator("enc");
    encounter_service::util::StdoutLogger logger;
    encounter_service::util::BasicRedactor redactor;

    encounter_service::domain::DefaultEncounterService service(
        encounter_repo,
        audit_repo,
        clock,
        id_generator);

    httplib::Server server;
    encounter_service::http::RegisterRoutes(server, service, logger, redactor);

    logger.Log(encounter_service::util::LogLevel::Info, "Starting Encounter Service on port 8080");
    const bool ok = server.listen("0.0.0.0", 8080);
    return ok ? 0 : 1;
}
