#include "analysis.h"

#include "encoder.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <stdexcept>

namespace crc {

namespace {

__extension__ typedef __int128 WideSigned;

constexpr std::uint64_t cancelCheckInterval = std::uint64_t{1} << 20;

using BinomialTable = std::vector<std::vector<std::uint64_t>>;

BinomialTable binomialTable(int n)
{
    BinomialTable table(static_cast<std::size_t>(n) + 1);
    for (int row = 0; row <= n; ++row) {
        table[static_cast<std::size_t>(row)].assign(static_cast<std::size_t>(row) + 1, 1);
        for (int column = 1; column < row; ++column) {
            const auto& previous = table[static_cast<std::size_t>(row) - 1];
            table[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] =
                previous[static_cast<std::size_t>(column) - 1] + previous[static_cast<std::size_t>(column)];
        }
    }
    return table;
}

std::uint64_t binomial(const BinomialTable& table, int total, int chosen)
{
    if (chosen < 0 || chosen > total)
        return 0;
    return table[static_cast<std::size_t>(total)][static_cast<std::size_t>(chosen)];
}

WideSigned krawtchouk(const BinomialTable& table, int n, int weight, int dualWeight)
{
    WideSigned sum = 0;
    for (int shared = 0; shared <= std::min(weight, dualWeight); ++shared) {
        const WideSigned term = static_cast<WideSigned>(binomial(table, dualWeight, shared))
            * static_cast<WideSigned>(binomial(table, n - dualWeight, weight - shared));
        sum += shared % 2 == 0 ? term : -term;
    }
    return sum;
}

int lowestSetBit(std::uint64_t value)
{
    return __builtin_ctzll(value);
}

int weightOf(Word word)
{
    return static_cast<int>(std::bitset<maxWordBits>(word).count());
}

bool cancelled(const std::atomic<bool>* cancelRequested)
{
    return cancelRequested != nullptr && cancelRequested->load(std::memory_order_relaxed);
}

}

int enumerationDimension(const CyclicCode& code)
{
    return std::min(code.dataBitCount(), code.checkBitCount());
}

bool spectrumComputable(const CyclicCode& code)
{
    return enumerationDimension(code) <= maxEnumerationDimension;
}

std::vector<Word> generatorRows(const CyclicCode& code)
{
    std::vector<Word> rows;
    for (int bit = 0; bit < code.dataBitCount(); ++bit)
        rows.push_back(encode(code, Word{1} << bit));
    return rows;
}

std::vector<Word> parityCheckRows(const CyclicCode& code)
{
    const std::vector<Word> generator = generatorRows(code);
    const int checkBits = code.checkBitCount();
    std::vector<Word> rows;
    for (int check = 0; check < checkBits; ++check) {
        Word row = Word{1} << check;
        for (int bit = 0; bit < code.dataBitCount(); ++bit) {
            if ((generator[static_cast<std::size_t>(bit)] >> check) & 1)
                row |= Word{1} << (checkBits + bit);
        }
        rows.push_back(row);
    }
    return rows;
}

WeightSpectrum enumerateWeights(const std::vector<Word>& basis, int n, const std::atomic<bool>* cancelRequested)
{
    if (basis.size() >= static_cast<std::size_t>(maxWordBits))
        throw std::invalid_argument("слишком большая размерность кода");
    WeightSpectrum weights(static_cast<std::size_t>(n) + 1, 0);
    ++weights[0];
    const std::uint64_t combinations = std::uint64_t{1} << basis.size();
    Word word = 0;
    for (std::uint64_t chunk = 1; chunk < combinations; chunk += cancelCheckInterval) {
        if (cancelled(cancelRequested))
            return {};
        const std::uint64_t chunkEnd = std::min(combinations, chunk + cancelCheckInterval);
        for (std::uint64_t step = chunk; step < chunkEnd; ++step) {
            word ^= basis[static_cast<std::size_t>(lowestSetBit(step))];
            ++weights[static_cast<std::size_t>(weightOf(word))];
        }
    }
    return weights;
}

WeightSpectrum macWilliamsTransform(const WeightSpectrum& dualSpectrum, int n, int dualDimension)
{
    const BinomialTable table = binomialTable(n);
    WeightSpectrum spectrum(static_cast<std::size_t>(n) + 1, 0);
    for (int weight = 0; weight <= n; ++weight) {
        WideSigned sum = 0;
        for (int dualWeight = 0; dualWeight <= n; ++dualWeight) {
            const std::uint64_t count = dualSpectrum[static_cast<std::size_t>(dualWeight)];
            if (count != 0)
                sum += static_cast<WideSigned>(count) * krawtchouk(table, n, weight, dualWeight);
        }
        spectrum[static_cast<std::size_t>(weight)] = static_cast<std::uint64_t>(sum / (WideSigned{1} << dualDimension));
    }
    return spectrum;
}

CodeAnalysis analyzeCode(const CyclicCode& code, const std::atomic<bool>* cancelRequested)
{
    CodeAnalysis analysis;
    analysis.code = code;
    if (!spectrumComputable(code)) {
        analysis.failureReason = notComputableReason();
        return analysis;
    }
    const bool enumerateCode = code.dataBitCount() <= code.checkBitCount();
    const std::vector<Word> basis = enumerateCode ? generatorRows(code) : parityCheckRows(code);
    WeightSpectrum weights = enumerateWeights(basis, code.n, cancelRequested);
    if (weights.empty()) {
        analysis.cancelled = true;
        analysis.failureReason = "расчёт прерван";
        return analysis;
    }
    analysis.spectrum = enumerateCode ? std::move(weights) : macWilliamsTransform(weights, code.n, code.checkBitCount());
    return analysis;
}

int minimumDistance(const WeightSpectrum& spectrum)
{
    for (std::size_t weight = 1; weight < spectrum.size(); ++weight) {
        if (spectrum[weight] > 0)
            return static_cast<int>(weight);
    }
    return 0;
}

int guaranteedErrorWeight(const WeightSpectrum& spectrum)
{
    return std::max(0, minimumDistance(spectrum) - 1);
}

int guaranteedBurstLength(const CyclicCode& code)
{
    return code.checkBitCount();
}

double undetectedProbability(const WeightSpectrum& spectrum, double bitErrorProbability)
{
    const long double p = bitErrorProbability;
    const int n = static_cast<int>(spectrum.size()) - 1;
    long double sum = 0.0L;
    for (int weight = 1; weight <= n; ++weight) {
        const std::uint64_t count = spectrum[static_cast<std::size_t>(weight)];
        if (count != 0)
            sum += static_cast<long double>(count) * std::pow(p, weight) * std::pow(1.0L - p, n - weight);
    }
    return static_cast<double>(sum);
}

std::string notComputableReason()
{
    return "спектр не вычисляется: 2^min(k,h) слишком много слов (допустимо min(k,h) ≤ "
        + std::to_string(maxEnumerationDimension) + ")";
}

std::string guaranteedErrorsText(const WeightSpectrum& spectrum)
{
    return "любые ошибки кратности до d − 1 = " + std::to_string(guaranteedErrorWeight(spectrum));
}

std::string guaranteedBurstsText(const CyclicCode& code)
{
    return "любая пачка ошибок длиной до h = " + std::to_string(guaranteedBurstLength(code)) + " бит (в пределах блока)";
}

}
