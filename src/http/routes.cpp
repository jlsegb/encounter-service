#include "src/http/routes.h"

#include <optional>
#include <string>
#include <variant>

#include "src/http/auth.h"
#include "src/http/error_mapper.h"
#include "src/http/validation.h"
#include "src/util/json_compat.h"
#include "src/util/time.h"

namespace encounter_service::http {

namespace {

std::optional<std::string> GetRequestId(const httplib::Request& req) {
    if (!req.has_header("X-Request-Id")) {
        return std::nullopt;
    }
    const auto value = req.get_header_value("X-Request-Id");
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
}

void WriteJson(httplib::Response& res, int status, const nlohmann::json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void WriteDomainError(httplib::Response& res,
                      const domain::DomainError& error,
                      std::optional<std::string> requestId) {
    const auto mapped = MapDomainError(error, requestId);
    WriteJson(res, mapped.status, mapped.body);
}

void LogHttpResult(util::Logger& logger,
                   util::Redactor& redactor,
                   const std::string& method,
                   const std::string& path,
                   std::optional<std::string> requestId,
                   int status) {
    nlohmann::json log = nlohmann::json::object();
    log["method"] = method;
    log["path"] = path;
    log["status"] = std::to_string(status);
    if (requestId) {
        log["requestId"] = *requestId;
    }
    const auto redacted = redactor.RedactJson(log);
    logger.Log(util::LogLevel::Info, redacted.dump());
}

std::optional<std::string> RegexCaptureEncounterId(const httplib::Request& req) {
    if (req.matches.size() < 2) {
        return std::nullopt;
    }
    return req.matches[1].str();
}

std::variant<nlohmann::json, domain::DomainError> ParseRequestJson(const httplib::Request& req) {
#if __has_include("vendor/json.hpp")
    try {
        return nlohmann::json::parse(req.body);
    } catch (...) {
        return domain::DomainError{
            .code = domain::DomainErrorCode::Validation,
            .message = "Request validation failed",
            .details = std::vector<domain::FieldError>{domain::FieldError{
                .path = "body",
                .message = "must be valid JSON"
            }}
        };
    }
#else
    (void)req;
    return domain::DomainError{
        .code = domain::DomainErrorCode::Validation,
        .message = "Request validation failed",
        .details = std::vector<domain::FieldError>{domain::FieldError{
            .path = "body",
            .message = "JSON parsing requires vendor/json.hpp"
        }}
    };
#endif
}

nlohmann::json EncounterToJson(const domain::Encounter& encounter) {
    nlohmann::json json = nlohmann::json::object();
    json["encounterId"] = encounter.encounterId;
    json["patientId"] = encounter.patientId;
    json["providerId"] = encounter.providerId;
    json["encounterDate"] = util::FormatIso8601Utc(encounter.encounterDate);
    json["encounterType"] = encounter.encounterType;
    json["clinicalData"] = encounter.clinicalData;

    nlohmann::json metadata = nlohmann::json::object();
    metadata["createdAt"] = util::FormatIso8601Utc(encounter.metadata.createdAt);
    metadata["updatedAt"] = util::FormatIso8601Utc(encounter.metadata.updatedAt);
    metadata["createdBy"] = encounter.metadata.createdBy;
    json["metadata"] = metadata;
    return json;
}

nlohmann::json AuditEntryToJson(const domain::AuditEntry& entry) {
    nlohmann::json json = nlohmann::json::object();
    json["timestamp"] = util::FormatIso8601Utc(entry.timestamp);
    json["actor"] = entry.actor;
    json["encounterId"] = entry.encounterId;
    switch (entry.action) {
        case domain::AuditAction::READ_ENCOUNTER:
            json["action"] = "READ_ENCOUNTER";
            break;
        case domain::AuditAction::CREATE_ENCOUNTER:
            json["action"] = "CREATE_ENCOUNTER";
            break;
    }
    return json;
}

nlohmann::json EncounterListToJson(const std::vector<domain::Encounter>& encounters) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& encounter : encounters) {
        arr.push_back(EncounterToJson(encounter));
    }
    return arr;
}

nlohmann::json AuditListToJson(const std::vector<domain::AuditEntry>& entries) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& entry : entries) {
        arr.push_back(AuditEntryToJson(entry));
    }
    return arr;
}

}  // namespace

