#include "PCH.h"
#include "Interface.h"
#include <unordered_map>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <memory>
#include <fstream>
#include "../CharacterControlCore/CharacterControl.h"
#include "../CharacterControlUI/CharacterControlUI.h"

namespace
{
	inline std::size_t hash_combine(std::size_t seed, std::size_t rhs)
	{
		seed ^= rhs + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
}

CharacterControlCore::ControlCorePtr CharacterControlCore::GetCharacterControl(std::string controlFile)
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
