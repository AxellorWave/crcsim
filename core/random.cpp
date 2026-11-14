#include "random.h"

#include <cmath>

namespace crc {

Random::Random(std::uint64_t seed)
    : engine_(seed)
{
}

Word Random::nextWord()
{
    return engine_();
}

double Random::nextUniform()
{
    constexpr double twoToMinus64 = 0x1p-64;
    const double value = static_cast<double>(engine_()) * twoToMinus64;
    return value < 1.0 ? value : std::nextafter(1.0, 0.0);
}

std::uint64_t makeRandomSeed()
{
    std::random_device device;
    std::uint64_t seed = 0;
    while (seed == 0)
        seed = (static_cast<std::uint64_t>(device()) << 32) ^ device();
    return seed;
}

}
