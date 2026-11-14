#pragma once

#include <QString>
#include <QWidget>

#include <vector>

struct FieldError {
    QWidget* widget = nullptr;
    QString message;
};

void addUnlessReported(std::vector<FieldError>& errors, const FieldError& error);

void highlightFields(const std::vector<QWidget*>& fields, const std::vector<FieldError>& errors);

QString joinFieldErrors(const std::vector<FieldError>& errors);
