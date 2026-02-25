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

TEST_CASE("BasicRedactor redacts nested PHI keys recursively") {
    encounter_service::util::BasicRedactor redactor;
    nlohmann::json input = nlohmann::json::object();
    input["clinicalData"] = nlohmann::json::object();
    input["clinicalData"]["name"] = "Jane Doe";
    input["clinicalData"]["dob"] = "1985-01-01";
    input["clinicalData"]["observation"] = "stable";
    input["clinicalData"]["nested"] = nlohmann::json::object();
    input["clinicalData"]["nested"]["full_name"] = "Jane Q. Doe";

    const auto output = redactor.RedactJson(input);
    REQUIRE(output.at("clinicalData").at("name").get<std::string>() == "[REDACTED]");
    REQUIRE(output.at("clinicalData").at("dob").get<std::string>() == "[REDACTED]");
    REQUIRE(output.at("clinicalData").at("observation").get<std::string>() == "stable");
    REQUIRE(output.at("clinicalData").at("nested").at("full_name").get<std::string>() == "[REDACTED]");
}

TEST_CASE("BasicRedactor redacts PHI keys inside arrays and mixed-case keys") {
    encounter_service::util::BasicRedactor redactor;
    nlohmann::json input = nlohmann::json::object();
    input["entries"] = nlohmann::json::array();
    input["entries"].push_back(nlohmann::json::object());
    input["entries"][0]["Name"] = "John";
    input["entries"][0]["DOB"] = "1990-10-10";
    input["entries"][0]["note"] = "ok";
    input["Patient_ID"] = "P123";

    const auto output = redactor.RedactJson(input);
    REQUIRE(output.at("Patient_ID").get<std::string>() == "[REDACTED]");
    REQUIRE(output.at("entries")[0].at("Name").get<std::string>() == "[REDACTED]");
    REQUIRE(output.at("entries")[0].at("DOB").get<std::string>() == "[REDACTED]");
    REQUIRE(output.at("entries")[0].at("note").get<std::string>() == "ok");
}

TEST_CASE("BasicRedactor does not mutate input object") {
    encounter_service::util::BasicRedactor redactor;
    nlohmann::json input = nlohmann::json::object();
    input["patientId"] = "P999";

    const auto output = redactor.RedactJson(input);
    REQUIRE(output.at("patientId").get<std::string>() == "[REDACTED]");
    REQUIRE(input.at("patientId").get<std::string>() == "P999");
}
