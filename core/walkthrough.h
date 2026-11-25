#pragma once

#include "code.h"

#include <string>
#include <vector>

namespace crc {

struct DivisionStep {
    int position = 0;
    Word before = 0;
    Word subtrahend = 0;
    Word after = 0;
};

struct Division {
    Word dividend = 0;
    int length = 0;
    Word divisor = 0;
    std::vector<DivisionStep> steps;
    Word remainder = 0;
};

enum class Verdict { Correct, Detected, Undetected };

struct Walkthrough {
    CyclicCode code;
    Word data = 0;
    Word shifted = 0;
    Division encoding;
    Word checkBits = 0;
    Word codeword = 0;
    Word errors = 0;
    Word received = 0;
    Division decoding;
    Word syndrome = 0;
    Verdict verdict = Verdict::Correct;
};

Division divideMod2(Word dividend, int length, Word divisor);

Walkthrough walkthrough(const CyclicCode& code, Word data, Word errors);

std::string verdictName(Verdict verdict);

std::string walkthroughText(const Walkthrough& trace);

}
