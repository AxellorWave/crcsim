#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace console {

std::optional<int> parseInteger(const std::string& text);

std::optional<std::uint64_t> parseCount(const std::string& text);

std::optional<double> parseReal(const std::string& text);

std::string trim(const std::string& text);

}
