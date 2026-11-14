#include "input.h"

#include <algorithm>
#include <cctype>
#include <locale>
#include <sstream>

namespace console {

namespace {

std::string withoutGroupSpaces(const std::string& text)
{
    std::string result;
    for (char symbol : text) {
        if (symbol != ' ' && symbol != '_')
            result += symbol;
    }
    return result;
}

template <typename Number>
std::optional<Number> parseWhole(const std::string& text)
{
    std::istringstream stream(text);
    stream.imbue(std::locale::classic());
    Number value{};
    stream >> std::noskipws >> value;
    if (!stream || stream.peek() != std::istringstream::traits_type::eof())
        return std::nullopt;
    return value;
}

}

std::string trim(const std::string& text)
{
    const auto isSpace = [](unsigned char symbol) { return std::isspace(symbol) != 0; };
    auto first = std::find_if_not(text.begin(), text.end(), isSpace);
    auto last = std::find_if_not(text.rbegin(), text.rend(), isSpace).base();
    return first < last ? std::string(first, last) : std::string();
}

std::optional<int> parseInteger(const std::string& text)
{
    return parseWhole<int>(withoutGroupSpaces(trim(text)));
}

std::optional<std::uint64_t> parseCount(const std::string& text)
{
    const std::string digits = withoutGroupSpaces(trim(text));
    if (digits.empty() || !std::all_of(digits.begin(), digits.end(), [](unsigned char c) { return std::isdigit(c); }))
        return std::nullopt;
    return parseWhole<std::uint64_t>(digits);
}

std::optional<double> parseReal(const std::string& text)
{
    std::string normalized = trim(text);
    std::replace(normalized.begin(), normalized.end(), ',', '.');
    if (normalized.empty())
        return std::nullopt;
    return parseWhole<double>(normalized);
}

}
