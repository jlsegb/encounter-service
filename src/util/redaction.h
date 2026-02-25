#pragma once

#include "src/util/json_compat.h"

namespace encounter_service::util {

class Redactor {
public:
    virtual ~Redactor() = default;
    virtual nlohmann::json RedactJson(const nlohmann::json& input) const = 0;
};

class BasicRedactor final : public Redactor {
public:
    nlohmann::json RedactJson(const nlohmann::json& input) const override;
};

}  // namespace encounter_service::util
