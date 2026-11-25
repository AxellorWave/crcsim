#include <boost/test/unit_test.hpp>

#include "codes.h"
#include "selection.h"

#include <atomic>
#include <cmath>
#include <stdexcept>

using namespace crc;

namespace {

SelectionRow makeRow(const std::string& generator, std::uint64_t undetected, std::uint64_t correct, double target = 1e-6)
{
    SimulationResult result;
    result.code = CyclicCode{15, parsePolynomial(generator)};
    result.counters.blocks = 3000000;
    result.counters.correct = correct;
    result.counters.undetected = undetected;
    result.counters.detected = result.counters.blocks - correct - undetected;
    SelectionRequest request;
    request.target = target;
    return evaluateCandidate(result, request);
}

SelectionRequest referenceRequest()
{
    SelectionRequest request;
    request.codeLength = 15;
    request.channel.bsc.bitErrorProbability = 0.005;
    request.blocks = 3000000;
    request.seed = 42;
    return request;
}

}

BOOST_AUTO_TEST_SUITE(selection)

BOOST_AUTO_TEST_CASE(judges_requirement)
{
    BOOST_TEST((judge({0, 3000000}, 1e-6) == Verdict::Satisfied));
    BOOST_TEST((judge({0, 10000000}, 1e-6) == Verdict::Satisfied));
    BOOST_TEST((judge({20, 3000000}, 1e-6) == Verdict::NotSatisfied));
    BOOST_TEST((judge({2, 3000000}, 1e-6) == Verdict::Borderline));
    BOOST_TEST((judge({4, 3000000}, 1e-6) == Verdict::NotSatisfied));
    BOOST_TEST((judge({1, 100000000}, 1e-6) == Verdict::Satisfied));
    BOOST_TEST((judge({0, 1000000}, 1e-6) == Verdict::Borderline));
}

BOOST_AUTO_TEST_CASE(borderline_means_point_below_and_bound_above)
{
    const SelectionRow row = makeRow("111010001", 2, 2782365);
    BOOST_TEST(row.undetected.value() <= 1e-6);
    BOOST_TEST(row.upperBound > 1e-6);
    BOOST_TEST((row.verdict == Verdict::Borderline));
    BOOST_TEST(verdictText(row.verdict) == "пограничный — увеличьте N");
    BOOST_TEST(verdictText(Verdict::Satisfied) == "выполнено");
    BOOST_TEST(verdictText(Verdict::NotSatisfied) == "не выполнено");
}

BOOST_AUTO_TEST_CASE(computes_attempts_and_transfer_time)
{
    const SelectionRow row = makeRow("111010001", 0, 2500000);
    BOOST_REQUIRE(row.meanAttempts.has_value());
    BOOST_TEST(*row.meanAttempts == 1.2, boost::test_tools::tolerance(1e-12));
    BOOST_REQUIRE(row.transferTime.has_value());
    BOOST_TEST(*row.transferTime == 1e6 / 7.0 * 15.0 / 9600.0 * 1.2, boost::test_tools::tolerance(1e-12));
    const SelectionRow lost = makeRow("111010001", 0, 0);
    BOOST_TEST(!lost.meanAttempts.has_value());
    BOOST_TEST(!lost.transferTime.has_value());
}

BOOST_AUTO_TEST_CASE(validates_transfer_settings)
{
    BOOST_TEST(validateTransfer({}).empty());
    const std::vector<ValidationError> errors = validateTransfer({0.0, -1.0});
    BOOST_REQUIRE(errors.size() == 2u);
    BOOST_TEST(errors[0].message == "размер файла M должен быть больше 0");
    BOOST_TEST(errors[1].message == "скорость передачи должна быть больше 0");
    SimulationResult result;
    result.code = CyclicCode{15, parsePolynomial("10011")};
    result.counters.blocks = 10;
    result.counters.correct = 5;
    BOOST_TEST(!transferTime(result, {1000.0, 0.0}).has_value());
    BOOST_TEST(*transferTime(result, {1100.0, 1.0}) == 3000.0, boost::test_tools::tolerance(1e-12));
}

BOOST_AUTO_TEST_CASE(picks_best_among_satisfied_rows)
{
    std::vector<SelectionRow> rows = {
        makeRow("10011", 20, 2782365),
        makeRow("111010001", 0, 2782365),
        makeRow("10100110111", 0, 2782365),
    };
    const BestCodes best = chooseBest(rows);
    BOOST_TEST(*best.byRedundancy == 1u);
    BOOST_TEST(*best.byTime == 1u);

    rows[1].transferTime = 1000.0;
    rows[2].transferTime = 900.0;
    const BestCodes swapped = chooseBest(rows);
    BOOST_TEST(*swapped.byRedundancy == 1u);
    BOOST_TEST(*swapped.byTime == 2u);
}

