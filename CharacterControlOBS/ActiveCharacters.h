#pragma once

#include "Character.h"
#include <unordered_map>

namespace CharacterControlOBS
{
	std::optional<std::filesystem::path> GetGlobalSettingsPath();

	class ActiveCharacters
	{
	public:
		ActiveCharacters();

		static ActiveCharacters& Get();

		void MakeActiveCharacter(std::string name, Character&& character);

		void RemoveActiveCharacter(std::string name);

		Character* GetActiveCharacter(std::string name);
	private:
		static ActiveCharacters s_singleton;

		std::filesystem::path m_path;

		std::unordered_map<std::string, Character> m_activeCharacters;
	};
}