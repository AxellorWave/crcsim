#include <boost/test/unit_test.hpp>

#include "channel.h"
#include "random.h"

#include <cmath>
#include <random>

using namespace crc;

namespace {

double fractionOfCorruptedBits(Channel& channel, Random& random, long long bitCount)
{
    long long corrupted = 0;
    for (long long bit = 0; bit < bitCount; ++bit)
        corrupted += channel.corruptsNextBit(random) ? 1 : 0;
    return static_cast<double>(corrupted) / bitCount;
}

}

BOOST_AUTO_TEST_SUITE(channels)

BOOST_AUTO_TEST_CASE(uniform_values_are_in_unit_interval)
{
    Random random(7);
    for (int i = 0; i < 100000; ++i) {
        const double value = random.nextUniform();
        BOOST_TEST_REQUIRE(value >= 0.0);
        BOOST_TEST_REQUIRE(value < 1.0);
    }
}

BOOST_AUTO_TEST_CASE(same_seed_gives_same_sequence)
{
    Random first(42);
    Random second(42);
    for (int i = 0; i < 1000; ++i)
        BOOST_TEST_REQUIRE(first.nextWord() == second.nextWord());
}

#ifdef __GLIBCXX__
BOOST_AUTO_TEST_CASE(uniform_matches_standard_distribution_of_libstdcxx)
{
    Random random(42);
    std::mt19937_64 engine(42);
    std::uniform_real_distribution<double> standard(0.0, 1.0);
    for (int i = 0; i < 100000; ++i)
        BOOST_TEST_REQUIRE(random.nextUniform() == standard(engine));
}
#endif

BOOST_AUTO_TEST_CASE(random_seed_is_not_zero)
{
    BOOST_TEST(makeRandomSeed() != 0u);
}

BOOST_AUTO_TEST_CASE(bsc_error_vector_fits_word_length)
{
    BscChannel channel(BscSettings{0.5});
    Random random(3);
    for (int length : {3, 15, 63, 64}) {
        for (int trial = 0; trial < 1000; ++trial)
            BOOST_TEST_REQUIRE((channel.errorVector(random, length) & ~lowBitsMask(length)) == 0u);
    }
}

BOOST_AUTO_TEST_CASE(bsc_fraction_of_corrupted_bits_matches_p)
{
    const double p = 0.01;
    const long long bitCount = 1000000;
    BscChannel channel(BscSettings{p});
    Random random(11);
    const double sigma = std::sqrt(p * (1 - p) / bitCount);
    BOOST_TEST(std::abs(fractionOfCorruptedBits(channel, random, bitCount) - p) < 4 * sigma);
}

BOOST_AUTO_TEST_CASE(gilbert_derived_characteristics)
{
    const GilbertSettings settings{0.001, 0.1, 0.5, 0.0};
    BOOST_TEST(settings.badStateShare() == 0.001 / 0.101);
    BOOST_TEST(std::abs(settings.meanBitErrorProbability() - 0.0049505) < 1e-7);
    BOOST_TEST(settings.meanBurstLength() == 10.0);
}

BOOST_AUTO_TEST_CASE(gilbert_fraction_of_corrupted_bits_matches_mean_p)
{
    const GilbertSettings settings{0.003, 0.1, 0.5, 0.01};
    GilbertChannel channel(settings);
    Random random(5);
    channel.start(random);
    const double measured = fractionOfCorruptedBits(channel, random, 20000000);
    BOOST_TEST(std::abs(measured - settings.meanBitErrorProbability()) < 0.03 * settings.meanBitErrorProbability());
}

BOOST_AUTO_TEST_CASE(gilbert_mean_bad_period_matches_burst_length)
{
    const GilbertSettings settings{0.001, 0.1, 0.5, 0.0};
    GilbertChannel channel(settings);
    Random random(9);
    channel.start(random);
    long long badBits = 0;
    long long badPeriods = 0;
    bool wasBad = channel.inBadState();
    for (long long bit = 0; bit < 20000000; ++bit) {
        if (channel.inBadState()) {
            ++badBits;
            if (!wasBad)
                ++badPeriods;
        }
        wasBad = channel.inBadState();
        channel.corruptsNextBit(random);
    }
    const double meanPeriod = static_cast<double>(badBits) / badPeriods;
    BOOST_TEST(std::abs(meanPeriod - settings.meanBurstLength()) < 0.03 * settings.meanBurstLength());
}

BOOST_AUTO_TEST_CASE(gilbert_good_state_without_errors_is_clean)
{
    GilbertChannel channel(GilbertSettings{0.001, 0.1, 0.5, 0.0});
    Random random(13);
    channel.start(random);
    for (int bit = 0; bit < 1000000; ++bit) {
        const bool wasBad = channel.inBadState();
        const bool corrupted = channel.corruptsNextBit(random);
        BOOST_TEST_REQUIRE(!(corrupted && !wasBad));
    }
}

BOOST_AUTO_TEST_CASE(gilbert_state_is_kept_between_blocks)
{
    const GilbertSettings settings{0.01, 0.1, 0.5, 0.0};
    GilbertChannel blockwise(settings);
    GilbertChannel bitwise(settings);
    Random first(21);
    Random second(21);
    blockwise.start(first);
    bitwise.start(second);
    for (int block = 0; block < 10000; ++block) {
        const Word errors = blockwise.errorVector(first, 15);
        Word expected = 0;
        for (int position = 14; position >= 0; --position) {
            if (bitwise.corruptsNextBit(second))
                expected |= Word{1} << position;
        }
        BOOST_TEST_REQUIRE(errors == expected);
    }
}

BOOST_AUTO_TEST_SUITE_END()
