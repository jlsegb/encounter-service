#include "tests/catch_compat.h"

#include "src/util/redaction.h"

TEST_CASE("Redaction skeleton compiles") {
    encounter_service::util::BasicRedactor redactor;
    nlohmann::json input = nlohmann::json::object();
    input["patientId"] = "12345";

    const auto output = redactor.RedactJson(input);
    (void)output;

    SUCCEED("TODO: add recursive PHI redaction assertions");
}
