#include "src/http/validation.h"

namespace encounter_service::http {

std::variant<domain::CreateEncounterInput, domain::DomainError>
ValidateCreateEncounterRequest(const nlohmann::json& body) {
    domain::CreateEncounterInput input{};
    input.clinicalData = body;
    return input;
}

std::variant<storage::EncounterQueryFilters, domain::DomainError>
ValidateEncounterQuery(const httplib::Request& request) {
    storage::EncounterQueryFilters filters{};
    (void)request;
    return filters;
}

std::variant<storage::AuditDateRange, domain::DomainError>
ValidateAuditQuery(const httplib::Request& request) {
    storage::AuditDateRange range{};
    (void)request;
    return range;
}

}  // namespace encounter_service::http
