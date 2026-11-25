#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace crc {

std::string trimSpaces(const std::string& text);

std::string lowercase(const std::string& text);

void appendUtf8(std::string& target, char32_t codePoint);

std::string withoutByteOrderMark(const std::string& text);

std::optional<double> parseRealText(const std::string& text, bool allowDecimalComma);

std::optional<std::uint64_t> parseCountText(const std::string& text);

std::string formatWithComma(double value, int significantDigits);

std::string formatSignificant(double value, int significantDigits, bool decimalComma);

}
