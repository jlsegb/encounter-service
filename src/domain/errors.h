#pragma once

#include <optional>
#include <string>
#include <vector>

namespace encounter_service::domain {

struct FieldError {
    std::string path;
    std::string message;
};

enum class DomainErrorCode {
    Validation,
    NotFound,
    Unauthorized,
    Internal
};

struct DomainError {
    DomainErrorCode code{DomainErrorCode::Internal};
    std::string message;  // Must be safe and contain no PHI.
    std::optional<std::vector<FieldError>> details;
};

}  // namespace encounter_service::domain
