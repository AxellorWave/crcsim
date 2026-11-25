#include "probability_format.h"

#include <cmath>

namespace {

const int plainExponentLimit = -2;
const double mantissaRoundingLimit = 9.995;

QString withDecimalComma(QString text)
{
    text.replace(QLatin1Char('.'), QLatin1Char(','));
    return text;
}

QChar superscriptDigit(int digit)
{
    static const char16_t digits[] = u"⁰¹²³⁴⁵⁶⁷⁸⁹";
    return QChar(digits[digit]);
}

}

QString superscriptNumber(int value)
{
    QString text = value < 0 ? QStringLiteral("⁻") : QString();
    for (const QChar digit : QString::number(std::abs(value)))
        text.append(superscriptDigit(digit.digitValue()));
    return text;
}

QString powerOfTen(int exponent)
{
    if (exponent == 0)
        return QStringLiteral("1");
    return QStringLiteral("10") + superscriptNumber(exponent);
}

QString formatSmallProbability(double value)
{
    if (!(value > 0.0))
        return QStringLiteral("0");
    int exponent = static_cast<int>(std::floor(std::log10(value)));
    if (exponent >= plainExponentLimit)
        return withDecimalComma(QString::number(value, 'g', 3));
    double mantissa = value / std::pow(10.0, exponent);
    if (mantissa >= mantissaRoundingLimit) {
        mantissa /= 10.0;
        ++exponent;
    }
    return withDecimalComma(QString::number(mantissa, 'f', 2)) + QStringLiteral("·") + powerOfTen(exponent);
}
