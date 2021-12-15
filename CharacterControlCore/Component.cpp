#include "PCH.h"
#include "Component.h"

CharacterControlCore::InputComponent::InputComponent(CharacterCreator& creator,CharacterMap& characters):
	m_characterCreator(creator), m_activeCharacters(characters){}

CharacterControlCore::InputComponent::~InputComponent(){}
