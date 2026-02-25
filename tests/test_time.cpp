#include "tests/catch_compat.h"

#include <chrono>

#include "src/util/time.h"

TEST_CASE("ParseIso8601Utc parses date-only as midnight UTC") {
    const auto parsed = encounter_service::util::ParseIso8601Utc("2026-02-25");
    REQUIRE(parsed.has_value());
    REQUIRE(encounter_service::util::FormatIso8601Utc(*parsed) == "2026-02-25T00:00:00Z");
}

TEST_CASE("ParseIso8601Utc parses full UTC timestamp") {
    const auto parsed = encounter_service::util::ParseIso8601Utc("2026-02-25T01:02:03Z");
    REQUIRE(parsed.has_value());
    REQUIRE(encounter_service::util::FormatIso8601Utc(*parsed) == "2026-02-25T01:02:03Z");
}

TEST_CASE("ParseIso8601Utc rejects invalid separators") {
    REQUIRE(!encounter_service::util::ParseIso8601Utc("2026/02/25").has_value());
}

TEST_CASE("ParseIso8601Utc rejects missing Z suffix for datetime") {
    REQUIRE(!encounter_service::util::ParseIso8601Utc("2026-02-25T01:02:03").has_value());
}

TEST_CASE("ParseIso8601Utc rejects malformed length") {
    REQUIRE(!encounter_service::util::ParseIso8601Utc("2026-02-2").has_value());
}

TEST_CASE("FormatIso8601Utc round-trips parsed value") {
    using namespace std::chrono;
    const auto tp = system_clock::time_point{seconds{1700000000}};
    const auto formatted = encounter_service::util::FormatIso8601Utc(tp);
    const auto parsed = encounter_service::util::ParseIso8601Utc(formatted);
    REQUIRE(parsed.has_value());
    REQUIRE(encounter_service::util::FormatIso8601Utc(*parsed) == formatted);
}
