#pragma once

#include "code.h"

namespace crc {

Word syndrome(const CyclicCode& code, Word received);

bool errorDetected(const CyclicCode& code, Word received);

}
