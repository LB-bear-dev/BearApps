#pragma once
#include "CharacterMap.h"
#include "CharacterCreator.h"
namespace CharacterControlCore
{
	class InputComponent
	{
	public:
		InputComponent(CharacterCreator& creator, CharacterMap& characters);
		virtual ~InputComponent();
		virtual void Update() = 0;

	protected:
		CharacterCreator& m_characterCreator;
		CharacterMap& m_activeCharacters;
	};
}