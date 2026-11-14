#pragma once

#include "code.h"

namespace crc {

Word checkBits(const CyclicCode& code, Word data);

Word encode(const CyclicCode& code, Word data);

}
