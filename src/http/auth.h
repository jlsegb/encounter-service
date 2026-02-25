#pragma once

#include <string>
#include <variant>

#include "src/domain/errors.h"
#include "src/http/httplib_compat.h"

namespace encounter_service::http {

std::variant<std::string, domain::DomainError> Authenticate(const httplib::Request& request);

}  // namespace encounter_service::http
