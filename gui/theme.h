#pragma once

#include <QColor>
#include <QString>

#include <functional>

class QWidget;

struct CellColors {
    QColor fill;
    QColor border;
    QColor text;
};

struct Theme {
    bool dark = false;

    QColor surface;
    QColor surfaceAlt;
    QColor border;
    QColor grid;

    QColor text;
    QColor secondaryText;
    QColor mutedText;

    QColor accent;
    QColor accentText;
    QColor accentSoft;
    QColor accentSoftBorder;
    QColor accentSoftText;
    QColor accentStrong;

    QColor orange;
    QColor orangeText;
    QColor success;
    QColor successText;
    QColor danger;
    QColor dangerText;
    QColor dangerSoft;
    QColor warningText;

    QColor highlightRow;
    QColor highlightRowText;
    QColor badgeText;
    QColor caption;

    QColor buttonFace;
    QColor buttonBorder;
    QColor buttonText;
    QColor disabledFace;
    QColor disabledBorder;
    QColor disabledText;

    CellColors dataCell;
    CellColors checkCell;
    CellColors placeholderCell;
    CellColors plainCell;
    CellColors divisorCell;
    CellColors dimCell;
    CellColors errorCell;
    CellColors syndromeCell;
};

const Theme& lightTheme();
const Theme& darkTheme();
bool paletteIsDark();
const Theme& currentTheme();

QString cssColor(const QColor& color);
QString textColorStyle(const QColor& color);

bool isThemeChange(int eventType);
void keepThemed(QWidget* widget, std::function<void()> apply);
void keepTextColor(QWidget* widget, QColor Theme::*role);
