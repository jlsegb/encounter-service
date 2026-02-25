# Encounter Service (Architecture Skeleton)

This repository contains a compile-ready C++20 architecture skeleton for a HIPAA-conscious REST API take-home.

- Clean layer boundaries: `http` / `domain` / `storage` / `util`
- Dependency injection (no globals)
- In-memory repository stubs
- HTTP route wiring with `/health` and `501` stubs for remaining endpoints
- Catch2-style test skeletons that compile (with a local compatibility shim when Catch2 is not installed yet)

Vendor dependencies expected to be added manually:

- `vendor/httplib.h` (cpp-httplib)
- `vendor/json.hpp` (nlohmann/json)
