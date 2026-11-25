#include "model_diagram.h"

#include "theme.h"

#include <QPainter>
#include <QPolygonF>

#include <algorithm>
#include <array>

namespace {

const int stageCount = 5;
const double arrowHead = 7.0;
const double margin = 8.0;

}

ModelDiagram::ModelDiagram(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip(QStringLiteral("Схема модели: источник выдаёт k информационных бит, кодер дописывает n − k "
                              "проверочных, канал искажает биты, декодер вычисляет синдром"));
}

QSize ModelDiagram::minimumSizeHint() const
{
    return {620, 120};
}

QSize ModelDiagram::sizeHint() const
{
    return {820, 120};
}

void ModelDiagram::paintEvent(QPaintEvent*)
{
    const std::array<Stage, stageCount> stages = {{
        {QStringLiteral("Источник"), QStringLiteral("случайные\nданные")},
        {QStringLiteral("Кодер"), QStringLiteral("+ (n − k)\nпроверочных")},
        {QStringLiteral("Канал"), QStringLiteral("ДСК или\nГилберта")},
        {QStringLiteral("Декодер"), QStringLiteral("синдром\nS(x) = 0 ?")},
        {QStringLiteral("Статистика"), QStringLiteral("Nв, Nобн,\nNлож")},
    }};
    const std::array<QString, stageCount - 1> captions = {
        QStringLiteral("k бит"), QStringLiteral("n бит"), QStringLiteral("n бит"), QStringLiteral("исход"),
    };

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().base());

    const double gap = std::max(46.0, width() * 0.07);
    const double boxWidth = (width() - 2 * margin - gap * (stageCount - 1)) / stageCount;
    const double boxHeight = height() - 2 * margin - 18.0;
    std::array<QRectF, stageCount> boxes;
    for (int index = 0; index < stageCount; ++index)
        boxes[index] = QRectF(margin + index * (boxWidth + gap), margin + 18.0, boxWidth, boxHeight);

    for (int index = 0; index < stageCount; ++index)
        drawStage(painter, boxes[index], stages[index]);
    for (int index = 0; index + 1 < stageCount; ++index)
        drawArrow(painter, boxes[index], boxes[index + 1], captions[index]);
}

void ModelDiagram::drawStage(QPainter& painter, const QRectF& box, const Stage& stage) const
{
    const Theme& theme = currentTheme();
    painter.setPen(QPen(theme.accentStrong, 1.5));
    painter.setBrush(theme.accentSoft);
    painter.drawRoundedRect(box, 6, 6);

    QFont titleFont = font();
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(theme.text);
    const QRectF titleArea(box.left(), box.top() + 4, box.width(), box.height() * 0.4);
    painter.drawText(titleArea, Qt::AlignCenter, stage.title);

    painter.setFont(font());
    painter.setPen(theme.secondaryText);
    const QRectF noteArea(box.left(), titleArea.bottom(), box.width(), box.bottom() - titleArea.bottom() - 4);
    painter.drawText(noteArea, Qt::AlignHCenter | Qt::AlignTop, stage.note);
}

void ModelDiagram::drawArrow(QPainter& painter, const QRectF& from, const QRectF& to, const QString& caption) const
{
    const double y = from.center().y();
    const QPointF start(from.right() + 2, y);
    const QPointF end(to.left() - 2, y);

    const Theme& theme = currentTheme();
    painter.setPen(QPen(theme.secondaryText, 1.5));
    painter.drawLine(start, end - QPointF(arrowHead, 0));
    painter.setBrush(theme.secondaryText);
    painter.drawPolygon(QPolygonF({end, end - QPointF(arrowHead, arrowHead / 2), end - QPointF(arrowHead, -arrowHead / 2)}));

    painter.setPen(theme.orangeText);
    const QRectF captionArea(start.x() - 10, y - 24, end.x() - start.x() + 20, 20);
    painter.drawText(captionArea, Qt::AlignHCenter | Qt::AlignBottom, caption);
}
