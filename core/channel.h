#pragma once

#include "poly.h"
#include "random.h"

#include <string>

namespace crc {

class Channel {
public:
    virtual ~Channel() = default;

    virtual void start(Random& random);
    virtual bool corruptsNextBit(Random& random) = 0;
    virtual double meanBitErrorProbability() const = 0;
    virtual std::string displayName() const = 0;
    virtual std::string csvName() const = 0;

    Word errorVector(Random& random, int length);
};

struct BscSettings {
    double bitErrorProbability = 0.005;
};

class BscChannel : public Channel {
public:
    explicit BscChannel(const BscSettings& settings);

    bool corruptsNextBit(Random& random) override;
    double meanBitErrorProbability() const override;
    std::string displayName() const override;
    std::string csvName() const override;

private:
    BscSettings settings_;
};

struct GilbertSettings {
    double goodToBad = 0.001;
    double badToGood = 0.1;
    double errorInBad = 0.5;
    double errorInGood = 0.0;

    double badStateShare() const;
    double goodStateShare() const;
    double meanBitErrorProbability() const;
    double meanBurstLength() const;
};

class GilbertChannel : public Channel {
public:
    explicit GilbertChannel(const GilbertSettings& settings);

    void start(Random& random) override;
    bool corruptsNextBit(Random& random) override;
    double meanBitErrorProbability() const override;
    std::string displayName() const override;
    std::string csvName() const override;

    bool inBadState() const;

private:
    GilbertSettings settings_;
    bool bad_ = false;
};

}
