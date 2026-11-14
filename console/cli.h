#pragma once

#include "params.h"

#include <string>
#include <vector>

namespace console {

struct CommandLine {
    crc::SimulationParams params;
    std::string csvPath;
    bool csvOutput = false;
    bool quiet = false;
    bool help = false;
    bool listCodes = false;
};

CommandLine parseCommandLine(const std::vector<std::string>& arguments);

std::string usage();

}
