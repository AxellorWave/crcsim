#include "plan.h"

#include "codes.h"
#include "json.h"
#include "results.h"
#include "simulator.h"
#include "text.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace crc {

namespace {

enum class Column { Length, Generator, Channel, BitError, GoodToBad, BadToGood, ErrorInBad, ErrorInGood, Blocks, Errors,
    Seed, Label };

struct ColumnInfo {
    Column column;
    std::string key;
};

struct RawField {
    Column column;
    std::string value;
    int line;
};

struct RawExperiment {
    int line = 0;
    std::vector<RawField> fields;
};

struct NumberedLine {
    int number;
    std::string text;
};

const std::vector<double> methodichkaBitErrors = {0.05, 0.01, 0.005};
const std::vector<double> methodichkaGoodToBad = {0.003, 0.001, 0.0005};
const GilbertSettings methodichkaGilbert{0.001, 0.1, 0.5, 0.0};
constexpr int maxParsedLength = 1000;
constexpr int exportDigits = 12;

const std::vector<ColumnInfo>& columns()
{
    static const std::vector<ColumnInfo> list = {
        {Column::Length, "n"},
        {Column::Generator, "g"},
        {Column::Channel, "channel"},
        {Column::BitError, "p"},
        {Column::GoodToBad, "p_gb"},
        {Column::BadToGood, "p_bg"},
        {Column::ErrorInBad, "p_bad"},
        {Column::ErrorInGood, "p_good"},
        {Column::Blocks, "blocks"},
        {Column::Errors, "errors"},
        {Column::Seed, "seed"},
        {Column::Label, "label"},
    };
    return list;
}

const std::map<std::string, Column>& aliases()
{
    static const std::map<std::string, Column> table = {
        {"n", Column::Length},
        {"длина", Column::Length},
        {"g", Column::Generator},
        {"g(x)", Column::Generator},
        {"многочлен", Column::Generator},
        {"channel", Column::Channel},
        {"канал", Column::Channel},
        {"p", Column::BitError},
        {"p_gb", Column::GoodToBad},
        {"p_хп", Column::GoodToBad},
        {"p_bg", Column::BadToGood},
        {"p_пх", Column::BadToGood},
        {"p_bad", Column::ErrorInBad},
        {"p_п", Column::ErrorInBad},
        {"p_good", Column::ErrorInGood},
        {"p_х", Column::ErrorInGood},
        {"blocks", Column::Blocks},
        {"блоки", Column::Blocks},
        {"блоков", Column::Blocks},
        {"errors", Column::Errors},
        {"ошибки", Column::Errors},
        {"ошибок", Column::Errors},
        {"seed", Column::Seed},
        {"label", Column::Label},
        {"метка", Column::Label},
        {"название", Column::Label},
    };
    return table;
}

std::string keyOf(Column column)
{
    for (const ColumnInfo& info : columns()) {
        if (info.column == column)
            return info.key;
    }
    return {};
}

std::string allowedKeys()
{
    std::string text;
    for (const ColumnInfo& info : columns())
        text += (text.empty() ? "" : ", ") + info.key;
    return text;
}

std::optional<Column> findColumn(const std::string& name)
{
    const auto found = aliases().find(lowercase(trimSpaces(name)));
    if (found == aliases().end())
        return std::nullopt;
    return found->second;
}

[[noreturn]] void failAt(int line, const std::string& message)
{
    throw std::invalid_argument("строка " + std::to_string(line) + ": " + message);
}

std::string quotedValue(const std::string& value)
{
    return "«" + value + "»";
}

double realField(const RawField& field, bool allowDecimalComma)
{
    const std::optional<double> value = parseRealText(field.value, allowDecimalComma);
    if (!value)
        failAt(field.line, keyOf(field.column) + " — ожидалось число, получено " + quotedValue(field.value));
    return *value;
}

std::uint64_t countField(const RawField& field)
{
    const std::optional<std::uint64_t> value = parseCountText(field.value);
    if (!value)
        failAt(field.line, keyOf(field.column) + " — ожидалось целое неотрицательное число, получено " + quotedValue(field.value));
    return *value;
}

ChannelKind channelField(const RawField& field)
{
    const std::string name = lowercase(trimSpaces(field.value));
    if (name == "bsc" || name == "дск")
        return ChannelKind::Bsc;
    if (name == "gilbert" || name == "гилберт")
        return ChannelKind::Gilbert;
    failAt(field.line, "канал должен быть bsc, дск, gilbert или гилберт, получено " + quotedValue(field.value));
}

void applyField(SimulationParams& params, const RawField& field, bool allowDecimalComma)
{
    switch (field.column) {
    case Column::Length:
        params.coding.n = static_cast<int>(std::min<std::uint64_t>(countField(field), maxParsedLength));
        return;
    case Column::Generator:
        params.coding.generator = trimSpaces(field.value);
        return;
    case Column::Channel:
        params.channel.kind = channelField(field);
        return;
    case Column::BitError:
        params.channel.bsc.bitErrorProbability = realField(field, allowDecimalComma);
        return;
    case Column::GoodToBad:
        params.channel.gilbert.goodToBad = realField(field, allowDecimalComma);
        return;
    case Column::BadToGood:
        params.channel.gilbert.badToGood = realField(field, allowDecimalComma);
        return;
    case Column::ErrorInBad:
        params.channel.gilbert.errorInBad = realField(field, allowDecimalComma);
        return;
    case Column::ErrorInGood:
        params.channel.gilbert.errorInGood = realField(field, allowDecimalComma);
        return;
    case Column::Blocks:
        params.control.blockLimit = countField(field);
        return;
    case Column::Errors:
        params.control.stopMode = StopMode::ErrorCount;
        params.control.errorLimit = countField(field);
        return;
    case Column::Seed:
        params.control.seed = countField(field);
        return;
    case Column::Label:
        return;
    }
}

PlannedExperiment buildExperiment(const RawExperiment& raw, bool allowDecimalComma)
{
    PlannedExperiment experiment;
    for (const RawField& field : raw.fields) {
        if (field.column == Column::Label)
            experiment.label = trimSpaces(field.value);
        else if (!trimSpaces(field.value).empty())
            applyField(experiment.params, field, allowDecimalComma);
    }
    const std::vector<ValidationError> errors = validate(experiment.params);
    if (!errors.empty())
        failAt(raw.line, joinMessages(errors));
    return experiment;
}

ExperimentPlan buildPlan(const std::vector<RawExperiment>& rawPlan, bool allowDecimalComma)
{
    if (rawPlan.empty())
        throw std::invalid_argument("в плане нет ни одного опыта");
    ExperimentPlan plan;
    for (const RawExperiment& raw : rawPlan)
        plan.push_back(buildExperiment(raw, allowDecimalComma));
    return plan;
}

std::vector<NumberedLine> splitLines(const std::string& text)
{
    std::vector<NumberedLine> lines;
    std::string current;
    int number = 1;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const char symbol = text[index];
        if (symbol != '\n' && symbol != '\r') {
            current += symbol;
            continue;
        }
        if (symbol == '\r' && index + 1 < text.size() && text[index + 1] == '\n')
            ++index;
        lines.push_back({number++, current});
        current.clear();
    }
    lines.push_back({number, current});
    return lines;
}

