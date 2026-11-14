#include "results.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace crc {

namespace {

constexpr double ruleOfThree = 3.0;
constexpr double oneSidedConfidenceQuantile = 1.6448536269514722;
constexpr std::uint64_t exactPoissonLimit = 1000;

double poissonCumulative(std::uint64_t events, double mean)
{
    double sum = 0.0;
    for (std::uint64_t k = 0; k <= events; ++k) {
        const double kd = static_cast<double>(k);
        sum += std::exp(-mean + kd * std::log(mean) - std::lgamma(kd + 1.0));
    }
    return sum;
}

double exactPoissonUpperBound(std::uint64_t events)
{
    double low = static_cast<double>(events);
    double high = low + 10.0 * std::sqrt(low + 1.0) + 10.0;
    for (int iteration = 0; iteration < 200; ++iteration) {
        const double middle = 0.5 * (low + high);
        if (poissonCumulative(events, middle) > 0.05)
            low = middle;
        else
            high = middle;
    }
    return 0.5 * (low + high);
}

double approximatePoissonUpperBound(std::uint64_t events)
{
    const double shifted = static_cast<double>(events) + 1.0;
    const double root = 1.0 - 1.0 / (9.0 * shifted) + oneSidedConfidenceQuantile / (3.0 * std::sqrt(shifted));
    return shifted * root * root * root;
}

std::ostringstream classicStream()
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    return stream;
}

std::string csvProbability(const ProbabilityEstimate& estimate)
{
    return estimate.observed() ? formatNumber(estimate.value(), 4) : "-";
}

void writeRow(std::ostream& output, const SimulationResult& result)
{
    output << csvRow(result) << '\n';
    if (!output)
        throw std::runtime_error("не удалось записать файл CSV");
}

}

double ProbabilityEstimate::value() const
{
    return trials == 0 ? 0.0 : static_cast<double>(events) / static_cast<double>(trials);
}

double ProbabilityEstimate::upperBound95() const
{
    return trials == 0 ? 1.0 : poissonUpperBound95(events) / static_cast<double>(trials);
}

double poissonUpperBound95(std::uint64_t events)
{
    if (events == 0)
        return ruleOfThree;
    if (events <= exactPoissonLimit)
        return exactPoissonUpperBound(events);
    return approximatePoissonUpperBound(events);
}

Summary summarize(const SimulationResult& result)
{
    const Counters& counters = result.counters;
    Summary summary;
    summary.correct = {counters.correct, counters.blocks};
    summary.detected = {counters.detected, counters.blocks};
    summary.undetected = {counters.undetected, counters.blocks};
    summary.redundancy = result.code.redundancy();
    if (counters.blocks > 0) {
        const double totalBits = static_cast<double>(counters.blocks) * result.code.n;
        summary.measuredBitErrorRate = static_cast<double>(counters.corruptedBits) / totalBits;
    }
    if (counters.correct > 0)
        summary.meanAttempts = static_cast<double>(counters.blocks) / static_cast<double>(counters.correct);
    return summary;
}

double theoreticalCorrectShare(int n, double bitErrorProbability)
{
    return std::pow(1.0 - bitErrorProbability, n);
}

double binomialProbability(int n, int errors, double bitErrorProbability)
{
    if (errors < 0 || errors > n)
        return 0.0;
    const double logCombinations = std::lgamma(n + 1.0) - std::lgamma(errors + 1.0) - std::lgamma(n - errors + 1.0);
    return std::exp(logCombinations) * std::pow(bitErrorProbability, errors)
        * std::pow(1.0 - bitErrorProbability, n - errors);
}

double fileTransferTime(double fileBits, const CyclicCode& code, double bitTime, double meanAttempts)
{
    return fileBits / code.dataBitCount() * code.n * bitTime * meanAttempts;
}

std::string formatNumber(double value, int significantDigits)
{
    std::ostringstream stream = classicStream();
    stream << std::setprecision(significantDigits) << value;
    return stream.str();
}

std::string formatProbability(const ProbabilityEstimate& estimate)
{
    if (!estimate.observed())
        return "—";
    std::ostringstream stream = classicStream();
    stream << std::scientific << std::setprecision(3) << estimate.value();
    return stream.str();
}

std::string describeUnobserved(const ProbabilityEstimate& estimate)
{
    std::ostringstream stream = classicStream();
    stream << "не наблюдалось; P < " << std::scientific << std::setprecision(1) << estimate.upperBound95()
           << " с доверием 95 %";
    return stream.str();
}

std::string csvHeader()
{
    return "n;k;g;channel;p;N;Nv;Nobn;Nlozh;Pv;Pobn;Plozh;I";
}

std::string csvRow(const SimulationResult& result)
{
    const Summary summary = summarize(result);
    const Counters& counters = result.counters;
    std::ostringstream row = classicStream();
    row << result.code.n << ';' << result.code.dataBitCount() << ';' << toBinary(result.code.generator) << ';'
        << result.channelCsvName << ';' << formatNumber(result.meanBitErrorProbability, 3) << ';'
        << counters.blocks << ';' << counters.correct << ';' << counters.detected << ';' << counters.undetected << ';'
        << csvProbability(summary.correct) << ';' << csvProbability(summary.detected) << ';'
        << csvProbability(summary.undetected) << ';' << formatNumber(summary.redundancy, 3);
    return row.str();
}

void writeCsv(const std::string& path, const std::vector<SimulationResult>& results)
{
    std::ofstream output(path, std::ios::trunc);
    if (!output)
        throw std::runtime_error("не удалось открыть файл " + path + " для записи");
    output << csvHeader() << '\n';
    for (const SimulationResult& result : results)
        writeRow(output, result);
}

void appendCsv(const std::string& path, const SimulationResult& result)
{
    const bool needsHeader = [&] {
        std::ifstream existing(path);
        return !existing || existing.peek() == std::ifstream::traits_type::eof();
    }();
    std::ofstream output(path, std::ios::app);
    if (!output)
        throw std::runtime_error("не удалось открыть файл " + path + " для записи");
    if (needsHeader)
        output << csvHeader() << '\n';
    writeRow(output, result);
}

}
