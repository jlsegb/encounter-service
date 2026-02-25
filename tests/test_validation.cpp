#include "tests/catch_compat.h"

#include "src/http/validation.h"

TEST_CASE("Validation skeleton compiles") {
    nlohmann::json body = nlohmann::json::object();
    body["encounterType"] = "visit";

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 0);
}
