#pragma once

#include <string>
#include <utility>
#include <vector>

namespace crc {

struct JsonValue {
    enum class Kind { Null, Boolean, Number, String, Array, Object };

    Kind kind = Kind::Null;
    int line = 1;
    bool boolean = false;
    std::string text;
    std::vector<JsonValue> items;
    std::vector<std::pair<std::string, JsonValue>> members;

    const JsonValue* find(const std::string& key) const;
    double number() const;
};

JsonValue parseJson(const std::string& text);

std::string quoteJson(const std::string& text);

}
