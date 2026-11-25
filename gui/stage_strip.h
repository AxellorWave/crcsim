#pragma once

#include <QString>
#include <QVariantAnimation>
#include <QWidget>

class QPainter;

class StageStrip : public QWidget {
public:
    explicit StageStrip(QWidget* parent = nullptr);

    void setStage(int stage, bool animated);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Stage {
        QString title;
        QString note;
    };

    void drawStage(QPainter& painter, const QRectF& box, const Stage& stage, int index) const;
    void drawArrow(QPainter& painter, const QRectF& from, const QRectF& to, double activity) const;
    double activity(int index) const;

    QVariantAnimation animation_;
    double position_ = 0.0;
};
