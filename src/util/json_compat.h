#pragma once

#if __has_include("vendor/json.hpp")
#include "vendor/json.hpp"
#else

#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace nlohmann {

class json {
public:
    enum class Kind {
        Null,
        Object,
        Array,
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

    static json array() {
        json j;
        j.kind_ = Kind::Array;
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
        array_value_.clear();
        return *this;
    }

    json& operator=(const std::string& value) {
        kind_ = Kind::String;
        string_value_ = value;
        object_value_.clear();
        array_value_.clear();
        return *this;
    }

    [[nodiscard]] bool is_object() const {
        return kind_ == Kind::Object;
    }

    [[nodiscard]] bool is_string() const {
        return kind_ == Kind::String;
    }

    [[nodiscard]] bool is_array() const {
        return kind_ == Kind::Array;
    }

    [[nodiscard]] bool contains(const std::string& key) const {
        return kind_ == Kind::Object && object_value_.find(key) != object_value_.end();
    }

    [[nodiscard]] const json& at(const std::string& key) const {
        if (kind_ != Kind::Object) {
            throw std::out_of_range("json is not an object");
        }
        return object_value_.at(key);
    }

    template <typename T>
    [[nodiscard]] T get() const {
        if constexpr (std::is_same_v<T, std::string>) {
            if (kind_ != Kind::String) {
                throw std::runtime_error("json value is not a string");
            }
            return string_value_;
        } else {
            static_assert(std::is_same_v<T, std::string>, "json_compat only supports get<std::string>()");
        }
    }

    void push_back(const json& value) {
        if (kind_ != Kind::Array) {
            kind_ = Kind::Array;
            object_value_.clear();
            string_value_.clear();
            array_value_.clear();
        }
        array_value_.push_back(value);
    }

    [[nodiscard]] std::string dump() const {
        switch (kind_) {
            case Kind::Null:
                return "null";
            case Kind::String:
                return "\"" + Escape(string_value_) + "\"";
            case Kind::Object:
                return DumpObject();
            case Kind::Array:
                return DumpArray();
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

    [[nodiscard]] std::string DumpArray() const {
        std::string out = "[";
        bool first = true;
        for (const auto& value : array_value_) {
            if (!first) {
                out += ",";
            }
            first = false;
            out += value.dump();
        }
        out += "]";
        return out;
    }

    Kind kind_{Kind::Null};
    std::string string_value_;
    std::map<std::string, json> object_value_;
    std::vector<json> array_value_;
};

}  // namespace nlohmann

#endif
