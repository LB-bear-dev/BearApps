#include "PCH.h"
#include "Component.h"

CharacterControlCore::InputComponent::InputComponent(CharacterMap& characters, const std::filesystem::path& path ):
	m_activeCharacters( characters ), m_path( path ){}

CharacterControlCore::InputComponent::~InputComponent(){}
