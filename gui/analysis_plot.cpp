#include "analysis_plot.h"

#include "probability_format.h"

#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>

#include <algorithm>
#include <cmath>

namespace {

const QColor theoryColor(0x2a, 0x78, 0xd6);
const QColor experimentColor(0xeb, 0x68, 0x34);
const QColor textColor(0x52, 0x51, 0x4e);
const QColor gridColor(0xe4, 0xe3, 0xdf);

const double lowestBitErrorProbability = 1e-4;
const double highestBitErrorProbability = 0.5;
const double requiredUndetectedProbability = 1e-6;
const int curveSamples = 240;
const int maxVerticalLabels = 9;
const double requirementHeadroom = -5.0;
const double minimumDecades = 3.0;
const double pointRadius = 4.5;
const double triangleSize = 6.0;
const double topMargin = 16.0;
const double rightMargin = 22.0;
const double bottomMargin = 50.0;
const double legendPadding = 8.0;
const double legendSample = 26.0;

double log10Of(double value)
{
    return std::log10(value);
}

QPolygonF downTriangle(const QPointF& center, double size)
{
    return QPolygonF({center + QPointF(-size, -size * 0.6), center + QPointF(size, -size * 0.6),
        center + QPointF(0, size * 0.9)});
}

double sampledProbability(int sample)
{
    const double low = log10Of(lowestBitErrorProbability);
    const double high = log10Of(highestBitErrorProbability);
    return std::pow(10.0, low + (high - low) * sample / curveSamples);
}

QFont scaledFont(const QFont& base, double factor, bool bold = false)
{
    QFont font = base;
    font.setPointSizeF(base.pointSizeF() * factor);
    font.setBold(bold);
    return font;
}

}

AnalysisPlot::AnalysisPlot(QWidget* parent)
    : QWidget(parent)
    , horizontal_{log10Of(lowestBitErrorProbability), log10Of(highestBitErrorProbability)}
    , vertical_{-8.0, 0.0}
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QSize AnalysisPlot::minimumSizeHint() const
{
    return {420, 300};
}

QSize AnalysisPlot::sizeHint() const
{
    return {640, 440};
}

void AnalysisPlot::setSpectrum(const std::optional<crc::WeightSpectrum>& spectrum)
{
    spectrum_ = spectrum;
    updateVerticalRange();
    update();
}

void AnalysisPlot::setPoints(const std::vector<ExperimentPoint>& points)
{
    points_.clear();
    for (const ExperimentPoint& point : points) {
        const bool inRange = point.bitErrorProbability >= lowestBitErrorProbability
            && point.bitErrorProbability <= highestBitErrorProbability && point.undetectedProbability > 0.0;
        if (inRange)
            points_.push_back(point);
    }
    updateVerticalRange();
    update();
}

void AnalysisPlot::setMarkedProbability(std::optional<double> bitErrorProbability)
{
    markedProbability_ = bitErrorProbability;
    update();
}

void AnalysisPlot::setMessage(const QString& message)
{
    message_ = message;
    update();
}

void AnalysisPlot::updateVerticalRange()
{
    double low = log10Of(requiredUndetectedProbability);
    double high = low;
    if (spectrum_) {
        for (int sample = 0; sample <= curveSamples; ++sample) {
            const double value = crc::undetectedProbability(*spectrum_, sampledProbability(sample));
            if (value > 0.0) {
                low = std::min(low, log10Of(value));
                high = std::max(high, log10Of(value));
            }
        }
    }
    for (const ExperimentPoint& point : points_) {
        low = std::min(low, log10Of(point.undetectedProbability));
        high = std::max(high, log10Of(point.undetectedProbability));
    }
    vertical_ = {std::floor(low), std::min(0.0, std::max(std::ceil(high), requirementHeadroom))};
    if (vertical_.high - vertical_.low < minimumDecades)
        vertical_.low = vertical_.high - minimumDecades;
    const int step = verticalStep();
    const int steps = static_cast<int>(std::ceil((vertical_.high - vertical_.low) / step));
    vertical_.low = vertical_.high - steps * step;
}

int AnalysisPlot::verticalStep() const
{
    const int decades = static_cast<int>(vertical_.high - vertical_.low);
    return std::max(1, (decades + maxVerticalLabels - 1) / maxVerticalLabels);
}

QPointF AnalysisPlot::toScreen(double p, double probability) const
{
    const double x = (log10Of(p) - horizontal_.low) / (horizontal_.high - horizontal_.low);
    const double y = (log10Of(probability) - vertical_.low) / (vertical_.high - vertical_.low);
    return {area_.left() + x * area_.width(), area_.bottom() - y * area_.height()};
}

