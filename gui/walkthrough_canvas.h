#pragma once

#include "walkthrough_script.h"
#include "walkthrough_style.h"

#include "walkthrough.h"

#include <QWidget>

#include <functional>
#include <vector>

class QPainter;

class WalkthroughCanvas : public QWidget {
public:
    explicit WalkthroughCanvas(QWidget* parent = nullptr);

    void setScript(const crc::Walkthrough& trace, const std::vector<WalkStep>& steps);
    void setPosition(int step, double progress);
    int contentHeight(int width) const;
    int minimumContentWidth() const;
    QRectF focusArea() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Layout {
        double pitch;
        double cell;
        double gridLeft;
        double top;
        double rowPitch;
    };

    using LookOf = std::function<walkstyle::CellLook(int column, bool bit)>;

    Layout layoutFor(int width) const;
    Layout sceneLayout(int width) const;
    WalkScene scene() const;
    WalkScene sceneAt(int step) const;
    int find(WalkScene scene, WalkAction action, int division) const;
    double reveal(int step) const;
    double reveal(WalkScene scene, WalkAction action, int division = -1) const;
    double sceneIntro() const;
    double sceneRows(WalkScene scene) const;
    int remainderRow(const crc::Division& division) const;
    bool bitAt(crc::Word value, int column) const;
    walkstyle::CellLook codewordLook(int column) const;
    walkstyle::CellLook receivedLook(int column) const;

    double rowTop(const Layout& layout, double row) const;
    QRectF cellRect(const Layout& layout, double row, int column) const;
    QRectF spanRect(const Layout& layout, double firstRow, double lastRow, int firstColumn, int lastColumn) const;
    void drawLabel(QPainter& painter, const Layout& layout, double row, const QString& text, double opacity) const;
    void drawHeader(QPainter& painter, const Layout& layout) const;
    void drawRow(QPainter& painter, const Layout& layout, double row, crc::Word value, int firstColumn,
        const LookOf& look, double opacity) const;
    void drawRule(QPainter& painter, const Layout& layout, double row, int firstColumn, double extent) const;
    void drawBand(QPainter& painter, const QRectF& area, const QColor& color, double strength) const;

    void drawEncoding(QPainter& painter, const Layout& layout) const;
    void drawShiftedData(QPainter& painter, const Layout& layout) const;
    void drawCodewordFlight(QPainter& painter, const Layout& layout) const;
    void drawDivision(QPainter& painter, const Layout& layout, WalkScene scene, const crc::Division& division,
        const walkstyle::CellLook& remainderLook, WalkAction markAction) const;
    void drawDivisionFocus(QPainter& painter, const Layout& layout, WalkScene scene, const crc::Division& division,
        WalkAction markAction) const;
    void drawChannel(QPainter& painter, const Layout& layout) const;
    void drawFlashingErrors(QPainter& painter, const Layout& layout, double row, double flash) const;
    void drawDecoding(QPainter& painter, const Layout& layout) const;
    void drawVerdict(QPainter& painter, const Layout& layout) const;
    void drawVerdictCard(QPainter& painter, const QRectF& card, double appear) const;
    QRectF verdictCardRect(int width) const;

    crc::Walkthrough trace_;
    std::vector<WalkStep> steps_;
    int step_ = 0;
    double progress_ = 0.0;
};
