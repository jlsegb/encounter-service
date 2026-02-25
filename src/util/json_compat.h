#pragma once

#if __has_include("vendor/json.hpp")
#include "vendor/json.hpp"
#else

#include <map>
#include <string>
#include <utility>

namespace nlohmann {

class json {
public:
    enum class Kind {
        Null,
        Object,
        String
    };

    json() = default;

    json(const char* value)
        : kind_(Kind::String), string_value_(value ? value : "") {}

    json(std::string value)
        : kind_(Kind::String), string_value_(std::move(value)) {}

    static json object() {
        json j;
        j.kind_ = Kind::Object;
        return j;
    }

    json& operator[](const std::string& key) {
        if (kind_ != Kind::Object) {
            kind_ = Kind::Object;
            object_value_.clear();
            string_value_.clear();
        }
        return object_value_[key];
    }

    json& operator=(const char* value) {
        kind_ = Kind::String;
        string_value_ = value ? value : "";
        object_value_.clear();
        return *this;
    }

    json& operator=(const std::string& value) {
        kind_ = Kind::String;
        string_value_ = value;
        object_value_.clear();
        return *this;
    }

    [[nodiscard]] bool is_object() const {
        return kind_ == Kind::Object;
    }

    [[nodiscard]] std::string dump() const {
        switch (kind_) {
            case Kind::Null:
                return "null";
            case Kind::String:
                return "\"" + Escape(string_value_) + "\"";
            case Kind::Object:
                return DumpObject();
        }
        return "null";
    }

private:
    [[nodiscard]] static std::string Escape(const std::string& input) {
        std::string out;
        out.reserve(input.size());
        for (char c : input) {
            if (c == '"' || c == '\\') {
                out.push_back('\\');
            }
            out.push_back(c);
        }
        return out;
    }

    [[nodiscard]] std::string DumpObject() const {
        std::string out = "{";
        bool first = true;
        for (const auto& [key, value] : object_value_) {
            if (!first) {
                out += ",";
            }
            first = false;
            out += "\"";
            out += Escape(key);
            out += "\":";
            out += value.dump();
        }
        out += "}";
        return out;
    }

    Kind kind_{Kind::Null};
    std::string string_value_;
    std::map<std::string, json> object_value_;
};

}  // namespace nlohmann

#endif
