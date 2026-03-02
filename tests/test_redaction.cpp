#include "tests/catch_compat.h"

#include "src/util/redaction.h"

TEST_CASE("BasicRedactor redacts top-level patientId") {
    encounter_service::util::BasicRedactor redactor;
    nlohmann::json input = nlohmann::json::object();
    input["patientId"] = "12345";
    input["encounterType"] = "visit";

    const auto output = redactor.RedactJson(input);
    REQUIRE(output.at("patientId").get<std::string>() == "[REDACTED]");
    REQUIRE(output.at("encounterType").get<std::string>() == "visit");
}

TEST_CASE("BasicRedactor redacts top-level clinicalData key") {
    encounter_service::util::BasicRedactor redactor;
    nlohmann::json input = nlohmann::json::object();
    input["clinicalData"] = nlohmann::json::object({{"note", "sensitive"}});
    input["providerId"] = "provider-1";

    const auto output = redactor.RedactJson(input);
    REQUIRE(output.at("clinicalData").get<std::string>() == "[REDACTED]");
    REQUIRE(output.at("providerId").get<std::string>() == "provider-1");
}

TEST_CASE("BasicRedactor redacts top-level PHI keys with mixed casing") {
    encounter_service::util::BasicRedactor redactor;
    nlohmann::json input = nlohmann::json::object();
    input["Patient_ID"] = "P123";
    input["Clinical_Data"] = nlohmann::json::object({{"free_text", "secret"}});
    input["note"] = "ok";

    const auto output = redactor.RedactJson(input);
    REQUIRE(output.at("Patient_ID").get<std::string>() == "[REDACTED]");
    REQUIRE(output.at("Clinical_Data").get<std::string>() == "[REDACTED]");
    REQUIRE(output.at("note").get<std::string>() == "ok");
}

TEST_CASE("BasicRedactor does not mutate input object") {
    encounter_service::util::BasicRedactor redactor;
    nlohmann::json input = nlohmann::json::object();
    input["patientId"] = "P999";

    const auto output = redactor.RedactJson(input);
    REQUIRE(output.at("patientId").get<std::string>() == "[REDACTED]");
    REQUIRE(input.at("patientId").get<std::string>() == "P999");
}
