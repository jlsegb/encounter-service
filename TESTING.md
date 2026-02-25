# Testing

This project uses a mix of unit tests and lightweight HTTP route integration tests.

## Quick Start

### Run build + tests

```bash
./scripts/check.sh
```

What it does:
1. Configures CMake (if needed)
2. Builds the project
3. Runs tests with `ctest --output-on-failure`

### Run smoke test (manual API verification)

Start the server first:

```bash
./build/encounter_service
```

Then in another terminal:

```bash
./scripts/smoke.sh
```

## Test Types

### Unit tests

These cover business logic and isolated helpers:
- Domain service behavior (`DefaultEncounterService`)
- Validation (`src/http/validation.cpp`)
- Auth (`src/http/auth.cpp`)
- Error mapping (`src/http/error_mapper.cpp`)
- In-memory repositories
- Time parsing/formatting utilities
- Redaction skeleton behavior

### Route integration tests

`tests/test_routes.cpp` starts a local HTTP server and sends real HTTP requests to verify:
- route registration/wiring
- auth enforcement
- validation/error mapping integration
- endpoint serialization

These tests bind localhost ports during execution.

## Determinism Strategy

Tests use deterministic helpers where appropriate:
- fixed clocks / sequence clocks
- fixed ID generators
- deterministic in-memory repo ordering

This keeps assertions stable and avoids flaky time-based behavior.

## Coverage Summary (Current)

Covered:
- `src/domain/encounter_service.cpp`
- `src/http/auth.cpp`
- `src/http/validation.cpp`
- `src/http/error_mapper.cpp`
- `src/http/routes.cpp` (integration-level route behavior)
- `src/storage/in_memory_encounter_repo.cpp`
- `src/storage/in_memory_audit_repo.cpp`
- `src/util/time.cpp`

Partially covered / light coverage:
- `src/util/redaction.cpp` (currently a stub passthrough)
- `src/util/logger.cpp` (not directly unit tested)
- `src/util/id_generator.h` / `src/util/request_id.h` (not directly unit tested)

## Notes

- With vendored `cpp-httplib` and `nlohmann/json` present, tests compile against the real headers.
- Route integration tests may require running outside restricted sandboxes because they bind localhost ports.
- The smoke test is not a replacement for unit tests; it is a quick end-to-end sanity check.
