#pragma once

#include <optional>
#include <string>

#include "src/domain/errors.h"
#include "src/util/json_compat.h"

namespace encounter_service::http {

struct HttpErrorResponse {
    int status{500};
    nlohmann::json body;
};

HttpErrorResponse MapDomainError(const domain::DomainError& error,
                                std::optional<std::string> requestId = std::nullopt);

}  // namespace encounter_service::http
