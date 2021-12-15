#pragma once
#include "Component.h"
namespace DiscordBear
{
	class DiscordClient;
	struct SpeakingEventInfo;
	using ClientPtr = std::unique_ptr<DiscordClient>;
}

namespace CharacterControlCore
{
	class DiscordComponent : public InputComponent
	{
	public:
		DiscordComponent(CharacterCreator& creator, CharacterMap& characters);
		~DiscordComponent();
		void Update();

		void OnSpeaking(DiscordBear::SpeakingEventInfo&& speakingEvent);
		void OnSpeakingStop(DiscordBear::SpeakingEventInfo&& speakingEvent);

	private:
		DiscordBear::ClientPtr m_discordClient;
	};
};