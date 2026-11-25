#include "json.h"

#include "text.h"

#include <cstdio>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace crc {

namespace {

constexpr int maxDepth = 64;
constexpr char32_t highSurrogateFirst = 0xD800;
constexpr char32_t lowSurrogateFirst = 0xDC00;
constexpr char32_t surrogateEnd = 0xE000;

bool isDigit(char symbol)
{
    return symbol >= '0' && symbol <= '9';
}

int hexDigit(char symbol)
{
    if (isDigit(symbol))
        return symbol - '0';
    if (symbol >= 'a' && symbol <= 'f')
        return symbol - 'a' + 10;
    if (symbol >= 'A' && symbol <= 'F')
        return symbol - 'A' + 10;
    return -1;
}

std::string quoted(char symbol)
{
    return std::string("«") + symbol + "»";
}

class Parser {
public:
    explicit Parser(const std::string& text)
        : text_(text)
    {
    }

    JsonValue parseDocument()
    {
        skipWhitespace();
        JsonValue value = parseValue(0);
        skipWhitespace();
        if (!atEnd())
            fail("лишние символы после конца документа");
        return value;
    }

private:
    [[noreturn]] void fail(const std::string& message) const
    {
        const std::size_t column = position_ - lineStart_ + 1;
        throw std::invalid_argument(
            "JSON, строка " + std::to_string(line_) + ", позиция " + std::to_string(column) + ": " + message);
    }

    bool atEnd() const { return position_ >= text_.size(); }

    char peek() const { return atEnd() ? '\0' : text_[position_]; }

    char take()
    {
        if (atEnd())
            fail("неожиданный конец документа");
        return text_[position_++];
    }

    void skipWhitespace()
    {
        while (!atEnd()) {
            const char symbol = text_[position_];
            if (symbol == '\n') {
                ++line_;
                lineStart_ = position_ + 1;
            } else if (symbol != ' ' && symbol != '\t' && symbol != '\r') {
                return;
            }
            ++position_;
        }
    }

    void expect(char symbol)
    {
        if (peek() != symbol)
            fail("ожидался символ " + quoted(symbol));
        ++position_;
    }

    JsonValue startValue(JsonValue::Kind kind) const
    {
        JsonValue value;
        value.kind = kind;
        value.line = line_;
        return value;
    }

    JsonValue parseValue(int depth)
    {
        if (depth > maxDepth)
            fail("слишком глубокая вложенность");
        const char symbol = peek();
        if (symbol == '{')
            return parseObject(depth);
        if (symbol == '[')
            return parseArray(depth);
        if (symbol == '"') {
            JsonValue value = startValue(JsonValue::Kind::String);
            value.text = parseString();
            return value;
        }
        if (symbol == '-' || isDigit(symbol))
            return parseNumber();
        return parseLiteral();
    }

    JsonValue parseObject(int depth)
    {
        JsonValue object = startValue(JsonValue::Kind::Object);
        expect('{');
        skipWhitespace();
        if (peek() == '}') {
            ++position_;
            return object;
        }
        while (true) {
            skipWhitespace();
            if (peek() != '"')
                fail("ожидалось имя поля в кавычках");
            std::string key = parseString();
            skipWhitespace();
            expect(':');
            skipWhitespace();
            object.members.emplace_back(std::move(key), parseValue(depth + 1));
            skipWhitespace();
            if (peek() == '}') {
                ++position_;
                return object;
            }
            if (peek() != ',')
                fail("ожидался символ «,» или «}»");
            ++position_;
        }
    }

    JsonValue parseArray(int depth)
    {
        JsonValue array = startValue(JsonValue::Kind::Array);
        expect('[');
        skipWhitespace();
        if (peek() == ']') {
            ++position_;
            return array;
        }
        while (true) {
            skipWhitespace();
            array.items.push_back(parseValue(depth + 1));
            skipWhitespace();
            if (peek() == ']') {
                ++position_;
                return array;
            }
            if (peek() != ',')
                fail("ожидался символ «,» или «]»");
            ++position_;
        }
    }

    char32_t parseHexQuad()
    {
        char32_t value = 0;
        for (int index = 0; index < 4; ++index) {
            const int digit = hexDigit(take());
            if (digit < 0)
                fail("после \\u должны идти четыре шестнадцатеричные цифры");
            value = value * 16 + static_cast<char32_t>(digit);
        }
        return value;
    }

