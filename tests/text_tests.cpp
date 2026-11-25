#include <boost/test/unit_test.hpp>

#include "text.h"

using namespace crc;

BOOST_AUTO_TEST_SUITE(text)

BOOST_AUTO_TEST_CASE(lowercases_latin_and_cyrillic)
{
    BOOST_TEST(lowercase("P_GB") == "p_gb");
    BOOST_TEST(lowercase("АБВЁЖЗИЙПРСТЯ") == "абвёжзийпрстя");
    BOOST_TEST(lowercase("Гилберт ДСК") == "гилберт дск");
    BOOST_TEST(lowercase("уже") == "уже");
}

BOOST_AUTO_TEST_CASE(parses_reals_with_optional_comma)
{
    BOOST_TEST(*parseRealText(" 0,005 ", true) == 0.005);
    BOOST_TEST(!parseRealText("0,005", false));
    BOOST_TEST(*parseRealText("1e-3", false) == 0.001);
    BOOST_TEST(!parseRealText("", true));
    BOOST_TEST(!parseRealText("0.1x", true));
    BOOST_TEST(!parseRealText("inf", true));
}

BOOST_AUTO_TEST_CASE(parses_counts)
{
    BOOST_TEST(*parseCountText("3 000 000") == 3000000u);
    BOOST_TEST(*parseCountText("3e6") == 3000000u);
    BOOST_TEST(*parseCountText("18446744073709551615") == 18446744073709551615u);
    BOOST_TEST(!parseCountText("-1"));
    BOOST_TEST(!parseCountText("1.5"));
    BOOST_TEST(!parseCountText("abc"));
}

BOOST_AUTO_TEST_CASE(formats_numbers)
{
    BOOST_TEST(formatWithComma(0.5333, 3) == "0,533");
    BOOST_TEST(formatSignificant(1.07826, 4, false) == "1.078");
    BOOST_TEST(formatSignificant(222.345, 4, true) == "222,3");
    BOOST_TEST(formatSignificant(12345.6, 4, false) == "12346");
}

BOOST_AUTO_TEST_SUITE_END()
