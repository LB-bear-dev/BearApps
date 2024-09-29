#include "PCH.h"
#include "ActiveCharacters.h"
#include "Messaging.h"
#include <shlobj_core.h>

std::optional<std::filesystem::path> CharacterControlOBS::GetGlobalSettingsPath()
{
	try
	{
		char documentsFolder[MAX_PATH] = { 0 };
		HRESULT result = SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, documentsFolder);

		if (SUCCEEDED(result))
		{
			return std::make_optional(std::filesystem::path(documentsFolder) / "OBSCharacterSettings");
		}
		else
		{
			error_("Could not access personal documents folder. It is required to store your image data in a folder called 'OBSCharacterSettings' in your user folder.");
		}
	}
	catch (...)
	{
	}

	return {};
}


CharacterControlOBS::ActiveCharacters CharacterControlOBS::ActiveCharacters::s_singleton = CharacterControlOBS::ActiveCharacters();

CharacterControlOBS::ActiveCharacters::ActiveCharacters(): m_path(GetGlobalSettingsPath().value_or(""))
{
}

CharacterControlOBS::ActiveCharacters& CharacterControlOBS::ActiveCharacters::Get()
{
	return s_singleton;
}

void CharacterControlOBS::ActiveCharacters::MakeActiveCharacter(std::string name, Character&& character)
{
	m_activeCharacters.emplace(name, std::move(character));
}

void CharacterControlOBS::ActiveCharacters::RemoveActiveCharacter(std::string name)
{
	m_activeCharacters.erase(name);
}

CharacterControlOBS::Character* CharacterControlOBS::ActiveCharacters::GetActiveCharacter(std::string name)
{
	auto foundIt = m_activeCharacters.find(name);

	if (foundIt != m_activeCharacters.end())
	{
		return &foundIt->second;
	}

	return nullptr;
}
