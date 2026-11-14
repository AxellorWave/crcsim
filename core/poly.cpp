#include "poly.h"

#include <cctype>
#include <stdexcept>
#include <vector>

namespace crc {

int degree(Word polynomial)
{
    int result = -1;
    while (polynomial != 0) {
        polynomial >>= 1;
        ++result;
    }
    return result;
}

Word lowBitsMask(int bitCount)
{
    if (bitCount <= 0)
        return 0;
    if (bitCount >= maxWordBits)
        return ~Word{0};
    return (Word{1} << bitCount) - 1;
}

Word remainderMod2(Word dividend, int dividendLength, Word divisor)
{
    const int divisorDegree = degree(divisor);
    if (divisorDegree < 0)
        throw std::invalid_argument("деление на нулевой многочлен");
    for (int bit = dividendLength - 1; bit >= divisorDegree; --bit) {
        if ((dividend >> bit) & 1)
            dividend ^= divisor << (bit - divisorDegree);
    }
    return dividend & lowBitsMask(divisorDegree);
}

std::string toBinary(Word value, int length)
{
    std::string text;
    for (int bit = length - 1; bit >= 0; --bit)
        text += ((value >> bit) & 1) ? '1' : '0';
    return text;
}

std::string toBinary(Word polynomial)
{
    return toBinary(polynomial, degree(polynomial) + 1);
}

std::string toDegreeList(Word polynomial)
{
    std::string text = "[";
    for (int bit = degree(polynomial); bit >= 0; --bit) {
        if (!((polynomial >> bit) & 1))
            continue;
        if (text.size() > 1)
            text += ';';
        text += std::to_string(bit);
    }
    return text + "]";
}

namespace {

std::string trimmed(const std::string& text)
{
    std::size_t first = 0;
    std::size_t last = text.size();
    while (first < last && std::isspace(static_cast<unsigned char>(text[first])))
        ++first;
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1])))
        --last;
    return text.substr(first, last - first);
}

Word parseBinary(const std::string& text)
{
    if (text.size() > static_cast<std::size_t>(maxWordBits))
        throw std::invalid_argument("g(x): не более 64 двоичных разрядов");
    Word polynomial = 0;
    for (char symbol : text) {
        if (symbol != '0' && symbol != '1')
            throw std::invalid_argument("g(x): в двоичной записи допустимы только символы 0 и 1");
        polynomial = (polynomial << 1) | static_cast<Word>(symbol - '0');
    }
    if (text.front() != '1')
        throw std::invalid_argument("g(x): первый (старший) коэффициент должен быть равен 1");
    return polynomial;
}

std::vector<std::string> splitDegrees(const std::string& text)
{
    std::vector<std::string> items;
    std::string current;
    for (char symbol : text) {
        if (symbol == ';' || symbol == ',') {
            items.push_back(trimmed(current));
            current.clear();
        } else {
            current += symbol;
        }
    }
    items.push_back(trimmed(current));
    return items;
}

int parseDegree(const std::string& item)
{
    if (item.empty())
        throw std::invalid_argument("g(x): пустой элемент в списке степеней");
    for (char symbol : item) {
        if (!std::isdigit(static_cast<unsigned char>(symbol)))
            throw std::invalid_argument("g(x): степень «" + item + "» должна быть целым неотрицательным числом");
    }
    if (item.size() > 2 || std::stoi(item) >= maxWordBits)
        throw std::invalid_argument("g(x): степень " + item + " больше 63");
    return std::stoi(item);
}

Word parseDegreeList(const std::string& text)
{
    if (text.back() != ']')
        throw std::invalid_argument("g(x): список степеней должен заканчиваться символом ]");
    const std::string body = trimmed(text.substr(1, text.size() - 2));
    if (body.empty())
        throw std::invalid_argument("g(x): список степеней пуст");
    Word polynomial = 0;
    for (const std::string& item : splitDegrees(body)) {
        const Word term = Word{1} << parseDegree(item);
        if (polynomial & term)
            throw std::invalid_argument("g(x): степень " + item + " указана дважды");
        polynomial |= term;
    }
    return polynomial;
}

}

Word parsePolynomial(const std::string& text)
{
    const std::string input = trimmed(text);
    if (input.empty())
        throw std::invalid_argument("g(x): не задан образующий многочлен");
    if (input.front() == '[')
        return parseDegreeList(input);
    return parseBinary(input);
}

}
