#include "walkthrough_script.h"

#include "poly.h"

namespace {

const int showDataMs = 800;
const int shiftZerosMs = 650;
const int placeDivisorMs = 500;
const int writeDifferenceMs = 550;
const int markRemainderMs = 500;
const int formCodewordMs = 900;
const int showErrorsMs = 600;
const int applyErrorsMs = 1000;
const int showReceivedMs = 550;
const int showVerdictMs = 700;

QString windowBits(crc::Word value, const crc::DivisionStep& step, int checkCount)
{
    const crc::Word window = (value >> (step.position - checkCount)) & crc::lowBitsMask(checkCount + 1);
    return bitText(window, checkCount + 1);
}

QString shiftedName(int checkCount)
{
    return QStringLiteral("D·x") + superscript(checkCount);
}

void addDivisionSteps(std::vector<WalkStep>& script, WalkStage stage, const crc::Division& division, int checkCount)
{
    const QString generator = bitText(division.divisor, checkCount + 1);
    for (int index = 0; index < static_cast<int>(division.steps.size()); ++index) {
        const crc::DivisionStep& step = division.steps[static_cast<std::size_t>(index)];
        script.push_back({stage, WalkAction::PlaceDivisor, index,
            QStringLiteral("Старшая единица стоит в разряде x%1: подписываем под ней g(x) = %2 и складываем по модулю 2")
                .arg(superscript(step.position), generator),
            placeDivisorMs});
        const QString rule = index == 0 ? QStringLiteral(" (1 ⊕ 1 = 0, 1 ⊕ 0 = 1)") : QString();
        script.push_back({stage, WalkAction::WriteDifference, index,
            QStringLiteral("%1 ⊕ %2 = %3%4; ведущие нули отбрасываем")
                .arg(windowBits(step.before, step, checkCount), generator, windowBits(step.after, step, checkCount), rule),
            writeDifferenceMs});
    }
}

QString remainderCaption(const crc::Walkthrough& trace)
{
    const int checkCount = trace.code.checkBitCount();
    const QString remainder = bitText(trace.checkBits, checkCount);
    if (trace.encoding.steps.empty())
        return QStringLiteral("Единиц в старших разрядах нет — делить нечего: остаток C = %1").arg(remainder);
    return QStringLiteral("Степень остатка меньше h = %1 — деление закончено. Остаток C = %2 — проверочные биты")
        .arg(checkCount)
        .arg(remainder);
}

QString errorsCaption(const crc::Walkthrough& trace)
{
    const QString errors = bitText(trace.errors, trace.code.n);
    if (trace.errors == 0)
        return QStringLiteral("Канал не внёс ошибок: вектор ошибок E = %1").arg(errors);
    return QStringLiteral("Канал накладывает вектор ошибок E = %1: единицы отмечают искажённые разряды (%2)")
        .arg(errors, bitCountText(countOnes(trace.errors)));
}

QString receivedCaption(const crc::Walkthrough& trace)
{
    const QString received = bitText(trace.received, trace.code.n);
    if (trace.errors == 0)
        return QStringLiteral("Y = F ⊕ E = %1 — блок пришёл без искажений").arg(received);
    return QStringLiteral("Принято Y = F ⊕ E = %1: искажённые биты перевернулись и выделены красным").arg(received);
}

void addEncoding(std::vector<WalkStep>& script, const crc::Walkthrough& trace)
{
    const crc::CyclicCode& code = trace.code;
    const int k = code.dataBitCount();
    const int h = code.checkBitCount();
    script.push_back({WalkStage::Source, WalkAction::ShowData, -1,
        QStringLiteral("Источник выдаёт блок данных D = %1 (k = %2)").arg(bitText(trace.data, k)).arg(k),
        showDataMs});
    script.push_back({WalkStage::Encoder, WalkAction::ShiftZeros, -1,
        QStringLiteral("Умножаем D(x) на x%1: справа дописываем нули (h = %2) — место для проверочных бит")
            .arg(superscript(h))
            .arg(h),
        shiftZerosMs});
    addDivisionSteps(script, WalkStage::Encoder, trace.encoding, h);
    script.push_back({WalkStage::Encoder, WalkAction::MarkRemainder, -1, remainderCaption(trace), markRemainderMs});
    script.push_back({WalkStage::Encoder, WalkAction::FormCodeword, -1,
        QStringLiteral("Кодовое слово F = %1 ⊕ C = %2: слева данные (k = %3), справа проверочные биты (h = %4)")
            .arg(shiftedName(h), bitText(trace.codeword, code.n))
            .arg(k)
            .arg(h),
        formCodewordMs});
}

void addDecoding(std::vector<WalkStep>& script, const crc::Walkthrough& trace)
{
    const int h = trace.code.checkBitCount();
    script.push_back({WalkStage::Decoder, WalkAction::ShowReceived, -1,
        QStringLiteral("Декодер делит принятое слово Y = %1 на g(x) = %2")
            .arg(bitText(trace.received, trace.code.n), bitText(trace.code.generator, h + 1)),
        showReceivedMs});
    addDivisionSteps(script, WalkStage::Decoder, trace.decoding, h);
    script.push_back({WalkStage::Decoder, WalkAction::MarkSyndrome, -1,
        QStringLiteral("Остаток от деления Y на g(x) — синдром S = %1").arg(bitText(trace.syndrome, h)),
        markRemainderMs});
}

}

