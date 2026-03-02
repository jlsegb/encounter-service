#include "src/util/redaction.h"

#include <cctype>
#include <string>
#include <unordered_set>

namespace encounter_service::util {

namespace {

constexpr const char* kRedacted = "[REDACTED]";

std::string NormalizeKey(std::string key) {
    std::string normalized;
    normalized.reserve(key.size());
    for (char ch : key) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (std::isalnum(c)) {
            normalized.push_back(static_cast<char>(std::tolower(c)));
        }
    }
    return normalized;
}

bool IsSensitiveKey(const std::string& key) {
    static const std::unordered_set<std::string> kSensitiveKeys = {
        "clinicaldata",
        "patientid"
    };
    return kSensitiveKeys.find(NormalizeKey(key)) != kSensitiveKeys.end();
}

}  // namespace

nlohmann::json BasicRedactor::RedactJson(const nlohmann::json& input) const {
    if (!input.is_object()) {
        return input;
    }

    nlohmann::json out = input;
    for (auto it = out.begin(); it != out.end(); ++it) {
        if (IsSensitiveKey(it.key())) {
            it.value() = kRedacted;
        }
    }
    return out;
}

}  // namespace encounter_service::util
