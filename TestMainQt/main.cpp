#include "TestMainQt.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TestMainQt w;
    w.show();
    return a.exec();
}
