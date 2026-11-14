#include "channel.h"

namespace crc {

void Channel::start(Random&)
{
}

Word Channel::errorVector(Random& random, int length)
{
    Word errors = 0;
    for (int position = length - 1; position >= 0; --position) {
        if (corruptsNextBit(random))
            errors |= Word{1} << position;
    }
    return errors;
}

BscChannel::BscChannel(const BscSettings& settings)
    : settings_(settings)
{
}

bool BscChannel::corruptsNextBit(Random& random)
{
    return random.nextUniform() < settings_.bitErrorProbability;
}

double BscChannel::meanBitErrorProbability() const
{
    return settings_.bitErrorProbability;
}

std::string BscChannel::displayName() const
{
    return "ДСК";
}

std::string BscChannel::csvName() const
{
    return "BSC";
}

double GilbertSettings::badStateShare() const
{
    return goodToBad / (goodToBad + badToGood);
}

double GilbertSettings::goodStateShare() const
{
    return badToGood / (goodToBad + badToGood);
}

double GilbertSettings::meanBitErrorProbability() const
{
    return errorInGood * goodStateShare() + errorInBad * badStateShare();
}

double GilbertSettings::meanBurstLength() const
{
    return 1.0 / badToGood;
}

GilbertChannel::GilbertChannel(const GilbertSettings& settings)
    : settings_(settings)
{
}

void GilbertChannel::start(Random& random)
{
    bad_ = random.nextUniform() < settings_.badStateShare();
}

bool GilbertChannel::corruptsNextBit(Random& random)
{
    const double errorProbability = bad_ ? settings_.errorInBad : settings_.errorInGood;
    const bool corrupted = random.nextUniform() < errorProbability;
    if (bad_)
        bad_ = random.nextUniform() >= settings_.badToGood;
    else
        bad_ = random.nextUniform() < settings_.goodToBad;
    return corrupted;
}

double GilbertChannel::meanBitErrorProbability() const
{
    return settings_.meanBitErrorProbability();
}

std::string GilbertChannel::displayName() const
{
    return "Гилберта";
}

std::string GilbertChannel::csvName() const
{
    return "Gilbert";
}

bool GilbertChannel::inBadState() const
{
    return bad_;
}

}
