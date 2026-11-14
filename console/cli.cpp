#include "cli.h"

#include "input.h"

#include <functional>
#include <map>
#include <stdexcept>

namespace console {

namespace {

template <typename Value>
Value parseOrThrow(const std::optional<Value>& value, const std::string& option, const std::string& text)
{
    if (!value)
        throw std::invalid_argument("неверное значение «" + text + "» для " + option);
    return *value;
}

double realValue(const std::string& option, const std::string& text)
{
    return parseOrThrow(parseReal(text), option, text);
}

std::uint64_t countValue(const std::string& option, const std::string& text)
{
    return parseOrThrow(parseCount(text), option, text);
}

crc::ChannelKind channelKind(const std::string& text)
{
    if (text == "bsc")
        return crc::ChannelKind::Bsc;
    if (text == "gilbert")
        return crc::ChannelKind::Gilbert;
    throw std::invalid_argument("тип канала должен быть bsc или gilbert, получено «" + text + "»");
}

using ValueHandler = std::function<void(CommandLine&, const std::string&, const std::string&)>;
using FlagHandler = std::function<void(CommandLine&)>;

const std::map<std::string, ValueHandler>& valueOptions()
{
    static const std::map<std::string, ValueHandler> options = {
        {"--n", [](CommandLine& c, const std::string& o, const std::string& v) {
             c.params.coding.n = parseOrThrow(parseInteger(v), o, v);
         }},
        {"--g", [](CommandLine& c, const std::string&, const std::string& v) { c.params.coding.generator = v; }},
        {"--channel", [](CommandLine& c, const std::string&, const std::string& v) { c.params.channel.kind = channelKind(v); }},
        {"--p", [](CommandLine& c, const std::string& o, const std::string& v) {
             c.params.channel.bsc.bitErrorProbability = realValue(o, v);
         }},
        {"--p-gb", [](CommandLine& c, const std::string& o, const std::string& v) {
             c.params.channel.gilbert.goodToBad = realValue(o, v);
         }},
        {"--p-bg", [](CommandLine& c, const std::string& o, const std::string& v) {
             c.params.channel.gilbert.badToGood = realValue(o, v);
         }},
        {"--p-bad", [](CommandLine& c, const std::string& o, const std::string& v) {
             c.params.channel.gilbert.errorInBad = realValue(o, v);
         }},
        {"--p-good", [](CommandLine& c, const std::string& o, const std::string& v) {
             c.params.channel.gilbert.errorInGood = realValue(o, v);
         }},
        {"--blocks", [](CommandLine& c, const std::string& o, const std::string& v) {
             c.params.control.blockLimit = countValue(o, v);
         }},
        {"--errors", [](CommandLine& c, const std::string& o, const std::string& v) {
             c.params.control.stopMode = crc::StopMode::ErrorCount;
             c.params.control.errorLimit = countValue(o, v);
         }},
        {"--seed", [](CommandLine& c, const std::string& o, const std::string& v) { c.params.control.seed = countValue(o, v); }},
        {"--csv", [](CommandLine& c, const std::string&, const std::string& v) { c.csvPath = v; }},
    };
    return options;
}

const std::map<std::string, FlagHandler>& flagOptions()
{
    static const std::map<std::string, FlagHandler> options = {
        {"--csv-stdout", [](CommandLine& c) { c.csvOutput = true; }},
        {"--quiet", [](CommandLine& c) { c.quiet = true; }},
        {"--list-codes", [](CommandLine& c) { c.listCodes = true; }},
        {"--help", [](CommandLine& c) { c.help = true; }},
        {"-h", [](CommandLine& c) { c.help = true; }},
    };
    return options;
}

}

CommandLine parseCommandLine(const std::vector<std::string>& arguments)
{
    CommandLine commandLine;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string& option = arguments[index];
        if (const auto flag = flagOptions().find(option); flag != flagOptions().end()) {
            flag->second(commandLine);
            continue;
        }
        const auto handler = valueOptions().find(option);
        if (handler == valueOptions().end())
            throw std::invalid_argument("неизвестный параметр «" + option + "» (см. --help)");
        if (index + 1 >= arguments.size())
            throw std::invalid_argument("после " + option + " нужно указать значение");
        handler->second(commandLine, option, arguments[++index]);
    }
    return commandLine;
}

std::string usage()
{
    return "Использование:\n"
           "  crcsim                      интерактивный режим\n"
           "  crcsim [параметры]          пакетный режим\n"
           "\n"
           "Кодирование:\n"
           "  --n ЧИСЛО                   длина кодового слова, 3..64 (по умолчанию 15)\n"
           "  --g МНОГОЧЛЕН               g(x): 10011 или [4;1;0] (по умолчанию 10011)\n"
           "Канал связи:\n"
           "  --channel bsc|gilbert       тип канала (по умолчанию bsc)\n"
           "  --p ЧИСЛО                   ДСК: вероятность ошибки на бит, (0; 0.5] (0.005)\n"
           "  --p-gb ЧИСЛО                Гилберт: P_ХП, (0; 1) (0.001)\n"
           "  --p-bg ЧИСЛО                Гилберт: P_ПХ, (0; 1) (0.1)\n"
           "  --p-bad ЧИСЛО               Гилберт: p_П, (0; 0.5] (0.5)\n"
           "  --p-good ЧИСЛО              Гилберт: p_Х, [0; 0.5] (0)\n"
           "Управление моделированием:\n"
           "  --blocks ЧИСЛО              число блоков N, 1..10^9 (1000000);\n"
           "                              вместе с --errors — предельное число блоков\n"
           "  --errors ЧИСЛО              остановка после заданного числа ошибочных блоков\n"
           "  --seed ЧИСЛО                начальное значение генератора (0 — случайное)\n"
           "Вывод:\n"
           "  --csv ФАЙЛ                  дописать строку результата в файл CSV\n"
           "  --csv-stdout                вывести на экран только CSV (заголовок и строку)\n"
           "  --quiet                     не показывать ход моделирования\n"
           "  --list-codes                вывести таблицу образующих многочленов (n;k;t;g)\n"
           "  --help                      эта справка\n";
}

}
