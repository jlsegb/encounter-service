#pragma once

#include <string>
#include <variant>

#include "src/domain/encounter_service.h"
#include "src/domain/errors.h"
#include "src/http/httplib_compat.h"
#include "src/storage/audit_repo.h"
#include "src/storage/encounter_repo.h"
#include "src/util/json_compat.h"

namespace encounter_service::http {

// Validates POST /encounters JSON and converts it to service-layer input.
// Returns Validation DomainError when required fields are missing, typed incorrectly,
// or contain invalid date/time values.
std::variant<domain::CreateEncounterInput, domain::DomainError>
ValidateCreateEncounterRequest(const nlohmann::json& body);

// Validates and parses GET /encounters query parameters into repository filters.
std::variant<storage::EncounterQueryFilters, domain::DomainError>
ValidateEncounterQuery(const httplib::Request& request);

// Validates and parses GET /audit/encounters range query parameters.
std::variant<storage::AuditDateRange, domain::DomainError>
ValidateAuditQuery(const httplib::Request& request);

}  // namespace encounter_service::http
