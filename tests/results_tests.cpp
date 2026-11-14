#include <boost/test/unit_test.hpp>

#include "results.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace crc;

namespace {

SimulationResult makeResult(const std::string& generator, const std::string& channel, double p,
    std::uint64_t correct, std::uint64_t detected, std::uint64_t undetected)
{
    SimulationResult result;
    result.code = CyclicCode{15, parsePolynomial(generator)};
    result.channelCsvName = channel;
    result.meanBitErrorProbability = p;
    result.counters.blocks = correct + detected + undetected;
    result.counters.correct = correct;
    result.counters.detected = detected;
    result.counters.undetected = undetected;
    return result;
}

std::string readFile(const std::string& path)
{
    std::ifstream input(path);
    std::stringstream content;
    content << input.rdbuf();
    return content.str();
}

}

BOOST_AUTO_TEST_SUITE(results)

BOOST_AUTO_TEST_CASE(rule_of_three_for_unobserved_events)
{
    const ProbabilityEstimate estimate{0, 3000000};
    BOOST_TEST(!estimate.observed());
    BOOST_TEST(estimate.upperBound95() == 3.0 / 3000000);
    BOOST_TEST(formatProbability(estimate) == "—");
    BOOST_TEST(describeUnobserved(estimate) == "не наблюдалось; P < 1.0e-06 с доверием 95 %");
}

BOOST_AUTO_TEST_CASE(observed_events_are_formatted)
{
    const ProbabilityEstimate estimate{20, 3000000};
    BOOST_TEST(estimate.observed());
    BOOST_TEST(formatProbability(estimate) == "6.667e-06");
}

BOOST_AUTO_TEST_CASE(poisson_upper_bounds)
{
    BOOST_TEST(poissonUpperBound95(0) == 3.0);
    BOOST_TEST(std::abs(poissonUpperBound95(1) - 4.7439) < 1e-3);
    BOOST_TEST(std::abs(poissonUpperBound95(10) - 16.962) < 1e-2);
    const double exact = poissonUpperBound95(1000);
    const double approximate = poissonUpperBound95(1001);
    BOOST_TEST(approximate > exact);
    BOOST_TEST(approximate - exact < 1.2);
}

BOOST_AUTO_TEST_CASE(csv_rows_match_specification_example)
{
    BOOST_TEST(csvHeader() == "n;k;g;channel;p;N;Nv;Nobn;Nlozh;Pv;Pobn;Plozh;I");
    BOOST_TEST(csvRow(makeResult("10011", "BSC", 0.005, 2782365, 217615, 20))
        == "15;11;10011;BSC;0.005;3000000;2782365;217615;20;0.9275;0.07254;6.667e-06;0.267");
    BOOST_TEST(csvRow(makeResult("111010001", "BSC", 0.005, 2782365, 217635, 0))
        == "15;7;111010001;BSC;0.005;3000000;2782365;217635;0;0.9275;0.07254;-;0.533");
    BOOST_TEST(csvRow(makeResult("10011", "Gilbert", 0.001 / 0.101 * 0.5, 2937355, 60585, 2060))
        == "15;11;10011;Gilbert;0.00495;3000000;2937355;60585;2060;0.9791;0.0202;0.0006867;0.267");
    BOOST_TEST(csvRow(makeResult("111010001", "Gilbert", 0.001 / 0.101 * 0.5, 2937355, 62585, 60))
        == "15;7;111010001;Gilbert;0.00495;3000000;2937355;62585;60;0.9791;0.02086;2e-05;0.533");
}

BOOST_AUTO_TEST_CASE(csv_never_prints_zero_probability)
{
    const std::string row = csvRow(makeResult("10011", "BSC", 0.5, 0, 1000, 0));
    BOOST_TEST(row == "15;11;10011;BSC;0.5;1000;0;1000;0;-;1;-;0.267");
}

BOOST_AUTO_TEST_CASE(summary_values)
{
    const Summary summary = summarize(makeResult("10011", "BSC", 0.005, 2782365, 217615, 20));
    BOOST_TEST(summary.redundancy == 4.0 / 15.0);
    BOOST_REQUIRE(summary.meanAttempts.has_value());
    BOOST_TEST(std::abs(*summary.meanAttempts - 3000000.0 / 2782365.0) < 1e-12);
    BOOST_TEST(!summarize(makeResult("10011", "BSC", 0.5, 0, 10, 0)).meanAttempts.has_value());
}

BOOST_AUTO_TEST_CASE(theoretical_formulas)
{
    BOOST_TEST(std::abs(theoreticalCorrectShare(15, 0.005) - std::pow(0.995, 15)) < 1e-15);
    BOOST_TEST(std::abs(binomialProbability(15, 0, 0.005) - std::pow(0.995, 15)) < 1e-12);
    BOOST_TEST(std::abs(binomialProbability(15, 2, 0.1) - 105 * 0.01 * std::pow(0.9, 13)) < 1e-12);
    double total = 0;
    for (int j = 0; j <= 31; ++j)
        total += binomialProbability(31, j, 0.05);
    BOOST_TEST(std::abs(total - 1.0) < 1e-12);
}

BOOST_AUTO_TEST_CASE(file_transfer_time)
{
    const CyclicCode code{15, 0b10011};
    BOOST_TEST(std::abs(fileTransferTime(1e6, code, 1.0 / 9600, 1.0) - 1e6 / 11 * 15 / 9600) < 1e-9);
    BOOST_TEST(std::abs(fileTransferTime(1e6, code, 1.0 / 9600, 2.0) - 2e6 / 11 * 15 / 9600) < 1e-9);
}

BOOST_AUTO_TEST_CASE(csv_files)
{
    const std::string path = "crcsim_test_results.csv";
    std::remove(path.c_str());
    const SimulationResult result = makeResult("10011", "BSC", 0.005, 2782365, 217615, 20);
    appendCsv(path, result);
    appendCsv(path, result);
    const std::string row = csvRow(result) + "\n";
    BOOST_TEST(readFile(path) == csvHeader() + "\n" + row + row);
    writeCsv(path, {result});
    BOOST_TEST(readFile(path) == csvHeader() + "\n" + row);
    std::remove(path.c_str());
}

BOOST_AUTO_TEST_SUITE_END()
