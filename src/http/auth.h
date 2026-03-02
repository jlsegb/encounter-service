#pragma once

#include <string>
#include <variant>

#include "src/domain/errors.h"
#include "src/http/httplib_compat.h"

namespace encounter_service::http {

// Authenticates the request using `X-API-Key`.
// Returns an actor identifier on success or a safe Unauthorized DomainError on failure.
std::variant<std::string, domain::DomainError> Authenticate(const httplib::Request& request);

}  // namespace encounter_service::http
