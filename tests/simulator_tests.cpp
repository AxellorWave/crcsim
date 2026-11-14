#include <boost/test/unit_test.hpp>

#include "simulator.h"

#include <cmath>
#include <numeric>

using namespace crc;

namespace {

class CleanChannel : public Channel {
public:
    bool corruptsNextBit(Random&) override { return false; }
    double meanBitErrorProbability() const override { return 0.0; }
    std::string displayName() const override { return "без ошибок"; }
    std::string csvName() const override { return "Clean"; }
};

SimulationParams referenceParams(const std::string& generator, ChannelKind kind)
{
    SimulationParams params;
    params.coding = CodingParams{15, generator};
    params.channel.kind = kind;
    params.channel.bsc.bitErrorProbability = 0.005;
    params.channel.gilbert = GilbertSettings{0.001, 0.1, 0.5, 0.0};
    params.control.blockLimit = 3000000;
    params.control.seed = 42;
    return params;
}

void checkCounters(const Counters& counters, std::uint64_t correct, std::uint64_t detected, std::uint64_t undetected)
{
    BOOST_TEST(counters.blocks == 3000000u);
    BOOST_TEST(counters.correct == correct);
    BOOST_TEST(counters.detected == detected);
    BOOST_TEST(counters.undetected == undetected);
}

ControlParams blocks(std::uint64_t count, std::uint64_t seed)
{
    ControlParams control;
    control.blockLimit = count;
    control.seed = seed;
    return control;
}

}

BOOST_AUTO_TEST_SUITE(simulator)

BOOST_AUTO_TEST_CASE(matches_prototype_on_bsc)
{
    checkCounters(Simulator(referenceParams("10011", ChannelKind::Bsc)).run().counters, 2782365, 217615, 20);
    checkCounters(Simulator(referenceParams("111010001", ChannelKind::Bsc)).run().counters, 2782365, 217635, 0);
}

BOOST_AUTO_TEST_CASE(matches_prototype_on_gilbert)
{
    const SimulationResult result = Simulator(referenceParams("10011", ChannelKind::Gilbert)).run();
    checkCounters(result.counters, 2937355, 60585, 2060);
    BOOST_TEST(result.counters.errorWeights[1] == 15234u);
    BOOST_TEST(result.counters.errorWeights[4] == 7028u);
    checkCounters(Simulator(referenceParams("111010001", ChannelKind::Gilbert)).run().counters, 2937355, 62585, 60);
}

