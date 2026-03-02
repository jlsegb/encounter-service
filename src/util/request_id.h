#pragma once

#include <string>

#include "src/util/id_generator.h"

namespace encounter_service::util {

using RequestId = std::string;

class RequestIdGenerator {
public:
    // Creates a request ID generator that prefixes generated IDs with `req-`.
    explicit RequestIdGenerator(IdGenerator& idGenerator)
        : idGenerator_(idGenerator) {}

    // Returns the next request ID.
    [[nodiscard]] RequestId Next() {
        return "req-" + idGenerator_.NextId();
    }

private:
    IdGenerator& idGenerator_;
};

}  // namespace encounter_service::util
