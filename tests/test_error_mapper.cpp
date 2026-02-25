#include "tests/catch_compat.h"

#include "src/http/error_mapper.h"

TEST_CASE("MapDomainError maps validation to 400 with envelope fields") {
    encounter_service::domain::DomainError error{
        .code = encounter_service::domain::DomainErrorCode::Validation,
        .message = "Request validation failed",
        .details = std::vector<encounter_service::domain::FieldError>{
            {"encounterDate", "must be YYYY-MM-DD or YYYY-MM-DDTHH:MM:SSZ"}
        }
    };

    const auto mapped = encounter_service::http::MapDomainError(error);
    REQUIRE(mapped.status == 400);

    const auto body = mapped.body.dump();
    REQUIRE(body.find("\"error\"") != std::string::npos);
    REQUIRE(body.find("\"code\":\"validation_error\"") != std::string::npos);
    REQUIRE(body.find("\"message\":\"Request validation failed\"") != std::string::npos);
    REQUIRE(body.find("\"details\"") != std::string::npos);
    REQUIRE(body.find("\"path\":\"encounterDate\"") != std::string::npos);
}

TEST_CASE("MapDomainError omits details when none are present") {
    encounter_service::domain::DomainError error{
        .code = encounter_service::domain::DomainErrorCode::NotFound,
        .message = "Encounter not found",
        .details = std::nullopt
    };

    const auto mapped = encounter_service::http::MapDomainError(error);
    REQUIRE(mapped.status == 404);

    const auto body = mapped.body.dump();
    REQUIRE(body.find("\"code\":\"not_found\"") != std::string::npos);
    REQUIRE(body.find("\"details\"") == std::string::npos);
}

TEST_CASE("MapDomainError includes requestId when provided") {
    encounter_service::domain::DomainError error{
        .code = encounter_service::domain::DomainErrorCode::Unauthorized,
        .message = "Unauthorized",
        .details = std::nullopt
    };

    const auto mapped = encounter_service::http::MapDomainError(error, std::string("req-123"));
    REQUIRE(mapped.status == 401);
    REQUIRE(mapped.body.dump().find("\"requestId\":\"req-123\"") != std::string::npos);
}

TEST_CASE("MapDomainError maps unauthorized to 401 without details") {
    encounter_service::domain::DomainError error{
        .code = encounter_service::domain::DomainErrorCode::Unauthorized,
        .message = "Missing API key",
        .details = std::nullopt
    };

    const auto mapped = encounter_service::http::MapDomainError(error);
    REQUIRE(mapped.status == 401);
    const auto body = mapped.body.dump();
    REQUIRE(body.find("\"code\":\"unauthorized\"") != std::string::npos);
    REQUIRE(body.find("\"details\"") == std::string::npos);
}

TEST_CASE("MapDomainError serializes multiple details") {
    encounter_service::domain::DomainError error{
        .code = encounter_service::domain::DomainErrorCode::Validation,
        .message = "Request validation failed",
        .details = std::vector<encounter_service::domain::FieldError>{
            {"patientId", "is required"},
            {"encounterDate", "must be YYYY-MM-DD"}
        }
    };

    const auto mapped = encounter_service::http::MapDomainError(error);
    REQUIRE(mapped.status == 400);
    const auto body = mapped.body.dump();
    REQUIRE(body.find("\"path\":\"patientId\"") != std::string::npos);
    REQUIRE(body.find("\"path\":\"encounterDate\"") != std::string::npos);
}

TEST_CASE("MapDomainError maps internal to 500") {
    encounter_service::domain::DomainError error{
        .code = encounter_service::domain::DomainErrorCode::Internal,
        .message = "Internal error",
        .details = std::nullopt
    };

    const auto mapped = encounter_service::http::MapDomainError(error);
    REQUIRE(mapped.status == 500);
    REQUIRE(mapped.body.dump().find("\"code\":\"internal_error\"") != std::string::npos);
}
