#pragma once
#include "Interface.h"
#include "Component.h"

namespace CharacterControlCore
{
	using ComponentList = std::vector<std::unique_ptr<CharacterControlCore::InputComponent>>;
	class CharacterControl : public ControlCore
	{
	public:
		CharacterControl(std::string& filename);
		~CharacterControl();
		void Update();

	private:
		CharacterCreator m_characterCreator;
		CharacterMap m_activeCharacters;
		ComponentList m_components;
	};
}