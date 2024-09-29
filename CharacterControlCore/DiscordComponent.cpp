#include "PCH.h"
#include "DiscordComponent.h"
#include "CharacterMap.h"
#include "../DiscordBear/Interface.h"
#pragma optimize("", off)

namespace
{

}

CharacterControlCore::DiscordComponent::DiscordComponent( CharacterMap& characters, const std::filesystem::path& path ):
	m_discordClient(nullptr), InputComponent( characters, path )
{
	static const std::string ID = "889010164760461322";
	static const std::string Secret = "vkfhch9vw8NNRnhVMPeQKoIb7exvCWOM";

	m_discordClient = DiscordBear::GetDiscordClient(ID, Secret);

	auto voiceChannelInfoCollector =
		[this](DiscordBear::VoiceChannelInfo&& info)
		{
			if (info.valid)
			{
				m_discordClient->UnsubscribeOnVoiceChannelSpeak();
				m_discordClient->UnsubscribeOnVoiceChannelEnter();
				m_discordClient->UnsubscribeOnVoiceChannelExit();

				m_activeCharacters.Clear();

				for (auto& voiceUser : info.voiceUserList)
				{
					if (voiceUser.valid)
					{
						std::hash<std::string> stringhash;
						auto character = m_activeCharacters.CreateCharacter( stringhash( voiceUser.userID ), (m_path.parent_path() / voiceUser.userID / "characterconfig.json").string().c_str() );
						if ( character != nullptr )
						{
							//Attributes& attrs = character->GetAttributesMutable();
							//attrs.CreateAttribute<std::string>( "nickname", voiceUser.serverNickname );
							//attrs.CreateAttribute<std::string>( "name", voiceUser.globalUsername );
							//attrs.CreateAttribute<std::string>( "ID", voiceUser.userID );
							//attrs.CreateAttribute<bool>( "speaking", false );
						}
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
	std::hash<std::string> stringhash;
	auto character = m_activeCharacters.GetCharacterMutable( stringhash(speakingEvent.userID) );
	if (character != nullptr)
	{
		//bool* speaking = character->GetAttributesMutable().GetAttribute<bool>("speaking");
		//if (speaking)
		//{
		//	*speaking = true;
		//}
	}
}

void CharacterControlCore::DiscordComponent::OnSpeakingStop(DiscordBear::SpeakingEventInfo&& speakingEvent)
{
	std::hash<std::string> stringhash;
	auto character = m_activeCharacters.GetCharacterMutable( stringhash( speakingEvent.userID ) );
	if ( character != nullptr )
	{
		//bool* speaking = character->GetAttributesMutable().GetAttribute<bool>( "speaking" );
		//if ( speaking )
		//{
		//	*speaking = false;
		//}
	}
}

void CharacterControlCore::DiscordComponent::OnUserEnter(DiscordBear::VoiceUserInfo&& voiceEnterEvent)
{
	if ( voiceEnterEvent.valid )
	{
		std::hash<std::string> stringhash;
		CharacterControlCore::Character* character = m_activeCharacters.CreateCharacter( 
			stringhash( voiceEnterEvent.userID ), 
			(m_path.parent_path() / voiceEnterEvent.userID / "characterconfig.json").string().c_str() );

		if ( character != nullptr )
		{
			//Attributes& attrs = character->GetAttributesMutable();
			//attrs.CreateAttribute<std::string>( "nickname", voiceEnterEvent.serverNickname );
			//attrs.CreateAttribute<std::string>( "name", voiceEnterEvent.globalUsername );
			//attrs.CreateAttribute<std::string>( "ID", voiceEnterEvent.userID );
			//attrs.CreateAttribute<bool>( "speaking", false );
		}
	}
}

void CharacterControlCore::DiscordComponent::OnUserExit(DiscordBear::VoiceUserInfo&& voiceExitEvent)
{
	if (voiceExitEvent.valid)
	{
		std::hash<std::string> stringhash;
		m_activeCharacters.RemoveCharacter( stringhash( voiceExitEvent.userID ) );
	}
}