    char32_t parseUnicodeEscape()
    {
        const char32_t first = parseHexQuad();
        if (first >= lowSurrogateFirst && first < surrogateEnd)
            fail("непарный суррогат \\u в строке");
        if (first < highSurrogateFirst || first >= lowSurrogateFirst)
            return first;
        if (take() != '\\' || take() != 'u')
            fail("после старшего суррогата ожидался младший \\uDC00…\\uDFFF");
        const char32_t second = parseHexQuad();
        if (second < lowSurrogateFirst || second >= surrogateEnd)
            fail("после старшего суррогата ожидался младший \\uDC00…\\uDFFF");
        return 0x10000 + ((first - highSurrogateFirst) << 10) + (second - lowSurrogateFirst);
    }

    void appendEscape(std::string& result)
    {
        const char symbol = take();
        switch (symbol) {
        case '"':
        case '\\':
        case '/':
            result += symbol;
            return;
        case 'b':
            result += '\b';
            return;
        case 'f':
            result += '\f';
            return;
        case 'n':
            result += '\n';
            return;
        case 'r':
            result += '\r';
            return;
        case 't':
            result += '\t';
            return;
        case 'u':
            appendUtf8(result, parseUnicodeEscape());
            return;
        default:
            --position_;
            fail("недопустимая escape-последовательность \\" + std::string(1, symbol));
        }
    }

    std::string parseString()
    {
        expect('"');
        std::string result;
        while (true) {
            if (atEnd())
                fail("строка не закрыта кавычкой");
            const char symbol = text_[position_];
            if (static_cast<unsigned char>(symbol) < 0x20)
                fail("управляющий символ внутри строки");
            ++position_;
            if (symbol == '"')
                return result;
            if (symbol == '\\')
                appendEscape(result);
            else
                result += symbol;
        }
    }

    void takeDigits()
    {
        if (!isDigit(peek()))
            fail("ожидалась цифра");
        while (isDigit(peek()))
            ++position_;
    }

    JsonValue parseNumber()
    {
        JsonValue value = startValue(JsonValue::Kind::Number);
        const std::size_t start = position_;
        if (peek() == '-')
            ++position_;
        if (peek() == '0') {
            ++position_;
            if (isDigit(peek()))
                fail("число не может начинаться с нуля");
        } else {
            takeDigits();
        }
        if (peek() == '.') {
            ++position_;
            takeDigits();
        }
        if (peek() == 'e' || peek() == 'E') {
            ++position_;
            if (peek() == '+' || peek() == '-')
                ++position_;
            takeDigits();
        }
        value.text = text_.substr(start, position_ - start);
        return value;
    }

    bool takeWord(const std::string& word)
    {
        if (text_.compare(position_, word.size(), word) != 0)
            return false;
        position_ += word.size();
        return true;
    }

    JsonValue parseLiteral()
    {
        JsonValue value = startValue(JsonValue::Kind::Null);
        if (takeWord("null"))
            return value;
        value.kind = JsonValue::Kind::Boolean;
        if (takeWord("true")) {
            value.boolean = true;
            return value;
        }
        if (takeWord("false"))
            return value;
        if (atEnd())
            fail("неожиданный конец документа");
        fail("неожиданный символ " + quoted(peek()));
    }

    const std::string& text_;
    std::size_t position_ = 0;
    std::size_t lineStart_ = 0;
    int line_ = 1;
};

void appendEscapedCharacter(std::string& result, unsigned char symbol)
{
    char buffer[8];
    std::snprintf(buffer, sizeof buffer, "\\u%04x", static_cast<unsigned>(symbol));
    result += buffer;
}

}

const JsonValue* JsonValue::find(const std::string& key) const
{
    for (const auto& member : members) {
        if (member.first == key)
            return &member.second;
    }
    return nullptr;
}

double JsonValue::number() const
{
    std::istringstream stream(text);
    stream.imbue(std::locale::classic());
    double value = 0.0;
    stream >> value;
    return value;
}

JsonValue parseJson(const std::string& text)
{
    return Parser(text).parseDocument();
}

std::string quoteJson(const std::string& text)
{
    std::string result = "\"";
    for (const char symbol : text) {
        const auto code = static_cast<unsigned char>(symbol);
        if (symbol == '"' || symbol == '\\') {
            result += '\\';
            result += symbol;
        } else if (symbol == '\n') {
            result += "\\n";
        } else if (symbol == '\t') {
            result += "\\t";
        } else if (code < 0x20) {
            appendEscapedCharacter(result, code);
        } else {
            result += symbol;
        }
    }
    return result + "\"";
}

}
