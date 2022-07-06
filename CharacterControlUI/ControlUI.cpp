#include "PCH.h"
#include "ControlUI.h"
#include "CharacterListViewModel.h"
#include <qpointer.h>
#include "../CharacterControlCore/CharacterControl.h"

QPointer<CharacterListViewModel> characterListViewModel;
ControlUI::ControlUI(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    characterListViewModel = new CharacterListViewModel;
    ui.listView->setModel(characterListViewModel);
}
