#pragma once

#if __has_include("vendor/httplib.h")
#include "vendor/httplib.h"
#else

#include <functional>
#include <cstring>
#include <map>
#include <sstream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace httplib {

struct Request {
    std::map<std::string, std::string> headers;
    std::string body;
    std::string path;
    std::map<std::string, std::string> params;

    [[nodiscard]] bool has_header(const std::string& key) const {
        return headers.find(key) != headers.end();
    }

    [[nodiscard]] std::string get_header_value(const std::string& key) const {
        const auto it = headers.find(key);
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
        get_handlers_[pattern] = std::move(handler);
    }

    void Post(const std::string& pattern, Handler handler) {
        post_handlers_[pattern] = std::move(handler);
    }

    bool listen(const char* host, int port) {
#if defined(_WIN32)
        (void)host;
        (void)port;
        return false;
#else
        (void)host;

        const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            return false;
        }

        int opt = 1;
        ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (::bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ::close(server_fd);
            return false;
        }

        if (::listen(server_fd, 16) < 0) {
            ::close(server_fd);
            return false;
        }

        for (;;) {
            const int client_fd = ::accept(server_fd, nullptr, nullptr);
            if (client_fd < 0) {
                continue;
            }
            HandleClient(client_fd);
            ::close(client_fd);
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
            req.headers[key] = value;
        }

        Response res{};
        res.status = 404;
        res.set_content("{\"error\":\"Not Found\"}", "application/json");

        if (method == "GET") {
            const auto it = get_handlers_.find(req.path);
            if (it != get_handlers_.end()) {
                res = Response{};
                it->second(req, res);
            }
        } else if (method == "POST") {
            const auto it = post_handlers_.find(req.path);
            if (it != post_handlers_.end()) {
                res = Response{};
                it->second(req, res);
            } else {
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

    std::unordered_map<std::string, Handler> get_handlers_;
    std::unordered_map<std::string, Handler> post_handlers_;
};

}  // namespace httplib

#endif
