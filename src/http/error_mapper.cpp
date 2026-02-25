#include "src/http/error_mapper.h"

namespace encounter_service::http {

namespace {

std::string CodeToString(domain::DomainErrorCode code) {
    switch (code) {
        case domain::DomainErrorCode::Validation:
            return "validation_error";
        case domain::DomainErrorCode::NotFound:
            return "not_found";
        case domain::DomainErrorCode::Unauthorized:
            return "unauthorized";
        case domain::DomainErrorCode::Internal:
            return "internal_error";
    }
    return "internal_error";
}

int StatusForCode(domain::DomainErrorCode code) {
    switch (code) {
        case domain::DomainErrorCode::Validation:
            return 400;
        case domain::DomainErrorCode::NotFound:
            return 404;
        case domain::DomainErrorCode::Unauthorized:
            return 401;
        case domain::DomainErrorCode::Internal:
            return 500;
    }
    return 500;
}

}  // namespace

HttpErrorResponse MapDomainError(const domain::DomainError& error) {
    nlohmann::json envelope = nlohmann::json::object();
    nlohmann::json inner = nlohmann::json::object();
    inner["code"] = CodeToString(error.code);
    inner["message"] = error.message;
    envelope["error"] = inner;

    return HttpErrorResponse{
        .status = StatusForCode(error.code),
        .body = envelope
    };
}

}  // namespace encounter_service::http
