#include <boost/test/unit_test.hpp>

#include "decoder.h"
#include "encoder.h"
#include "walkthrough.h"

#include <string>
#include <vector>

using namespace crc;

namespace {

const CyclicCode hammingCode{7, 0b1011};

std::vector<int> positions(const Division& division)
{
    std::vector<int> result;
    for (const DivisionStep& step : division.steps)
        result.push_back(step.position);
    return result;
}

}

BOOST_AUTO_TEST_SUITE(walkthrough_trace)

BOOST_AUTO_TEST_CASE(encoding_follows_long_division)
{
    const Walkthrough trace = walkthrough(hammingCode, 0b1101, 0);
    BOOST_TEST(toBinary(trace.shifted, 7) == "1101000");
    BOOST_TEST(positions(trace.encoding) == std::vector<int>({6, 5, 4, 3}), boost::test_tools::per_element());
    const std::vector<DivisionStep>& steps = trace.encoding.steps;
    BOOST_TEST(toBinary(steps[0].before, 7) == "1101000");
    BOOST_TEST(toBinary(steps[0].subtrahend, 7) == "1011000");
    BOOST_TEST(toBinary(steps[0].after, 7) == "0110000");
    BOOST_TEST(toBinary(steps[1].subtrahend, 7) == "0101100");
    BOOST_TEST(toBinary(steps[1].after, 7) == "0011100");
    BOOST_TEST(toBinary(steps[2].subtrahend, 7) == "0010110");
    BOOST_TEST(toBinary(steps[2].after, 7) == "0001010");
    BOOST_TEST(toBinary(steps[3].subtrahend, 7) == "0001011");
    BOOST_TEST(toBinary(steps[3].after, 7) == "0000001");
    BOOST_TEST(toBinary(trace.checkBits, 3) == "001");
    BOOST_TEST(toBinary(trace.codeword, 7) == "1101001");
}

BOOST_AUTO_TEST_CASE(division_steps_chain_together)
{
    const Walkthrough trace = walkthrough(hammingCode, 0b1101, 0b0010000);
    const std::vector<DivisionStep>& steps = trace.decoding.steps;
    BOOST_TEST_REQUIRE(!steps.empty());
    BOOST_TEST(steps.front().before == trace.received);
    for (std::size_t index = 1; index < steps.size(); ++index)
        BOOST_TEST(steps[index].before == steps[index - 1].after);
    BOOST_TEST(steps.back().after == trace.syndrome);
}

BOOST_AUTO_TEST_CASE(single_error_is_detected)
{
    const Walkthrough trace = walkthrough(hammingCode, 0b1101, 0b0010000);
    BOOST_TEST(toBinary(trace.received, 7) == "1111001");
    BOOST_TEST(toBinary(trace.syndrome, 3) == "110");
    BOOST_TEST((trace.verdict == Verdict::Detected));
}

BOOST_AUTO_TEST_CASE(codeword_error_slips_through)
{
    const Walkthrough trace = walkthrough(hammingCode, 0b1101, 0b1011000);
    BOOST_TEST(toBinary(trace.received, 7) == "0110001");
    BOOST_TEST(toBinary(trace.syndrome, 3) == "000");
    BOOST_TEST((trace.verdict == Verdict::Undetected));
}

BOOST_AUTO_TEST_CASE(clean_block_is_correct)
{
    const Walkthrough trace = walkthrough(hammingCode, 0b1101, 0);
    BOOST_TEST(trace.received == trace.codeword);
    BOOST_TEST(trace.syndrome == 0u);
    BOOST_TEST((trace.verdict == Verdict::Correct));
}

BOOST_AUTO_TEST_CASE(zero_data_needs_no_division_steps)
{
    const Walkthrough trace = walkthrough(hammingCode, 0, 0);
    BOOST_TEST(trace.encoding.steps.empty());
    BOOST_TEST(trace.codeword == 0u);
}

BOOST_AUTO_TEST_CASE(longest_code_does_not_overflow)
{
    const CyclicCode code{64, parsePolynomial("[32;26;23;22;16;12;11;10;8;7;5;4;2;1;0]")};
    const Word data = ~Word{0};
    const Word errors = Word{1} << 63;
    const Walkthrough trace = walkthrough(code, data, errors);
    BOOST_TEST(trace.data == lowBitsMask(32));
    BOOST_TEST(trace.codeword == encode(code, data & lowBitsMask(32)));
    BOOST_TEST(trace.syndrome == syndrome(code, trace.received));
    BOOST_TEST(trace.encoding.steps.front().position == 63);
    BOOST_TEST((trace.verdict == Verdict::Detected));
}

BOOST_AUTO_TEST_CASE(text_contains_whole_trace)
{
    const std::string text = walkthroughText(walkthrough(hammingCode, 0b1101, 0b0010000));
    BOOST_TEST(text.find("D·x^3 = 1101000") != std::string::npos);
    BOOST_TEST(text.find("Остаток C = 001") != std::string::npos);
    BOOST_TEST(text.find("Принято Y = F ⊕ E = 1111001") != std::string::npos);
    BOOST_TEST(text.find("Синдром S = 110") != std::string::npos);
    BOOST_TEST(text.find("Итог: ошибка обнаружена") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
