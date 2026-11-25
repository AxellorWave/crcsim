#include <boost/test/unit_test.hpp>

#include "json.h"

#include <stdexcept>
#include <string>

using namespace crc;

namespace {

std::string errorOf(const std::string& text)
{
    try {
        parseJson(text);
    } catch (const std::invalid_argument& error) {
        return error.what();
    }
    return {};
}

bool rejects(const std::string& text)
{
    return !errorOf(text).empty();
}

}

BOOST_AUTO_TEST_SUITE(json)

BOOST_AUTO_TEST_CASE(reads_nested_structures)
{
    const JsonValue root = parseJson(R"({"a": [1, -2.5e3, true, false, null], "b": {"c": "x"}})");
    BOOST_TEST(static_cast<int>(root.kind) == static_cast<int>(JsonValue::Kind::Object));
    const JsonValue* list = root.find("a");
    BOOST_REQUIRE(list != nullptr);
    BOOST_TEST(list->items.size() == 5u);
    BOOST_TEST(list->items[0].number() == 1.0);
    BOOST_TEST(list->items[1].number() == -2500.0);
    BOOST_TEST(list->items[1].text == "-2.5e3");
    BOOST_TEST(list->items[2].boolean);
    BOOST_TEST(!list->items[3].boolean);
    BOOST_TEST(static_cast<int>(list->items[4].kind) == static_cast<int>(JsonValue::Kind::Null));
    BOOST_TEST(root.find("b")->find("c")->text == "x");
    BOOST_TEST(root.find("missing") == nullptr);
}

BOOST_AUTO_TEST_CASE(reads_empty_containers_and_whitespace)
{
    BOOST_TEST(parseJson(" \r\n\t[ ] ").items.empty());
    BOOST_TEST(parseJson("{}").members.empty());
    BOOST_TEST(parseJson("0").number() == 0.0);
    BOOST_TEST(parseJson("\"\"").text.empty());
}

BOOST_AUTO_TEST_CASE(decodes_escapes)
{
    BOOST_TEST(parseJson(R"("a\"b\\c\/d\n\t\r\b\f")").text == "a\"b\\c/d\n\t\r\b\f");
    BOOST_TEST(parseJson(R"("ДСК")").text == "ДСК");
    BOOST_TEST(parseJson(R"("é")").text == "\xC3\xA9");
    BOOST_TEST(parseJson(R"("😀")").text == "\xF0\x9F\x98\x80");
    BOOST_TEST(parseJson("\"ДСК\"").text == "ДСК");
}

BOOST_AUTO_TEST_CASE(tracks_lines_of_values)
{
    const JsonValue root = parseJson("[\n  {\"n\": 7},\n\n  {\"n\": 15}\n]");
    BOOST_TEST(root.items[0].line == 2);
    BOOST_TEST(root.items[1].line == 4);
    BOOST_TEST(root.items[1].find("n")->line == 4);
}

BOOST_AUTO_TEST_CASE(rejects_malformed_documents)
{
    BOOST_TEST(rejects(""));
    BOOST_TEST(rejects("[1, 2,]"));
    BOOST_TEST(rejects("{\"a\": 1,}"));
    BOOST_TEST(rejects("{a: 1}"));
    BOOST_TEST(rejects("[01]"));
    BOOST_TEST(rejects("[1.]"));
    BOOST_TEST(rejects("[.5]"));
    BOOST_TEST(rejects("[-]"));
    BOOST_TEST(rejects("[1e]"));
    BOOST_TEST(rejects("[+1]"));
    BOOST_TEST(rejects("[NaN]"));
    BOOST_TEST(rejects("[tru]"));
    BOOST_TEST(rejects("\"abc"));
    BOOST_TEST(rejects("\"a\nb\""));
    BOOST_TEST(rejects(R"("\x")"));
    BOOST_TEST(rejects(R"("\u12")"));
    BOOST_TEST(rejects(R"("\ud83d")"));
    BOOST_TEST(rejects(R"("\ude00")"));
    BOOST_TEST(rejects("[1] 2"));
    BOOST_TEST(rejects("{\"a\" 1}"));
    BOOST_TEST(rejects(std::string(100, '[') + std::string(100, ']')));
}

BOOST_AUTO_TEST_CASE(error_reports_line_and_column)
{
    BOOST_TEST(errorOf("[\n  1,\n  2,\n]") == "JSON, строка 4, позиция 1: неожиданный символ «]»");
    BOOST_TEST(errorOf("{\"a\": 1 \"b\": 2}") == "JSON, строка 1, позиция 9: ожидался символ «,» или «}»");
}

BOOST_AUTO_TEST_CASE(quotes_strings_for_output)
{
    BOOST_TEST(quoteJson("a\"b\\c\nd") == R"("a\"b\\c\nd")");
    BOOST_TEST(quoteJson(std::string(1, '\x01')) == R"("\u0001")");
    BOOST_TEST(parseJson(quoteJson("ДСК \"x\"\t")).text == "ДСК \"x\"\t");
}

BOOST_AUTO_TEST_SUITE_END()
