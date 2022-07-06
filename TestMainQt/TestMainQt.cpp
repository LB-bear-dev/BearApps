#include "TestMainQt.h"
#include <thread>
#include "../CharacterControlInterface/Interface.h"

TestMainQt::TestMainQt(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
	CharacterControlCore::ControlCorePtr characterControl;
	try
	{
		characterControl = CharacterControlCore::GetCharacterControl("C:\\Users\\chris\\OneDrive\\Documents\\OBSCharacterSettings\\globalSettings.json");
		CharacterControlCore::InitControlUI(characterControl);
	}
	catch (...)
	{

	}

	if (characterControl)
	{
		while (1)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(32));
			characterControl->Update();
		}
	}
}
