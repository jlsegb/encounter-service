#include "tests/catch_compat.h"

#include "src/http/auth.h"

TEST_CASE("Authenticate returns unauthorized when API key is missing") {
    httplib::Request req{};

    const auto result = encounter_service::http::Authenticate(req);
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.code == encounter_service::domain::DomainErrorCode::Unauthorized);
    REQUIRE(error.message == "Missing API key");
}

TEST_CASE("Authenticate returns unauthorized when API key is empty") {
    httplib::Request req{};
    req.headers["X-API-Key"] = "";

    const auto result = encounter_service::http::Authenticate(req);
    REQUIRE(result.index() == 1);

    const auto error = std::get<encounter_service::domain::DomainError>(result);
    REQUIRE(error.code == encounter_service::domain::DomainErrorCode::Unauthorized);
    REQUIRE(error.message == "Invalid API key");
}

TEST_CASE("Authenticate returns actor when API key is present") {
    httplib::Request req{};
    req.headers["X-API-Key"] = "test-key";

    const auto result = encounter_service::http::Authenticate(req);
    REQUIRE(result.index() == 0);
    REQUIRE(std::get<std::string>(result) == "api-key-actor");
}