char detectSeparator(const std::string& header)
{
    if (header.find(';') != std::string::npos)
        return ';';
    if (header.find('\t') != std::string::npos)
        return '\t';
    return ',';
}

std::vector<std::string> splitFields(const NumberedLine& line, char separator)
{
    std::vector<std::string> fields(1);
    bool quoted = false;
    const std::string& text = line.text;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const char symbol = text[index];
        if (quoted && symbol == '"' && index + 1 < text.size() && text[index + 1] == '"') {
            fields.back() += '"';
            ++index;
        } else if (symbol == '"') {
            quoted = !quoted;
        } else if (symbol == separator && !quoted) {
            fields.emplace_back();
        } else {
            fields.back() += symbol;
        }
    }
    if (quoted)
        failAt(line.number, "кавычка не закрыта");
    return fields;
}

bool blank(const std::vector<std::string>& fields)
{
    return std::all_of(fields.begin(), fields.end(), [](const std::string& field) { return trimSpaces(field).empty(); });
}

std::vector<std::optional<Column>> headerColumns(const NumberedLine& line, const std::vector<std::string>& names)
{
    std::vector<std::optional<Column>> header;
    for (const std::string& name : names) {
        if (trimSpaces(name).empty()) {
            header.push_back(std::nullopt);
            continue;
        }
        const std::optional<Column> column = findColumn(name);
        if (!column)
            failAt(line.number, "неизвестный столбец " + quotedValue(trimSpaces(name)) + " (допустимые: " + allowedKeys() + ")");
        if (std::find(header.begin(), header.end(), column) != header.end())
            failAt(line.number, "столбец " + quotedValue(trimSpaces(name)) + " указан дважды");
        header.push_back(column);
    }
    return header;
}

