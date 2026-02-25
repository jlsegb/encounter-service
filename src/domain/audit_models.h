#pragma once

#include <chrono>
#include <string>

namespace encounter_service::domain {

enum class AuditAction {
    READ_ENCOUNTER,
    CREATE_ENCOUNTER
};

struct AuditEntry {
    std::chrono::system_clock::time_point timestamp{};
    std::string actor;
    AuditAction action{AuditAction::READ_ENCOUNTER};
    std::string encounterId;
};

}  // namespace encounter_service::domain
