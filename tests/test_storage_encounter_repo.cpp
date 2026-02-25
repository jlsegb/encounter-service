#include "tests/catch_compat.h"

#include <chrono>

#include "src/storage/in_memory_encounter_repo.h"

namespace {

encounter_service::domain::Encounter MakeEncounter(const std::string& id,
                                                   const std::string& patientId,
                                                   const std::string& providerId,
                                                   std::chrono::system_clock::time_point when,
                                                   const std::string& type = "visit") {
    encounter_service::domain::Encounter e{};
    e.encounterId = id;
    e.patientId = patientId;
    e.providerId = providerId;
    e.encounterDate = when;
    e.encounterType = type;
    e.clinicalData = nlohmann::json::object();
    e.metadata.createdAt = when;
    e.metadata.updatedAt = when;
    e.metadata.createdBy = "tester";
    return e;
}

}  // namespace

TEST_CASE("InMemoryEncounterRepository GetById returns missing for unknown id") {
    encounter_service::storage::InMemoryEncounterRepository repo;
    REQUIRE(!repo.GetById("missing").has_value());
}

TEST_CASE("InMemoryEncounterRepository Query filters by encounterType") {
    using namespace std::chrono;
    encounter_service::storage::InMemoryEncounterRepository repo;
    repo.Create(MakeEncounter("enc-1", "p", "prov", system_clock::time_point{seconds{1}}, "visit"));
    repo.Create(MakeEncounter("enc-2", "p", "prov", system_clock::time_point{seconds{2}}, "lab"));

    encounter_service::storage::EncounterQueryFilters filters{};
    filters.encounterType = "lab";
    const auto results = repo.Query(filters);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].encounterId == "enc-2");
}

TEST_CASE("InMemoryEncounterRepository Query applies limit and offset after sorting") {
    using namespace std::chrono;
    encounter_service::storage::InMemoryEncounterRepository repo;
    repo.Create(MakeEncounter("enc-b", "p", "prov", system_clock::time_point{seconds{2}}));
    repo.Create(MakeEncounter("enc-a", "p", "prov", system_clock::time_point{2s}));
    repo.Create(MakeEncounter("enc-c", "p", "prov", system_clock::time_point{seconds{1}}));
    repo.Create(MakeEncounter("enc-d", "p", "prov", system_clock::time_point{seconds{3}}));

    encounter_service::storage::EncounterQueryFilters filters{};
    filters.offset = 1;
    filters.limit = 2;

    const auto results = repo.Query(filters);
    REQUIRE(results.size() == 2);
    REQUIRE(results[0].encounterId == "enc-a");
    REQUIRE(results[1].encounterId == "enc-b");
}

TEST_CASE("InMemoryEncounterRepository Create overwrites duplicate encounterId") {
    using namespace std::chrono;
    encounter_service::storage::InMemoryEncounterRepository repo;
    repo.Create(MakeEncounter("enc-1", "patient-old", "prov", system_clock::time_point{seconds{1}}));
    repo.Create(MakeEncounter("enc-1", "patient-new", "prov", system_clock::time_point{seconds{2}}));

    const auto found = repo.GetById("enc-1");
    REQUIRE(found.has_value());
    REQUIRE(found->patientId == "patient-new");
    REQUIRE(found->encounterDate == system_clock::time_point{seconds{2}});
}
