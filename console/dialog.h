#pragma once

#include "params.h"

#include <iosfwd>
#include <optional>
#include <string>

namespace console {

std::optional<crc::SimulationParams> askParams(std::istream& input, std::ostream& output);

std::optional<std::string> askCsvPath(std::istream& input, std::ostream& output);

}
