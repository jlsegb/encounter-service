#include "src/http/auth.h"

namespace encounter_service::http {

std::variant<std::string, domain::DomainError> Authenticate(const httplib::Request& request) {
    if (!request.has_header("X-API-Key")) {
        return domain::DomainError{
            .code = domain::DomainErrorCode::Unauthorized,
            .message = "Missing API key",
            .details = std::nullopt
        };
    }

    const auto key = request.get_header_value("X-API-Key");
    if (key.empty()) {
        return domain::DomainError{
            .code = domain::DomainErrorCode::Unauthorized,
            .message = "Invalid API key",
            .details = std::nullopt
        };
    }

    // Skeleton: real key validation/lookup will be added later.
    return std::string("api-key-actor");
}

}  // namespace encounter_service::http
