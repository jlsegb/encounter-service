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
        "patientid",
        "name",
        "firstname",
        "lastname",
        "fullname",
        "dob",
        "dateofbirth",
        "ssn",
        "mrn"
    };
    return kSensitiveKeys.find(NormalizeKey(key)) != kSensitiveKeys.end();
}

nlohmann::json RedactRecursive(const nlohmann::json& input) {
    if (input.is_object()) {
        nlohmann::json out = nlohmann::json::object();
        for (auto it = input.begin(); it != input.end(); ++it) {
            if (IsSensitiveKey(it.key())) {
                out[it.key()] = kRedacted;
            } else {
                out[it.key()] = RedactRecursive(it.value());
            }
        }
        return out;
    }

    if (input.is_array()) {
        nlohmann::json out = nlohmann::json::array();
        for (const auto& item : input) {
            out.push_back(RedactRecursive(item));
        }
        return out;
    }

    return input;
}

}  // namespace

nlohmann::json BasicRedactor::RedactJson(const nlohmann::json& input) const {
    return RedactRecursive(input);
}

}  // namespace encounter_service::util
