#include <boost/test/unit_test.hpp>

#include "codes.h"
#include "decoder.h"
#include "encoder.h"

#include <random>

using namespace crc;

namespace {

CyclicCode codeFromTable(const KnownCode& known)
{
    return CyclicCode{known.n, parsePolynomial(known.binary)};
}

Word randomData(std::mt19937_64& engine, const CyclicCode& code)
{
    return engine() & lowBitsMask(code.dataBitCount());
}

}

BOOST_AUTO_TEST_SUITE(coder)

BOOST_AUTO_TEST_CASE(code_parameters_are_derived_from_generator)
{
    const CyclicCode code{15, 0b111010001};
    BOOST_TEST(code.checkBitCount() == 8);
    BOOST_TEST(code.dataBitCount() == 7);
    BOOST_TEST(code.redundancy() == 8.0 / 15.0);
}

BOOST_AUTO_TEST_CASE(hand_calculated_example)
{
    const CyclicCode code{7, 0b1011};
    BOOST_TEST(toBinary(checkBits(code, 0b1101), 3) == "001");
    BOOST_TEST(toBinary(encode(code, 0b1101), 7) == "1101001");
}

BOOST_AUTO_TEST_CASE(every_codeword_has_zero_syndrome)
{
    std::mt19937_64 engine(1);
    for (const KnownCode& known : knownCodes()) {
        const CyclicCode code = codeFromTable(known);
        for (int trial = 0; trial < 2000; ++trial) {
            const Word codeword = encode(code, randomData(engine, code));
            BOOST_TEST_REQUIRE(syndrome(code, codeword) == 0u);
        }
    }
}

BOOST_AUTO_TEST_CASE(all_single_errors_are_detected)
{
    for (const KnownCode& known : knownCodes()) {
        const CyclicCode code = codeFromTable(known);
        const Word codeword = encode(code, lowBitsMask(code.dataBitCount()));
        for (int bit = 0; bit < code.n; ++bit)
            BOOST_TEST_REQUIRE(errorDetected(code, codeword ^ (Word{1} << bit)));
    }
}

BOOST_AUTO_TEST_CASE(all_double_errors_are_detected)
{
    for (const KnownCode& known : knownCodes()) {
        const CyclicCode code = codeFromTable(known);
        for (int first = 0; first < code.n; ++first) {
            for (int second = first + 1; second < code.n; ++second) {
                const Word error = (Word{1} << first) | (Word{1} << second);
                BOOST_TEST_REQUIRE(syndrome(code, error) != 0u);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(syndrome_depends_only_on_error)
{
    std::mt19937_64 engine(2);
    const CyclicCode code{31, parsePolynomial("11101101001")};
    for (int trial = 0; trial < 1000; ++trial) {
        const Word codeword = encode(code, randomData(engine, code));
        const Word error = engine() & lowBitsMask(code.n);
        BOOST_TEST_REQUIRE(syndrome(code, codeword ^ error) == syndrome(code, error));
    }
}

BOOST_AUTO_TEST_CASE(length_64_with_high_degree_generator)
{
    const CyclicCode code{64, (Word{1} << 63) | 0b11};
    BOOST_TEST(code.dataBitCount() == 1);
    const Word codeword = encode(code, 1);
    BOOST_TEST(codeword == code.generator);
    BOOST_TEST(syndrome(code, codeword) == 0u);
    BOOST_TEST(errorDetected(code, codeword ^ (Word{1} << 63)));
}

BOOST_AUTO_TEST_CASE(length_64_with_low_degree_generator)
{
    const CyclicCode code{64, 0b11};
    const Word data = lowBitsMask(63);
    const Word codeword = encode(code, data);
    BOOST_TEST(syndrome(code, codeword) == 0u);
    BOOST_TEST(codeword == ~Word{0});
}

BOOST_AUTO_TEST_SUITE_END()
