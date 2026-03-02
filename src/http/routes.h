#pragma once

#include "src/domain/encounter_service.h"
#include "src/http/httplib_compat.h"
#include "src/util/logger.h"
#include "src/util/redaction.h"

namespace encounter_service::http {

// Registers all HTTP handlers on `server`.
// The server stores handlers that reference `encounterService`, `logger`, and `redactor`;
// these dependencies must outlive request handling.
void RegisterRoutes(httplib::Server& server,
                    domain::EncounterService& encounterService,
                    util::Logger& logger,
                    util::Redactor& redactor);

}  // namespace encounter_service::http
