#include <boost/test/unit_test.hpp>

#include "analysis.h"
#include "codes.h"

#include <cmath>
#include <numeric>
#include <stdexcept>

using namespace crc;

namespace {

CyclicCode knownCode(int n, int k)
{
    for (const KnownCode& known : knownCodes()) {
        if (known.n == n && known.k == k)
            return CyclicCode{n, parsePolynomial(known.binary)};
    }
    throw std::invalid_argument("нет такого кода");
}

WeightSpectrum spectrumOf(const CyclicCode& code)
{
    const CodeAnalysis analysis = analyzeCode(code);
    BOOST_REQUIRE(analysis.computable());
    return *analysis.spectrum;
}

std::uint64_t total(const WeightSpectrum& spectrum)
{
    return std::accumulate(spectrum.begin(), spectrum.end(), std::uint64_t{0});
}

double relativeDifference(double actual, double expected)
{
    return std::abs(actual - expected) / expected;
}

}

BOOST_AUTO_TEST_SUITE(analysis)

BOOST_AUTO_TEST_CASE(hamming_7_4_spectrum)
{
    const WeightSpectrum spectrum = spectrumOf(knownCode(7, 4));
    BOOST_TEST(spectrum == (WeightSpectrum{1, 0, 0, 7, 7, 0, 0, 1}), boost::test_tools::per_element());
    BOOST_TEST(minimumDistance(spectrum) == 3);
    BOOST_TEST(guaranteedErrorWeight(spectrum) == 2);
}

BOOST_AUTO_TEST_CASE(minimum_distances_of_known_codes)
{
    BOOST_TEST(minimumDistance(spectrumOf(knownCode(15, 11))) == 3);
    BOOST_TEST(minimumDistance(spectrumOf(knownCode(15, 7))) == 5);
    BOOST_TEST(minimumDistance(spectrumOf(knownCode(31, 16))) == 7);
}

BOOST_AUTO_TEST_CASE(bch_63_30_spectrum)
{
    const WeightSpectrum spectrum = spectrumOf(knownCode(63, 30));
    BOOST_TEST(minimumDistance(spectrum) == 13);
    BOOST_TEST(spectrum[13] == 1764u);
    BOOST_TEST(total(spectrum) == std::uint64_t{1} << 30);
}

BOOST_AUTO_TEST_CASE(mac_williams_matches_direct_enumeration)
{
    for (const CyclicCode& code : {knownCode(15, 7), knownCode(31, 16), knownCode(15, 11), knownCode(31, 21)}) {
        const WeightSpectrum direct = enumerateWeights(generatorRows(code), code.n);
        const WeightSpectrum dual = enumerateWeights(parityCheckRows(code), code.n);
        const WeightSpectrum converted = macWilliamsTransform(dual, code.n, code.checkBitCount());
        BOOST_TEST(converted == direct, boost::test_tools::per_element());
    }
}

BOOST_AUTO_TEST_CASE(spectrum_sums_to_code_size)
{
    for (const KnownCode& known : knownCodes()) {
        const CyclicCode code{known.n, parsePolynomial(known.binary)};
        if (enumerationDimension(code) > 24)
            continue;
        BOOST_TEST(total(spectrumOf(code)) == std::uint64_t{1} << code.dataBitCount());
    }
}

BOOST_AUTO_TEST_CASE(exact_undetected_probability)
{
    BOOST_TEST(relativeDifference(undetectedProbability(spectrumOf(knownCode(7, 4)), 0.01), 6.792e-06) < 1e-3);
    BOOST_TEST(relativeDifference(undetectedProbability(spectrumOf(knownCode(15, 11)), 0.005), 4.182e-06) < 1e-3);
}

BOOST_AUTO_TEST_CASE(undetected_probability_at_half_is_share_of_nonzero_codewords)
{
    const double expected = (std::pow(2.0, 7) - 1) / std::pow(2.0, 15);
    BOOST_TEST(relativeDifference(undetectedProbability(spectrumOf(knownCode(15, 7)), 0.5), expected) < 1e-12);
}

BOOST_AUTO_TEST_CASE(too_large_code_is_not_computable)
{
    const CyclicCode code{64, (Word{1} << 32) | 0b10000011};
    BOOST_TEST(!spectrumComputable(code));
    const CodeAnalysis analysis = analyzeCode(code);
    BOOST_TEST(!analysis.computable());
    BOOST_TEST(analysis.failureReason.find("спектр не вычисляется") == 0u);
}

BOOST_AUTO_TEST_CASE(cancelled_enumeration_is_reported)
{
    const std::atomic<bool> cancelRequested{true};
    const CodeAnalysis analysis = analyzeCode(knownCode(63, 30), &cancelRequested);
    BOOST_TEST(!analysis.computable());
    BOOST_TEST(analysis.cancelled);
}

BOOST_AUTO_TEST_CASE(guarantee_texts)
{
    const CyclicCode code = knownCode(15, 7);
    BOOST_TEST(guaranteedErrorsText(spectrumOf(code)) == "любые ошибки кратности до d − 1 = 4");
    BOOST_TEST(guaranteedBurstsText(code).find("до h = 8 бит") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
