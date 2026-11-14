#include "encoder.h"

namespace crc {

Word checkBits(const CyclicCode& code, Word data)
{
    const Word shifted = data << code.checkBitCount();
    return remainderMod2(shifted, code.n, code.generator);
}

Word encode(const CyclicCode& code, Word data)
{
    return (data << code.checkBitCount()) | checkBits(code, data);
}

}
