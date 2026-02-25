#pragma once

#include <string>

#include "src/util/id_generator.h"

namespace encounter_service::util {

using RequestId = std::string;

class RequestIdGenerator {
public:
    explicit RequestIdGenerator(IdGenerator& idGenerator)
        : idGenerator_(idGenerator) {}

    [[nodiscard]] RequestId Next() {
        return "req-" + idGenerator_.NextId();
    }

private:
    IdGenerator& idGenerator_;
};

}  // namespace encounter_service::util
