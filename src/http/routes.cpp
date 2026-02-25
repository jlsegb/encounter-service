#include "src/http/routes.h"

#include <variant>

#include "src/http/auth.h"
#include "src/http/error_mapper.h"
#include "src/http/validation.h"
#include "src/util/json_compat.h"
#include "src/util/time.h"

namespace encounter_service::http {

namespace {

void WriteJson(httplib::Response& res, int status, const nlohmann::json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

nlohmann::json NotImplementedBody(const std::string& endpoint) {
    nlohmann::json json = nlohmann::json::object();
    json["error"] = "Not Implemented";
    json["endpoint"] = endpoint;
    return json;
}

nlohmann::json EncounterToJson(const domain::Encounter& encounter) {
    nlohmann::json json = nlohmann::json::object();
    json["encounterId"] = encounter.encounterId;
    json["patientId"] = encounter.patientId;
    json["providerId"] = encounter.providerId;
    json["encounterDate"] = util::FormatIso8601Utc(encounter.encounterDate);
    json["encounterType"] = encounter.encounterType;

    nlohmann::json metadata = nlohmann::json::object();
    metadata["createdAt"] = util::FormatIso8601Utc(encounter.metadata.createdAt);
    metadata["updatedAt"] = util::FormatIso8601Utc(encounter.metadata.updatedAt);
    metadata["createdBy"] = encounter.metadata.createdBy;
    json["metadata"] = metadata;
    return json;
}

}  // namespace

void RegisterRoutes(httplib::Server& server,
                    domain::EncounterService& encounterService,
                    util::Logger& logger,
                    util::Redactor& redactor) {
    (void)encounterService;
    (void)redactor;
    (void)EncounterToJson;

    server.Get("/health", [&logger](const httplib::Request&, httplib::Response& res) {
        logger.Log(util::LogLevel::Info, "GET /health");
        nlohmann::json body = nlohmann::json::object();
        body["status"] = "ok";
        WriteJson(res, 200, body);
    });

    server.Post("/encounters", [&logger](const httplib::Request& req, httplib::Response& res) {
        logger.Log(util::LogLevel::Info, "POST /encounters");
        (void)Authenticate(req);
        WriteJson(res, 501, NotImplementedBody("/encounters"));
    });

    server.Get("/encounters/:encounterId", [&logger](const httplib::Request& req, httplib::Response& res) {
        logger.Log(util::LogLevel::Info, "GET /encounters/:encounterId");
        (void)Authenticate(req);
        WriteJson(res, 501, NotImplementedBody("/encounters/:encounterId"));
    });

    server.Get("/encounters", [&logger](const httplib::Request& req, httplib::Response& res) {
        logger.Log(util::LogLevel::Info, "GET /encounters");
        (void)Authenticate(req);
        WriteJson(res, 501, NotImplementedBody("/encounters"));
    });

    server.Get("/audit/encounters", [&logger](const httplib::Request& req, httplib::Response& res) {
        logger.Log(util::LogLevel::Info, "GET /audit/encounters");
        (void)Authenticate(req);
        WriteJson(res, 501, NotImplementedBody("/audit/encounters"));
    });
}

}  // namespace encounter_service::http