void AnalysisPlot::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::white);

    const QFontMetricsF metrics(font());
    const double leftMargin = metrics.horizontalAdvance(powerOfTen(-88)) + metrics.height() + 22.0;
    area_ = QRectF(leftMargin, topMargin, width() - leftMargin - rightMargin, height() - topMargin - bottomMargin);

    if (!spectrum_) {
        drawMessage(painter);
        return;
    }
    drawGrid(painter);
    drawAxes(painter);
    painter.save();
    painter.setClipRect(area_.adjusted(-triangleSize, -triangleSize, triangleSize, triangleSize));
    drawRequirement(painter);
    drawCurve(painter);
    drawMarkedProbability(painter);
    drawPoints(painter);
    painter.restore();
    drawLegend(painter);
}

void AnalysisPlot::drawGrid(QPainter& painter) const
{
    painter.setPen(QPen(gridColor, 1.0));
    for (int exponent = static_cast<int>(horizontal_.low); exponent <= 0; ++exponent) {
        for (int factor = 1; factor < 10; ++factor) {
            const double p = factor * std::pow(10.0, exponent);
            if (p < lowestBitErrorProbability || p > highestBitErrorProbability)
                continue;
            const double x = toScreen(p, 1.0).x();
            painter.setPen(QPen(gridColor, factor == 1 ? 1.2 : 0.6));
            painter.drawLine(QPointF(x, area_.top()), QPointF(x, area_.bottom()));
        }
    }
    const int step = verticalStep();
    painter.setPen(QPen(gridColor, 1.2));
    for (int exponent = static_cast<int>(vertical_.high); exponent >= static_cast<int>(vertical_.low); exponent -= step) {
        const double y = toScreen(1.0, std::pow(10.0, exponent)).y();
        painter.drawLine(QPointF(area_.left(), y), QPointF(area_.right(), y));
    }
}

