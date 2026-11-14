#include <boost/test/unit_test.hpp>

#include "codes.h"
#include "poly.h"

#include <stdexcept>

using namespace crc;

namespace {

Word multiplyMod2(Word left, Word right)
{
    Word product = 0;
    for (int bit = 0; bit <= degree(right); ++bit) {
        if ((right >> bit) & 1)
            product ^= left << bit;
    }
    return product;
}

bool dividesXnPlusOne(Word generator, int n)
{
    Word remainder = 1;
    for (int step = 0; step < n; ++step) {
        remainder <<= 1;
        if ((remainder >> degree(generator)) & 1)
            remainder ^= generator;
    }
    return (remainder ^ 1) == 0;
}

}

BOOST_AUTO_TEST_SUITE(poly)

BOOST_AUTO_TEST_CASE(degree_of_simple_polynomials)
{
    BOOST_TEST(degree(0) == -1);
    BOOST_TEST(degree(1) == 0);
    BOOST_TEST(degree(0b1011) == 3);
    BOOST_TEST(degree(Word{1} << 63) == 63);
}

BOOST_AUTO_TEST_CASE(low_bits_mask_edges)
{
    BOOST_TEST(lowBitsMask(0) == 0u);
    BOOST_TEST(lowBitsMask(3) == 0b111u);
    BOOST_TEST(lowBitsMask(63) == (~Word{0} >> 1));
    BOOST_TEST(lowBitsMask(64) == ~Word{0});
}

BOOST_AUTO_TEST_CASE(parses_binary_and_degree_list)
{
    BOOST_TEST(parsePolynomial("10011") == 0b10011u);
    BOOST_TEST(parsePolynomial("[4;1;0]") == 0b10011u);
    BOOST_TEST(parsePolynomial(" [4; 1; 0] ") == 0b10011u);
    BOOST_TEST(parsePolynomial("[0;1;4]") == 0b10011u);
    BOOST_TEST(parsePolynomial("[63;0]") == ((Word{1} << 63) | 1));
}

BOOST_AUTO_TEST_CASE(rejects_invalid_input)
{
    BOOST_CHECK_THROW(parsePolynomial(""), std::invalid_argument);
    BOOST_CHECK_THROW(parsePolynomial("10201"), std::invalid_argument);
    BOOST_CHECK_THROW(parsePolynomial("abc"), std::invalid_argument);
    BOOST_CHECK_THROW(parsePolynomial("0011"), std::invalid_argument);
    BOOST_CHECK_THROW(parsePolynomial(std::string(65, '1')), std::invalid_argument);
    BOOST_CHECK_THROW(parsePolynomial("[4;1;0"), std::invalid_argument);
    BOOST_CHECK_THROW(parsePolynomial("[]"), std::invalid_argument);
    BOOST_CHECK_THROW(parsePolynomial("[4;;0]"), std::invalid_argument);
    BOOST_CHECK_THROW(parsePolynomial("[4;-1;0]"), std::invalid_argument);
    BOOST_CHECK_THROW(parsePolynomial("[64;0]"), std::invalid_argument);
    BOOST_CHECK_THROW(parsePolynomial("[4;4;0]"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(binary_output)
{
    BOOST_TEST(toBinary(0b1011, 7) == "0001011");
    BOOST_TEST(toBinary(0b10011) == "10011");
    BOOST_TEST(toDegreeList(0b10011) == "[4;1;0]");
    BOOST_TEST(toBinary(~Word{0}, 64) == std::string(64, '1'));
}

BOOST_AUTO_TEST_CASE(remainder_by_long_division)
{
    BOOST_TEST(remainderMod2(0b1101000, 7, 0b1011) == 0b001u);
    BOOST_TEST(remainderMod2(0b1101001, 7, 0b1011) == 0u);
    BOOST_TEST(remainderMod2(0b10, 2, 0b1011) == 0b10u);
    BOOST_CHECK_THROW(remainderMod2(1, 1, 0), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(remainder_with_full_64_bit_words)
{
    const Word generator = (Word{1} << 63) | 1;
    BOOST_TEST(remainderMod2(~Word{0}, 64, generator) == ((~Word{0} >> 1) ^ 1));
    const Word product = multiplyMod2(0b11, 0b1000011);
    BOOST_TEST(remainderMod2(product, 64, 0b1000011) == 0u);
}

BOOST_AUTO_TEST_CASE(known_codes_are_consistent)
{
    for (const KnownCode& code : knownCodes()) {
        BOOST_TEST_CONTEXT("n = " << code.n << ", k = " << code.k)
        {
            const Word fromBinary = parsePolynomial(code.binary);
            const Word fromDegrees = parsePolynomial(code.degrees);
            BOOST_TEST(fromBinary == fromDegrees);
            BOOST_TEST(degree(fromBinary) == code.n - code.k);
            BOOST_TEST(dividesXnPlusOne(fromBinary, code.n));
        }
    }
}

BOOST_AUTO_TEST_CASE(textbook_variant_of_31_6_does_not_divide)
{
    const Word textbook = parsePolynomial("11001011011110101000100101");
    BOOST_TEST(!dividesXnPlusOne(textbook, 31));
}

BOOST_AUTO_TEST_SUITE_END()
