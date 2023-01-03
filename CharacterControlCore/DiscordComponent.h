#pragma once
#include "Component.h"
namespace DiscordBear
{
	class DiscordClient;
	struct SpeakingEventInfo;
	struct VoiceUserInfo;
	using ClientPtr = std::unique_ptr<DiscordClient>;
}

namespace CharacterControlCore
{
	class DiscordComponent : public InputComponent
	{
	public:
		DiscordComponent( CharacterMap& characters, const std::filesystem::path& path );
		~DiscordComponent();
		void Update();


		void OnSpeaking(DiscordBear::SpeakingEventInfo&& speakingEvent);
		void OnSpeakingStop(DiscordBear::SpeakingEventInfo&& speakingEvent);

		void OnUserEnter(DiscordBear::VoiceUserInfo&& voiceEnterEvent);
		void OnUserExit(DiscordBear::VoiceUserInfo&& voiceExitEvent);

	private:
		DiscordBear::ClientPtr m_discordClient;
	};
};