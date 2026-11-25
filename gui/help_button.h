#pragma once

#include "parameter_help.h"

class QToolButton;
class QWidget;

QToolButton* createHelpButton(HelpTopic topic, QWidget* parent = nullptr);

QWidget* withHelpButton(QWidget* field, HelpTopic topic);

void showHelpPopup(HelpTopic topic, QWidget* anchor);
