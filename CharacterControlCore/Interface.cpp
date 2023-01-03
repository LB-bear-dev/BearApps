#include "PCH.h"
#include "Interface.h"
#include "../CharacterControlCore/CharacterControl.h"

CharacterControlCore::ControlCorePtr CharacterControlCore::GetCharacterControl( const char* controlFile )
{
	static std::weak_ptr<ControlCore> alreadyCreatedControl;
	if ( alreadyCreatedControl.expired() )
	{
		CharacterControlCore::ControlCorePtr ptr = std::make_shared<CharacterControl>( controlFile );
		alreadyCreatedControl = ptr;
		return ptr;
	}
	
	return alreadyCreatedControl.lock();
}
