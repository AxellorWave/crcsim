#pragma once

#include <QRectF>
#include <QString>
#include <QWidget>

class QPainter;

class ModelDiagram : public QWidget {
public:
    explicit ModelDiagram(QWidget* parent = nullptr);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Stage {
        QString title;
        QString note;
    };

    void drawStage(QPainter& painter, const QRectF& box, const Stage& stage) const;
    void drawArrow(QPainter& painter, const QRectF& from, const QRectF& to, const QString& caption) const;
};
