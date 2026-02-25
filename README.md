# Encounter Service (HIPAA-Conscious REST API)

C++20 REST API take-home project for managing encounter records with audit logging.

Current state:
- Layered architecture (`http` -> `domain` -> `storage`)
- In-memory repositories
- Working HTTP endpoints (`/health`, `/encounters`, `/audit/encounters`)
- Validation + error mapping
- Unit and route integration tests

## What Is Implemented

Endpoints:
- `GET /health`
- `POST /encounters`
- `GET /encounters/<encounterId>`
- `GET /encounters`
- `GET /audit/encounters`

Auth:
- `X-API-Key` required on all non-health endpoints
- Demo auth implementation currently maps any non-empty key to actor `"api-key-actor"`

Storage:
- In-memory encounter repository
- In-memory audit repository
- Deterministic ordering for stable tests

## Project Layout

```text
src/
  http/      HTTP routing, auth, validation, error mapping
  domain/    Models, errors, service logic
  storage/   Repository interfaces + in-memory implementations
  util/      Clock, IDs, time parsing/formatting, logging, redaction
tests/       Unit tests + route integration tests
scripts/     Local check/smoke scripts
vendor/      Single-header dependencies (cpp-httplib, nlohmann/json)
```

## Dependencies

Vendored single-header dependencies (already in this repo):
- `vendor/httplib.h` (`cpp-httplib`)
- `vendor/json.hpp` (`nlohmann/json`)

## Testing

Quick commands:
- `./scripts/check.sh` (configure/build/unit+integration tests via `ctest`)
- `./scripts/smoke.sh` (manual API smoke flow against a running server)

See `TESTING.md` for test structure, coverage areas, and notes about route integration tests binding localhost ports.

## API Notes

### Create Encounter (`POST /encounters`)

Required JSON fields:
- `patientId` (string, patient identifier / PHI)
- `providerId` (string, provider identifier)
- `encounterDate` (ISO-8601 UTC string for the clinical encounter timestamp: `YYYY-MM-DD` or `YYYY-MM-DDTHH:MM:SSZ`)
- `encounterType` (string, encounter classification such as `initial_assessment`)
- `clinicalData` (object, structured clinical payload)

### List Encounters (`GET /encounters`)

Supported query params:
- `patientId` (filter by patient identifier)
- `providerId` (filter by provider/clinician identifier)
- `encounterType` (filter by encounter type)
- `from` (ISO-8601 UTC date/datetime, inclusive lower bound for `encounterDate`)
- `to` (ISO-8601 UTC date/datetime, inclusive upper bound for `encounterDate`)

### Audit Query (`GET /audit/encounters`)

Supported query params:
- `from` (ISO-8601 UTC date/datetime, inclusive lower bound for `AuditEntry.timestamp`)
- `to` (ISO-8601 UTC date/datetime, inclusive upper bound for `AuditEntry.timestamp`)

Audit timestamps represent **access time** (`CREATE_ENCOUNTER` / `READ_ENCOUNTER` events), not clinical encounter date.

## Architecture Summary

### HTTP Layer (`src/http`)
- Auth (`X-API-Key`)
- Input parsing/validation
- Boundary conversion (strings -> `std::chrono::system_clock::time_point`)
- Error mapping (`DomainError` -> HTTP response)
- JSON serialization

### Domain Layer (`src/domain`)
- Encounter + audit models
- Domain errors (`DomainError`, `FieldError`)
- `DefaultEncounterService`
- Returns `std::variant<Result, DomainError>`

### Storage Layer (`src/storage`)
- Repository interfaces
- In-memory implementations
- Deterministic result ordering for tests

### Util Layer (`src/util`)
- `Clock` abstraction (injectable)
- `IdGenerator`
- `RequestIdGenerator`
- ISO-8601 UTC parse/format helpers
- Logger + redactor abstractions

## Security Notes

Sensitive fields include:
- `patientId`
- `clinicalData`

Current safeguards:
- Domain error messages are safe (no PHI)
- HTTP logs avoid request bodies and PHI fields
- Redactor interface exists for structured log scrubbing
- Audit entries record actor/action/encounter ID without clinical payloads

## Current Limitations

- In-memory storage only (no persistence)
- Not thread-safe
- Demo auth (no real API key management / identity provider)
- No pagination metadata in list responses
- Redaction implementation is currently a stub passthrough
- No production hardening (TLS, rate limiting, metrics, structured tracing, etc.)
