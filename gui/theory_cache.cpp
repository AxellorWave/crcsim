#include "theory_cache.h"

namespace {

const int quickEnumerationDimension = 22;

}

std::optional<double> TheoryCache::undetectedProbability(const crc::CyclicCode& code, double bitErrorProbability)
{
    const crc::WeightSpectrum* spectrum = spectrumOf(code);
    if (spectrum == nullptr)
        return std::nullopt;
    return crc::undetectedProbability(*spectrum, bitErrorProbability);
}

void TheoryCache::store(const crc::CyclicCode& code, const crc::WeightSpectrum& spectrum)
{
    spectra_[std::make_pair(code.n, code.generator)] = spectrum;
}

const crc::WeightSpectrum* TheoryCache::spectrumOf(const crc::CyclicCode& code)
{
    const auto key = std::make_pair(code.n, code.generator);
    if (const auto cached = spectra_.find(key); cached != spectra_.end())
        return &cached->second;
    if (crc::enumerationDimension(code) > quickEnumerationDimension)
        return nullptr;
    crc::CodeAnalysis analysis = crc::analyzeCode(code);
    if (!analysis.spectrum)
        return nullptr;
    return &spectra_.emplace(key, std::move(*analysis.spectrum)).first->second;
}
