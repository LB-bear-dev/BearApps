#pragma once
#include "../CharacterControlInterface/Interface.h"
#include "Component.h"
#include "CharacterCreator.h"
#include "CharacterMap.h"
namespace CharacterControlCore
{
	class CharacterMapElementInstanceMutable;
	using ComponentList = std::vector<std::unique_ptr<InputComponent>>;

	class CharacterControl : public ControlCore
	{
	public:
		CharacterControl(std::string& filename);
		void Update();

		const std::filesystem::path& GetContentRoot();
		const CharacterInfoList& GetCharacterList();
	private:
		void UpdateCharacterInfo(const std::string& characterID, CharacterMapElementInstanceMutable& element);

		CharacterCreator m_characterCreator;
		CharacterMap m_activeCharacters;
		CharacterInfoList m_characterList;
		ComponentList m_components;
	};
}