BOOST_AUTO_TEST_CASE(same_seed_gives_identical_counters)
{
    SimulationParams params = referenceParams("10011", ChannelKind::Gilbert);
    params.control.blockLimit = 200000;
    params.control.seed = 123;
    const Counters first = Simulator(params).run().counters;
    const Counters second = Simulator(params).run().counters;
    BOOST_TEST(first.correct == second.correct);
    BOOST_TEST(first.detected == second.detected);
    BOOST_TEST(first.undetected == second.undetected);
    BOOST_TEST(first.errorWeights == second.errorWeights, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(random_seed_is_reported)
{
    SimulationParams params = referenceParams("10011", ChannelKind::Bsc);
    params.control.blockLimit = 10;
    params.control.seed = 0;
    BOOST_TEST(Simulator(params).run().seed != 0u);
}

BOOST_AUTO_TEST_CASE(clean_channel_delivers_every_block)
{
    const CyclicCode code{15, 0b10011};
    const Counters counters = Simulator(code, std::make_unique<CleanChannel>(), blocks(100000, 1)).run().counters;
    BOOST_TEST(counters.correct == 100000u);
    BOOST_TEST(counters.detected == 0u);
    BOOST_TEST(counters.undetected == 0u);
}

BOOST_AUTO_TEST_CASE(zero_probability_bsc_delivers_every_block)
{
    const CyclicCode code{63, 0b1000011};
    auto channel = std::make_unique<BscChannel>(BscSettings{0.0});
    const Counters counters = Simulator(code, std::move(channel), blocks(100000, 1)).run().counters;
    BOOST_TEST(counters.correct == 100000u);
    BOOST_TEST(counters.erroneous() == 0u);
}

BOOST_AUTO_TEST_CASE(bsc_correct_share_matches_theory)
{
    const double p = 0.01;
    const int n = 31;
    const std::uint64_t count = 1000000;
    const CyclicCode code{n, 0b100101};
    auto channel = std::make_unique<BscChannel>(BscSettings{p});
    const Counters counters = Simulator(code, std::move(channel), blocks(count, 77)).run().counters;

    const double expected = std::pow(1 - p, n);
    const double sigma = std::sqrt(expected * (1 - expected) / count);
    BOOST_TEST(std::abs(static_cast<double>(counters.correct) / count - expected) < 4 * sigma);

    const double bitSigma = std::sqrt(p * (1 - p) / (static_cast<double>(count) * n));
    BOOST_TEST(std::abs(static_cast<double>(counters.corruptedBits) / (count * n) - p) < 4 * bitSigma);

    const double singleExpected = n * p * std::pow(1 - p, n - 1);
    const double singleSigma = std::sqrt(singleExpected * (1 - singleExpected) / count);
    BOOST_TEST(std::abs(static_cast<double>(counters.errorWeights[1]) / count - singleExpected) < 4 * singleSigma);
}

BOOST_AUTO_TEST_CASE(counters_are_consistent)
{
    SimulationParams params = referenceParams("10011", ChannelKind::Gilbert);
    params.control.blockLimit = 100000;
    const Counters counters = Simulator(params).run().counters;
    BOOST_TEST(counters.correct + counters.detected + counters.undetected == counters.blocks);
    const auto total = std::accumulate(counters.errorWeights.begin(), counters.errorWeights.end(), std::uint64_t{0});
    BOOST_TEST(total == counters.blocks);
    BOOST_TEST(counters.errorWeights[0] == counters.correct);
}

BOOST_AUTO_TEST_CASE(stops_after_requested_number_of_erroneous_blocks)
{
    SimulationParams params = referenceParams("10011", ChannelKind::Bsc);
    params.control.stopMode = StopMode::ErrorCount;
    params.control.errorLimit = 500;
    const Counters counters = Simulator(params).run().counters;
    BOOST_TEST(counters.erroneous() == 500u);
    BOOST_TEST(counters.blocks < 3000000u);
}

BOOST_AUTO_TEST_CASE(error_mode_respects_block_cap)
{
    SimulationParams params = referenceParams("10011", ChannelKind::Bsc);
    params.control.stopMode = StopMode::ErrorCount;
    params.control.errorLimit = 1000000;
    params.control.blockLimit = 1000;
    BOOST_TEST(Simulator(params).run().counters.blocks == 1000u);
}

BOOST_AUTO_TEST_CASE(first_block_is_traced)
{
    SimulationParams params = referenceParams("10011", ChannelKind::Bsc);
    params.control.blockLimit = 1;
    const BlockTrace trace = Simulator(params).run().firstBlock;
    BOOST_TEST(trace.codeword == ((trace.data << 4) | trace.check));
    BOOST_TEST(trace.received == (trace.codeword ^ trace.errors));
}

BOOST_AUTO_TEST_CASE(progress_and_cancellation)
{
    SimulationParams params = referenceParams("10011", ChannelKind::Bsc);
    std::atomic<bool> cancel{false};
    int calls = 0;
    RunControl runControl;
    runControl.progressInterval = 1000;
    runControl.cancelRequested = &cancel;
    runControl.onProgress = [&](const Counters& counters) {
        ++calls;
        if (counters.blocks >= 5000)
            cancel = true;
    };
    const SimulationResult result = Simulator(params).run(runControl);
    BOOST_TEST(result.cancelled);
    BOOST_TEST(result.counters.blocks == 5000u);
    BOOST_TEST(calls == 6);
}

BOOST_AUTO_TEST_CASE(invalid_params_are_rejected_with_reason)
{
    SimulationParams params;
    params.coding = CodingParams{7, "111010001"};
    BOOST_CHECK_EXCEPTION(Simulator{params}, std::invalid_argument,
        [](const std::invalid_argument& error) { return std::string(error.what()).find("меньше n") != std::string::npos; });
}

BOOST_AUTO_TEST_SUITE_END()
