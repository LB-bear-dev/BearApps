#include "DiscordCharacterInfo.h"
#include "WatchedImage.h"
#include <CharacterControlCore/Interface.h>

CharacterControlOBS::DiscordCharacterInfo::DiscordCharacterInfo( const char* ID, const char* controlFile ) :
	m_currentImage(nullptr), m_ID(ID)
{
	std::hash<std::string> stringHash;
	m_IDHash = stringHash( m_ID );
	m_characterControl = CharacterControlCore::GetCharacterControl( controlFile );
}

const char* CharacterControlOBS::DiscordCharacterInfo::GetID()
{
	return m_ID.c_str();
}

const CharacterControlOBS::WatchedImage* CharacterControlOBS::DiscordCharacterInfo::GetCurrentImage()
{
	return m_currentImage;
}

void CharacterControlOBS::DiscordCharacterInfo::Update()
{
	m_characterControl->Update();

	if ( const CharacterControlCore::CharacterInfo* characterInfo = m_characterControl->GetCharacter( m_IDHash ) )
	{
		for ( const CharacterControlCore::ImageInfo& imageInfo : characterInfo->ImageLibrary() )
		{
			std::hash<std::string> stringHash;
			size_t imageID = stringHash( imageInfo.filename );
			if ( m_images.find( imageID ) == m_images.end() )
			{
				m_images.emplace( imageID, imageInfo.filename );
			}
		}

		for ( const CharacterControlCore::ImageInfo& imageInfo : characterInfo->CurrentImageStack() )
		{
			std::hash<std::string> stringHash;
			size_t hash = stringHash( imageInfo.filename );

			m_currentImage = nullptr;
			auto foundImage = m_images.find( hash );
			if ( foundImage != m_images.end() )
			{
				m_currentImage = &foundImage->second;
			}
		}
	}
}
