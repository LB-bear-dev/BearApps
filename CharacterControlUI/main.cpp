
#include "PCH.h"
#include "ControlUI.h"
#include "CharacterControlUI.h"
#include <QtWidgets/QApplication>
#include <qpointer.h>

QPointer<ControlUI> w;
int StartUp()
{
    w = new ControlUI();
    w->show();
    return 0;
}
