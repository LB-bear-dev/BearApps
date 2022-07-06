#include "PCH.h"
#include "DiscordComponent.h"
#include "CharacterMap.h"
#include "CharacterCreator.h"
#include "../DiscordBear/Interface.h"
#pragma optimize("", off)

CharacterControlCore::DiscordComponent::DiscordComponent(CharacterCreator& creator, CharacterMap& characters):
	m_discordClient(nullptr), InputComponent(creator, characters)
{
	std::string ID = "889010164760461322";
	std::string Secret = "vkfhch9vw8NNRnhVMPeQKoIb7exvCWOM";

	m_discordClient = DiscordBear::GetDiscordClient(ID, Secret);

	auto voiceChannelInfoCollector =
		[this](DiscordBear::VoiceChannelInfo&& info)
		{
			if (info.valid)
			{
				m_discordClient->UnsubscribeOnVoiceChannelSpeak();
				m_discordClient->UnsubscribeOnVoiceChannelEnter();
				m_discordClient->UnsubscribeOnVoiceChannelExit();

				CharacterMapInstance characterMap = m_activeCharacters.GetRawContainerBlocking();
				characterMap->clear();

				for (auto& voiceUser : info.voiceUserList)
				{
					if (voiceUser.valid)
					{
						Attributes& attrs = characterMap->emplace(voiceUser.userID, m_characterCreator.GetCharacter(voiceUser.userID)).first->second.character.GetAttributesMutable();
						attrs.CreateAttribute<std::string>("nickname", voiceUser.serverNickname);
						attrs.CreateAttribute<std::string>("name", voiceUser.globalUsername);
					}
				}

				m_discordClient->SubscribeOnVoiceChannelSpeak(
					{ info.channelID },
					[this](DiscordBear::SpeakingEventInfo&& speakingEvent) { this->OnSpeaking(std::move(speakingEvent)); },
					[this](DiscordBear::SpeakingEventInfo&& speakingEvent) { this->OnSpeakingStop(std::move(speakingEvent)); });

				m_discordClient->SubscribeOnVoiceChannelEnter(
					{ info.channelID },
					[this](DiscordBear::VoiceUserInfo&& voiceEnterEvent) { this->OnUserEnter(std::move(voiceEnterEvent)); });

				m_discordClient->SubscribeOnVoiceChannelExit(
					{ info.channelID },
					[this](DiscordBear::VoiceUserInfo&& voiceExitEvent) { this->OnUserExit(std::move(voiceExitEvent)); });
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
	auto character = m_activeCharacters.GetCharacterMutable(speakingEvent.userID);
	if (character.Valid())
	{
		bool* speaking = character->GetAttributesMutable().GetAttribute<bool>("speaking");
		if (speaking)
		{
			*speaking = true;
		}
	}

}

void CharacterControlCore::DiscordComponent::OnSpeakingStop(DiscordBear::SpeakingEventInfo&& speakingEvent)
{
	auto character = m_activeCharacters.GetCharacterMutable(speakingEvent.userID);
	if (character.Valid())
	{
		bool* speaking = character->GetAttributesMutable().GetAttribute<bool>("speaking");
		if (speaking)
		{
			*speaking = false;
		}
	}
}

void CharacterControlCore::DiscordComponent::OnUserEnter(DiscordBear::VoiceUserInfo&& voiceEnterEvent)
{
	if (voiceEnterEvent.valid)
	{
		CharacterMapInstance characterMap = m_activeCharacters.GetRawContainerBlocking();
		characterMap->emplace(voiceEnterEvent.userID, m_characterCreator.GetCharacter(voiceEnterEvent.userID));
	}
}

void CharacterControlCore::DiscordComponent::OnUserExit(DiscordBear::VoiceUserInfo&& voiceExitEvent)
{
	if (voiceExitEvent.valid)
	{
		CharacterMapInstance characterMap = m_activeCharacters.GetRawContainerBlocking();
		characterMap->erase(voiceExitEvent.userID);
	}
}