void AnalysisPlot::drawAxes(QPainter& painter) const
{
    painter.setPen(QPen(textColor, 1.2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(area_);

    const QFontMetricsF metrics(font());
    painter.setFont(font());
    for (int exponent = static_cast<int>(horizontal_.low); exponent < 0; ++exponent) {
        const double x = toScreen(std::pow(10.0, exponent), 1.0).x();
        painter.drawText(QRectF(x - 40, area_.bottom() + 4, 80, metrics.height()), Qt::AlignCenter, powerOfTen(exponent));
    }
    const double halfX = toScreen(highestBitErrorProbability, 1.0).x();
    painter.drawText(QRectF(halfX - 40, area_.bottom() + 4, 80, metrics.height()), Qt::AlignCenter, QStringLiteral("0,5"));

    const int step = verticalStep();
    for (int exponent = static_cast<int>(vertical_.high); exponent >= static_cast<int>(vertical_.low); exponent -= step) {
        const double y = toScreen(1.0, std::pow(10.0, exponent)).y();
        painter.drawText(QRectF(0, y - metrics.height() / 2, area_.left() - 6, metrics.height()),
            Qt::AlignRight | Qt::AlignVCenter, powerOfTen(exponent));
    }

    painter.setFont(scaledFont(font(), 1.1, true));
    painter.drawText(QRectF(area_.left(), area_.bottom() + metrics.height() + 6, area_.width(), metrics.height() + 6),
        Qt::AlignCenter, QStringLiteral("p"));
    painter.save();
    painter.translate(metrics.height() * 0.4, area_.center().y());
    painter.rotate(-90);
    painter.drawText(QRectF(-area_.height() / 2, -2, area_.height(), metrics.height() + 6), Qt::AlignCenter,
        QStringLiteral("Pлож"));
    painter.restore();
    painter.setFont(font());
}

void AnalysisPlot::drawRequirement(QPainter& painter) const
{
    const double y = toScreen(1.0, requiredUndetectedProbability).y();
    QPen pen(textColor, 1.2, Qt::DashLine);
    painter.setPen(pen);
    painter.drawLine(QPointF(area_.left(), y), QPointF(area_.right(), y));
    const QFontMetricsF metrics(font());
    const bool roomAbove = y - area_.top() > metrics.height() + 4;
    const double top = roomAbove ? y - metrics.height() - 2 : y + 2;
    painter.drawText(QRectF(area_.right() - 120, top, 114, metrics.height()), Qt::AlignRight | Qt::AlignVCenter,
        powerOfTen(-6));
}

void AnalysisPlot::drawCurve(QPainter& painter) const
{
    QPainterPath path;
    for (int sample = 0; sample <= curveSamples; ++sample) {
        const double p = sampledProbability(sample);
        const QPointF point = toScreen(p, crc::undetectedProbability(*spectrum_, p));
        if (sample == 0)
            path.moveTo(point);
        else
            path.lineTo(point);
    }
    painter.setPen(QPen(theoryColor, 2.4));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void AnalysisPlot::drawMarkedProbability(QPainter& painter) const
{
    if (!markedProbability_ || *markedProbability_ < lowestBitErrorProbability)
        return;
    const QPointF point = toScreen(*markedProbability_, crc::undetectedProbability(*spectrum_, *markedProbability_));
    painter.setPen(QPen(theoryColor, 1.0, Qt::DotLine));
    painter.drawLine(QPointF(point.x(), area_.top()), QPointF(point.x(), area_.bottom()));
    painter.setPen(QPen(theoryColor, 2.0));
    painter.setBrush(Qt::white);
    painter.drawEllipse(point, pointRadius, pointRadius);
}

void AnalysisPlot::drawPoints(QPainter& painter) const
{
    for (const ExperimentPoint& point : points_) {
        const QPointF center = toScreen(point.bitErrorProbability, point.undetectedProbability);
        if (point.observed) {
            painter.setPen(QPen(Qt::white, 1.0));
            painter.setBrush(experimentColor);
            painter.drawEllipse(center, pointRadius, pointRadius);
        } else {
            painter.setPen(QPen(experimentColor, 1.8));
            painter.setBrush(Qt::white);
            painter.drawPolygon(downTriangle(center, triangleSize));
        }
    }
}

void AnalysisPlot::drawLegend(QPainter& painter) const
{
    const bool anyObserved = std::any_of(points_.begin(), points_.end(), [](const ExperimentPoint& p) { return p.observed; });
    const bool anyUnobserved = std::any_of(points_.begin(), points_.end(), [](const ExperimentPoint& p) { return !p.observed; });
    std::vector<QString> labels = {QStringLiteral("теория, ДСК")};
    if (anyObserved)
        labels.push_back(QStringLiteral("эксперимент"));
    if (anyUnobserved)
        labels.push_back(QStringLiteral("Nлож = 0: оценка 3/N"));

    const QFontMetricsF metrics(font());
    double textWidth = 0.0;
    for (const QString& label : labels)
        textWidth = std::max(textWidth, metrics.horizontalAdvance(label));
    const double rowHeight = metrics.height() + 4;
    const QSizeF size(legendPadding * 3 + legendSample + textWidth, legendPadding * 2 + rowHeight * labels.size());
    const QRectF box(area_.bottomRight() - QPointF(size.width() + 10, size.height() + 10), size);
    painter.setPen(QPen(gridColor, 1.0));
    painter.setBrush(Qt::white);
    painter.drawRect(box);

    double y = box.top() + legendPadding + rowHeight / 2;
    const double sampleLeft = box.left() + legendPadding;
    const QPointF sampleCenter(sampleLeft + legendSample / 2, 0);
    painter.setPen(QPen(theoryColor, 2.4));
    painter.drawLine(QPointF(sampleLeft, y), QPointF(sampleLeft + legendSample, y));
    if (anyObserved) {
        y += rowHeight;
        painter.setPen(QPen(Qt::white, 1.0));
        painter.setBrush(experimentColor);
        painter.drawEllipse(sampleCenter + QPointF(0, y), pointRadius, pointRadius);
    }
    if (anyUnobserved) {
        y += rowHeight;
        painter.setPen(QPen(experimentColor, 1.8));
        painter.setBrush(Qt::white);
        painter.drawPolygon(downTriangle(sampleCenter + QPointF(0, y), triangleSize));
    }

    painter.setPen(textColor);
    const double textLeft = sampleLeft + legendSample + legendPadding;
    for (std::size_t row = 0; row < labels.size(); ++row) {
        const double top = box.top() + legendPadding + rowHeight * row;
        painter.drawText(QRectF(textLeft, top, textWidth + 2, rowHeight), Qt::AlignLeft | Qt::AlignVCenter, labels[row]);
    }
}

void AnalysisPlot::drawMessage(QPainter& painter) const
{
    painter.setPen(textColor);
    painter.setFont(scaledFont(font(), 1.05));
    painter.drawText(QRectF(rect()).adjusted(40, 40, -40, -40), Qt::AlignCenter | Qt::TextWordWrap, message_);
    painter.setFont(font());
}