BOOST_AUTO_TEST_CASE(breaks_ties_by_the_other_criterion)
{
    std::vector<SelectionRow> rows = {makeRow("111010001", 0, 2782365), makeRow("111010001", 0, 2782365)};
    rows[0].transferTime = 250.0;
    rows[1].transferTime = 240.0;
    BOOST_TEST(*chooseBest(rows).byRedundancy == 1u);
    std::vector<SelectionRow> sameTime = {makeRow("10100110111", 0, 2782365), makeRow("111010001", 0, 2782365)};
    sameTime[0].transferTime = 240.0;
    sameTime[1].transferTime = 240.0;
    BOOST_TEST(*chooseBest(sameTime).byTime == 1u);
}

BOOST_AUTO_TEST_CASE(no_best_without_satisfied_rows)
{
    const std::vector<SelectionRow> rows = {makeRow("10011", 20, 2782365), makeRow("111010001", 2, 2782365)};
    const BestCodes best = chooseBest(rows);
    BOOST_TEST(!best.byRedundancy.has_value());
    BOOST_TEST(!best.byTime.has_value());
    BOOST_TEST(selectionConclusion(referenceRequest(), rows, best)
        == "Для n = 15 на ДСК p = 0,005 ни один код не выполнил требование по Pлож; есть пограничные коды — увеличьте N.");
}

BOOST_AUTO_TEST_CASE(lists_candidates)
{
    BOOST_TEST(candidateCodes(15).size() == 3u);
    BOOST_TEST(candidateCodes(63).size() == 8u);
    BOOST_TEST(candidateCodes(std::nullopt).size() == knownCodes().size());
    BOOST_TEST(candidateCodes(16).empty());
    BOOST_TEST(knownCodeLengths() == std::vector<int>({7, 15, 31, 63}), boost::test_tools::per_element());
    SelectionRequest request = referenceRequest();
    request.codeLength = 16;
    BOOST_TEST(joinMessages(validateSelection(request)) == "нет известных кодов длины n = 16");
    request = referenceRequest();
    request.target = 0.0;
    request.blocks = 0;
    BOOST_TEST(validateSelection(request).size() == 2u);
    BOOST_CHECK_THROW(selectCode(request), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(reproduces_reference_selection_for_n15)
{
    std::size_t lastIndex = 0;
    SelectionControl control;
    control.onProgress = [&lastIndex](std::size_t index, std::size_t count, const Counters&) {
        lastIndex = index;
        BOOST_TEST(count == 3u);
    };
    const SelectionOutcome outcome = selectCode(referenceRequest(), control);
    BOOST_REQUIRE(outcome.rows.size() == 3u);
    BOOST_TEST(!outcome.cancelled);
    BOOST_TEST(lastIndex == 2u);
    BOOST_TEST(outcome.rows[0].result.counters.undetected == 20u);
    BOOST_TEST(outcome.rows[1].result.counters.undetected == 0u);
    BOOST_TEST(outcome.rows[2].result.counters.undetected == 0u);
    BOOST_TEST((outcome.rows[0].verdict == Verdict::NotSatisfied));
    BOOST_TEST((outcome.rows[1].verdict == Verdict::Satisfied));
    BOOST_TEST((outcome.rows[2].verdict == Verdict::Satisfied));
    BOOST_TEST(*outcome.best.byRedundancy == 1u);
    BOOST_TEST(*outcome.best.byTime == 1u);
    const std::string conclusion = selectionConclusion(referenceRequest(), outcome.rows, outcome.best);
    BOOST_TEST(conclusion.rfind("Для n = 15 на ДСК p = 0,005: лучший по избыточности — (15, 7), I = 0,533; лучший по времени — (15, 7), Tф = ", 0) == 0);
}

BOOST_AUTO_TEST_CASE(cancel_stops_selection)
{
    std::atomic<bool> cancel{true};
    SelectionControl control;
    control.cancelRequested = &cancel;
    control.progressInterval = 1000;
    const SelectionOutcome outcome = selectCode(referenceRequest(), control);
    BOOST_TEST(outcome.cancelled);
    BOOST_TEST(outcome.rows.empty());
}

BOOST_AUTO_TEST_SUITE_END()
