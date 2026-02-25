#include "tests/catch_compat.h"

#include <chrono>
#include <cstring>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "src/domain/encounter_service.h"
#include "src/http/routes.h"

namespace {

class FakeLogger final : public encounter_service::util::Logger {
public:
    void Log(encounter_service::util::LogLevel, std::string_view message) override {
        messages.emplace_back(message);
    }

    std::vector<std::string> messages;
};

class FakeRedactor final : public encounter_service::util::Redactor {
public:
    nlohmann::json RedactJson(const nlohmann::json& input) const override {
        return input;
    }
};

class FakeEncounterService final : public encounter_service::domain::EncounterService {
public:
    encounter_service::domain::ServiceResult<encounter_service::domain::Encounter>
    CreateEncounter(const encounter_service::domain::CreateEncounterInput& input,
                    const std::string& actor) override {
        create_called = true;
        last_create_actor = actor;
        last_create_input = input;
        return create_result;
    }

    encounter_service::domain::ServiceResult<encounter_service::domain::Encounter>
    GetEncounter(const std::string& id, const std::string& actor) override {
        get_called = true;
        last_get_id = id;
        last_get_actor = actor;
        return get_result;
    }

    encounter_service::domain::ServiceResult<std::vector<encounter_service::domain::Encounter>> QueryEncounters(
        const encounter_service::storage::EncounterQueryFilters& filters) override {
        query_called = true;
        last_query_filters = filters;
        return query_result;
    }

    encounter_service::domain::ServiceResult<std::vector<encounter_service::domain::AuditEntry>> QueryAudit(
        const encounter_service::storage::AuditDateRange& range) override {
        audit_query_called = true;
        last_audit_range = range;
        return audit_result;
    }

    bool create_called{false};
    bool get_called{false};
    bool query_called{false};
    bool audit_query_called{false};

    std::string last_create_actor;
    std::optional<encounter_service::domain::CreateEncounterInput> last_create_input;
    std::string last_get_id;
    std::string last_get_actor;
    encounter_service::storage::EncounterQueryFilters last_query_filters{};
    encounter_service::storage::AuditDateRange last_audit_range{};

    encounter_service::domain::ServiceResult<encounter_service::domain::Encounter> create_result{
        encounter_service::domain::Encounter{}
    };
    encounter_service::domain::ServiceResult<encounter_service::domain::Encounter> get_result{
        encounter_service::domain::Encounter{}
    };
    encounter_service::domain::ServiceResult<std::vector<encounter_service::domain::Encounter>> query_result{
        std::vector<encounter_service::domain::Encounter>{}
    };
    encounter_service::domain::ServiceResult<std::vector<encounter_service::domain::AuditEntry>> audit_result{
        std::vector<encounter_service::domain::AuditEntry>{}
    };
};

struct RawHttpResponse {
    int status{0};
    std::string body;
};

#if !defined(_WIN32)
RawHttpResponse SendHttpRequest(int port, const std::string& raw) {
    RawHttpResponse out{};

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    bool connected = false;
    int sock = -1;
    for (int attempt = 0; attempt < 50; ++attempt) {
        sock = ::socket(AF_INET, SOCK_STREAM, 0);
        REQUIRE(sock >= 0);
        if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
            connected = true;
            break;
        }
        ::close(sock);
        sock = -1;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(connected);
    REQUIRE(::send(sock, raw.data(), raw.size(), 0) >= 0);
    ::shutdown(sock, SHUT_WR);

    std::string response;
    char buffer[4096];
    for (;;) {
        const auto n = ::recv(sock, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            break;
        }
        response.append(buffer, static_cast<std::size_t>(n));
    }
    ::close(sock);

    const auto status_pos = response.find(' ');
    REQUIRE(status_pos != std::string::npos);
    const auto status_end = response.find(' ', status_pos + 1);
    REQUIRE(status_end != std::string::npos);
    out.status = std::stoi(response.substr(status_pos + 1, status_end - status_pos - 1));

    const auto body_pos = response.find("\r\n\r\n");
    if (body_pos != std::string::npos) {
        out.body = response.substr(body_pos + 4);
    }
    return out;
}

class TestServer {
public:
    explicit TestServer(int port)
        : port_(port) {}

    ~TestServer() {
        stop();
    }

