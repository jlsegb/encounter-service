#include "tests/catch_compat.h"

#include <chrono>

#include "src/http/validation.h"
#include "src/util/time.h"

TEST_CASE("ValidateCreateEncounterRequest parses valid payload") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = "patient-1";
    body["providerId"] = "provider-1";
    body["encounterType"] = "visit";
    body["encounterDate"] = "2026-02-25T00:00:00Z";
    body["clinicalData"] = nlohmann::json::object();

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 0);

    const auto parsed = encounter_service::util::ParseIso8601Utc("2026-02-25T00:00:00Z");
    REQUIRE(parsed.has_value());
    REQUIRE(std::get<encounter_service::domain::CreateEncounterInput>(result).encounterDate == *parsed);
}

TEST_CASE("ValidateCreateEncounterRequest requires body object") {
    nlohmann::json body;

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.code == encounter_service::domain::DomainErrorCode::Validation);
    REQUIRE(error.details.has_value());
    REQUIRE(error.details->size() == 1);
    REQUIRE((*error.details)[0].path == "body");
}

TEST_CASE("ValidateCreateEncounterRequest requires patientId") {
    nlohmann::json body = nlohmann::json::object();
    body["providerId"] = "provider-1";
    body["encounterType"] = "visit";
    body["encounterDate"] = "2026-02-25";
    body["clinicalData"] = nlohmann::json::object();

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.code == encounter_service::domain::DomainErrorCode::Validation);
    REQUIRE(error.details.has_value());
    REQUIRE((*error.details)[0].path == "patientId");
}

TEST_CASE("ValidateCreateEncounterRequest requires providerId") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = "patient-1";
    body["encounterType"] = "visit";
    body["encounterDate"] = "2026-02-25";
    body["clinicalData"] = nlohmann::json::object();

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);
    REQUIRE(std::get<encounter_service::domain::DomainError>(result).details->at(0).path == "providerId");
}

TEST_CASE("ValidateCreateEncounterRequest requires encounterType") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = "patient-1";
    body["providerId"] = "provider-1";
    body["encounterDate"] = "2026-02-25";
    body["clinicalData"] = nlohmann::json::object();

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);
    REQUIRE(std::get<encounter_service::domain::DomainError>(result).details->at(0).path == "encounterType");
}

TEST_CASE("ValidateCreateEncounterRequest requires clinicalData") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = "patient-1";
    body["providerId"] = "provider-1";
    body["encounterType"] = "visit";
    body["encounterDate"] = "2026-02-25";

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);
    REQUIRE(std::get<encounter_service::domain::DomainError>(result).details->at(0).path == "clinicalData");
}

TEST_CASE("ValidateCreateEncounterRequest validates patientId type") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = nlohmann::json::object();
    body["providerId"] = "provider-1";
    body["encounterType"] = "visit";
    body["encounterDate"] = "2026-02-25";
    body["clinicalData"] = nlohmann::json::object();

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.details->at(0).path == "patientId");
    REQUIRE(error.details->at(0).message == "must be a string");
}

TEST_CASE("ValidateCreateEncounterRequest validates providerId type") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = "patient-1";
    body["providerId"] = nlohmann::json::object();
    body["encounterType"] = "visit";
    body["encounterDate"] = "2026-02-25";
    body["clinicalData"] = nlohmann::json::object();

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);
    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.details->at(0).path == "providerId");
    REQUIRE(error.details->at(0).message == "must be a string");
}

TEST_CASE("ValidateCreateEncounterRequest validates encounterType type") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = "patient-1";
    body["providerId"] = "provider-1";
    body["encounterType"] = nlohmann::json::object();
    body["encounterDate"] = "2026-02-25";
    body["clinicalData"] = nlohmann::json::object();

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);
    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.details->at(0).path == "encounterType");
    REQUIRE(error.details->at(0).message == "must be a string");
}

TEST_CASE("ValidateCreateEncounterRequest validates encounterDate type") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = "patient-1";
    body["providerId"] = "provider-1";
    body["encounterType"] = "visit";
    body["encounterDate"] = nlohmann::json::object();
    body["clinicalData"] = nlohmann::json::object();

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);
    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.details->at(0).path == "encounterDate");
    REQUIRE(error.details->at(0).message == "must be a string");
}

TEST_CASE("ValidateCreateEncounterRequest validates encounterDate parse") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = "patient-1";
    body["providerId"] = "provider-1";
    body["encounterType"] = "visit";
    body["encounterDate"] = "not-a-date";
    body["clinicalData"] = nlohmann::json::object();

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.code == encounter_service::domain::DomainErrorCode::Validation);
    REQUIRE(error.details.has_value());
    REQUIRE(error.details->at(0).path == "encounterDate");
}

TEST_CASE("ValidateCreateEncounterRequest accepts date only encounterDate") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = "patient-1";
    body["providerId"] = "provider-1";
    body["encounterType"] = "visit";
    body["encounterDate"] = "2026-02-25";
    body["clinicalData"] = nlohmann::json::object();

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 0);
}

TEST_CASE("ValidateCreateEncounterRequest validates clinicalData type") {
    nlohmann::json body = nlohmann::json::object();
    body["patientId"] = "patient-1";
    body["providerId"] = "provider-1";
    body["encounterType"] = "visit";
    body["encounterDate"] = "2026-02-25";
    body["clinicalData"] = "not-an-object";

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 1);
    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.details.has_value());
    REQUIRE(error.details->at(0).path == "clinicalData");
    REQUIRE(error.details->at(0).message == "must be an object");
}

