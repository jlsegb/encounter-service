#include "tests/catch_compat.h"

#include <chrono>

#include "src/http/validation.h"
#include "src/util/time.h"

TEST_CASE("Validation skeleton compiles") {
    nlohmann::json body = nlohmann::json::object();
    body["encounterType"] = "visit";
    body["encounterDate"] = "2026-02-25T00:00:00Z";

    const auto result = encounter_service::http::ValidateCreateEncounterRequest(body);
    REQUIRE(result.index() == 0);

    const auto parsed = encounter_service::util::ParseIso8601Utc("2026-02-25T00:00:00Z");
    REQUIRE(parsed.has_value());
    REQUIRE(std::get<encounter_service::domain::CreateEncounterInput>(result).encounterDate == *parsed);
}
