#include "PCH.h"
#include "DiscordComponent.h"
#include "../DiscordBear/Interface.h"

namespace
{
	struct DiscordUser
	{
		DiscordBear::VoiceUserInfo info;
		bool speaking;
	};

	class DiscordInterface
	{
	public:
		static DiscordInterface& Get()
		{
			if (s_singleton == nullptr)
			{
				s_singleton = std::unique_ptr<DiscordInterface>(new DiscordInterface());
			}

			return *s_singleton;
		}

		DiscordUser* GetCharacterFromUserID(const DiscordBear::UserID& ID)
		{
			auto foundUser = m_voiceUserList.find(ID);

			if (foundUser != m_voiceUserList.end())
			{
				return &foundUser->second;
			}

			return nullptr;
		}

		void StartAsyncUpdate()
		{
			m_discordClient->UpdateAsync();
		}

	private:
		DiscordInterface()
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

					for (auto& user : info.voiceUserList)
					{
						m_voiceUserList.emplace(user.userID, DiscordUser{ user, false });
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

		void OnSpeaking(DiscordBear::SpeakingEventInfo&& speakingEvent)
		{
			auto foundUser = m_voiceUserList.find(speakingEvent.userID);

			if (foundUser != m_voiceUserList.end())
			{
				foundUser->second.speaking = true;
			}
		}

		void OnSpeakingStop(DiscordBear::SpeakingEventInfo&& speakingEvent)
		{
			auto foundUser = m_voiceUserList.find(speakingEvent.userID);

			if (foundUser != m_voiceUserList.end())
			{
				foundUser->second.speaking = false;
			}
		}

		void OnUserEnter(DiscordBear::VoiceUserInfo&& voiceEnterEvent)
		{
			if (voiceEnterEvent.valid)
			{
				m_voiceUserList.emplace(voiceEnterEvent.userID, DiscordUser{ voiceEnterEvent, false });
			}
		}

		void OnUserExit(DiscordBear::VoiceUserInfo&& voiceExitEvent)
		{
			if (voiceExitEvent.valid)
			{
				m_voiceUserList.erase(voiceExitEvent.userID);
			}
		}

		static std::unique_ptr<DiscordInterface> s_singleton;
		DiscordBear::ClientPtr m_discordClient;
		std::unordered_map<DiscordBear::UserID, DiscordUser> m_voiceUserList;
	};

	std::unique_ptr<DiscordInterface> DiscordInterface::s_singleton = nullptr;
}

void CharacterControlOBS::DiscordComponent::DeclareAttributes(AttributeMap& map)
{
	map.try_emplace("Talking", std::make_unique<Attribute<int>>(0, 0, 1));
	map.try_emplace("InCall", std::make_unique<Attribute<int>>(0, 0, 1));
	DiscordInterface::Get().StartAsyncUpdate();
}

void CharacterControlOBS::DiscordComponent::UpdateAttributes(const AttributeMap& map)
{
	if (auto* info = DiscordInterface::Get().GetCharacterFromUserID(m_ID))
	{
		auto talking = map.find("Talking");
		
		if (talking != map.end())
		{
			talking->second->SetValue(info->speaking);
		}

		auto inCall = map.find("InCall");

		if (inCall != map.end())
		{
			inCall->second->SetValue(true);
		}
	}
	else
	{
		auto inCall = map.find("InCall");

		if (inCall != map.end())
		{
			inCall->second->SetValue(false);
		}
	}
}

