#pragma once

#include "simulator.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace crc {

struct ProbabilityEstimate {
    std::uint64_t events = 0;
    std::uint64_t trials = 0;

    bool observed() const { return events > 0; }
    double value() const;
    double upperBound95() const;
};

double poissonUpperBound95(std::uint64_t events);

struct Summary {
    ProbabilityEstimate correct;
    ProbabilityEstimate detected;
    ProbabilityEstimate undetected;
    double redundancy = 0.0;
    double measuredBitErrorRate = 0.0;
    std::optional<double> meanAttempts;
};

Summary summarize(const SimulationResult& result);

double theoreticalCorrectShare(int n, double bitErrorProbability);

double binomialProbability(int n, int errors, double bitErrorProbability);

double fileTransferTime(double fileBits, const CyclicCode& code, double bitTime, double meanAttempts);

std::string formatNumber(double value, int significantDigits);

std::string formatProbability(const ProbabilityEstimate& estimate);

std::string describeUnobserved(const ProbabilityEstimate& estimate);

std::string csvHeader();

std::string csvRow(const SimulationResult& result);

void writeCsv(const std::string& path, const std::vector<SimulationResult>& results);

void appendCsv(const std::string& path, const SimulationResult& result);

}