RawExperiment csvExperiment(
    const NumberedLine& line, const std::vector<std::string>& fields, const std::vector<std::optional<Column>>& header)
{
    if (fields.size() != header.size()) {
        failAt(line.number, "ожидалось значений: " + std::to_string(header.size()) + ", получено: "
                + std::to_string(fields.size()));
    }
    RawExperiment raw;
    raw.line = line.number;
    for (std::size_t index = 0; index < fields.size(); ++index) {
        if (header[index])
            raw.fields.push_back({*header[index], fields[index], line.number});
        else if (!trimSpaces(fields[index]).empty())
            failAt(line.number, "значение в столбце без названия");
    }
    return raw;
}

std::string fieldText(const std::string& key, const JsonValue& value)
{
    switch (value.kind) {
    case JsonValue::Kind::Number:
    case JsonValue::Kind::String:
        return value.text;
    case JsonValue::Kind::Null:
        return {};
    default:
        failAt(value.line, "значение поля " + quotedValue(key) + " должно быть числом или строкой");
    }
}

RawExperiment jsonExperiment(const JsonValue& object, std::size_t number)
{
    if (object.kind != JsonValue::Kind::Object)
        failAt(object.line, "опыт " + std::to_string(number) + " должен быть объектом JSON");
    RawExperiment raw;
    raw.line = object.line;
    std::vector<Column> seen;
    for (const auto& [key, value] : object.members) {
        const std::optional<Column> column = findColumn(key);
        if (!column)
            failAt(value.line, "неизвестное поле " + quotedValue(key) + " (допустимые: " + allowedKeys() + ")");
        if (std::find(seen.begin(), seen.end(), *column) != seen.end())
            failAt(value.line, "поле " + quotedValue(key) + " указано дважды");
        seen.push_back(*column);
        raw.fields.push_back({*column, fieldText(key, value), value.line});
    }
    return raw;
}

const JsonValue& experimentList(const JsonValue& root)
{
    if (root.kind == JsonValue::Kind::Array)
        return root;
    const JsonValue* list = root.kind == JsonValue::Kind::Object ? root.find("experiments") : nullptr;
    if (list == nullptr || list->kind != JsonValue::Kind::Array)
        failAt(root.line, "ожидался массив опытов или объект с полем «experiments» — массивом опытов");
    return *list;
}

std::string number(double value)
{
    return formatNumber(value, exportDigits);
}

std::string csvField(const std::string& value)
{
    if (value.find_first_of(";\"\n") == std::string::npos)
        return value;
    std::string escaped;
    for (const char symbol : value)
        escaped += symbol == '"' ? std::string("\"\"") : std::string(1, symbol);
    return "\"" + escaped + "\"";
}

std::vector<std::string> exportValues(const PlannedExperiment& experiment)
{
    const SimulationParams& params = experiment.params;
    const bool bsc = params.channel.kind == ChannelKind::Bsc;
    const GilbertSettings& gilbert = params.channel.gilbert;
    const bool countsErrors = params.control.stopMode == StopMode::ErrorCount;
    return {std::to_string(params.coding.n), params.coding.generator, bsc ? "bsc" : "gilbert",
        bsc ? number(params.channel.bsc.bitErrorProbability) : "", bsc ? "" : number(gilbert.goodToBad),
        bsc ? "" : number(gilbert.badToGood), bsc ? "" : number(gilbert.errorInBad), bsc ? "" : number(gilbert.errorInGood),
        std::to_string(params.control.blockLimit), countsErrors ? std::to_string(params.control.errorLimit) : "",
        std::to_string(params.control.seed), experiment.label};
}

bool numericColumn(Column column)
{
    return column != Column::Generator && column != Column::Channel && column != Column::Label;
}

std::string jsonObject(const PlannedExperiment& experiment)
{
    const std::vector<std::string> values = exportValues(experiment);
    std::string object;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (values[index].empty())
            continue;
        const Column column = columns()[index].column;
        object += object.empty() ? "{" : ", ";
        object += quoteJson(columns()[index].key) + ": " + (numericColumn(column) ? values[index] : quoteJson(values[index]));
    }
    return object + "}";
}

PlannedExperiment methodichkaExperiment(const KnownCode& code, const ChannelParams& channel, const std::string& level)
{
    PlannedExperiment experiment;
    experiment.params.coding = CodingParams{code.n, code.binary};
    experiment.params.channel = channel;
    experiment.params.control.blockLimit = methodichkaBlocks;
    experiment.params.control.seed = methodichkaSeed;
    experiment.label = "(" + std::to_string(code.n) + ", " + std::to_string(code.k) + "), " + level;
    return experiment;
}

