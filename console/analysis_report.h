#pragma once

#include "analysis.h"

#include <iosfwd>

namespace console {

void printAnalysis(std::ostream& output, const crc::CodeAnalysis& analysis, double bitErrorProbability);

}