    void start(FakeEncounterService& service, FakeLogger& logger, FakeRedactor& redactor) {
        encounter_service::http::RegisterRoutes(server_, service, logger, redactor);
        thread_ = std::thread([this]() {
            (void)server_.listen("127.0.0.1", port_);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void stop() {
        if (thread_.joinable()) {
            server_.stop();
            thread_.join();
        }
    }

    int port() const { return port_; }

private:
    int port_;
    httplib::Server server_{};
    std::thread thread_{};
};
#endif

encounter_service::domain::Encounter MakeEncounter(std::string id, std::chrono::system_clock::time_point ts) {
    encounter_service::domain::Encounter encounter{};
    encounter.encounterId = std::move(id);
    encounter.patientId = "patient-x";
    encounter.providerId = "provider-y";
    encounter.encounterDate = ts;
    encounter.encounterType = "visit";
    encounter.clinicalData = nlohmann::json::object();
    encounter.metadata.createdAt = ts;
    encounter.metadata.updatedAt = ts;
    encounter.metadata.createdBy = "tester";
    return encounter;
}

}  // namespace

TEST_CASE("Routes health endpoint returns 200") {
#if defined(_WIN32)
    SUCCEED("Windows socket path not implemented in test");
#else
    FakeEncounterService service;
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18080);
    server.start(service, logger, redactor);

    const auto resp = SendHttpRequest(server.port(),
        "GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n");

    REQUIRE(resp.status == 200);
    REQUIRE(resp.body.find("\"status\":\"ok\"") != std::string::npos);
#endif
}

TEST_CASE("Routes enforce auth on non-health endpoints") {
#if defined(_WIN32)
    SUCCEED("Windows socket path not implemented in test");
#else
    FakeEncounterService service;
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18081);
    server.start(service, logger, redactor);

    const auto resp = SendHttpRequest(server.port(),
        "GET /encounters HTTP/1.1\r\nHost: localhost\r\n\r\n");

    REQUIRE(resp.status == 401);
    REQUIRE(resp.body.find("\"code\":\"unauthorized\"") != std::string::npos);
    REQUIRE(service.query_called == false);
#endif
}

TEST_CASE("Routes GET encounter by id uses regex route and calls service") {
#if defined(_WIN32)
    SUCCEED("Windows socket path not implemented in test");
#else
    using namespace std::chrono;
    FakeEncounterService service;
    service.get_result = MakeEncounter("enc-abc_123", system_clock::time_point{seconds{1700000000}});
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18082);
    server.start(service, logger, redactor);

    const auto resp = SendHttpRequest(server.port(),
        "GET /encounters/enc-abc_123 HTTP/1.1\r\nHost: localhost\r\nX-API-Key: key\r\n\r\n");

    REQUIRE(resp.status == 200);
    REQUIRE(service.get_called == true);
    REQUIRE(service.last_get_id == "enc-abc_123");
    REQUIRE(service.last_get_actor == "api-key-actor");
    REQUIRE(resp.body.find("\"encounterId\":\"enc-abc_123\"") != std::string::npos);
#endif
}

TEST_CASE("Routes GET encounters maps validation error for bad from query") {
#if defined(_WIN32)
    SUCCEED("Windows socket path not implemented in test");
#else
    FakeEncounterService service;
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18083);
    server.start(service, logger, redactor);

    const auto resp = SendHttpRequest(server.port(),
        "GET /encounters?from=bad-date HTTP/1.1\r\nHost: localhost\r\nX-API-Key: key\r\n\r\n");

    REQUIRE(resp.status == 400);
    REQUIRE(resp.body.find("\"code\":\"validation_error\"") != std::string::npos);
    REQUIRE(resp.body.find("\"path\":\"from\"") != std::string::npos);
    REQUIRE(service.query_called == false);
#endif
}

TEST_CASE("Routes GET audit encounters returns serialized audit array") {
#if defined(_WIN32)
    SUCCEED("Windows socket path not implemented in test");
#else
    using namespace std::chrono;
    FakeEncounterService service;
    service.audit_result = std::vector<encounter_service::domain::AuditEntry>{
        encounter_service::domain::AuditEntry{
            .timestamp = system_clock::time_point{seconds{1700000000}},
            .actor = "actor-1",
            .action = encounter_service::domain::AuditAction::READ_ENCOUNTER,
            .encounterId = "enc-1"
        }
    };
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18084);
    server.start(service, logger, redactor);

    const auto resp = SendHttpRequest(server.port(),
        "GET /audit/encounters?from=2026-02-25 HTTP/1.1\r\nHost: localhost\r\nX-API-Key: key\r\n\r\n");

    REQUIRE(resp.status == 200);
    REQUIRE(service.audit_query_called == true);
    REQUIRE(resp.body.find("\"action\":\"READ_ENCOUNTER\"") != std::string::npos);
    REQUIRE(resp.body.find("\"encounterId\":\"enc-1\"") != std::string::npos);
#endif
}
