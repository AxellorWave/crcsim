#include "bit_cells_edit.h"

#include <QMouseEvent>
#include <QPainter>

namespace {

const double cellGap = 4.0;
const int verticalPadding = 3;

}

BitCellsEdit::BitCellsEdit(QWidget* parent)
    : QWidget(parent)
    , lookProvider_([](int, bool) { return walkstyle::plainLook(); })
{
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void BitCellsEdit::setBits(crc::Word bits, int length)
{
    const bool resized = length != length_;
    bits_ = bits & crc::lowBitsMask(length);
    length_ = length;
    if (resized)
        updateGeometry();
    update();
}

void BitCellsEdit::setCellPitch(double pitch)
{
    pitch_ = pitch;
    updateGeometry();
    update();
}

crc::Word BitCellsEdit::bits() const
{
    return bits_;
}

void BitCellsEdit::setLookProvider(LookProvider provider)
{
    lookProvider_ = std::move(provider);
    update();
}

void BitCellsEdit::setOnToggled(std::function<void(crc::Word)> handler)
{
    onToggled_ = std::move(handler);
}

QSize BitCellsEdit::sizeHint() const
{
    return {static_cast<int>(length_ * pitch_ + 2), static_cast<int>(pitch_ - cellGap) + 2 * verticalPadding};
}

QSize BitCellsEdit::minimumSizeHint() const
{
    return sizeHint();
}

QRectF BitCellsEdit::cellRect(int column) const
{
    const double size = pitch_ - cellGap;
    return {1 + column * pitch_ + cellGap / 2, static_cast<double>(verticalPadding), size, size};
}

int BitCellsEdit::columnAt(const QPoint& point) const
{
    const int column = static_cast<int>((point.x() - 1) / pitch_);
    return column >= 0 && column < length_ ? column : -1;
}

void BitCellsEdit::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    for (int column = 0; column < length_; ++column) {
        const bool bit = (bits_ >> (length_ - 1 - column)) & 1;
        walkstyle::CellLook look = lookProvider_(column, bit);
        if (column == hovered_ && isEnabled())
            look.border = currentTheme().text;
        walkstyle::paintBitCell(painter, cellRect(column), bit, look, isEnabled() ? 1.0 : 0.5);
    }
}

void BitCellsEdit::mousePressEvent(QMouseEvent* event)
{
    const int column = columnAt(event->position().toPoint());
    if (event->button() != Qt::LeftButton || column < 0)
        return;
    bits_ ^= crc::Word{1} << (length_ - 1 - column);
    update();
    if (onToggled_)
        onToggled_(bits_);
}

void BitCellsEdit::mouseMoveEvent(QMouseEvent* event)
{
    const int column = columnAt(event->position().toPoint());
    if (column != hovered_) {
        hovered_ = column;
        update();
    }
}

void BitCellsEdit::leaveEvent(QEvent*)
{
    hovered_ = -1;
    update();
}