void appendMethodichkaRuns(ExperimentPlan& plan, const KnownCode& code, bool includeGilbert)
{
    for (const double bitError : methodichkaBitErrors) {
        ChannelParams channel;
        channel.bsc.bitErrorProbability = bitError;
        plan.push_back(methodichkaExperiment(code, channel, "ДСК p = " + formatNumber(bitError, 4)));
    }
    if (!includeGilbert)
        return;
    for (const double goodToBad : methodichkaGoodToBad) {
        ChannelParams channel;
        channel.kind = ChannelKind::Gilbert;
        channel.gilbert = methodichkaGilbert;
        channel.gilbert.goodToBad = goodToBad;
        plan.push_back(methodichkaExperiment(code, channel, "Гилберт P_ХП = " + formatNumber(goodToBad, 4)));
    }
}

bool endsWithJson(const std::string& path)
{
    const std::string extension = ".json";
    return path.size() >= extension.size() && lowercase(path.substr(path.size() - extension.size())) == extension;
}

bool looksLikeJson(const std::string& text)
{
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    return first != std::string::npos && (text[first] == '{' || text[first] == '[');
}

}

ExperimentPlan parsePlanCsv(const std::string& text)
{
    const std::vector<NumberedLine> lines = splitLines(withoutByteOrderMark(text));
    std::vector<RawExperiment> rawPlan;
    std::vector<std::optional<Column>> header;
    char separator = ';';
    bool headerRead = false;
    for (const NumberedLine& line : lines) {
        if (trimSpaces(line.text).empty())
            continue;
        if (!headerRead) {
            separator = detectSeparator(line.text);
            header = headerColumns(line, splitFields(line, separator));
            headerRead = true;
            continue;
        }
        const std::vector<std::string> fields = splitFields(line, separator);
        if (!blank(fields))
            rawPlan.push_back(csvExperiment(line, fields, header));
    }
    if (!headerRead)
        throw std::invalid_argument("файл плана пуст: нужна строка заголовка, например n;g;channel;p;blocks;seed");
    return buildPlan(rawPlan, separator != ',');
}

ExperimentPlan parsePlanJson(const std::string& text)
{
    const JsonValue root = parseJson(withoutByteOrderMark(text));
    const JsonValue& list = experimentList(root);
    std::vector<RawExperiment> rawPlan;
    for (std::size_t index = 0; index < list.items.size(); ++index)
        rawPlan.push_back(jsonExperiment(list.items[index], index + 1));
    return buildPlan(rawPlan, true);
}

ExperimentPlan parsePlan(const std::string& text)
{
    const std::string content = withoutByteOrderMark(text);
    return looksLikeJson(content) ? parsePlanJson(content) : parsePlanCsv(content);
}

ExperimentPlan loadPlan(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("не удалось открыть файл " + path);
    std::ostringstream content;
    content << input.rdbuf();
    return parsePlan(content.str());
}

ExperimentPlan methodichkaPlan(bool includeGilbert)
{
    ExperimentPlan plan;
    for (const KnownCode& code : knownCodes())
        appendMethodichkaRuns(plan, code, includeGilbert);
    return plan;
}

std::string planToCsv(const ExperimentPlan& plan)
{
    std::string text;
    for (const ColumnInfo& info : columns())
        text += (text.empty() ? "" : ";") + info.key;
    text += '\n';
    for (const PlannedExperiment& experiment : plan) {
        const std::vector<std::string> values = exportValues(experiment);
        for (std::size_t index = 0; index < values.size(); ++index)
            text += (index == 0 ? "" : ";") + csvField(values[index]);
        text += '\n';
    }
    return text;
}

std::string planToJson(const ExperimentPlan& plan)
{
    std::string text = "{\n  \"experiments\": [\n";
    for (std::size_t index = 0; index < plan.size(); ++index)
        text += "    " + jsonObject(plan[index]) + (index + 1 < plan.size() ? ",\n" : "\n");
    return text + "  ]\n}\n";
}

void savePlan(const std::string& path, const ExperimentPlan& plan)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output)
        throw std::runtime_error("не удалось открыть файл " + path + " для записи");
    output << (endsWithJson(path) ? planToJson(plan) : planToCsv(plan));
    if (!output)
        throw std::runtime_error("не удалось записать файл " + path);
}

std::string channelParametersText(const ChannelParams& channel)
{
    if (channel.kind == ChannelKind::Bsc)
        return "p = " + formatNumber(channel.bsc.bitErrorProbability, 4);
    const GilbertSettings& gilbert = channel.gilbert;
    return "P_ХП = " + formatNumber(gilbert.goodToBad, 4) + ", P_ПХ = " + formatNumber(gilbert.badToGood, 4)
        + ", p_П = " + formatNumber(gilbert.errorInBad, 4) + ", p_Х = " + formatNumber(gilbert.errorInGood, 4);
}

std::string codeName(const CyclicCode& code)
{
    return "(" + std::to_string(code.n) + ", " + std::to_string(code.dataBitCount()) + ")";
}

}