WalkScene sceneOf(WalkStage stage)
{
    switch (stage) {
    case WalkStage::Source:
    case WalkStage::Encoder:
        return WalkScene::Encoding;
    case WalkStage::Channel:
        return WalkScene::Channel;
    case WalkStage::Decoder:
        return WalkScene::Decoding;
    case WalkStage::Result:
        return WalkScene::Verdict;
    }
    return WalkScene::Encoding;
}

std::vector<WalkStep> buildScript(const crc::Walkthrough& trace)
{
    std::vector<WalkStep> script;
    addEncoding(script, trace);
    script.push_back({WalkStage::Channel, WalkAction::ShowErrors, -1, errorsCaption(trace), showErrorsMs});
    script.push_back({WalkStage::Channel, WalkAction::ApplyErrors, -1, receivedCaption(trace), applyErrorsMs});
    addDecoding(script, trace);
    script.push_back({WalkStage::Result, WalkAction::ShowVerdict, -1, verdictTitle(trace.verdict), showVerdictMs});
    return script;
}

QString superscript(int value)
{
    static const QString digits = QStringLiteral("⁰¹²³⁴⁵⁶⁷⁸⁹");
    QString text;
    for (QChar digit : QString::number(value))
        text += digits.at(digit.digitValue());
    return text;
}

QString bitText(crc::Word value, int length)
{
    return QString::fromStdString(crc::toBinary(value, length));
}

QString bitCountText(int count)
{
    const int lastTwo = count % 100;
    const int last = count % 10;
    const bool paucal = last >= 2 && last <= 4 && !(lastTwo >= 12 && lastTwo <= 14);
    return QStringLiteral("%1 %2").arg(count).arg(paucal ? QStringLiteral("бита") : QStringLiteral("бит"));
}

QString polynomialText(crc::Word polynomial)
{
    QString text;
    for (int bit = crc::degree(polynomial); bit >= 0; --bit) {
        if (!((polynomial >> bit) & 1))
            continue;
        if (!text.isEmpty())
            text += QStringLiteral(" + ");
        if (bit == 0)
            text += QStringLiteral("1");
        else if (bit == 1)
            text += QStringLiteral("x");
        else
            text += QStringLiteral("x") + superscript(bit);
    }
    return text;
}

int countOnes(crc::Word value)
{
    int count = 0;
    for (; value != 0; value &= value - 1)
        ++count;
    return count;
}

QString verdictTitle(crc::Verdict verdict)
{
    switch (verdict) {
    case crc::Verdict::Correct:
        return QStringLiteral("Ошибок нет");
    case crc::Verdict::Detected:
        return QStringLiteral("Ошибка обнаружена: S ≠ 0, блок запрашивается повторно");
    case crc::Verdict::Undetected:
        return QStringLiteral("Ложный приём: S = 0, хотя блок искажён");
    }
    return QString();
}

QString verdictExplanation(const crc::Walkthrough& trace)
{
    const int n = trace.code.n;
    switch (trace.verdict) {
    case crc::Verdict::Correct:
        return QStringLiteral("Синдром равен нулю и канал не исказил ни одного бита — блок принят верно.");
    case crc::Verdict::Detected:
        return QStringLiteral("Y = %1 не делится на g(x) без остатка, значит, это не кодовое слово: декодер "
                              "замечает искажение (%2) и просит передать блок ещё раз.")
            .arg(bitText(trace.received, n), bitCountText(countOnes(trace.errors)));
    case crc::Verdict::Undetected:
        return QStringLiteral("Вектор ошибок E = %1 сам является кодовым словом: он делится на g(x), поэтому "
                              "Y = F ⊕ E — тоже кодовое слово, и декодер не видит искажения.")
            .arg(bitText(trace.errors, n));
    }
    return QString();
}
