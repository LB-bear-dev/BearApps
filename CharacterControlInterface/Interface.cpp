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

std::size_t CharacterControlCore::GetImageInfoHash(const std::string& characterID, const std::string& imageName)
{
	std::hash<std::string> strHash;
	std::size_t imageToken = strHash(imageName);
	imageToken = hash_combine(imageToken, strHash(characterID));

	return imageToken;
}

CharacterControlCore::ControlCorePtr CharacterControlCore::GetCharacterControl(std::string controlFile)
{
	return std::make_shared<CharacterControl>(controlFile);
}

void CharacterControlCore::InitControlUI(ControlCorePtr controlPtr)
{
	if (controlPtr != nullptr)
	{
		StartUp();
	}
}
