#pragma once

#include "poly.h"

namespace crc {

struct CyclicCode {
    int n = 0;
    Word generator = 0;

    int checkBitCount() const { return degree(generator); }
    int dataBitCount() const { return n - checkBitCount(); }
    double redundancy() const { return static_cast<double>(checkBitCount()) / n; }
};

}
