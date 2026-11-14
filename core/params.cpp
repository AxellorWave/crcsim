#include "params.h"

#include <stdexcept>

namespace crc {

namespace {

bool inOpenUnitInterval(double value)
{
    return value > 0.0 && value < 1.0;
}

bool inHalfOpenHalf(double value)
{
    return value > 0.0 && value <= 0.5;
}

bool inClosedHalf(double value)
{
    return value >= 0.0 && value <= 0.5;
}

void requireThat(std::vector<ValidationError>& errors, bool condition, Field field, const std::string& message)
{
    if (!condition)
        errors.push_back({field, message});
}

std::vector<ValidationError> validateGenerator(const CodingParams& coding)
{
    Word generator = 0;
    try {
        generator = parsePolynomial(coding.generator);
    } catch (const std::invalid_argument& error) {
        return {{Field::Generator, error.what()}};
    }
    const int checkBits = degree(generator);
    std::vector<ValidationError> errors;
    requireThat(errors, checkBits >= 1, Field::Generator, "степень g(x) должна быть не меньше 1");
    requireThat(errors, (generator & 1) != 0, Field::Generator, "свободный член (последний коэффициент) g(x) должен быть равен 1");
    const bool lengthIsValid = coding.n >= minCodeLength && coding.n <= maxCodeLength;
    requireThat(errors, !lengthIsValid || checkBits < coding.n, Field::Generator,
        "старшая степень g(x) должна быть меньше n (сейчас h = " + std::to_string(checkBits)
            + ", n = " + std::to_string(coding.n) + ")");
    return errors;
}

void append(std::vector<ValidationError>& target, const std::vector<ValidationError>& source)
{
    target.insert(target.end(), source.begin(), source.end());
}

}

std::vector<ValidationError> validateCoding(const CodingParams& coding)
{
    std::vector<ValidationError> errors;
    requireThat(errors, coding.n >= minCodeLength && coding.n <= maxCodeLength, Field::CodeLength,
        "n должно быть целым числом от 3 до 64");
    append(errors, validateGenerator(coding));
    return errors;
}

std::vector<ValidationError> validateChannel(const ChannelParams& channel)
{
    std::vector<ValidationError> errors;
    if (channel.kind == ChannelKind::Bsc) {
        requireThat(errors, inHalfOpenHalf(channel.bsc.bitErrorProbability), Field::BitErrorProbability,
            "p должна быть в интервале (0; 0,5]");
        return errors;
    }
    const GilbertSettings& gilbert = channel.gilbert;
    requireThat(errors, inOpenUnitInterval(gilbert.goodToBad), Field::GoodToBad, "P_ХП должна быть в интервале (0; 1)");
    requireThat(errors, inOpenUnitInterval(gilbert.badToGood), Field::BadToGood, "P_ПХ должна быть в интервале (0; 1)");
    requireThat(errors, inHalfOpenHalf(gilbert.errorInBad), Field::ErrorInBad, "p_П должна быть в интервале (0; 0,5]");
    requireThat(errors, inClosedHalf(gilbert.errorInGood), Field::ErrorInGood, "p_Х должна быть в интервале [0; 0,5]");
    return errors;
}

std::vector<ValidationError> validateControl(const ControlParams& control)
{
    std::vector<ValidationError> errors;
    const bool countsErrors = control.stopMode == StopMode::ErrorCount;
    requireThat(errors, control.blockLimit >= 1 && control.blockLimit <= maxBlockLimit, Field::BlockLimit,
        countsErrors ? "предельное число блоков должно быть от 1 до 1 000 000 000"
                     : "число блоков N должно быть от 1 до 1 000 000 000");
    if (countsErrors) {
        requireThat(errors, control.errorLimit >= 1 && control.errorLimit <= maxBlockLimit, Field::ErrorLimit,
            "число ошибочных блоков должно быть от 1 до 1 000 000 000");
    }
    return errors;
}

std::vector<ValidationError> validate(const SimulationParams& params)
{
    std::vector<ValidationError> errors = validateCoding(params.coding);
    append(errors, validateChannel(params.channel));
    append(errors, validateControl(params.control));
    return errors;
}

CyclicCode makeCode(const CodingParams& coding)
{
    const std::vector<ValidationError> errors = validateCoding(coding);
    if (!errors.empty())
        throw std::invalid_argument(errors.front().message);
    return CyclicCode{coding.n, parsePolynomial(coding.generator)};
}

std::unique_ptr<Channel> makeChannel(const ChannelParams& channel)
{
    switch (channel.kind) {
    case ChannelKind::Bsc:
        return std::make_unique<BscChannel>(channel.bsc);
    case ChannelKind::Gilbert:
        return std::make_unique<GilbertChannel>(channel.gilbert);
    }
    throw std::invalid_argument("неизвестный тип канала");
}

double meanBitErrorProbability(const ChannelParams& channel)
{
    return makeChannel(channel)->meanBitErrorProbability();
}

}
