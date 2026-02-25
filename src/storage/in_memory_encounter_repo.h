#pragma once

#include <mutex>
#include <unordered_map>

#include "src/storage/encounter_repo.h"

namespace encounter_service::storage {

class InMemoryEncounterRepository final : public EncounterRepository {
public:
    domain::Encounter Create(const domain::Encounter& encounter) override;
    std::optional<domain::Encounter> GetById(const std::string& encounterId) const override;
    std::vector<domain::Encounter> Query(const EncounterQueryFilters& filters) const override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, domain::Encounter> encounters_;
};

}  // namespace encounter_service::storage
