#pragma once

#include "walkthrough_style.h"

#include "poly.h"

#include <QWidget>

#include <functional>

class BitCellsEdit : public QWidget {
public:
    using LookProvider = std::function<walkstyle::CellLook(int column, bool bit)>;

    explicit BitCellsEdit(QWidget* parent = nullptr);

    void setBits(crc::Word bits, int length);
    crc::Word bits() const;
    void setCellPitch(double pitch);
    void setLookProvider(LookProvider provider);
    void setOnToggled(std::function<void(crc::Word)> handler);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    int columnAt(const QPoint& point) const;
    QRectF cellRect(int column) const;

    crc::Word bits_ = 0;
    int length_ = 0;
    int hovered_ = -1;
    double pitch_ = 30.0;
    LookProvider lookProvider_;
    std::function<void(crc::Word)> onToggled_;
};
