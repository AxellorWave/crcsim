#pragma once

#include <cstdint>
#include <string>

namespace crc {

using Word = std::uint64_t;

constexpr int maxWordBits = 64;

int degree(Word polynomial);

Word lowBitsMask(int bitCount);

Word remainderMod2(Word dividend, int dividendLength, Word divisor);

std::string toBinary(Word value, int length);

std::string toBinary(Word polynomial);

std::string toDegreeList(Word polynomial);

Word parsePolynomial(const std::string& text);

}
