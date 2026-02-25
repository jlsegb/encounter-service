#pragma once

#if __has_include(<catch2/catch_test_macros.hpp>)

#if defined(ENCOUNTER_SERVICE_CATCH_COMPAT_MAIN)
#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch_test_macros.hpp>
#endif

#else

#include <cstdlib>
#include <iostream>

#define ECS_CONCAT_IMPL(x, y) x##y
#define ECS_CONCAT(x, y) ECS_CONCAT_IMPL(x, y)

#if defined(ENCOUNTER_SERVICE_CATCH_COMPAT_MAIN)
int main() { return 0; }
#endif

#define TEST_CASE(name) \
    static void ECS_CONCAT(test_case_fn_, __LINE__)(); \
    static const int ECS_CONCAT(test_case_reg_, __LINE__) = []() { \
        ECS_CONCAT(test_case_fn_, __LINE__)(); \
        return 0; \
    }(); \
    static void ECS_CONCAT(test_case_fn_, __LINE__)()

#define REQUIRE(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "REQUIRE failed: " #expr << '\n'; \
            std::abort(); \
        } \
    } while (0)

#define SUCCEED(...) \
    do { \
    } while (0)

#endif
