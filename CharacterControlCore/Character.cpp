#include "PCH.h"
#include "Character.h"

namespace
{
}

void CharacterControlCore::Character::Update()
{
	m_characterJson.Update();
}

CharacterControlCore::Character::Character(std::filesystem::path filename):
	m_characterJson(filename)
{
	Parse();
}

void CharacterControlCore::Character::Parse()
{
	auto characterJson = m_characterJson.Get();
	if (characterJson.has_value())
	{
		auto attributesJson = characterJson->find("Attributes");
		if (attributesJson != characterJson->end())
		{
			m_attributes = attributesJson->get<Attributes>();
		}

		auto imageResourcesJson = characterJson->find("ImageResources");
		if (imageResourcesJson != characterJson->end())
		{
			m_imageResources = imageResourcesJson->get<ImageResources>();
		}


	}
}

