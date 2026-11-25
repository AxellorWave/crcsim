#include <boost/test/unit_test.hpp>

#include "codes.h"
#include "plan.h"

#include <cstdio>
#include <stdexcept>
#include <string>

using namespace crc;

namespace {

std::string csvError(const std::string& text)
{
    try {
        parsePlanCsv(text);
    } catch (const std::invalid_argument& error) {
        return error.what();
    }
    return {};
}

std::string jsonError(const std::string& text)
{
    try {
        parsePlanJson(text);
    } catch (const std::invalid_argument& error) {
        return error.what();
    }
    return {};
}

bool sameParams(const SimulationParams& left, const SimulationParams& right)
{
    const bool sameChannel = left.channel.kind == right.channel.kind
        && (left.channel.kind == ChannelKind::Bsc
                ? left.channel.bsc.bitErrorProbability == right.channel.bsc.bitErrorProbability
                : left.channel.gilbert.goodToBad == right.channel.gilbert.goodToBad
                    && left.channel.gilbert.badToGood == right.channel.gilbert.badToGood
                    && left.channel.gilbert.errorInBad == right.channel.gilbert.errorInBad
                    && left.channel.gilbert.errorInGood == right.channel.gilbert.errorInGood);
    return sameChannel && left.coding.n == right.coding.n && left.coding.generator == right.coding.generator
        && left.control.stopMode == right.control.stopMode && left.control.blockLimit == right.control.blockLimit
        && (left.control.stopMode == StopMode::BlockCount || left.control.errorLimit == right.control.errorLimit)
        && left.control.seed == right.control.seed;
}

void checkSamePlan(const ExperimentPlan& left, const ExperimentPlan& right)
{
    BOOST_REQUIRE(left.size() == right.size());
    for (std::size_t index = 0; index < left.size(); ++index) {
        BOOST_TEST(sameParams(left[index].params, right[index].params));
        BOOST_TEST(left[index].label == right[index].label);
    }
}

ExperimentPlan mixedPlan()
{
    ExperimentPlan plan = parsePlanCsv("n;g;channel;p;p_gb;errors;blocks;seed;label\n"
                                       "15;10011;bsc;0.005;;;3000000;42;\"первый; \"\"с кавычкой\"\"\"\n"
                                       "31;\"[5;2;0]\";gilbert;;0.003;100;500000;7;\n");
    return plan;
}

}

BOOST_AUTO_TEST_SUITE(plan)

BOOST_AUTO_TEST_CASE(reads_semicolon_csv_with_decimal_comma)
{
    const ExperimentPlan plan = parsePlanCsv("n;g;channel;p;blocks;seed\n15;10011;bsc;0,005;3000000;42\n\n7;1011;дск;0.05;1 000;0\n");
    BOOST_REQUIRE(plan.size() == 2u);
    BOOST_TEST(plan[0].params.coding.n == 15);
    BOOST_TEST(plan[0].params.coding.generator == "10011");
    BOOST_TEST(plan[0].params.channel.bsc.bitErrorProbability == 0.005);
    BOOST_TEST(plan[0].params.control.blockLimit == 3000000u);
    BOOST_TEST(plan[0].params.control.seed == 42u);
    BOOST_TEST((plan[0].params.control.stopMode == StopMode::BlockCount));
    BOOST_TEST(plan[1].params.channel.bsc.bitErrorProbability == 0.05);
    BOOST_TEST(plan[1].params.control.blockLimit == 1000u);
}

BOOST_AUTO_TEST_CASE(reads_comma_and_tab_separated_csv)
{
    const ExperimentPlan comma = parsePlanCsv("n,g,p\r\n7,1011,0.01\r\n");
    BOOST_REQUIRE(comma.size() == 1u);
    BOOST_TEST(comma[0].params.coding.n == 7);
    BOOST_TEST(comma[0].params.channel.bsc.bitErrorProbability == 0.01);
    BOOST_TEST(!csvError("n,g,p\n7,1011,\"0,01\"\n").empty());

    const ExperimentPlan tab = parsePlanCsv("n\tg\tp\n15\t10011\t0,02\n");
    BOOST_REQUIRE(tab.size() == 1u);
    BOOST_TEST(tab[0].params.channel.bsc.bitErrorProbability == 0.02);
}

