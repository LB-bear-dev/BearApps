#include "PCH.h"
#include "CharacterControl.h"
#include "Character.h"
#include "DiscordComponent.h"

CharacterControlCore::CharacterControl::CharacterControl( const char* filename):
	m_root( filename )
{
	m_components.push_back( std::unique_ptr<InputComponent>( new DiscordComponent( m_activeCharacters, m_root.GetPath() ) ) );
}

void CharacterControlCore::CharacterControl::Update()
{
	for (auto& component : m_components)
	{
		component->Update();
	}

	for (size_t updatingCharacter : m_activeCharacters.GetUpdatedCharacters())
	{
		Character* element = m_activeCharacters.GetCharacterMutable(updatingCharacter);
		if (element != nullptr)
		{
			element->Update();

			UpdateCharacterInfo(updatingCharacter, *element);
		}
	}
}

const CharacterControlCore::CharacterInfo* CharacterControlCore::CharacterControl::GetCharacter( size_t m_ID ) const
{
	auto foundCharacter = m_characterList.find( m_ID );
	if ( foundCharacter != m_characterList.end() )
	{
		return &foundCharacter->second;
	}

	return nullptr;
}

void CharacterControlCore::CharacterControl::UpdateCharacterInfo( size_t characterID, Character& element )
{
	auto character = m_characterList.try_emplace( characterID, m_root.GetPath().parent_path(), element );

	const ImageResource* image = element.GetImageResources().GetResource( element.GetVisualLayerRoot().GetCurrentImageName() );

	auto IDName = element.GetAttributes().GetAttribute<std::string>( "ID" );
	if ( image != nullptr && IDName != nullptr )
	{
		std::filesystem::path path = m_root.GetPath().parent_path() / *IDName / image->GetFilename();

		character.first->second.m_imageStack.clear();
		character.first->second.m_imageStack.emplace_back ( path.string() );
	}
}

CharacterControlCore::CharacterInfoInternal::CharacterInfoInternal( const std::filesystem::path& rootPath, const Character& element )
{
	auto nickname = element.GetAttributes().GetAttribute<std::string>( "nickname" );
	if ( nickname != nullptr )
	{
		m_name = *nickname;
	}

	m_imageLibrary.clear();

	auto IDName = element.GetAttributes().GetAttribute<std::string>( "ID" );
	if ( IDName != nullptr )
	{
		for ( auto& imageResource : element.GetImageResources().GetRawResources() )
		{
			std::filesystem::path path = rootPath / *IDName / imageResource.second.GetFilename();
			m_imageLibrary.emplace_back( path.string() );
		}
	}

	m_imageStack.clear();
}

const char* CharacterControlCore::CharacterInfoInternal::Name() const
{
	return m_name.c_str();
}

const CharacterControlCore::ImageInfoList& CharacterControlCore::CharacterInfoInternal::CurrentImageStack() const
{
	return m_imageStack;
}

const CharacterControlCore::ImageInfoList& CharacterControlCore::CharacterInfoInternal::ImageLibrary() const
{
	return m_imageLibrary;
}
