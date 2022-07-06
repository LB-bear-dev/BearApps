#pragma once
#include "WatchedJson.h"
#include "Character.h"
namespace CharacterControlCore
{
	class CharacterCreator
	{
	public:
		CharacterCreator(std::string& filename);

		void Update();

		Character GetCharacter(std::string id);

		const std::filesystem::path& GetPath();

	private:
		WatchedJson m_rootFile;
	};
}