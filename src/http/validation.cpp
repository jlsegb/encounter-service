#include "src/http/validation.h"

#include <optional>
#include <string>
#include <vector>

#include "src/util/time.h"

namespace encounter_service::http {

namespace {

domain::DomainError ValidationError(std::string path, std::string message) {
    return domain::DomainError{
        .code = domain::DomainErrorCode::Validation,
        .message = "Request validation failed",
        .details = std::vector<domain::FieldError>{domain::FieldError{.path = std::move(path), .message = std::move(message)}}
    };
}

std::optional<std::string> GetOptionalStringField(const nlohmann::json& body, const std::string& key) {
    if (!body.is_object() || !body.contains(key)) {
        return std::nullopt;
    }
    const auto& value = body.at(key);
    if (!value.is_string()) {
        return std::nullopt;
    }
    return value.get<std::string>();
}

std::variant<std::string, domain::DomainError>
GetRequiredStringField(const nlohmann::json& body, const std::string& key) {
    if (!body.contains(key)) {
        return ValidationError(key, "is required");
    }
    const auto& value = body.at(key);
    if (!value.is_string()) {
        return ValidationError(key, "must be a string");
    }
    return value.get<std::string>();
}

std::variant<std::optional<std::chrono::system_clock::time_point>, domain::DomainError>
ParseOptionalQueryTime(const httplib::Request& request, const std::string& paramName) {
    if (!request.has_param(paramName)) {
        return std::optional<std::chrono::system_clock::time_point>{};
    }

    const auto raw = request.get_param_value(paramName);
    const auto parsed = util::ParseIso8601Utc(raw);
    if (!parsed) {
        return ValidationError(paramName, "must be YYYY-MM-DD or YYYY-MM-DDTHH:MM:SSZ");
    }
    return parsed;
}

}  // namespace

std::variant<domain::CreateEncounterInput, domain::DomainError>
ValidateCreateEncounterRequest(const nlohmann::json& body) {
    if (!body.is_object()) {
        return ValidationError("body", "must be a JSON object");
    }
    if (!body.contains("clinicalData")) {
        return ValidationError("clinicalData", "is required");
    }
    if (!body.at("clinicalData").is_object()) {
        return ValidationError("clinicalData", "must be an object");
    }

    domain::CreateEncounterInput input{};
    auto patientId = GetRequiredStringField(body, "patientId");
    if (std::holds_alternative<domain::DomainError>(patientId)) {
        return std::get<domain::DomainError>(std::move(patientId));
    }
    input.patientId = std::get<std::string>(std::move(patientId));

    auto providerId = GetRequiredStringField(body, "providerId");
    if (std::holds_alternative<domain::DomainError>(providerId)) {
        return std::get<domain::DomainError>(std::move(providerId));
    }
    input.providerId = std::get<std::string>(std::move(providerId));

    auto encounterType = GetRequiredStringField(body, "encounterType");
    if (std::holds_alternative<domain::DomainError>(encounterType)) {
        return std::get<domain::DomainError>(std::move(encounterType));
    }
    input.encounterType = std::get<std::string>(std::move(encounterType));

    auto encounterDateRaw = GetRequiredStringField(body, "encounterDate");
    if (std::holds_alternative<domain::DomainError>(encounterDateRaw)) {
        return std::get<domain::DomainError>(std::move(encounterDateRaw));
    }

    const auto parsedEncounterDate = util::ParseIso8601Utc(std::get<std::string>(encounterDateRaw));
    if (!parsedEncounterDate) {
        return ValidationError("encounterDate", "must be YYYY-MM-DD or YYYY-MM-DDTHH:MM:SSZ");
    }

    input.encounterDate = *parsedEncounterDate;
    input.clinicalData = body.at("clinicalData");
    return input;
}

std::variant<storage::EncounterQueryFilters, domain::DomainError>
ValidateEncounterQuery(const httplib::Request& request) {
    storage::EncounterQueryFilters filters{};
    if (request.has_param("patientId")) {
        filters.patientId = request.get_param_value("patientId");
    }
    if (request.has_param("providerId")) {
        filters.providerId = request.get_param_value("providerId");
    }
    if (request.has_param("encounterType")) {
        filters.encounterType = request.get_param_value("encounterType");
    }

    auto from = ParseOptionalQueryTime(request, "from");
    if (std::holds_alternative<domain::DomainError>(from)) {
        return std::get<domain::DomainError>(std::move(from));
    }
    filters.encounterDateFrom = std::get<std::optional<std::chrono::system_clock::time_point>>(std::move(from));

    auto to = ParseOptionalQueryTime(request, "to");
    if (std::holds_alternative<domain::DomainError>(to)) {
        return std::get<domain::DomainError>(std::move(to));
    }
    filters.encounterDateTo = std::get<std::optional<std::chrono::system_clock::time_point>>(std::move(to));

    return filters;
}

std::variant<storage::AuditDateRange, domain::DomainError>
ValidateAuditQuery(const httplib::Request& request) {
    storage::AuditDateRange range{};

    auto from = ParseOptionalQueryTime(request, "from");
    if (std::holds_alternative<domain::DomainError>(from)) {
        return std::get<domain::DomainError>(std::move(from));
    }
    range.from = std::get<std::optional<std::chrono::system_clock::time_point>>(std::move(from));

    auto to = ParseOptionalQueryTime(request, "to");
    if (std::holds_alternative<domain::DomainError>(to)) {
        return std::get<domain::DomainError>(std::move(to));
    }
    range.to = std::get<std::optional<std::chrono::system_clock::time_point>>(std::move(to));

    return range;
}

}  // namespace encounter_service::http