void RegisterRoutes(httplib::Server& server,
                    domain::EncounterService& encounterService,
                    util::Logger& logger,
                    util::Redactor& redactor) {
    auto* service = &encounterService;
    auto* log = &logger;
    auto* redact = &redactor;

    server.Get("/health", [log](const httplib::Request&, httplib::Response& res) {
        log->Log(util::LogLevel::Info, "GET /health");
        nlohmann::json body = nlohmann::json::object();
        body["status"] = "ok";
        WriteJson(res, 200, body);
    });

    server.Post("/encounters", [service, log, redact](const httplib::Request& req, httplib::Response& res) {
        const auto requestId = GetRequestId(req);

        const auto auth = Authenticate(req);
        if (std::holds_alternative<domain::DomainError>(auth)) {
            WriteDomainError(res, std::get<domain::DomainError>(auth), requestId);
            LogHttpResult(*log, *redact, "POST", "/encounters", requestId, res.status);
            return;
        }
        const auto actor = std::get<std::string>(auth);

        const auto parsedBody = ParseRequestJson(req);
        if (std::holds_alternative<domain::DomainError>(parsedBody)) {
            WriteDomainError(res, std::get<domain::DomainError>(parsedBody), requestId);
            LogHttpResult(*log, *redact, "POST", "/encounters", requestId, res.status);
            return;
        }

        const auto validation = ValidateCreateEncounterRequest(std::get<nlohmann::json>(parsedBody));
        if (std::holds_alternative<domain::DomainError>(validation)) {
            WriteDomainError(res, std::get<domain::DomainError>(validation), requestId);
            LogHttpResult(*log, *redact, "POST", "/encounters", requestId, res.status);
            return;
        }

        const auto serviceResult = service->CreateEncounter(
            std::get<domain::CreateEncounterInput>(validation), actor);
        if (std::holds_alternative<domain::DomainError>(serviceResult)) {
            WriteDomainError(res, std::get<domain::DomainError>(serviceResult), requestId);
            LogHttpResult(*log, *redact, "POST", "/encounters", requestId, res.status);
            return;
        }

        WriteJson(res, 201, EncounterToJson(std::get<domain::Encounter>(serviceResult)));
        LogHttpResult(*log, *redact, "POST", "/encounters", requestId, res.status);
    });

    server.Get(R"(/encounters/([A-Za-z0-9_-]+))", [service, log, redact](const httplib::Request& req, httplib::Response& res) {
        const auto requestId = GetRequestId(req);

        const auto auth = Authenticate(req);
        if (std::holds_alternative<domain::DomainError>(auth)) {
            WriteDomainError(res, std::get<domain::DomainError>(auth), requestId);
            LogHttpResult(*log, *redact, "GET", "/encounters/:encounterId", requestId, res.status);
            return;
        }
        const auto actor = std::get<std::string>(auth);

        const auto encounterId = RegexCaptureEncounterId(req);
        if (!encounterId) {
            WriteDomainError(res,
                             domain::DomainError{.code = domain::DomainErrorCode::Internal,
                                                .message = "Internal error",
                                                .details = std::nullopt},
                             requestId);
            LogHttpResult(*log, *redact, "GET", "/encounters/:encounterId", requestId, res.status);
            return;
        }

        const auto serviceResult = service->GetEncounter(*encounterId, actor);
        if (std::holds_alternative<domain::DomainError>(serviceResult)) {
            WriteDomainError(res, std::get<domain::DomainError>(serviceResult), requestId);
            LogHttpResult(*log, *redact, "GET", "/encounters/:encounterId", requestId, res.status);
            return;
        }

        WriteJson(res, 200, EncounterToJson(std::get<domain::Encounter>(serviceResult)));
        LogHttpResult(*log, *redact, "GET", "/encounters/:encounterId", requestId, res.status);
    });

    server.Get("/encounters", [service, log, redact](const httplib::Request& req, httplib::Response& res) {
        const auto requestId = GetRequestId(req);

        const auto auth = Authenticate(req);
        if (std::holds_alternative<domain::DomainError>(auth)) {
            WriteDomainError(res, std::get<domain::DomainError>(auth), requestId);
            LogHttpResult(*log, *redact, "GET", "/encounters", requestId, res.status);
            return;
        }
        (void)std::get<std::string>(auth);

        const auto validation = ValidateEncounterQuery(req);
        if (std::holds_alternative<domain::DomainError>(validation)) {
            WriteDomainError(res, std::get<domain::DomainError>(validation), requestId);
            LogHttpResult(*log, *redact, "GET", "/encounters", requestId, res.status);
            return;
        }

        const auto serviceResult = service->QueryEncounters(std::get<storage::EncounterQueryFilters>(validation));
        if (std::holds_alternative<domain::DomainError>(serviceResult)) {
            WriteDomainError(res, std::get<domain::DomainError>(serviceResult), requestId);
            LogHttpResult(*log, *redact, "GET", "/encounters", requestId, res.status);
            return;
        }

        WriteJson(res, 200, EncounterListToJson(std::get<std::vector<domain::Encounter>>(serviceResult)));
        LogHttpResult(*log, *redact, "GET", "/encounters", requestId, res.status);
    });

    server.Get("/audit/encounters", [service, log, redact](const httplib::Request& req, httplib::Response& res) {
        const auto requestId = GetRequestId(req);

        const auto auth = Authenticate(req);
        if (std::holds_alternative<domain::DomainError>(auth)) {
            WriteDomainError(res, std::get<domain::DomainError>(auth), requestId);
            LogHttpResult(*log, *redact, "GET", "/audit/encounters", requestId, res.status);
            return;
        }
        (void)std::get<std::string>(auth);

        const auto validation = ValidateAuditQuery(req);
        if (std::holds_alternative<domain::DomainError>(validation)) {
            WriteDomainError(res, std::get<domain::DomainError>(validation), requestId);
            LogHttpResult(*log, *redact, "GET", "/audit/encounters", requestId, res.status);
            return;
        }

        const auto serviceResult = service->QueryAudit(std::get<storage::AuditDateRange>(validation));
        if (std::holds_alternative<domain::DomainError>(serviceResult)) {
            WriteDomainError(res, std::get<domain::DomainError>(serviceResult), requestId);
            LogHttpResult(*log, *redact, "GET", "/audit/encounters", requestId, res.status);
            return;
        }

        WriteJson(res, 200, AuditListToJson(std::get<std::vector<domain::AuditEntry>>(serviceResult)));
        LogHttpResult(*log, *redact, "GET", "/audit/encounters", requestId, res.status);
    });
}

}  // namespace encounter_service::http
