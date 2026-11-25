#pragma once

#include "params.h"
#include "results.h"
#include "simulator.h"
#include "transfer.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace crc {

constexpr double defaultSelectionTarget = 1e-6;
constexpr std::uint64_t defaultSelectionBlocks = 3000000;
constexpr std::uint64_t defaultSelectionSeed = 42;

enum class Verdict { Satisfied, NotSatisfied, Borderline };

struct SelectionRequest {
    std::optional<int> codeLength = 15;
    ChannelParams channel;
    std::uint64_t blocks = defaultSelectionBlocks;
    std::uint64_t seed = defaultSelectionSeed;
    double target = defaultSelectionTarget;
    TransferSettings transfer;
};

struct SelectionRow {
    SimulationResult result;
    ProbabilityEstimate undetected;
    double upperBound = 0.0;
    Verdict verdict = Verdict::NotSatisfied;
    std::optional<double> meanAttempts;
    std::optional<double> transferTime;
};

struct BestCodes {
    std::optional<std::size_t> byRedundancy;
    std::optional<std::size_t> byTime;
};

struct SelectionOutcome {
    std::vector<SelectionRow> rows;
    BestCodes best;
    bool cancelled = false;
};

using SelectionProgress = std::function<void(std::size_t index, std::size_t count, const Counters& counters)>;

struct SelectionControl {
    SelectionProgress onProgress;
    const std::atomic<bool>* cancelRequested = nullptr;
    std::uint64_t progressInterval = 65536;
};

std::vector<ValidationError> validateSelection(const SelectionRequest& request);

std::vector<CyclicCode> candidateCodes(std::optional<int> codeLength);

std::vector<int> knownCodeLengths();

SimulationParams candidateParams(const CyclicCode& code, const SelectionRequest& request);

Verdict judge(const ProbabilityEstimate& undetected, double target);

SelectionRow evaluateCandidate(const SimulationResult& result, const SelectionRequest& request);

BestCodes chooseBest(const std::vector<SelectionRow>& rows);

SelectionOutcome selectCode(const SelectionRequest& request, const SelectionControl& control = {});

std::string verdictText(Verdict verdict);

std::string selectionConclusion(const SelectionRequest& request, const std::vector<SelectionRow>& rows, const BestCodes& best);

}
