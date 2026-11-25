#pragma once

#include "analysis.h"

#include <QRectF>
#include <QString>
#include <QWidget>

#include <optional>
#include <vector>

class QPainter;

struct ExperimentPoint {
    double bitErrorProbability = 0.0;
    double undetectedProbability = 0.0;
    bool observed = true;
};

class AnalysisPlot : public QWidget {
public:
    explicit AnalysisPlot(QWidget* parent = nullptr);

    void setSpectrum(const std::optional<crc::WeightSpectrum>& spectrum);
    void setPoints(const std::vector<ExperimentPoint>& points);
    void setMarkedProbability(std::optional<double> bitErrorProbability);
    void setMessage(const QString& message);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Range {
        double low = 0.0;
        double high = 0.0;
    };

    void updateVerticalRange();
    int verticalStep() const;
    QPointF toScreen(double p, double probability) const;
    void drawGrid(QPainter& painter) const;
    void drawAxes(QPainter& painter) const;
    void drawRequirement(QPainter& painter) const;
    void drawCurve(QPainter& painter) const;
    void drawMarkedProbability(QPainter& painter) const;
    void drawPoints(QPainter& painter) const;
    void drawLegend(QPainter& painter) const;
    void drawMessage(QPainter& painter) const;

    std::optional<crc::WeightSpectrum> spectrum_;
    std::vector<ExperimentPoint> points_;
    std::optional<double> markedProbability_;
    QString message_;
    Range horizontal_;
    Range vertical_;
    QRectF area_;
};
