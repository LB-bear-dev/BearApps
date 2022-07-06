#pragma once
namespace CharacterControlCore
{
	class CharacterCreator;
	class CharacterMap;
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