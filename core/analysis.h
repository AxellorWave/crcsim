#pragma once

#include "code.h"

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace crc {

constexpr int maxEnumerationDimension = 30;

using WeightSpectrum = std::vector<std::uint64_t>;

struct CodeAnalysis {
    CyclicCode code;
    std::optional<WeightSpectrum> spectrum;
    std::string failureReason;
    bool cancelled = false;

    bool computable() const { return spectrum.has_value(); }
};

int enumerationDimension(const CyclicCode& code);

bool spectrumComputable(const CyclicCode& code);

std::vector<Word> generatorRows(const CyclicCode& code);

std::vector<Word> parityCheckRows(const CyclicCode& code);

WeightSpectrum enumerateWeights(const std::vector<Word>& basis, int n, const std::atomic<bool>* cancelRequested = nullptr);

WeightSpectrum macWilliamsTransform(const WeightSpectrum& dualSpectrum, int n, int dualDimension);

CodeAnalysis analyzeCode(const CyclicCode& code, const std::atomic<bool>* cancelRequested = nullptr);

int minimumDistance(const WeightSpectrum& spectrum);

int guaranteedErrorWeight(const WeightSpectrum& spectrum);

int guaranteedBurstLength(const CyclicCode& code);

double undetectedProbability(const WeightSpectrum& spectrum, double bitErrorProbability);

std::string notComputableReason();

std::string guaranteedErrorsText(const WeightSpectrum& spectrum);

std::string guaranteedBurstsText(const CyclicCode& code);

}
