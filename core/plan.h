#pragma once

#include "params.h"

#include <cstdint>
#include <string>
#include <vector>

namespace crc {

struct PlannedExperiment {
    SimulationParams params;
    std::string label;
};

using ExperimentPlan = std::vector<PlannedExperiment>;

constexpr std::uint64_t methodichkaBlocks = 3000000;
constexpr std::uint64_t methodichkaSeed = 42;

ExperimentPlan parsePlanCsv(const std::string& text);

ExperimentPlan parsePlanJson(const std::string& text);

ExperimentPlan parsePlan(const std::string& text);

ExperimentPlan loadPlan(const std::string& path);

ExperimentPlan methodichkaPlan(bool includeGilbert);

std::string planToCsv(const ExperimentPlan& plan);

std::string planToJson(const ExperimentPlan& plan);

void savePlan(const std::string& path, const ExperimentPlan& plan);

std::string channelParametersText(const ChannelParams& channel);

std::string codeName(const CyclicCode& code);

}
