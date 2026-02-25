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

std::variant<domain::CreateEncounterInput, domain::DomainError>
ValidateCreateEncounterRequest(const nlohmann::json& body);

std::variant<storage::EncounterQueryFilters, domain::DomainError>
ValidateEncounterQuery(const httplib::Request& request);

std::variant<storage::AuditDateRange, domain::DomainError>
ValidateAuditQuery(const httplib::Request& request);

}  // namespace encounter_service::http
