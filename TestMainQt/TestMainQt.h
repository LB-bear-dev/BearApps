#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_TestMainQt.h"

class TestMainQt : public QMainWindow
{
    Q_OBJECT

public:
    TestMainQt(QWidget *parent = Q_NULLPTR);

private:
    Ui::TestMainQtClass ui;
};
