#include <boost/test/unit_test.hpp>

#include "params.h"

#include <cmath>
#include <limits>

using namespace crc;

namespace {

bool hasError(const std::vector<ValidationError>& errors, Field field)
{
    for (const ValidationError& error : errors) {
        if (error.field == field)
            return true;
    }
    return false;
}

CodingParams coding(int n, const std::string& generator)
{
    return CodingParams{n, generator};
}

}

BOOST_AUTO_TEST_SUITE(params)

BOOST_AUTO_TEST_CASE(defaults_are_valid)
{
    SimulationParams params;
    BOOST_TEST(validate(params).empty());
    params.channel.kind = ChannelKind::Gilbert;
    params.control.stopMode = StopMode::ErrorCount;
    BOOST_TEST(validate(params).empty());
}

BOOST_AUTO_TEST_CASE(code_length_range)
{
    BOOST_TEST(hasError(validateCoding(coding(2, "11")), Field::CodeLength));
    BOOST_TEST(hasError(validateCoding(coding(65, "10011")), Field::CodeLength));
    BOOST_TEST(validateCoding(coding(3, "11")).empty());
    BOOST_TEST(validateCoding(coding(64, "10011")).empty());
}

BOOST_AUTO_TEST_CASE(generator_checks)
{
    BOOST_TEST(hasError(validateCoding(coding(15, "10021")), Field::Generator));
    BOOST_TEST(hasError(validateCoding(coding(15, "10010")), Field::Generator));
    BOOST_TEST(hasError(validateCoding(coding(15, "[4;1]")), Field::Generator));
    BOOST_TEST(hasError(validateCoding(coding(15, "1")), Field::Generator));
    BOOST_TEST(hasError(validateCoding(coding(7, "111010001")), Field::Generator));
    BOOST_TEST(hasError(validateCoding(coding(8, "111010001")), Field::Generator));
    BOOST_TEST(validateCoding(coding(9, "111010001")).empty());
    BOOST_TEST(validateCoding(coding(64, "[63;0]")).empty());
}

BOOST_AUTO_TEST_CASE(generator_message_explains_the_reason)
{
    const auto errors = validateCoding(coding(7, "111010001"));
    BOOST_REQUIRE(!errors.empty());
    BOOST_TEST(errors.front().message.find("меньше n") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(code_is_built_from_valid_params)
{
    const CyclicCode code = makeCode(coding(15, "[8;7;6;4;0]"));
    BOOST_TEST(code.checkBitCount() == 8);
    BOOST_TEST(code.dataBitCount() == 7);
    BOOST_CHECK_THROW(makeCode(coding(7, "111010001")), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(bsc_probability_range)
{
    ChannelParams channel;
    for (double p : {0.0, -0.1, 0.51, std::numeric_limits<double>::quiet_NaN()}) {
        channel.bsc.bitErrorProbability = p;
        BOOST_TEST(hasError(validateChannel(channel), Field::BitErrorProbability));
    }
    channel.bsc.bitErrorProbability = 0.5;
    BOOST_TEST(validateChannel(channel).empty());
}

BOOST_AUTO_TEST_CASE(gilbert_ranges)
{
    ChannelParams channel;
    channel.kind = ChannelKind::Gilbert;
    channel.gilbert = GilbertSettings{0.0, 1.0, 0.0, 0.6};
    const auto errors = validateChannel(channel);
    BOOST_TEST(hasError(errors, Field::GoodToBad));
    BOOST_TEST(hasError(errors, Field::BadToGood));
    BOOST_TEST(hasError(errors, Field::ErrorInBad));
    BOOST_TEST(hasError(errors, Field::ErrorInGood));
    channel.gilbert = GilbertSettings{0.5, 0.5, 0.5, 0.0};
    BOOST_TEST(validateChannel(channel).empty());
}

BOOST_AUTO_TEST_CASE(control_limits)
{
    ControlParams control;
    control.blockLimit = 0;
    BOOST_TEST(hasError(validateControl(control), Field::BlockLimit));
    control.blockLimit = maxBlockLimit + 1;
    BOOST_TEST(hasError(validateControl(control), Field::BlockLimit));
    control.blockLimit = maxBlockLimit;
    control.stopMode = StopMode::ErrorCount;
    control.errorLimit = 0;
    BOOST_TEST(hasError(validateControl(control), Field::ErrorLimit));
}

BOOST_AUTO_TEST_CASE(channel_factory)
{
    ChannelParams channel;
    BOOST_TEST(makeChannel(channel)->csvName() == "BSC");
    BOOST_TEST(meanBitErrorProbability(channel) == 0.005);
    channel.kind = ChannelKind::Gilbert;
    BOOST_TEST(makeChannel(channel)->csvName() == "Gilbert");
    BOOST_TEST(std::abs(meanBitErrorProbability(channel) - 0.0049505) < 1e-7);
}

BOOST_AUTO_TEST_SUITE_END()
