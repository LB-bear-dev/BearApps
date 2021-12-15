#include "PCH.h"
#include "DiscordComponent.h"
#include "../DiscordBear/Interface.h"

CharacterControlCore::DiscordComponent::DiscordComponent(CharacterCreator& creator, CharacterMap& characters):
	m_discordClient(nullptr), InputComponent(creator, characters)
{
	m_discordClient = DiscordBear::GetDiscordClient("889010164760461322", "vkfhch9vw8NNRnhVMPeQKoIb7exvCWOM");

	auto voiceChannelInfoCollector =
		[this](DiscordBear::VoiceChannelInfo&& info)
		{
			if (info.valid)
			{
				m_discordClient->UnsubscribeOnVoiceChannelSpeak();

				CharacterMapInstance characterMap = m_activeCharacters.GetRawContainerBlocking();
				characterMap->clear();

				for (auto& voiceUser : info.voiceUserList)
				{
					if (voiceUser.valid)
					{
						characterMap->emplace(voiceUser.userID, m_characterCreator.GetCharacter(voiceUser.userID));
					}
				}

				m_discordClient->SubscribeOnVoiceChannelSpeak(
					{ info.channelID },
					[this](DiscordBear::SpeakingEventInfo&& speakingEvent) { this->OnSpeaking(std::move(speakingEvent)); },
					[this](DiscordBear::SpeakingEventInfo&& speakingEvent) { this->OnSpeakingStop(std::move(speakingEvent)); });
			}
		};
	m_discordClient->Connect();

	m_discordClient->GetSelectedVoiceChannel(voiceChannelInfoCollector);

	m_discordClient->SubscribeOnVoiceChannelJoin([this, voiceChannelInfoCollector](DiscordBear::GenericChannelInfo&& info)
	{
		m_discordClient->GetSelectedVoiceChannel(voiceChannelInfoCollector);
	});

}

CharacterControlCore::DiscordComponent::~DiscordComponent()
{
	m_discordClient->Disconnect();
	m_discordClient.reset();
}

void CharacterControlCore::DiscordComponent::Update()
{
	m_discordClient->UpdateAsync();
}

void CharacterControlCore::DiscordComponent::OnSpeaking(DiscordBear::SpeakingEventInfo&& speakingEvent)
{
}

void CharacterControlCore::DiscordComponent::OnSpeakingStop(DiscordBear::SpeakingEventInfo&& speakingEvent)
{
}
