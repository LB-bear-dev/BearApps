#pragma once

#include <QtWidgets/qwidget.h>
#include "ui_ControlUI.h"

namespace CharacterControlCore
{
    class CharacterControl;
}

class ControlUI : public QWidget
{
    Q_OBJECT

public:
    ControlUI(QWidget *parent = Q_NULLPTR);

private:
    Ui::ControlUIClass ui;
};

