#pragma once

#include <QColor>
#include <QFont>
#include <QRectF>

class QPainter;

namespace walkstyle {

inline const QColor ink(0x14, 0x21, 0x3d);
inline const QColor secondary(0x4a, 0x55, 0x68);
inline const QColor muted(0x8a, 0x93, 0xa3);
inline const QColor grid(0xe4, 0xe3, 0xdf);
inline const QColor paper(0xff, 0xff, 0xff);
inline const QColor dataColor(0x2a, 0x78, 0xd6);
inline const QColor checkColor(0xeb, 0x68, 0x34);
inline const QColor errorColor(0xd0, 0x34, 0x2c);
inline const QColor successColor(0x2e, 0x9e, 0x5b);

struct CellLook {
    QColor fill;
    QColor border;
    QColor text;
};

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
