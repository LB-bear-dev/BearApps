#pragma once
#include <CharacterControlCore/Interface.h>
#include <unordered_map>

namespace CharacterControlOBS
{
	class WatchedImage;

	class DiscordCharacterInfo
	{
	public:
		DiscordCharacterInfo( const char* ID, const char* controlFile );

		const char* GetID();
		const WatchedImage* GetCurrentImage();
		void Update();

	private:
		CharacterControlCore::ControlCorePtr m_characterControl;
		std::string m_ID;
		size_t m_IDHash;
		std::unordered_map<size_t, WatchedImage> m_images;
		WatchedImage* m_currentImage;
	};
}