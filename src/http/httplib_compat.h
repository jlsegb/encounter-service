#pragma once

#if __has_include("vendor/httplib.h")
#include "vendor/httplib.h"
#else

#include <atomic>
#include <cctype>
#include <functional>
#include <cstring>
#include <regex>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace httplib {

namespace detail {

inline std::string ascii_lower(std::string s) {
    for (char& ch : s) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

}  // namespace detail

struct Request {
    struct Match {
        std::string value;

        [[nodiscard]] std::string str() const {
            return value;
        }
    };

    std::map<std::string, std::string> headers;
    std::string body;
    std::string path;
    std::map<std::string, std::string> params;
    std::vector<Match> matches;

    [[nodiscard]] bool has_header(const std::string& key) const {
        return headers.find(detail::ascii_lower(key)) != headers.end();
    }

    [[nodiscard]] std::string get_header_value(const std::string& key) const {
        const auto it = headers.find(detail::ascii_lower(key));
        return it == headers.end() ? std::string{} : it->second;
    }

    [[nodiscard]] bool has_param(const std::string& key) const {
        return params.find(key) != params.end();
    }

    [[nodiscard]] std::string get_param_value(const std::string& key) const {
        const auto it = params.find(key);
        return it == params.end() ? std::string{} : it->second;
    }
};

struct Response {
    int status{200};
    std::string body;
    std::string content_type;

    void set_content(const std::string& content, const std::string& type) {
        body = content;
        content_type = type;
    }
};

class Server {
public:
    using Handler = std::function<void(const Request&, Response&)>;

    void Get(const std::string& pattern, Handler handler) {
        get_routes_.push_back(Route{
            .pattern = pattern,
            .handler = std::move(handler),
            .is_regex = LooksLikeRegex(pattern),
            .regex = std::regex(pattern)
        });
    }

    void Post(const std::string& pattern, Handler handler) {
        post_routes_.push_back(Route{
            .pattern = pattern,
            .handler = std::move(handler),
            .is_regex = LooksLikeRegex(pattern),
            .regex = std::regex(pattern)
        });
    }

    bool listen(const char* host, int port) {
#if defined(_WIN32)
        (void)host;
        (void)port;
        return false;
#else
        (void)host;
        listen_port_ = port;

        const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            return false;
        }
        server_fd_ = server_fd;
        running_.store(true);

        int opt = 1;
        ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (::bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            running_.store(false);
            server_fd_ = -1;
            ::close(server_fd);
            return false;
        }

        if (::listen(server_fd, 16) < 0) {
            running_.store(false);
            server_fd_ = -1;
            ::close(server_fd);
            return false;
        }

        for (; running_.load();) {
            const int client_fd = ::accept(server_fd, nullptr, nullptr);
            if (client_fd < 0) {
                if (!running_.load()) {
                    break;
                }
                continue;
            }
            HandleClient(client_fd);
            ::close(client_fd);
        }
        running_.store(false);
        server_fd_ = -1;
        ::close(server_fd);
        return true;
#endif
    }

    void stop() {
#if !defined(_WIN32)
        running_.store(false);
        if (listen_port_ > 0) {
            const int wake_fd = ::socket(AF_INET, SOCK_STREAM, 0);
            if (wake_fd >= 0) {
                sockaddr_in addr{};
                addr.sin_family = AF_INET;
                addr.sin_port = htons(static_cast<uint16_t>(listen_port_));
                addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                (void)::connect(wake_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
                ::close(wake_fd);
            }
        }
#endif
    }

private:
#if !defined(_WIN32)
    static std::string Trim(const std::string& value) {
        std::size_t start = 0;
        while (start < value.size() && (value[start] == ' ' || value[start] == '\t' || value[start] == '\r')) {
            ++start;
        }
        std::size_t end = value.size();
        while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r')) {
            --end;
        }
        return value.substr(start, end - start);
    }

    static std::string ReasonPhrase(int status) {
        switch (status) {
            case 200:
                return "OK";
            case 404:
                return "Not Found";
            case 405:
                return "Method Not Allowed";
            default:
                return "OK";
        }
    }

    static bool LooksLikeRegex(const std::string& pattern) {
        return pattern.find('(') != std::string::npos ||
               pattern.find('[') != std::string::npos ||
               pattern.find('\\') != std::string::npos;
    }