TEST_CASE("ValidateEncounterQuery parses from and to timestamps") {
    httplib::Request request{};
    request.params["patientId"] = "patient-1";
    request.params["providerId"] = "provider-1";
    request.params["from"] = "2026-02-24";
    request.params["to"] = "2026-02-25T01:02:03Z";

    const auto result = encounter_service::http::ValidateEncounterQuery(request);
    REQUIRE(result.index() == 0);

    const auto filters = std::get<encounter_service::storage::EncounterQueryFilters>(result);
    REQUIRE(filters.patientId.has_value());
    REQUIRE(*filters.patientId == "patient-1");
    REQUIRE(filters.providerId.has_value());
    REQUIRE(*filters.providerId == "provider-1");
    REQUIRE(filters.encounterDateFrom.has_value());
    REQUIRE(filters.encounterDateTo.has_value());
}

TEST_CASE("ValidateEncounterQuery parses encounterType passthrough") {
    httplib::Request request{};
    request.params["encounterType"] = "follow-up";

    const auto result = encounter_service::http::ValidateEncounterQuery(request);
    REQUIRE(result.index() == 0);

    const auto filters = std::get<encounter_service::storage::EncounterQueryFilters>(result);
    REQUIRE(filters.encounterType.has_value());
    REQUIRE(*filters.encounterType == "follow-up");
}

TEST_CASE("ValidateEncounterQuery returns validation error for invalid from") {
    httplib::Request request{};
    request.params["from"] = "bad-date";

    const auto result = encounter_service::http::ValidateEncounterQuery(request);
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.code == encounter_service::domain::DomainErrorCode::Validation);
    REQUIRE(error.details.has_value());
    REQUIRE(error.details->at(0).path == "from");
}

TEST_CASE("ValidateEncounterQuery returns validation error for empty from") {
    httplib::Request request{};
    request.params["from"] = "";

    const auto result = encounter_service::http::ValidateEncounterQuery(request);
    REQUIRE(result.index() == 1);
    REQUIRE(std::get<encounter_service::domain::DomainError>(result).details->at(0).path == "from");
}

TEST_CASE("ValidateEncounterQuery returns validation error for invalid to") {
    httplib::Request request{};
    request.params["to"] = "bad-date";

    const auto result = encounter_service::http::ValidateEncounterQuery(request);
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.code == encounter_service::domain::DomainErrorCode::Validation);
    REQUIRE(error.details.has_value());
    REQUIRE(error.details->at(0).path == "to");
}

TEST_CASE("ValidateEncounterQuery returns validation error for empty to") {
    httplib::Request request{};
    request.params["to"] = "";

    const auto result = encounter_service::http::ValidateEncounterQuery(request);
    REQUIRE(result.index() == 1);
    REQUIRE(std::get<encounter_service::domain::DomainError>(result).details->at(0).path == "to");
}

TEST_CASE("ValidateAuditQuery parses from and to timestamps") {
    httplib::Request request{};
    request.params["from"] = "2026-02-24";
    request.params["to"] = "2026-02-25";

    const auto result = encounter_service::http::ValidateAuditQuery(request);
    REQUIRE(result.index() == 0);

    const auto range = std::get<encounter_service::storage::AuditDateRange>(result);
    REQUIRE(range.from.has_value());
    REQUIRE(range.to.has_value());
}

TEST_CASE("ValidateAuditQuery accepts datetime Z values") {
    httplib::Request request{};
    request.params["from"] = "2026-02-25T01:02:03Z";

    const auto result = encounter_service::http::ValidateAuditQuery(request);
    REQUIRE(result.index() == 0);
    REQUIRE(std::get<encounter_service::storage::AuditDateRange>(result).from.has_value());
}

TEST_CASE("ValidateAuditQuery returns validation error for invalid from") {
    httplib::Request request{};
    request.params["from"] = "invalid";

    const auto result = encounter_service::http::ValidateAuditQuery(request);
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.code == encounter_service::domain::DomainErrorCode::Validation);
    REQUIRE(error.details.has_value());
    REQUIRE(error.details->at(0).path == "from");
}

TEST_CASE("ValidateAuditQuery returns validation error for empty from") {
    httplib::Request request{};
    request.params["from"] = "";

    const auto result = encounter_service::http::ValidateAuditQuery(request);
    REQUIRE(result.index() == 1);
    REQUIRE(std::get<encounter_service::domain::DomainError>(result).details->at(0).path == "from");
}

TEST_CASE("ValidateAuditQuery returns validation error for invalid to") {
    httplib::Request request{};
    request.params["to"] = "invalid";

    const auto result = encounter_service::http::ValidateAuditQuery(request);
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.code == encounter_service::domain::DomainErrorCode::Validation);
    REQUIRE(error.details.has_value());
    REQUIRE(error.details->at(0).path == "to");
}

TEST_CASE("ValidateAuditQuery returns validation error for empty to") {
    httplib::Request request{};
    request.params["to"] = "";

    const auto result = encounter_service::http::ValidateAuditQuery(request);
    REQUIRE(result.index() == 1);
    REQUIRE(std::get<encounter_service::domain::DomainError>(result).details->at(0).path == "to");
}