BOOST_AUTO_TEST_CASE(accepts_russian_aliases_in_any_case)
{
    const ExperimentPlan plan = parsePlanCsv(
        "\xEF\xBB\xBF" "Длина;Многочлен;КАНАЛ;P_ХП;P_ПХ;p_П;p_Х;Блоков;Ошибок;Seed;Метка\n"
        "15;\"[4;1;0]\";Гилберт;0,003;0,1;0,5;0;1000000;50;5;опыт\n");
    BOOST_REQUIRE(plan.size() == 1u);
    const SimulationParams& params = plan[0].params;
    BOOST_TEST(params.coding.generator == "[4;1;0]");
    BOOST_TEST((params.channel.kind == ChannelKind::Gilbert));
    BOOST_TEST(params.channel.gilbert.goodToBad == 0.003);
    BOOST_TEST(params.channel.gilbert.badToGood == 0.1);
    BOOST_TEST(params.channel.gilbert.errorInBad == 0.5);
    BOOST_TEST(params.channel.gilbert.errorInGood == 0.0);
    BOOST_TEST((params.control.stopMode == StopMode::ErrorCount));
    BOOST_TEST(params.control.errorLimit == 50u);
    BOOST_TEST(params.control.seed == 5u);
    BOOST_TEST(plan[0].label == "опыт");
}

BOOST_AUTO_TEST_CASE(missing_columns_take_defaults)
{
    const ExperimentPlan plan = parsePlanCsv("n;g\n7;1011\n");
    BOOST_REQUIRE(plan.size() == 1u);
    const SimulationParams defaults;
    BOOST_TEST((plan[0].params.channel.kind == ChannelKind::Bsc));
    BOOST_TEST(plan[0].params.channel.bsc.bitErrorProbability == defaults.channel.bsc.bitErrorProbability);
    BOOST_TEST(plan[0].params.control.blockLimit == defaults.control.blockLimit);
    BOOST_TEST((plan[0].params.control.stopMode == StopMode::BlockCount));
    BOOST_TEST(plan[0].params.control.seed == 0u);
}

BOOST_AUTO_TEST_CASE(blocks_accept_exponent_notation)
{
    BOOST_TEST(parsePlanCsv("n;g;blocks\n15;10011;3e6\n")[0].params.control.blockLimit == 3000000u);
    BOOST_TEST(!csvError("n;g;blocks\n15;10011;2.5\n").empty());
}

BOOST_AUTO_TEST_CASE(reports_validation_errors_with_line_numbers)
{
    BOOST_TEST(csvError("n;g;p\n15;10011;0,005\n\n15;10011;0,7\n") == "строка 4: p должна быть в интервале (0; 0,5]");
    BOOST_TEST(csvError("n;g\n15;10011\n70;10011\n") == "строка 3: n должно быть целым числом от 3 до 64");
    BOOST_TEST(csvError("n;g;p\n15;10011;abc\n") == "строка 2: p — ожидалось число, получено «abc»");
    BOOST_TEST(csvError("n;g;channel\n15;10011;awgn\n")
        == "строка 2: канал должен быть bsc, дск, gilbert или гилберт, получено «awgn»");
    BOOST_TEST(csvError("n;g;p\n15;10011\n") == "строка 2: ожидалось значений: 3, получено: 2");
    BOOST_TEST(csvError("n;g;x\n15;10011;1\n").rfind("строка 1: неизвестный столбец «x»", 0) == 0);
    BOOST_TEST(csvError("n;N\n15;15\n") == "строка 1: столбец «N» указан дважды");
    BOOST_TEST(csvError("n;g\n15;\"10011\n") == "строка 2: кавычка не закрыта");
    BOOST_TEST(csvError("n;g\n\n") == "в плане нет ни одного опыта");
    BOOST_TEST(!csvError("").empty());
}

BOOST_AUTO_TEST_CASE(reads_json_array_and_object_forms)
{
    const ExperimentPlan array = parsePlanJson(R"([{"n": 15, "g": "10011", "p": 0.005, "blocks": 3000000, "seed": 42}])");
    BOOST_REQUIRE(array.size() == 1u);
    BOOST_TEST(array[0].params.control.blockLimit == 3000000u);
    BOOST_TEST(array[0].params.channel.bsc.bitErrorProbability == 0.005);

    const ExperimentPlan object = parsePlanJson(
        R"({"experiments": [{"n": "7", "g": "1011", "канал": "gilbert", "p_gb": "0,003", "errors": 10, "label": null}]})");
    BOOST_REQUIRE(object.size() == 1u);
    BOOST_TEST(object[0].params.coding.n == 7);
    BOOST_TEST(object[0].params.channel.gilbert.goodToBad == 0.003);
    BOOST_TEST((object[0].params.control.stopMode == StopMode::ErrorCount));
}

