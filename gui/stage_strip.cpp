#include "stage_strip.h"

#include "theme.h"
#include "walkthrough_style.h"

#include <QPainter>
#include <QPolygonF>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

const int stageCount = 5;
const double arrowHead = 7.0;
const double margin = 6.0;
const int moveMs = 380;

}

StageStrip::StageStrip(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    animation_.setDuration(moveMs);
    animation_.setEasingCurve(QEasingCurve::InOutCubic);
    connect(&animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        position_ = value.toDouble();
        update();
    });
}

QSize StageStrip::minimumSizeHint() const
{
    return {560, 64};
}

QSize StageStrip::sizeHint() const
{
    return {820, 64};
}

void StageStrip::setStage(int stage, bool animated)
{
    const double target = std::clamp(stage, 0, stageCount - 1);
    animation_.stop();
    if (!animated || std::abs(target - position_) < 1e-9) {
        position_ = target;
        update();
        return;
    }
    animation_.setStartValue(position_);
    animation_.setEndValue(target);
    animation_.start();
}

double StageStrip::activity(int index) const
{
    return std::max(0.0, 1.0 - std::abs(position_ - index));
}

void StageStrip::paintEvent(QPaintEvent*)
{
    const std::array<Stage, stageCount> stages = {{
        {QStringLiteral("Источник"), QStringLiteral("данные D")},
        {QStringLiteral("Кодер"), QStringLiteral("F = D·xʰ ⊕ C")},
        {QStringLiteral("Канал"), QStringLiteral("Y = F ⊕ E")},
        {QStringLiteral("Декодер"), QStringLiteral("S = Y mod g")},
        {QStringLiteral("Итог"), QStringLiteral("вердикт")},
    }};

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), currentTheme().surface);

    const double gap = std::max(30.0, width() * 0.04);
    const double boxWidth = (width() - 2 * margin - gap * (stageCount - 1)) / stageCount;
    const double boxHeight = height() - 2 * margin;
    std::array<QRectF, stageCount> boxes;
    for (int index = 0; index < stageCount; ++index)
        boxes[index] = QRectF(margin + index * (boxWidth + gap), margin, boxWidth, boxHeight);

    for (int index = 0; index + 1 < stageCount; ++index)
        drawArrow(painter, boxes[index], boxes[index + 1], std::clamp(position_ - index, 0.0, 1.0));
    for (int index = 0; index < stageCount; ++index)
        drawStage(painter, boxes[index], stages[index], index);
}

void StageStrip::drawStage(QPainter& painter, const QRectF& box, const Stage& stage, int index) const
{
    const Theme& theme = currentTheme();
    const double active = activity(index);
    const bool passed = position_ > index;
    const QColor restFill = passed ? theme.accentSoft : theme.surface;
    const QColor restBorder = passed ? theme.accentSoftBorder : theme.border;
    const QColor restTitle = passed ? theme.text : theme.secondaryText;

    painter.setPen(QPen(walkstyle::mix(restBorder, theme.accent, active), 1.5));
    painter.setBrush(walkstyle::mix(restFill, theme.accent, active));
    painter.drawRoundedRect(box.adjusted(0.75, 0.75, -0.75, -0.75), 8, 8);

    const QRectF titleArea(box.left(), box.top() + 3, box.width(), box.height() * 0.52);
    painter.setFont(walkstyle::labelFont(14.0, true));
    painter.setPen(walkstyle::mix(restTitle, theme.accentText, active));
    painter.drawText(titleArea, Qt::AlignHCenter | Qt::AlignBottom, stage.title);

    const QRectF noteArea(box.left(), titleArea.bottom(), box.width(), box.bottom() - titleArea.bottom() - 3);
    painter.setFont(walkstyle::labelFont(13.0));
    painter.setPen(walkstyle::mix(theme.mutedText, theme.accentSoftText, active));
    painter.drawText(noteArea, Qt::AlignHCenter | Qt::AlignTop, stage.note);
}

void StageStrip::drawArrow(QPainter& painter, const QRectF& from, const QRectF& to, double activity) const
{
    const Theme& theme = currentTheme();
    const double y = from.center().y();
    const QPointF start(from.right() + 4, y);
    const QPointF end(to.left() - 4, y);
    const QColor rest = theme.dark ? theme.mutedText.darker(130) : theme.border.darker(115);
    const QColor color = walkstyle::mix(rest, theme.accent, activity);
    painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(start, end - QPointF(arrowHead, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPolygon(QPolygonF({end, end - QPointF(arrowHead, arrowHead / 2), end - QPointF(arrowHead, -arrowHead / 2)}));
}
