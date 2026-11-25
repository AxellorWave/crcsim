#include "media_icons.h"

#include "walkthrough_style.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>

namespace {

const int iconSide = 16;
const int pixelRatio = 3;

void drawTriangle(QPainter& painter, double left, double right, bool pointsRight)
{
    const double tip = pointsRight ? right : left;
    const double base = pointsRight ? left : right;
    painter.drawPolygon(QPolygonF({QPointF(base, 3), QPointF(tip, 8), QPointF(base, 13)}));
}

void drawRestart(QPainter& painter, const QColor& color)
{
    painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(QRectF(2.5, 2.5, 11, 11), 100 * 16, 290 * 16);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPolygon(QPolygonF({QPointF(5.2, 0.6), QPointF(9.6, 3.0), QPointF(5.4, 5.8)}));
}

void paintGlyph(QPainter& painter, MediaGlyph glyph, const QColor& color)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    switch (glyph) {
    case MediaGlyph::Restart:
        drawRestart(painter, color);
        break;
    case MediaGlyph::Back:
        painter.drawRoundedRect(QRectF(2.5, 3, 2.2, 10), 0.8, 0.8);
        drawTriangle(painter, 5.5, 13.5, false);
        break;
    case MediaGlyph::Play:
        drawTriangle(painter, 4, 13.5, true);
        break;
    case MediaGlyph::Pause:
        painter.drawRoundedRect(QRectF(4, 3, 2.8, 10), 0.8, 0.8);
        painter.drawRoundedRect(QRectF(9.2, 3, 2.8, 10), 0.8, 0.8);
        break;
    case MediaGlyph::Step:
        drawTriangle(painter, 2.5, 10.5, true);
        painter.drawRoundedRect(QRectF(11.3, 3, 2.2, 10), 0.8, 0.8);
        break;
    }
}

QPixmap glyphPixmap(MediaGlyph glyph, const QColor& color)
{
    QPixmap pixmap(iconSide * pixelRatio, iconSide * pixelRatio);
    pixmap.setDevicePixelRatio(pixelRatio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    paintGlyph(painter, glyph, color);
    return pixmap;
}

}

QIcon mediaIcon(MediaGlyph glyph)
{
    const QColor activeColor = glyph == MediaGlyph::Play ? walkstyle::dataColor : walkstyle::ink;
    QIcon icon;
    icon.addPixmap(glyphPixmap(glyph, activeColor), QIcon::Normal);
    icon.addPixmap(glyphPixmap(glyph, walkstyle::muted.lighter(125)), QIcon::Disabled);
    return icon;
}
