#include "walkthrough_style.h"

#include <QFontDatabase>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace walkstyle {

CellLook dataLook()
{
    return {QColor(0xe6, 0xf0, 0xfb), dataColor, ink};
}

CellLook checkLook()
{
    return {QColor(0xfd, 0xeb, 0xe2), checkColor, ink};
}

CellLook placeholderLook()
{
    return {paper, QColor(0xf4, 0xb8, 0x9c), secondary};
}

CellLook plainLook()
{
    return {QColor(0xfa, 0xf9, 0xf7), QColor(0xcf, 0xcd, 0xc7), ink};
}

CellLook divisorLook()
{
    return {QColor(0xec, 0xef, 0xf5), QColor(0x7d, 0x88, 0x9c), secondary};
}

CellLook dimLook()
{
    return {paper, QColor(0xee, 0xed, 0xea), QColor(0xc4, 0xc7, 0xcd)};
}

CellLook errorLook()
{
    return {errorColor, QColor(0xa8, 0x24, 0x1d), paper};
}

CellLook syndromeLook()
{
    return {ink, ink, paper};
}

QColor mix(const QColor& from, const QColor& to, double amount)
{
    const double t = clamp01(amount);
    return QColor::fromRgbF(static_cast<float>(from.redF() + (to.redF() - from.redF()) * t),
        static_cast<float>(from.greenF() + (to.greenF() - from.greenF()) * t),
        static_cast<float>(from.blueF() + (to.blueF() - from.blueF()) * t),
        static_cast<float>(from.alphaF() + (to.alphaF() - from.alphaF()) * t));
}

CellLook mix(const CellLook& from, const CellLook& to, double amount)
{
    return {mix(from.fill, to.fill, amount), mix(from.border, to.border, amount), mix(from.text, to.text, amount)};
}

double clamp01(double value)
{
    return std::clamp(value, 0.0, 1.0);
}

double easeOut(double value)
{
    const double t = 1.0 - clamp01(value);
    return 1.0 - t * t * t;
}

double easeInOut(double value)
{
    const double t = clamp01(value);
    return t < 0.5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;
}

double stagger(double progress, int index, int count, double share)
{
    if (count <= 1)
        return clamp01(progress);
    const double start = (1.0 - share) * index / (count - 1);
    return clamp01((progress - start) / share);
}

QFont bitFont(double cellSize)
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSizeF(std::max(13.0, cellSize * 0.42));
    font.setBold(true);
    return font;
}

QFont labelFont(double pointSize, bool bold)
{
    QFont font;
    font.setPointSizeF(pointSize);
    font.setBold(bold);
    return font;
}

void paintBitCell(QPainter& painter, const QRectF& rect, bool bit, const CellLook& look, double opacity, double scale)
{
    if (opacity <= 0.0)
        return;
    const QPointF center = rect.center();
    const QRectF box(center.x() - rect.width() * scale / 2, center.y() - rect.height() * scale / 2,
        rect.width() * scale, rect.height() * scale);
    const double radius = box.width() * 0.22;
    painter.save();
    painter.setOpacity(painter.opacity() * clamp01(opacity));
    painter.setPen(QPen(look.border, 1.4));
    painter.setBrush(look.fill);
    painter.drawRoundedRect(box.adjusted(0.7, 0.7, -0.7, -0.7), radius, radius);
    painter.setPen(look.text);
    painter.setFont(bitFont(rect.width()));
    painter.drawText(box, Qt::AlignCenter, bit ? QStringLiteral("1") : QStringLiteral("0"));
    painter.restore();
}

}
