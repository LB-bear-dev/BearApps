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

void CharacterControlCore::CharacterControl::Update()
{
	m_characterCreator.Update();

	for (auto& component : m_components)
	{
		component->Update();
	}

	for (const std::string& updatingCharacter : m_activeCharacters.GetUpdatedCharacters())
	{
		CharacterMapElementInstanceMutable element = m_activeCharacters.GetCharacterMutable(updatingCharacter);
		if (element.Valid())
		{
			element->Update();

			UpdateCharacterInfo(updatingCharacter, element);
		}
	}
}

const std::filesystem::path& CharacterControlCore::CharacterControl::GetContentRoot()
{
	return m_characterCreator.GetPath();
}

const CharacterControlCore::CharacterInfoList& CharacterControlCore::CharacterControl::GetCharacterList()
{
	return m_characterList;
}

void CharacterControlCore::CharacterControl::UpdateCharacterInfo(const std::string& characterID, CharacterMapElementInstanceMutable& element)
{
	auto character = std::find_if(m_characterList.begin(), m_characterList.end(), [&characterID](const CharacterInfo& lhs) { return lhs.ID == characterID; });
	if (character == m_characterList.end())
	{
		m_characterList.resize(m_characterList.size()+1);
		character = m_characterList.end();
		character--;
	}

	character->ID = characterID;
	auto nickname = element->GetAttributes().GetAttribute<std::string>("nickname");
	if (nickname != nullptr)
	{
		character->name = *nickname;
	}

	const ImageResource* image = element->GetImageResources().GetResource( element->GetVisualLayerRoot().GetCurrentImageName() );

	if ( image != nullptr )
	{
		std::filesystem::path path = m_characterCreator.GetPath ().parent_path ();
		path /= characterID;
		path /= image->GetFilename();

		character->imageInfos.clear ();
		character->imageInfos.emplace_back ( path.string ().c_str () );
	}
}
