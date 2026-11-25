#include "walkthrough.h"

#include <stdexcept>

namespace crc {

Division divideMod2(Word dividend, int length, Word divisor)
{
    const int divisorDegree = degree(divisor);
    if (divisorDegree < 0)
        throw std::invalid_argument("деление на нулевой многочлен");
    Division division{dividend, length, divisor, {}, 0};
    Word current = dividend;
    for (int bit = length - 1; bit >= divisorDegree; --bit) {
        if (!((current >> bit) & 1))
            continue;
        const Word subtrahend = divisor << (bit - divisorDegree);
        division.steps.push_back({bit, current, subtrahend, current ^ subtrahend});
        current ^= subtrahend;
    }
    division.remainder = current & lowBitsMask(divisorDegree);
    return division;
}

namespace {

Verdict judge(Word errors, Word syndrome)
{
    if (syndrome != 0)
        return Verdict::Detected;
    return errors == 0 ? Verdict::Correct : Verdict::Undetected;
}

std::string spaces(int count)
{
    return std::string(static_cast<std::size_t>(count > 0 ? count : 0), ' ');
}

std::string tail(Word value, int length, int fromColumn)
{
    return toBinary(value, length).substr(static_cast<std::size_t>(fromColumn));
}

std::string divisionText(const Division& division, const std::string& indent)
{
    const int length = division.length;
    const int divisorLength = degree(division.divisor) + 1;
    std::string text = indent + toBinary(division.dividend, length) + "\n";
    for (const DivisionStep& step : division.steps) {
        const int column = length - 1 - step.position;
        text += indent + spaces(column) + toBinary(division.divisor, divisorLength)
            + spaces(length - column - divisorLength) + "   ⊕ g(x) под разрядом x^" + std::to_string(step.position)
            + "\n";
        text += indent + spaces(column) + std::string(static_cast<std::size_t>(length - column), '-') + "\n";
        text += indent + spaces(column) + tail(step.after, length, column) + "\n";
    }
    return text;
}

std::string line(const std::string& name, const std::string& value)
{
    return name + " = " + value + "\n";
}

}

Walkthrough walkthrough(const CyclicCode& code, Word data, Word errors)
{
    const int checkCount = code.checkBitCount();
    Walkthrough trace;
    trace.code = code;
    trace.data = data & lowBitsMask(code.dataBitCount());
    trace.shifted = trace.data << checkCount;
    trace.encoding = divideMod2(trace.shifted, code.n, code.generator);
    trace.checkBits = trace.encoding.remainder;
    trace.codeword = trace.shifted | trace.checkBits;
    trace.errors = errors & lowBitsMask(code.n);
    trace.received = trace.codeword ^ trace.errors;
    trace.decoding = divideMod2(trace.received, code.n, code.generator);
    trace.syndrome = trace.decoding.remainder;
    trace.verdict = judge(trace.errors, trace.syndrome);
    return trace;
}

std::string verdictName(Verdict verdict)
{
    switch (verdict) {
    case Verdict::Correct:
        return "верно";
    case Verdict::Detected:
        return "ошибка обнаружена";
    case Verdict::Undetected:
        return "ложный приём";
    }
    return "";
}

std::string walkthroughText(const Walkthrough& trace)
{
    const CyclicCode& code = trace.code;
    const int n = code.n;
    const int h = code.checkBitCount();
    const std::string shiftedName = "D·x^" + std::to_string(h);
    std::string text = "Код (" + std::to_string(n) + ", " + std::to_string(code.dataBitCount())
        + "), g(x) = " + toBinary(code.generator) + ", h = " + std::to_string(h) + "\n\n";
    text += line("Данные D", toBinary(trace.data, code.dataBitCount()));
    text += line(shiftedName, toBinary(trace.shifted, n));
    text += "\nДеление " + shiftedName + " на g(x):\n";
    text += divisionText(trace.encoding, "  ");
    text += "\n" + line("Остаток C", toBinary(trace.checkBits, h));
    text += line("Кодовое слово F = " + shiftedName + " ⊕ C", toBinary(trace.codeword, n));
    text += "\n" + line("Вектор ошибок E", toBinary(trace.errors, n));
    text += line("Принято Y = F ⊕ E", toBinary(trace.received, n));
    text += "\nДеление Y на g(x):\n";
    text += divisionText(trace.decoding, "  ");
    text += "\n" + line("Синдром S", toBinary(trace.syndrome, h));
    text += "Итог: " + verdictName(trace.verdict) + "\n";
    return text;
}

}
