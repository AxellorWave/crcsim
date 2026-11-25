#pragma once

#include <QString>

struct UserGuideColors {
    QString text;
    QString base;
    QString rule;
};

QString userGuideStyleSheet(const UserGuideColors& colors);

QString userGuideHtml();
