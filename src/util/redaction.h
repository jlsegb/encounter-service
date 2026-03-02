#pragma once

#include "src/util/json_compat.h"

namespace encounter_service::util {

class Redactor {
public:
    virtual ~Redactor() = default;
    // Returns a redacted copy of `input`.
    virtual nlohmann::json RedactJson(const nlohmann::json& input) const = 0;
};

class BasicRedactor final : public Redactor {
public:
    // Redacts top-level sensitive keys (currently `patientId` and `clinicalData`).
    nlohmann::json RedactJson(const nlohmann::json& input) const override;
};

}  // namespace encounter_service::util
