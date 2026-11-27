#pragma once

#include "analysis.h"

#include <map>
#include <optional>
#include <utility>

class TheoryCache {
public:
    std::optional<double> undetectedProbability(const crc::CyclicCode& code, double bitErrorProbability);

private:
    const crc::WeightSpectrum* spectrumOf(const crc::CyclicCode& code);

    std::map<std::pair<int, crc::Word>, crc::WeightSpectrum> spectra_;
};
