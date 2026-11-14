#pragma once

#include <QString>

#include <cstdint>
#include <optional>

std::optional<double> parseDecimal(const QString& text);

std::optional<std::uint64_t> parseUnsigned(const QString& text);

QString formatCount(std::uint64_t value);
