#include "dialog.h"

#include "input.h"
#include "report.h"

#include <functional>
#include <istream>
#include <ostream>
#include <stdexcept>

namespace console {

namespace {

struct EndOfInput {
};

class Dialog {
public:
    Dialog(std::istream& input, std::ostream& output)
        : input_(input)
        , output_(output)
    {
    }

    std::string askLine(const std::string& prompt)
    {
        output_ << prompt << std::flush;
        std::string line;
        if (!std::getline(input_, line))
            throw EndOfInput{};
        return trim(line);
    }

    template <typename Value>
    Value askUntilValid(const std::string& prompt, const std::function<std::optional<Value>(const std::string&)>& parse,
        const std::string& formatError, const std::function<std::string(const Value&)>& check)
    {
        while (true) {
            const std::optional<Value> value = parse(askLine(prompt));
            const std::string problem = value ? check(*value) : formatError;
            if (problem.empty())
                return *value;
            output_ << "  Ошибка: " << problem << ". Повторите ввод.\n";
        }
    }

    std::ostream& output() { return output_; }

private:
    std::istream& input_;
    std::ostream& output_;
};

std::string problemFor(const std::vector<crc::ValidationError>& errors, crc::Field field)
{
    for (const crc::ValidationError& error : errors) {
        if (error.field == field)
            return error.message;
    }
    return {};
}

std::string firstProblem(const std::vector<crc::ValidationError>& errors)
{
    return errors.empty() ? std::string() : errors.front().message;
}

const std::function<std::optional<int>(const std::string&)> integerParser = parseInteger;
const std::function<std::optional<double>(const std::string&)> realParser = parseReal;
const std::function<std::optional<std::uint64_t>(const std::string&)> countParser = parseCount;
const std::function<std::optional<std::string>(const std::string&)> textParser
    = [](const std::string& text) { return std::optional<std::string>(text); };

int askChoice(Dialog& dialog, const std::string& prompt)
{
    return dialog.askUntilValid<int>(prompt, integerParser, "введите 1 или 2",
        [](const int& choice) { return choice == 1 || choice == 2 ? std::string() : "введите 1 или 2"; });
}

crc::CodingParams askCoding(Dialog& dialog)
{
    crc::CodingParams coding;
    dialog.output() << "[Кодирование]\n";
    coding.n = dialog.askUntilValid<int>("  Длина кодового слова n (3..64): ", integerParser,
        "n должно быть целым числом", [&](const int& n) {
            return problemFor(crc::validateCoding({n, coding.generator}), crc::Field::CodeLength);
        });
    coding.generator = dialog.askUntilValid<std::string>(
        "  Образующий многочлен g(x) (например 10011 или [4;1;0]): ", textParser, "", [&](const std::string& text) {
            return firstProblem(crc::validateCoding({coding.n, text}));
        });
    const crc::CyclicCode code = crc::makeCode(coding);
    dialog.output() << "  -> h = " << code.checkBitCount() << " проверочных бит, k = " << code.dataBitCount()
                    << " бит данных\n\n";
    return coding;
}

double askProbability(Dialog& dialog, const std::string& prompt, crc::ChannelParams& channel, double& target,
    crc::Field field)
{
    return dialog.askUntilValid<double>(prompt, realParser, "введите число, например 0.005", [&](const double& value) {
        target = value;
        return problemFor(crc::validateChannel(channel), field);
    });
}

crc::ChannelParams askChannel(Dialog& dialog)
{
    crc::ChannelParams channel;
    dialog.output() << "[Канал связи]\n";
    const int kind = askChoice(dialog, "  Тип канала: 1 - ДСК (независимые ошибки), 2 - Гилберта (пачки ошибок): ");
    if (kind == 1) {
        channel.kind = crc::ChannelKind::Bsc;
        askProbability(dialog, "  Вероятность ошибки на бит p (0..0.5]: ", channel, channel.bsc.bitErrorProbability,
            crc::Field::BitErrorProbability);
    } else {
        channel.kind = crc::ChannelKind::Gilbert;
        crc::GilbertSettings& gilbert = channel.gilbert;
        askProbability(dialog, "  Вероятность перехода хорошее -> плохое P_ХП (0..1): ", channel, gilbert.goodToBad,
            crc::Field::GoodToBad);
        askProbability(dialog, "  Вероятность перехода плохое -> хорошее P_ПХ (0..1): ", channel, gilbert.badToGood,
            crc::Field::BadToGood);
        askProbability(dialog, "  Вероятность ошибки в плохом состоянии p_П (0..0.5]: ", channel, gilbert.errorInBad,
            crc::Field::ErrorInBad);
        askProbability(dialog, "  Вероятность ошибки в хорошем состоянии p_Х [0..0.5]: ", channel,
            gilbert.errorInGood, crc::Field::ErrorInGood);
        printChannelCharacteristics(dialog.output(), channel);
    }
    dialog.output() << '\n';
    return channel;
}

std::uint64_t askLimit(Dialog& dialog, const std::string& prompt, crc::ControlParams& control,
    std::uint64_t& target, crc::Field field)
{
    return dialog.askUntilValid<std::uint64_t>(prompt, countParser, "введите целое положительное число",
        [&](const std::uint64_t& value) {
            target = value;
            return problemFor(crc::validateControl(control), field);
        });
}

std::uint64_t askSeed(Dialog& dialog)
{
    return dialog.askUntilValid<std::uint64_t>("  Seed (0 или пусто - случайный): ",
        [](const std::string& text) { return text.empty() ? std::optional<std::uint64_t>(0) : parseCount(text); },
        "seed должен быть целым неотрицательным числом", [](const std::uint64_t&) { return std::string(); });
}

crc::ControlParams askControl(Dialog& dialog)
{
    crc::ControlParams control;
    dialog.output() << "[Управление моделированием]\n";
    const int mode = askChoice(dialog, "  Остановка: 1 - по числу блоков, 2 - по числу ошибочных блоков: ");
    if (mode == 1) {
        control.stopMode = crc::StopMode::BlockCount;
        askLimit(dialog, "  Число блоков N: ", control, control.blockLimit, crc::Field::BlockLimit);
    } else {
        control.stopMode = crc::StopMode::ErrorCount;
        askLimit(dialog, "  Число ошибочных блоков до остановки: ", control, control.errorLimit, crc::Field::ErrorLimit);
        askLimit(dialog, "  Не более N блоков: ", control, control.blockLimit, crc::Field::BlockLimit);
    }
    control.seed = askSeed(dialog);
    return control;
}

}

std::optional<crc::SimulationParams> askParams(std::istream& input, std::ostream& output)
{
    Dialog dialog(input, output);
    try {
        crc::SimulationParams params;
        params.coding = askCoding(dialog);
        params.channel = askChannel(dialog);
        params.control = askControl(dialog);
        return params;
    } catch (const EndOfInput&) {
        output << "\nВвод прерван.\n";
        return std::nullopt;
    }
}

std::optional<std::string> askCsvPath(std::istream& input, std::ostream& output)
{
    Dialog dialog(input, output);
    try {
        const std::string answer = dialog.askLine("\nСохранить результат в CSV? (y/n): ");
        if (answer != "y" && answer != "Y" && answer != "д" && answer != "Д")
            return std::nullopt;
        const std::string path = dialog.askLine("Имя файла [results.csv]: ");
        return path.empty() ? std::string("results.csv") : path;
    } catch (const EndOfInput&) {
        return std::nullopt;
    }
}

}
