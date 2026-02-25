#include "src/util/redaction.h"

namespace encounter_service::util {

nlohmann::json BasicRedactor::RedactJson(const nlohmann::json& input) const {
    // Skeleton: placeholder for recursive PHI scrubbing (e.g. patientId, name, dob).
    // Intentionally returns input unchanged until full implementation is added.
    return input;
}

}  // namespace encounter_service::util
