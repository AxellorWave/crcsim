#include "decoder.h"

namespace crc {

Word syndrome(const CyclicCode& code, Word received)
{
    return remainderMod2(received, code.n, code.generator);
}

bool errorDetected(const CyclicCode& code, Word received)
{
    return syndrome(code, received) != 0;
}

}
