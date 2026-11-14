#include "input_parsing.h"

#include <QLocale>

namespace {

QString withoutSpaces(const QString& text)
{
    QString result;
    for (const QChar character : text) {
        if (!character.isSpace())
            result.append(character);
    }
    return result;
}

bool onlyDigits(const QString& text)
{
    for (const QChar character : text) {
        if (character < QLatin1Char('0') || character > QLatin1Char('9'))
            return false;
    }
    return !text.isEmpty();
}

}

std::optional<double> parseDecimal(const QString& text)
{
    QString normalized = withoutSpaces(text);
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    const double value = QLocale::c().toDouble(normalized, &ok);
    if (!ok)
        return std::nullopt;
    return value;
}

std::optional<std::uint64_t> parseUnsigned(const QString& text)
{
    const QString normalized = withoutSpaces(text);
    if (!onlyDigits(normalized))
        return std::nullopt;
    bool ok = false;
    const qulonglong value = normalized.toULongLong(&ok);
    if (!ok)
        return std::nullopt;
    return static_cast<std::uint64_t>(value);
}

QString formatCount(std::uint64_t value)
{
    QString digits = QString::number(static_cast<qulonglong>(value));
    for (int position = static_cast<int>(digits.size()) - 3; position > 0; position -= 3)
        digits.insert(position, QChar(0x2009));
    return digits;
}
