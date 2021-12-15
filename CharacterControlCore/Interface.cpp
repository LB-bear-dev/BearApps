#include "PCH.h"
#include "Interface.h"
#include "CharacterControl.h"


CharacterControlCore::ControlCorePtr CharacterControlCore::GetCharacterControl(std::string controlFile)
{
	return std::make_unique<CharacterControl>(controlFile);
}
