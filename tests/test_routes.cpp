#include "tests/catch_compat.h"

#include <chrono>
#include <cstring>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

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

RawHttpResponse SendHttpRequest(int port, const std::string& raw) {
#if __has_include("vendor/httplib.h")
    std::istringstream stream(raw);
    std::string request_line;
    REQUIRE(static_cast<bool>(std::getline(stream, request_line)));
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }

    std::istringstream request_line_stream(request_line);
    std::string method;
    std::string path;
    std::string version;
    request_line_stream >> method >> path >> version;
    REQUIRE(!method.empty());
    REQUIRE(!path.empty());

    httplib::Headers headers;
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            break;
        }
        const auto colon = line.find(':');
        REQUIRE(colon != std::string::npos);
        const auto key = line.substr(0, colon);
        auto value = line.substr(colon + 1);
        while (!value.empty() && value.front() == ' ') {
            value.erase(value.begin());
        }
        headers.emplace(key, value);
    }

    std::string body;
    std::getline(stream, body, '\0');

    httplib::Client client("127.0.0.1", port);
    client.set_keep_alive(false);

    decltype(client.Get(path.c_str(), headers)) res;
    if (method == "GET") {
        res = client.Get(path.c_str(), headers);
    } else if (method == "POST") {
        auto content_type = std::string("application/json");
        if (const auto it = headers.find("Content-Type"); it != headers.end()) {
            content_type = it->second;
        }
        res = client.Post(path.c_str(), headers, body, content_type.c_str());
    } else {
        REQUIRE(false);
    }

    REQUIRE(static_cast<bool>(res));
    return RawHttpResponse{
        .status = res->status,
        .body = res->body
    };
#else
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    for (int attempt = 0; attempt < 20; ++attempt) {
        bool connected = false;
        int sock = -1;
        for (int connect_attempt = 0; connect_attempt < 50; ++connect_attempt) {
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
        const auto status_end = (status_pos == std::string::npos)
            ? std::string::npos
            : response.find(' ', status_pos + 1);
        if (status_pos == std::string::npos || status_end == std::string::npos) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        RawHttpResponse out{};
        out.status = std::stoi(response.substr(status_pos + 1, status_end - status_pos - 1));
        const auto body_pos = response.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            out.body = response.substr(body_pos + 4);
        }
        return out;
    }

    REQUIRE(false);
    return {};
#endif
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
    FakeEncounterService service;
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18080);
    server.start(service, logger, redactor);

    const auto resp = SendHttpRequest(server.port(),
        "GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n");

    REQUIRE(resp.status == 200);
    REQUIRE(resp.body.find("\"status\":\"ok\"") != std::string::npos);
}

TEST_CASE("Routes enforce auth on non-health endpoints") {
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
}

TEST_CASE("Routes GET encounter by id uses regex route and calls service") {
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
}

TEST_CASE("Routes GET encounter by id maps not found error") {
    FakeEncounterService service;
    service.get_result = encounter_service::domain::DomainError{
        .code = encounter_service::domain::DomainErrorCode::NotFound,
        .message = "Encounter not found",
        .details = std::nullopt
    };
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18085);
    server.start(service, logger, redactor);

    const auto resp = SendHttpRequest(server.port(),
        "GET /encounters/enc-missing HTTP/1.1\r\nHost: localhost\r\nX-API-Key: key\r\n\r\n");

    REQUIRE(resp.status == 404);
    REQUIRE(service.get_called == true);
    REQUIRE(resp.body.find("\"code\":\"not_found\"") != std::string::npos);
}

TEST_CASE("Routes GET encounters maps validation error for bad from query") {
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
}

TEST_CASE("Routes GET encounters includes requestId in mapped error response") {
    FakeEncounterService service;
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18086);
    server.start(service, logger, redactor);

    const auto resp = SendHttpRequest(server.port(),
        "GET /encounters?from=bad-date HTTP/1.1\r\nHost: localhost\r\nX-API-Key: key\r\nX-Request-Id: req-abc\r\n\r\n");

    REQUIRE(resp.status == 400);
    REQUIRE(resp.body.find("\"requestId\":\"req-abc\"") != std::string::npos);
}

TEST_CASE("Routes GET encounters returns serialized encounter array") {
    using namespace std::chrono;
    FakeEncounterService service;
    service.query_result = std::vector<encounter_service::domain::Encounter>{
        MakeEncounter("enc-1", system_clock::time_point{seconds{1700000000}}),
        MakeEncounter("enc-2", system_clock::time_point{seconds{1700000100}})
    };
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18087);
    server.start(service, logger, redactor);

    const auto resp = SendHttpRequest(server.port(),
        "GET /encounters HTTP/1.1\r\nHost: localhost\r\nX-API-Key: key\r\n\r\n");

    REQUIRE(resp.status == 200);
    REQUIRE(service.query_called == true);
    REQUIRE(resp.body.find("\"encounterId\":\"enc-1\"") != std::string::npos);
    REQUIRE(resp.body.find("\"encounterId\":\"enc-2\"") != std::string::npos);
}

TEST_CASE("Routes POST encounters maps validation error for invalid JSON") {
    FakeEncounterService service;
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18088);
    server.start(service, logger, redactor);

    const std::string body = "{invalid-json";
    const auto request =
        "POST /encounters HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "X-API-Key: key\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" +
        body;

    const auto resp = SendHttpRequest(server.port(), request);
    REQUIRE(resp.status == 400);
    REQUIRE(resp.body.find("\"code\":\"validation_error\"") != std::string::npos);
    REQUIRE(service.create_called == false);
}

TEST_CASE("Routes POST encounters returns 201 on success when real json parser is available") {
#if __has_include("vendor/json.hpp")
    using namespace std::chrono;
    FakeEncounterService service;
    service.create_result = MakeEncounter("enc-created", system_clock::time_point{seconds{1700000000}});
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18089);
    server.start(service, logger, redactor);

    const std::string body =
        "{\"patientId\":\"patient-1\",\"providerId\":\"provider-1\",\"encounterType\":\"visit\","
        "\"encounterDate\":\"2026-02-25T00:00:00Z\",\"clinicalData\":{}}";
    const auto request =
        "POST /encounters HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "X-API-Key: key\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" +
        body;

    const auto resp = SendHttpRequest(server.port(), request);
    REQUIRE(resp.status == 201);
    REQUIRE(service.create_called == true);
    REQUIRE(service.last_create_actor == "api-key-actor");
    REQUIRE(resp.body.find("\"encounterId\":\"enc-created\"") != std::string::npos);
#else
    SUCCEED("POST success route test requires vendor/json.hpp");
#endif
}

TEST_CASE("Routes GET audit encounters returns serialized audit array") {
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
}

TEST_CASE("Routes GET audit encounters maps validation error for bad to query") {
    FakeEncounterService service;
    FakeLogger logger;
    FakeRedactor redactor;
    TestServer server(18090);
    server.start(service, logger, redactor);

    const auto resp = SendHttpRequest(server.port(),
        "GET /audit/encounters?to=bad-date HTTP/1.1\r\nHost: localhost\r\nX-API-Key: key\r\n\r\n");

    REQUIRE(resp.status == 400);
    REQUIRE(resp.body.find("\"path\":\"to\"") != std::string::npos);
    REQUIRE(service.audit_query_called == false);
}