    static void ParseQueryString(const std::string& query, Request& req) {
        std::size_t start = 0;
        while (start <= query.size()) {
            const auto end = query.find('&', start);
            const auto token = query.substr(start, end == std::string::npos ? std::string::npos : end - start);
            if (!token.empty()) {
                const auto eq = token.find('=');
                if (eq == std::string::npos) {
                    req.params[token] = "";
                } else {
                    req.params[token.substr(0, eq)] = token.substr(eq + 1);
                }
            }
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
    }

    struct Route {
        std::string pattern;
        Handler handler;
        bool is_regex{false};
        std::regex regex;
    };

    bool Dispatch(const std::vector<Route>& routes, Request& req, Response& res) {
        for (const auto& route : routes) {
            if (!route.is_regex) {
                if (route.pattern == req.path) {
                    route.handler(req, res);
                    return true;
                }
                continue;
            }

            std::smatch match;
            if (std::regex_match(req.path, match, route.regex)) {
                req.matches.clear();
                for (std::size_t i = 0; i < match.size(); ++i) {
                    req.matches.push_back(Request::Match{.value = match[i].str()});
                }
                route.handler(req, res);
                return true;
            }
        }
        return false;
    }

    void HandleClient(int client_fd) {
        std::string raw;
        char buffer[4096];
        for (;;) {
            const auto n = ::recv(client_fd, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                return;
            }
            raw.append(buffer, static_cast<std::size_t>(n));
            if (raw.find("\r\n\r\n") != std::string::npos || raw.find("\n\n") != std::string::npos) {
                break;
            }
            if (raw.size() > 1024 * 1024) {
                return;
            }
        }

        std::istringstream stream(raw);
        std::string request_line;
        if (!std::getline(stream, request_line)) {
            return;
        }
        if (!request_line.empty() && request_line.back() == '\r') {
            request_line.pop_back();
        }

        std::istringstream request_line_stream(request_line);
        std::string method;
        std::string target;
        std::string version;
        request_line_stream >> method >> target >> version;
        if (method.empty() || target.empty()) {
            return;
        }

        Request req{};
        req.path = target;
        if (const auto q = req.path.find('?'); q != std::string::npos) {
            ParseQueryString(req.path.substr(q + 1), req);
            req.path = req.path.substr(0, q);
        }

        std::string header_line;
        while (std::getline(stream, header_line)) {
            if (!header_line.empty() && header_line.back() == '\r') {
                header_line.pop_back();
            }
            if (header_line.empty()) {
                break;
            }
            const auto colon = header_line.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            const auto key = Trim(header_line.substr(0, colon));
            const auto value = Trim(header_line.substr(colon + 1));
            req.headers[detail::ascii_lower(key)] = value;
        }

        Response res{};
        res.status = 404;
        res.set_content("{\"error\":\"Not Found\"}", "application/json");

        std::size_t content_length = 0;
        if (const auto it = req.headers.find("Content-Length"); it != req.headers.end()) {
            try {
                content_length = static_cast<std::size_t>(std::stoul(it->second));
            } catch (...) {
                content_length = 0;
            }
        }

        std::string body_remainder;
        std::getline(stream, body_remainder, '\0');
        req.body = body_remainder;
        while (req.body.size() < content_length) {
            const auto n = ::recv(client_fd, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                break;
            }
            req.body.append(buffer, static_cast<std::size_t>(n));
        }
        if (req.body.size() > content_length) {
            req.body.resize(content_length);
        }

        if (method == "GET") {
            if (Dispatch(get_routes_, req, res)) {
                // handled
            }
        } else if (method == "POST") {
            if (!Dispatch(post_routes_, req, res)) {
                res.status = 405;
                res.set_content("{\"error\":\"Method Not Allowed\"}", "application/json");
            }
        } else {
            res.status = 405;
            res.set_content("{\"error\":\"Method Not Allowed\"}", "application/json");
        }

        if (res.content_type.empty()) {
            res.content_type = "text/plain";
        }

        std::ostringstream response;
        response << "HTTP/1.1 " << res.status << " " << ReasonPhrase(res.status) << "\r\n";
        response << "Content-Type: " << res.content_type << "\r\n";
        response << "Content-Length: " << res.body.size() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << res.body;
        const auto payload = response.str();
        (void)::send(client_fd, payload.data(), payload.size(), 0);
    }
#endif

    std::vector<Route> get_routes_;
    std::vector<Route> post_routes_;
    std::atomic<bool> running_{false};
    int server_fd_{-1};
    int listen_port_{0};
};

}  // namespace httplib

#endif
