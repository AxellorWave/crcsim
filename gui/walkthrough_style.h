#pragma once

#include "theme.h"

#include <QColor>
#include <QFont>
#include <QRectF>

class QPainter;

namespace walkstyle {

using CellLook = CellColors;

CellLook dataLook();
CellLook checkLook();
CellLook placeholderLook();
CellLook plainLook();
CellLook divisorLook();
CellLook dimLook();
CellLook errorLook();
CellLook syndromeLook();

QColor mix(const QColor& from, const QColor& to, double amount);
CellLook mix(const CellLook& from, const CellLook& to, double amount);

double clamp01(double value);
double easeOut(double value);
double easeInOut(double value);
double stagger(double progress, int index, int count, double share = 0.55);

QFont bitFont(double cellSize);
QFont labelFont(double pointSize = 13.0, bool bold = false);

void paintBitCell(QPainter& painter, const QRectF& rect, bool bit, const CellLook& look, double opacity = 1.0,
    double scale = 1.0);

}
