#pragma once

#include <QString>

enum class HelpTopic {
    Code,
    CodeLength,
    Generator,
    ChannelModel,
    BitErrorProbability,
    GoodToBad,
    BadToGood,
    ErrorInBad,
    ErrorInGood,
    StopMode,
    BlockLimit,
    ErrorLimit,
    Seed,
};

struct ParameterHelp {
    QString title;
    QString meaning;
    QString allowed;
    QString example;
    QString effect;
};

ParameterHelp parameterHelp(HelpTopic topic);

QString parameterHelpHtml(HelpTopic topic);

QString parameterHelpToolTip(HelpTopic topic);
