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

HttpErrorResponse MapDomainError(const domain::DomainError& error, std::optional<std::string> requestId) {
    nlohmann::json envelope = nlohmann::json::object();
    nlohmann::json errorJson = nlohmann::json::object();
    errorJson["code"] = CodeToString(error.code);
    errorJson["message"] = error.message;

    if (error.details && !error.details->empty()) {
        nlohmann::json details = nlohmann::json::array();
        for (const auto& detail : *error.details) {
            nlohmann::json item = nlohmann::json::object();
            item["path"] = detail.path;
            item["message"] = detail.message;
            details.push_back(item);
        }
        errorJson["details"] = details;
    }

    envelope["error"] = errorJson;
    if (requestId && !requestId->empty()) {
        envelope["requestId"] = *requestId;
    }

    return HttpErrorResponse{
        .status = StatusForCode(error.code),
        .body = envelope
    };
}

}  // namespace encounter_service::http
