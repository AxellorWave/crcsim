#pragma once

#include "selection.h"

#include <QTableWidget>

#include <vector>

class SelectionTable : public QTableWidget {
public:
    explicit SelectionTable(QWidget* parent = nullptr);

    void showRows(const std::vector<crc::SelectionRow>& rows, const crc::BestCodes& best);

private:
    QTableWidgetItem* setCell(int row, int column, const QString& text, const QString& toolTip = QString());
    void fillRow(int row, const crc::SelectionRow& selection);
    void paintVerdict(int row, crc::Verdict verdict);
    void markBest(int row, bool byRedundancy, bool byTime);
};
