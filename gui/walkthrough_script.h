#pragma once

#include "walkthrough.h"

#include <QString>

#include <vector>

enum class WalkStage { Source, Encoder, Channel, Decoder, Result };

enum class WalkScene { Encoding, Channel, Decoding, Verdict };

enum class WalkAction {
    ShowData,
    ShiftZeros,
    PlaceDivisor,
    WriteDifference,
    MarkRemainder,
    FormCodeword,
    ShowErrors,
    ApplyErrors,
    ShowReceived,
    MarkSyndrome,
    ShowVerdict,
};

struct WalkStep {
    WalkStage stage;
    WalkAction action;
    int division;
    QString caption;
    int durationMs;
};

WalkScene sceneOf(WalkStage stage);

std::vector<WalkStep> buildScript(const crc::Walkthrough& trace);

QString superscript(int value);
QString bitText(crc::Word value, int length);
QString bitCountText(int count);
QString polynomialText(crc::Word polynomial);
int countOnes(crc::Word value);

QString verdictTitle(crc::Verdict verdict);
QString verdictExplanation(const crc::Walkthrough& trace);
