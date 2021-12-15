#include "PCH.h"
#include "CharacterControl.h"
#include "Character.h"
#include "DiscordComponent.h"

CharacterControlCore::CharacterControl::CharacterControl(std::string& filename):
	m_characterCreator(filename)
{
	std::unique_ptr<InputComponent> discordPtr(new DiscordComponent(m_characterCreator, m_activeCharacters));
	m_components.push_back(std::move(discordPtr));
}

CharacterControlCore::CharacterControl::~CharacterControl()
{
}

void CharacterControlCore::CharacterControl::Update()
{
	m_characterCreator.Update();

	for (auto& component : m_components)
	{
		component->Update();
	}

	for (auto updatingCharacter : m_activeCharacters.GetUpdatedCharacters())
	{
		auto element = m_activeCharacters.GetElement(updatingCharacter);
		if (element.has_value())
		{
			element.value()->Update();
		}
	}
}
