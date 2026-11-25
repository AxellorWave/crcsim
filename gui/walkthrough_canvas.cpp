#include "walkthrough_canvas.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

using namespace walkstyle;

namespace {

const double margin = 18.0;
const double labelWidth = 128.0;
const double labelGap = 14.0;
const double headerHeight = 30.0;
const double minPitch = 26.0;
const double maxPitch = 46.0;
const double codewordGap = 0.5;
const double cardGap = 22.0;
const double minCardWidth = 680.0;
const double stripeWidth = 6.0;
const double cardPadding = 18.0;
const double iconSize = 44.0;
const double dimmedOpacity = 0.42;
const double pi = 3.14159265358979323846;

QPointF lerp(const QPointF& from, const QPointF& to, double amount)
{
    return from + (to - from) * amount;
}

QColor withAlpha(QColor color, double alpha)
{
    color.setAlphaF(static_cast<float>(clamp01(alpha)));
    return color;
}

QColor verdictAccent(crc::Verdict verdict)
{
    switch (verdict) {
    case crc::Verdict::Correct:
        return successColor;
    case crc::Verdict::Detected:
        return dataColor;
    case crc::Verdict::Undetected:
        return errorColor;
    }
    return dataColor;
}

}

WalkthroughCanvas::WalkthroughCanvas(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void WalkthroughCanvas::setScript(const crc::Walkthrough& trace, const std::vector<WalkStep>& steps)
{
    trace_ = trace;
    steps_ = steps;
    step_ = 0;
    progress_ = 0.0;
    update();
}

void WalkthroughCanvas::setPosition(int step, double progress)
{
    step_ = std::clamp(step, 0, std::max(0, static_cast<int>(steps_.size()) - 1));
    progress_ = clamp01(progress);
    update();
}

WalkthroughCanvas::Layout WalkthroughCanvas::layoutFor(int width) const
{
    const int n = std::max(1, trace_.code.n);
    const double available = width - 2 * margin - labelWidth;
    const double pitch = std::clamp(available / n, minPitch, maxPitch);
    const double cell = std::round(pitch * 0.84);
    const double gridWidth = labelWidth + n * pitch;
    const double left = std::max(margin, (width - gridWidth) / 2);
    return {pitch, cell, left + labelWidth, margin, cell + std::max(9.0, pitch * 0.28)};
}

WalkthroughCanvas::Layout WalkthroughCanvas::sceneLayout(int width) const
{
    Layout layout = layoutFor(width);
    if (scene() == WalkScene::Verdict)
        layout.top = verdictCardRect(width).bottom() + cardGap;
    return layout;
}

int WalkthroughCanvas::minimumContentWidth() const
{
    return static_cast<int>(2 * margin + labelWidth + std::max(1, trace_.code.n) * minPitch);
}

WalkScene WalkthroughCanvas::sceneAt(int step) const
{
    if (steps_.empty())
        return WalkScene::Encoding;
    return sceneOf(steps_[static_cast<std::size_t>(step)].stage);
}

WalkScene WalkthroughCanvas::scene() const
{
    return sceneAt(step_);
}

int WalkthroughCanvas::find(WalkScene scene, WalkAction action, int division) const
{
    for (int index = 0; index < static_cast<int>(steps_.size()); ++index) {
        const WalkStep& step = steps_[static_cast<std::size_t>(index)];
        if (step.action == action && step.division == division && sceneOf(step.stage) == scene)
            return index;
    }
    return -1;
}

double WalkthroughCanvas::reveal(int step) const
{
    if (step < 0 || step > step_)
        return 0.0;
    return step < step_ ? 1.0 : progress_;
}

double WalkthroughCanvas::reveal(WalkScene scene, WalkAction action, int division) const
{
    return reveal(find(scene, action, division));
}

double WalkthroughCanvas::sceneIntro() const
{
    const bool firstOfScene = step_ == 0 || sceneAt(step_ - 1) != scene();
    return firstOfScene ? easeOut(progress_ / 0.4) : 1.0;
}

int WalkthroughCanvas::remainderRow(const crc::Division& division) const
{
    return 2 * static_cast<int>(division.steps.size());
}

double WalkthroughCanvas::sceneRows(WalkScene scene) const
{
    switch (scene) {
    case WalkScene::Encoding:
        return remainderRow(trace_.encoding) + 1 + codewordGap + 1;
    case WalkScene::Channel:
        return 3;
    case WalkScene::Decoding:
        return remainderRow(trace_.decoding) + 1;
    case WalkScene::Verdict:
        return 4;
    }
    return 1;
}

int WalkthroughCanvas::contentHeight(int width) const
{
    const Layout layout = sceneLayout(width);
    const double rows = sceneRows(scene()) * layout.rowPitch;
    return static_cast<int>(std::ceil(layout.top + headerHeight + rows + margin));
}

QRectF WalkthroughCanvas::focusArea() const
{
    const Layout layout = sceneLayout(width());
    if (steps_.empty())
        return {};
    const WalkStep& step = steps_[static_cast<std::size_t>(step_)];
    double firstRow = 0;
    double lastRow = 0;
    const WalkScene current = scene();
    const crc::Division& division = current == WalkScene::Decoding ? trace_.decoding : trace_.encoding;
    if (step.action == WalkAction::PlaceDivisor || step.action == WalkAction::WriteDifference) {
        firstRow = std::max(0, 2 * step.division);
        lastRow = 2 * step.division + 2;
    } else if (step.action == WalkAction::MarkRemainder || step.action == WalkAction::MarkSyndrome) {
        firstRow = lastRow = remainderRow(division);
    } else if (step.action == WalkAction::FormCodeword) {
        firstRow = lastRow = sceneRows(current) - 1;
    } else if (current == WalkScene::Channel || current == WalkScene::Verdict) {
        return QRectF(0, 0, width(), 1);
    }
    const double top = firstRow == 0 ? 0 : rowTop(layout, firstRow) - layout.rowPitch * 0.3;
    return QRectF(0, top, width(), rowTop(layout, lastRow + 1) + layout.rowPitch * 0.3 - top);
}

bool WalkthroughCanvas::bitAt(crc::Word value, int column) const
{
    return (value >> (trace_.code.n - 1 - column)) & 1;
}

CellLook WalkthroughCanvas::codewordLook(int column) const
{
    return column < trace_.code.dataBitCount() ? dataLook() : checkLook();
}

CellLook WalkthroughCanvas::receivedLook(int column) const
{
    return bitAt(trace_.errors, column) ? errorLook() : codewordLook(column);
}

double WalkthroughCanvas::rowTop(const Layout& layout, double row) const
{
    return layout.top + headerHeight + row * layout.rowPitch;
}

QRectF WalkthroughCanvas::cellRect(const Layout& layout, double row, int column) const
{
    const double inset = (layout.pitch - layout.cell) / 2;
    return {layout.gridLeft + column * layout.pitch + inset, rowTop(layout, row), layout.cell, layout.cell};
}

QRectF WalkthroughCanvas::spanRect(
    const Layout& layout, double firstRow, double lastRow, int firstColumn, int lastColumn) const
{
    const QRectF first = cellRect(layout, firstRow, firstColumn);
    const QRectF last = cellRect(layout, lastRow, lastColumn);
    const double pad = std::max(4.0, (layout.pitch - layout.cell) / 2 + 2);
    return QRectF(first.topLeft(), last.bottomRight()).adjusted(-pad, -pad, pad, pad);
}

void WalkthroughCanvas::drawLabel(
    QPainter& painter, const Layout& layout, double row, const QString& text, double opacity) const
{
    if (opacity <= 0.0)
        return;
    painter.save();
    painter.setOpacity(painter.opacity() * clamp01(opacity));
    painter.setFont(labelFont(13.0));
    painter.setPen(secondary);
    const QRectF area(layout.gridLeft - labelWidth, rowTop(layout, row), labelWidth - labelGap, layout.cell);
    painter.drawText(area, Qt::AlignRight | Qt::AlignVCenter, text);
    painter.restore();
}

void WalkthroughCanvas::drawHeader(QPainter& painter, const Layout& layout) const
{
    painter.save();
    painter.setFont(labelFont(13.0));
    painter.setPen(muted);
    const double top = layout.top;
    const QRectF labelArea(layout.gridLeft - labelWidth, top, labelWidth - labelGap, headerHeight - 6);
    painter.drawText(labelArea, Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("степень"));
    for (int column = 0; column < trace_.code.n; ++column) {
        const QRectF area(layout.gridLeft + column * layout.pitch, top, layout.pitch, headerHeight - 6);
        painter.drawText(area, Qt::AlignCenter, QString::number(trace_.code.n - 1 - column));
    }
    painter.setPen(QPen(grid, 1.0));
    const double lineY = top + headerHeight - 3;
    painter.drawLine(QPointF(layout.gridLeft, lineY), QPointF(layout.gridLeft + trace_.code.n * layout.pitch, lineY));
    painter.restore();
}

void WalkthroughCanvas::drawRow(QPainter& painter, const Layout& layout, double row, crc::Word value, int firstColumn,
    const LookOf& look, double opacity) const
{
    for (int column = firstColumn; column < trace_.code.n; ++column) {
        const bool bit = bitAt(value, column);
        paintBitCell(painter, cellRect(layout, row, column), bit, look(column, bit), opacity);
    }
}

void WalkthroughCanvas::drawRule(QPainter& painter, const Layout& layout, double row, int firstColumn, double extent) const
{
    if (extent <= 0.0)
        return;
    const double y = rowTop(layout, row) + layout.cell + (layout.rowPitch - layout.cell) / 2;
    const double left = layout.gridLeft + firstColumn * layout.pitch + 3;
    const double right = layout.gridLeft + trace_.code.n * layout.pitch - 3;
    painter.save();
    painter.setPen(QPen(secondary, 1.4, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(left, y), QPointF(left + (right - left) * clamp01(extent), y));
    painter.restore();
}

void WalkthroughCanvas::drawBand(QPainter& painter, const QRectF& area, const QColor& color, double strength) const
{
    if (strength <= 0.0)
        return;
    painter.save();
    painter.setPen(QPen(withAlpha(color, 0.35 * strength), 1.2));
    painter.setBrush(withAlpha(color, 0.09 * strength));
    painter.drawRoundedRect(area, 10, 10);
    painter.restore();
}

void WalkthroughCanvas::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.fillRect(rect(), paper);
    if (steps_.empty())
        return;
    const Layout layout = sceneLayout(width());
    drawHeader(painter, layout);
    switch (scene()) {
    case WalkScene::Encoding:
        drawEncoding(painter, layout);
        break;
    case WalkScene::Channel:
        drawChannel(painter, layout);
        break;
    case WalkScene::Decoding:
        drawDecoding(painter, layout);
        break;
    case WalkScene::Verdict:
        drawVerdict(painter, layout);
        break;
    }
}

void WalkthroughCanvas::drawEncoding(QPainter& painter, const Layout& layout) const
{
    const double formed = reveal(WalkScene::Encoding, WalkAction::FormCodeword);
    painter.save();
    painter.setOpacity(1.0 - 0.45 * easeInOut(formed));
    drawDivisionFocus(painter, layout, WalkScene::Encoding, trace_.encoding, WalkAction::MarkRemainder);
    drawShiftedData(painter, layout);
    drawDivision(painter, layout, WalkScene::Encoding, trace_.encoding, checkLook(), WalkAction::MarkRemainder);
    painter.restore();
    drawCodewordFlight(painter, layout);
}

void WalkthroughCanvas::drawShiftedData(QPainter& painter, const Layout& layout) const
{
    const int k = trace_.code.dataBitCount();
    const int h = trace_.code.checkBitCount();
    const double shown = reveal(WalkScene::Encoding, WalkAction::ShowData);
    const double shifted = reveal(WalkScene::Encoding, WalkAction::ShiftZeros);
    const double marked = trace_.encoding.steps.empty() ? reveal(WalkScene::Encoding, WalkAction::MarkRemainder) : 0.0;
    const double dimmed = reveal(WalkScene::Encoding, WalkAction::PlaceDivisor, 1);

    painter.save();
    painter.setOpacity(painter.opacity() * (1.0 - (1.0 - dimmedOpacity) * dimmed));
    drawLabel(painter, layout, 0, QStringLiteral("D"), easeOut(shown) * (1.0 - easeOut(shifted)));
    drawLabel(painter, layout, 0, QStringLiteral("D·x") + superscript(h), easeOut(shifted));
    for (int column = 0; column < k; ++column) {
        const double appear = easeOut(stagger(shown, column, k));
        paintBitCell(painter, cellRect(layout, 0, column), bitAt(trace_.shifted, column), dataLook(), appear,
            0.65 + 0.35 * appear);
    }
    for (int column = k; column < trace_.code.n; ++column) {
        const double appear = easeOut(stagger(shifted, column - k, h));
        const QRectF cell = cellRect(layout, 0, column).translated((1.0 - appear) * layout.pitch * 1.4, 0);
        paintBitCell(painter, cell, false, mix(placeholderLook(), checkLook(), marked), appear);
    }
    painter.restore();
}

void WalkthroughCanvas::drawDivisionFocus(QPainter& painter, const Layout& layout, WalkScene scene,
    const crc::Division& division, WalkAction markAction) const
{
    const int n = trace_.code.n;
    const int h = trace_.code.checkBitCount();
    const int count = static_cast<int>(division.steps.size());
    for (int index = 0; index < count; ++index) {
        const int column = n - 1 - division.steps[static_cast<std::size_t>(index)].position;
        const double placed = reveal(scene, WalkAction::PlaceDivisor, index);
        const double next = index + 1 < count ? reveal(scene, WalkAction::PlaceDivisor, index + 1)
                                              : reveal(scene, markAction);
        const double strength = easeOut(placed) * (1.0 - easeOut(next));
        if (strength <= 0.0)
            continue;
        const double sourceRow = 2.0 * index;
        drawBand(painter, spanRect(layout, sourceRow, sourceRow + 1, column, column + h), dataColor, strength);
        painter.save();
        painter.setPen(QPen(withAlpha(dataColor, strength), 2.4));
        painter.setBrush(Qt::NoBrush);
        const QRectF lead = cellRect(layout, sourceRow, column).adjusted(-3.5, -3.5, 3.5, 3.5);
        painter.drawRoundedRect(lead, lead.width() * 0.26, lead.width() * 0.26);
        painter.restore();
    }
}

void WalkthroughCanvas::drawDivision(QPainter& painter, const Layout& layout, WalkScene scene,
    const crc::Division& division, const CellLook& remainderLook, WalkAction markAction) const
{
    const int n = trace_.code.n;
    const int h = trace_.code.checkBitCount();
    const int count = static_cast<int>(division.steps.size());
    const double marked = reveal(scene, markAction);
    for (int index = 0; index < count; ++index) {
        const crc::DivisionStep& step = division.steps[static_cast<std::size_t>(index)];
        const int column = n - 1 - step.position;
        const double placed = easeOut(reveal(scene, WalkAction::PlaceDivisor, index));
        if (placed <= 0.0)
            break;
        const double written = reveal(scene, WalkAction::WriteDifference, index);
        const double divisorRow = 2.0 * index + 1;
        const double differenceRow = divisorRow + 1;
        const double divisorDim = reveal(scene, WalkAction::PlaceDivisor, index + 1);
        const double differenceDim = reveal(scene, WalkAction::PlaceDivisor, index + 2);

        painter.save();
        painter.setOpacity(painter.opacity() * (1.0 - (1.0 - dimmedOpacity) * divisorDim));
        drawLabel(painter, layout, divisorRow, QStringLiteral("⊕ g(x)"), placed);
        for (int offset = 0; offset <= h; ++offset) {
            const bool bit = (division.divisor >> (h - offset)) & 1;
            const QRectF cell = cellRect(layout, divisorRow, column + offset).translated(-(1.0 - placed) * layout.pitch * 2, 0);
            paintBitCell(painter, cell, bit, divisorLook(), placed);
        }
        drawRule(painter, layout, divisorRow, column, easeOut(written / 0.45));
        painter.restore();

        if (written <= 0.0)
            continue;
        const bool last = index + 1 == count;
        const crc::Word after = step.after;
        int lead = column;
        while (lead < n - h && !bitAt(after, lead))
            ++lead;
        painter.save();
        painter.setOpacity(painter.opacity() * (1.0 - (1.0 - dimmedOpacity) * differenceDim));
        if (last)
            drawLabel(painter, layout, differenceRow, scene == WalkScene::Encoding ? QStringLiteral("C") : QStringLiteral("S"),
                easeOut(marked));
        for (int cellColumn = column; cellColumn < n; ++cellColumn) {
            const double appear = easeOut(stagger(written, cellColumn - column, n - column, 0.5));
            const bool remainder = last && cellColumn >= n - h;
            CellLook look = cellColumn < lead ? dimLook() : plainLook();
            if (remainder)
                look = mix(look, remainderLook, easeInOut(marked));
            const double scale = remainder ? 1.0 + 0.1 * std::sin(pi * marked) : 1.0;
            paintBitCell(painter, cellRect(layout, differenceRow, cellColumn), bitAt(after, cellColumn), look, appear, scale);
        }
        painter.restore();
    }
}

void WalkthroughCanvas::drawCodewordFlight(QPainter& painter, const Layout& layout) const
{
    const double formed = reveal(WalkScene::Encoding, WalkAction::FormCodeword);
    if (formed <= 0.0)
        return;
    const int n = trace_.code.n;
    const int k = trace_.code.dataBitCount();
    const double sourceRemainderRow = remainderRow(trace_.encoding);
    const double targetRow = sceneRows(WalkScene::Encoding) - 1;
    drawLabel(painter, layout, targetRow, QStringLiteral("F"), easeOut(formed / 0.5));
    for (int column = 0; column < n; ++column) {
        const double sourceRow = column < k ? 0.0 : sourceRemainderRow;
        const double travel = easeInOut(stagger(formed, column, n, 0.6));
        const QRectF from = cellRect(layout, sourceRow, column);
        const QRectF to = cellRect(layout, targetRow, column);
        const QRectF cell(lerp(from.topLeft(), to.topLeft(), travel), from.size());
        paintBitCell(painter, cell, bitAt(trace_.codeword, column), codewordLook(column), std::min(1.0, travel * 4 + 0.2));
    }
}

void WalkthroughCanvas::drawChannel(QPainter& painter, const Layout& layout) const
{
    const int n = trace_.code.n;
    const double intro = sceneIntro();
    const double errorsShown = reveal(WalkScene::Channel, WalkAction::ShowErrors);
    const double applied = reveal(WalkScene::Channel, WalkAction::ApplyErrors);
    const double flash = clamp01((applied - 0.35) / 0.65);

    for (int column = 0; column < n; ++column) {
        if (bitAt(trace_.errors, column))
            drawBand(painter, spanRect(layout, 0, 2, column, column), errorColor, easeOut(errorsShown) * 0.9);
    }
    drawLabel(painter, layout, 0, QStringLiteral("F"), intro);
    drawRow(painter, layout, 0, trace_.codeword, 0, [this](int column, bool) { return codewordLook(column); }, intro);

    drawLabel(painter, layout, 1, QStringLiteral("⊕ E"), easeOut(errorsShown));
    for (int column = 0; column < n; ++column) {
        const double appear = easeOut(stagger(errorsShown, column, n));
        const bool bit = bitAt(trace_.errors, column);
        paintBitCell(painter, cellRect(layout, 1, column), bit, bit ? errorLook() : dimLook(), appear, 0.7 + 0.3 * appear);
    }
    drawRule(painter, layout, 1, 0, easeOut(applied / 0.3));

    drawLabel(painter, layout, 2, QStringLiteral("Y"), easeOut(applied / 0.3));
    for (int column = 0; column < n; ++column) {
        const double appear = easeOut(stagger(applied / 0.4, column, n));
        const bool corrupted = bitAt(trace_.errors, column);
        const CellLook look = corrupted ? mix(codewordLook(column), errorLook(), easeOut(flash)) : codewordLook(column);
        const double pulse = corrupted ? std::sin(pi * flash) : 0.0;
        paintBitCell(painter, cellRect(layout, 2, column), bitAt(trace_.received, column), look, appear, 1.0 + 0.14 * pulse);
    }
    drawFlashingErrors(painter, layout, 2, std::sin(pi * flash));
}

void WalkthroughCanvas::drawFlashingErrors(QPainter& painter, const Layout& layout, double row, double flash) const
{
    if (flash <= 0.0)
        return;
    painter.save();
    painter.setBrush(Qt::NoBrush);
    for (int column = 0; column < trace_.code.n; ++column) {
        if (!bitAt(trace_.errors, column))
            continue;
        const double grow = 4.0 + 6.0 * flash;
        const QRectF halo = cellRect(layout, row, column).adjusted(-grow, -grow, grow, grow);
        painter.setPen(QPen(withAlpha(errorColor, 0.55 * flash), 3.0));
        painter.drawRoundedRect(halo, halo.width() * 0.28, halo.width() * 0.28);
    }
    painter.restore();
}

void WalkthroughCanvas::drawDecoding(QPainter& painter, const Layout& layout) const
{
    const int n = trace_.code.n;
    const double shown = reveal(WalkScene::Decoding, WalkAction::ShowReceived);
    const double marked = trace_.decoding.steps.empty() ? reveal(WalkScene::Decoding, WalkAction::MarkSyndrome) : 0.0;
    const double dimmed = reveal(WalkScene::Decoding, WalkAction::PlaceDivisor, 1);
    const int h = trace_.code.checkBitCount();

    drawDivisionFocus(painter, layout, WalkScene::Decoding, trace_.decoding, WalkAction::MarkSyndrome);
    painter.save();
    painter.setOpacity(1.0 - (1.0 - dimmedOpacity) * dimmed);
    drawLabel(painter, layout, 0, QStringLiteral("Y"), easeOut(shown));
    for (int column = 0; column < n; ++column) {
        const double appear = easeOut(stagger(shown, column, n));
        CellLook look = receivedLook(column);
        if (column >= n - h)
            look = mix(look, syndromeLook(), easeInOut(marked));
        paintBitCell(painter, cellRect(layout, 0, column), bitAt(trace_.received, column), look, appear);
    }
    painter.restore();
    drawDivision(painter, layout, WalkScene::Decoding, trace_.decoding, syndromeLook(), WalkAction::MarkSyndrome);
}

QRectF WalkthroughCanvas::verdictCardRect(int width) const
{
    const Layout layout = layoutFor(width);
    const double gridWidth = labelWidth + trace_.code.n * layout.pitch;
    const double cardWidth = std::min(width - 2 * margin, std::max(gridWidth, minCardWidth));
    const double textWidth = cardWidth - 3 * cardPadding - iconSize - stripeWidth;
    const QFontMetricsF titleMetrics(labelFont(16.0, true));
    const QFontMetricsF bodyMetrics(labelFont(13.0));
    const QRectF bounds(0, 0, textWidth, 1000);
    const double titleHeight = titleMetrics.boundingRect(bounds, Qt::TextWordWrap, verdictTitle(trace_.verdict)).height();
    const double bodyHeight = bodyMetrics.boundingRect(bounds, Qt::TextWordWrap, verdictExplanation(trace_)).height();
    const double height = std::max(iconSize, titleHeight + 8 + bodyHeight) + 2 * cardPadding;
    return QRectF((width - cardWidth) / 2, margin, cardWidth, height);
}

void WalkthroughCanvas::drawVerdict(QPainter& painter, const Layout& layout) const
{
    const int n = trace_.code.n;
    const int h = trace_.code.checkBitCount();
    const double shown = reveal(WalkScene::Verdict, WalkAction::ShowVerdict);
    const double rows = easeOut((shown - 0.3) / 0.7);

    drawLabel(painter, layout, 0, QStringLiteral("F"), rows);
    drawRow(painter, layout, 0, trace_.codeword, 0, [this](int column, bool) { return codewordLook(column); }, rows);
    drawLabel(painter, layout, 1, QStringLiteral("E"), rows);
    drawRow(painter, layout, 1, trace_.errors, 0, [](int, bool bit) { return bit ? errorLook() : dimLook(); }, rows);
    drawLabel(painter, layout, 2, QStringLiteral("Y"), rows);
    drawRow(painter, layout, 2, trace_.received, 0, [this](int column, bool) { return receivedLook(column); }, rows);
    drawLabel(painter, layout, 3, QStringLiteral("S"), rows);
    for (int column = n - h; column < n; ++column)
        paintBitCell(painter, cellRect(layout, 3, column), bitAt(trace_.syndrome, column), syndromeLook(), rows);

    drawVerdictCard(painter, verdictCardRect(width()), easeOut(shown / 0.7));
}

void WalkthroughCanvas::drawVerdictCard(QPainter& painter, const QRectF& card, double appear) const
{
    if (appear <= 0.0)
        return;
    const QColor accent = verdictAccent(trace_.verdict);
    const QRectF box = card.translated(0, (1.0 - appear) * 18);
    painter.save();
    painter.setOpacity(appear);
    painter.setPen(QPen(withAlpha(accent, 0.55), 1.4));
    painter.setBrush(mix(paper, accent, 0.10));
    painter.drawRoundedRect(box, 12, 12);

    QPainterPath stripe;
    stripe.addRect(QRectF(box.left(), box.top(), stripeWidth, box.height()));
    painter.fillPath(stripe.intersected([&box] {
        QPainterPath clip;
        clip.addRoundedRect(box, 12, 12);
        return clip;
    }()), accent);

    const QRectF icon(box.left() + stripeWidth + cardPadding, box.top() + cardPadding, iconSize, iconSize);
    painter.setPen(Qt::NoPen);
    painter.setBrush(accent);
    painter.drawEllipse(icon);
    painter.setPen(QPen(paper, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const QPointF c = icon.center();
    const double s = iconSize / 44.0;
    if (trace_.verdict == crc::Verdict::Correct) {
        painter.drawPolyline(QPolygonF({c + QPointF(-10, 1) * s, c + QPointF(-3, 8) * s, c + QPointF(11, -7) * s}));
    } else if (trace_.verdict == crc::Verdict::Detected) {
        painter.drawLine(c + QPointF(0, -11) * s, c + QPointF(0, 3) * s);
        painter.drawPoint(c + QPointF(0, 11) * s);
    } else {
        painter.drawLine(c + QPointF(-8, -8) * s, c + QPointF(8, 8) * s);
        painter.drawLine(c + QPointF(8, -8) * s, c + QPointF(-8, 8) * s);
    }

    const double textLeft = icon.right() + cardPadding;
    const QRectF textArea(textLeft, box.top() + cardPadding, box.right() - cardPadding - textLeft, box.height() - 2 * cardPadding);
    painter.setFont(labelFont(16.0, true));
    painter.setPen(ink);
    QRectF titleBounds;
    painter.drawText(textArea, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, verdictTitle(trace_.verdict), &titleBounds);
    painter.setFont(labelFont(13.0));
    painter.setPen(secondary);
    const QRectF bodyArea(textArea.left(), titleBounds.bottom() + 8, textArea.width(), textArea.bottom() - titleBounds.bottom());
    painter.drawText(bodyArea, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, verdictExplanation(trace_));
    painter.restore();
}
