#include "text.h"

#include "results.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace crc {

namespace {

constexpr unsigned char cyrillicLeadLow = 0xD0;
constexpr unsigned char cyrillicLeadHigh = 0xD1;
constexpr double largestExactCount = 9007199254740992.0;

bool isSpace(unsigned char symbol)
{
    return std::isspace(symbol) != 0;
}

void appendLowerCyrillic(std::string& result, unsigned char second)
{
    if (second >= 0x90 && second <= 0x9F) {
        result += static_cast<char>(cyrillicLeadLow);
        result += static_cast<char>(second + 0x20);
    } else if (second >= 0xA0 && second <= 0xAF) {
        result += static_cast<char>(cyrillicLeadHigh);
        result += static_cast<char>(second - 0x20);
    } else if (second == 0x81) {
        result += static_cast<char>(cyrillicLeadHigh);
        result += static_cast<char>(0x91);
    } else {
        result += static_cast<char>(cyrillicLeadLow);
        result += static_cast<char>(second);
    }
}

std::string withoutGroupSeparators(const std::string& text)
{
    std::string result;
    for (char symbol : text) {
        if (symbol != ' ' && symbol != '_')
            result += symbol;
    }
    return result;
}

std::optional<double> parseClassic(const std::string& text)
{
    if (text.empty() || isSpace(static_cast<unsigned char>(text.front())))
        return std::nullopt;
    std::istringstream stream(text);
    stream.imbue(std::locale::classic());
    double value = 0.0;
    stream >> std::noskipws >> value;
    if (!stream || stream.peek() != std::istringstream::traits_type::eof() || !std::isfinite(value))
        return std::nullopt;
    return value;
}

bool onlyDigits(const std::string& text)
{
    return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}

std::optional<std::uint64_t> parseDigits(const std::string& digits)
{
    std::istringstream stream(digits);
    stream.imbue(std::locale::classic());
    std::uint64_t value = 0;
    stream >> value;
    if (!stream || stream.peek() != std::istringstream::traits_type::eof())
        return std::nullopt;
    return value;
}

}

std::string trimSpaces(const std::string& text)
{
    auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char c) { return isSpace(c); });
    auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) { return isSpace(c); }).base();
    return first < last ? std::string(first, last) : std::string();
}

std::string lowercase(const std::string& text)
{
    std::string result;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const auto symbol = static_cast<unsigned char>(text[index]);
        if (symbol == cyrillicLeadLow && index + 1 < text.size()) {
            appendLowerCyrillic(result, static_cast<unsigned char>(text[++index]));
            continue;
        }
        result += symbol < 0x80 ? static_cast<char>(std::tolower(symbol)) : static_cast<char>(symbol);
    }
    return result;
}

void appendUtf8(std::string& target, char32_t codePoint)
{
    if (codePoint < 0x80) {
        target += static_cast<char>(codePoint);
    } else if (codePoint < 0x800) {
        target += static_cast<char>(0xC0 | (codePoint >> 6));
        target += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x10000) {
        target += static_cast<char>(0xE0 | (codePoint >> 12));
        target += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        target += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
        target += static_cast<char>(0xF0 | (codePoint >> 18));
        target += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        target += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        target += static_cast<char>(0x80 | (codePoint & 0x3F));
    }
}

std::string withoutByteOrderMark(const std::string& text)
{
    const std::string mark = "\xEF\xBB\xBF";
    return text.compare(0, mark.size(), mark) == 0 ? text.substr(mark.size()) : text;
}

std::optional<double> parseRealText(const std::string& text, bool allowDecimalComma)
{
    std::string normalized = trimSpaces(text);
    if (allowDecimalComma)
        std::replace(normalized.begin(), normalized.end(), ',', '.');
    return parseClassic(normalized);
}

std::optional<std::uint64_t> parseCountText(const std::string& text)
{
    const std::string compact = withoutGroupSeparators(trimSpaces(text));
    if (onlyDigits(compact))
        return parseDigits(compact);
    const std::optional<double> real = parseClassic(compact);
    if (!real || *real < 0.0 || *real > largestExactCount || std::floor(*real) != *real)
        return std::nullopt;
    return static_cast<std::uint64_t>(*real);
}

std::string formatWithComma(double value, int significantDigits)
{
    std::string text = formatNumber(value, significantDigits);
    std::replace(text.begin(), text.end(), '.', ',');
    return text;
}

std::string formatSignificant(double value, int significantDigits, bool decimalComma)
{
    const int integerDigits = value >= 1.0 ? static_cast<int>(std::floor(std::log10(value))) + 1 : 1;
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(std::max(0, significantDigits - integerDigits)) << value;
    std::string text = stream.str();
    if (decimalComma)
        std::replace(text.begin(), text.end(), '.', ',');
    return text;
}

}
