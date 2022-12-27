#include "PCH.h"
#include "Character.h"

namespace
{
}

void CharacterControlCore::Character::Update()
{
	m_characterJson.Update();
	m_visualLayerRoot.Update(m_attributes);
}

const CharacterControlCore::Attributes& CharacterControlCore::Character::GetAttributes() const
{
	return m_attributes;

}

CharacterControlCore::Attributes& CharacterControlCore::Character::GetAttributesMutable()
{
	return m_attributes;
}

const CharacterControlCore::VisualLayer& CharacterControlCore::Character::GetVisualLayerRoot() const
{
	return m_visualLayerRoot;
}

const CharacterControlCore::ImageResources& CharacterControlCore::Character::GetImageResources() const
{
	return m_imageResources;
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

		auto visualLayersJson = characterJson->find("VisualLayers");
		if (visualLayersJson != characterJson->end())
		{
			m_visualLayerRoot = visualLayersJson->get<VisualLayer>();
		}
	}
}