BOOST_AUTO_TEST_CASE(reports_json_errors_with_line_numbers)
{
    BOOST_TEST(jsonError("[\n {\"n\": 15},\n {\"n\": 15,\n  \"p\": 0.9}\n]") == "строка 3: p должна быть в интервале (0; 0,5]");
    BOOST_TEST(jsonError("[\n {\"n\": 15, \"p\": true}\n]") == "строка 2: значение поля «p» должно быть числом или строкой");
    BOOST_TEST(jsonError("[\n 5\n]") == "строка 2: опыт 1 должен быть объектом JSON");
    BOOST_TEST(jsonError("{\"plan\": []}")
        == "строка 1: ожидался массив опытов или объект с полем «experiments» — массивом опытов");
    BOOST_TEST(jsonError("[\n {\"n\": 15,\n \"color\": 1}]").rfind("строка 3: неизвестное поле «color»", 0) == 0);
    BOOST_TEST(jsonError("[{\"n\": 15, \"длина\": 7}]") == "строка 1: поле «длина» указано дважды");
    BOOST_TEST(jsonError("[]") == "в плане нет ни одного опыта");
    BOOST_TEST(jsonError("[{\"n\": 15,}]").rfind("JSON, строка 1", 0) == 0);
}

BOOST_AUTO_TEST_CASE(detects_format_by_content)
{
    BOOST_TEST(parsePlan("  [{\"n\": 7, \"g\": \"1011\"}]").size() == 1u);
    BOOST_TEST(parsePlan("n;g\n7;1011\n").size() == 1u);
}

BOOST_AUTO_TEST_CASE(methodichka_plan_covers_all_codes)
{
    const std::size_t codeCount = knownCodes().size();
    BOOST_TEST(codeCount == 17u);
    const ExperimentPlan bscOnly = methodichkaPlan(false);
    const ExperimentPlan withGilbert = methodichkaPlan(true);
    BOOST_TEST(bscOnly.size() == codeCount * 3);
    BOOST_TEST(withGilbert.size() == codeCount * 6);
    for (const PlannedExperiment& experiment : withGilbert) {
        BOOST_TEST(validate(experiment.params).empty());
        BOOST_TEST(experiment.params.control.blockLimit == 3000000u);
        BOOST_TEST(experiment.params.control.seed == 42u);
    }
    BOOST_TEST(bscOnly[2].params.channel.bsc.bitErrorProbability == 0.005);
    BOOST_TEST(bscOnly[2].label == "(7, 4), ДСК p = 0.005");
    const GilbertSettings& gilbert = withGilbert[5].params.channel.gilbert;
    BOOST_TEST((withGilbert[5].params.channel.kind == ChannelKind::Gilbert));
    BOOST_TEST(gilbert.goodToBad == 0.0005);
    BOOST_TEST(gilbert.badToGood == 0.1);
    BOOST_TEST(gilbert.errorInBad == 0.5);
    BOOST_TEST(gilbert.errorInGood == 0.0);
}

BOOST_AUTO_TEST_CASE(round_trips_through_csv_and_json)
{
    const ExperimentPlan plan = mixedPlan();
    BOOST_REQUIRE(plan.size() == 2u);
    BOOST_TEST(plan[0].label == "первый; \"с кавычкой\"");
    checkSamePlan(parsePlanCsv(planToCsv(plan)), plan);
    checkSamePlan(parsePlanJson(planToJson(plan)), plan);
    checkSamePlan(parsePlanCsv(planToCsv(methodichkaPlan(true))), methodichkaPlan(true));
    checkSamePlan(parsePlanJson(planToJson(methodichkaPlan(true))), methodichkaPlan(true));
}

BOOST_AUTO_TEST_CASE(saves_and_loads_files_by_extension)
{
    const ExperimentPlan plan = mixedPlan();
    for (const std::string path : {"plan_test_tmp.csv", "plan_test_tmp.json"}) {
        savePlan(path, plan);
        checkSamePlan(loadPlan(path), plan);
        std::remove(path.c_str());
    }
    BOOST_CHECK_THROW(loadPlan("no_such_plan.csv"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(describes_channel_and_code)
{
    ChannelParams channel;
    BOOST_TEST(channelParametersText(channel) == "p = 0.005");
    channel.kind = ChannelKind::Gilbert;
    BOOST_TEST(channelParametersText(channel) == "P_ХП = 0.001, P_ПХ = 0.1, p_П = 0.5, p_Х = 0");
    BOOST_TEST(codeName(CyclicCode{15, parsePolynomial("10011")}) == "(15, 11)");
}

BOOST_AUTO_TEST_SUITE_END()
