cmake_minimum_required(VERSION 3.20)
project(encounter_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENCOUNTER_SERVICE_BUILD_TESTS "Build skeleton tests" ON)

add_library(encounter_service_lib
    src/domain/encounter_service.cpp
    src/http/routes.cpp
    src/http/auth.cpp
    src/http/validation.cpp
    src/http/error_mapper.cpp
    src/storage/in_memory_encounter_repo.cpp
    src/storage/in_memory_audit_repo.cpp
    src/util/logger.cpp
    src/util/redaction.cpp
)

target_include_directories(encounter_service_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(encounter_service
    src/main.cpp
)

target_link_libraries(encounter_service PRIVATE encounter_service_lib)

if (ENCOUNTER_SERVICE_BUILD_TESTS)
    enable_testing()

    add_executable(encounter_service_tests
        tests/test_redaction.cpp
        tests/test_validation.cpp
        tests/test_audit.cpp
        src/http/validation.cpp
        src/storage/in_memory_audit_repo.cpp
        src/util/redaction.cpp
    )

    target_include_directories(encounter_service_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME encounter_service_tests COMMAND encounter_service_tests)
endif()
