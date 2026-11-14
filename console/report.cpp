#include "report.h"

#include "results.h"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <string>

namespace console {

namespace {

constexpr int labelWidth = 36;
constexpr int countWidth = 10;

void printLabel(std::ostream& output, const std::string& label)
{
    int visibleLength = 0;
    for (unsigned char symbol : label) {
        if ((symbol & 0xC0) != 0x80)
            ++visibleLength;
    }
    output << "  " << label << std::string(static_cast<std::size_t>(std::max(1, labelWidth - visibleLength)), ' ');
}

void printCount(std::ostream& output, const std::string& label, std::uint64_t value)
{
    printLabel(output, label);
    output << std::setw(countWidth) << value;
}

void printOutcome(std::ostream& output, const std::string& label, const crc::ProbabilityEstimate& estimate)
{
    printCount(output, label, estimate.events);
    output << "   P = " << crc::formatProbability(estimate);
    if (!estimate.observed())
        output << "  (" << crc::describeUnobserved(estimate) << ")";
    output << '\n';
}

void printValue(std::ostream& output, const std::string& label, const std::string& value)
{
    printLabel(output, label);
    output << std::setw(countWidth) << value << '\n';
}

void printWeightDistribution(std::ostream& output, const crc::SimulationResult& result, const crc::ChannelParams& channel)
{
    const crc::Counters& counters = result.counters;
    const bool independentErrors = channel.kind == crc::ChannelKind::Bsc;
    output << "\nРаспределение числа искажённых бит в блоке";
    output << (independentErrors ? " (в скобках — ожидаемое по биномиальному закону):\n" : ":\n");
    int lastObserved = 0;
    for (std::size_t weight = 0; weight < counters.errorWeights.size(); ++weight) {
        if (counters.errorWeights[weight] > 0)
            lastObserved = static_cast<int>(weight);
    }
    for (int weight = 0; weight <= lastObserved; ++weight) {
        output << "  j = " << std::setw(2) << weight << ": " << std::setw(10) << counters.errorWeights[static_cast<std::size_t>(weight)];
        if (independentErrors) {
            const double expected = static_cast<double>(counters.blocks)
                * crc::binomialProbability(result.code.n, weight, channel.bsc.bitErrorProbability);
            output << "  (" << crc::formatNumber(expected, 6) << ")";
        }
        output << '\n';
    }
}

}

void printChannelCharacteristics(std::ostream& output, const crc::ChannelParams& channel)
{
    if (channel.kind != crc::ChannelKind::Gilbert)
        return;
    const crc::GilbertSettings& gilbert = channel.gilbert;
    output << "  -> доля времени в плохом состоянии " << crc::formatNumber(gilbert.badStateShare(), 3)
           << ", средняя p = " << crc::formatNumber(gilbert.meanBitErrorProbability(), 3) << ",\n"
           << "     средняя длина пачки " << crc::formatNumber(gilbert.meanBurstLength(), 3) << " бит\n";
}

void printFirstBlock(std::ostream& output, const crc::SimulationResult& result)
{
    const crc::CyclicCode& code = result.code;
    const crc::BlockTrace& block = result.firstBlock;
    output << "Первый блок:\n"
           << "  данные D   = " << crc::toBinary(block.data, code.dataBitCount()) << '\n'
           << "  проверка C = " << crc::toBinary(block.check, code.checkBitCount()) << '\n'
           << "  слово F    = " << crc::toBinary(block.codeword, code.n) << '\n'
           << "  ошибки E   = " << crc::toBinary(block.errors, code.n) << '\n'
           << "  принято Y  = " << crc::toBinary(block.received, code.n) << '\n'
           << "  синдром S  = " << crc::toBinary(block.syndrome, code.checkBitCount()) << '\n';
}

void printResults(std::ostream& output, const crc::SimulationResult& result, const crc::ChannelParams& channel)
{
    const crc::CyclicCode& code = result.code;
    const crc::Summary summary = crc::summarize(result);
    output << "\nРезультаты (n = " << code.n << ", k = " << code.dataBitCount() << ", g(x) = "
           << crc::toBinary(code.generator) << ", канал " << result.channelDisplayName << ", p = "
           << crc::formatNumber(result.meanBitErrorProbability, 3) << ", seed = " << result.seed << "):\n";
    if (result.cancelled)
        output << "  Моделирование прервано, результаты неполные.\n";
    printCount(output, "Передано блоков N", result.counters.blocks);
    output << '\n';
    printOutcome(output, "Принято верно Nв", summary.correct);
    printOutcome(output, "Ошибка обнаружена Nобн", summary.detected);
    printOutcome(output, "Ложный приём Nлож", summary.undetected);
    printValue(output, "Измеренная доля ошибочных бит", crc::formatNumber(summary.measuredBitErrorRate, 3));
    if (channel.kind == crc::ChannelKind::Bsc) {
        const double theory = crc::theoreticalCorrectShare(code.n, channel.bsc.bitErrorProbability);
        printValue(output, "Теоретическая Pв = (1-p)^n", crc::formatNumber(theory, 4));
    }
    printValue(output, "Избыточность I = (n-k)/n", crc::formatNumber(summary.redundancy, 3));
    printValue(output, "Среднее число попыток L = N/Nв",
        summary.meanAttempts ? crc::formatNumber(*summary.meanAttempts, 4) : "—");
    printWeightDistribution(output, result, channel);
}

}
