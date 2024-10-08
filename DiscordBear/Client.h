#pragma once
#include "Pipe.h"
#include "interface.h"
namespace DiscordBear
{
	class Client final : public DiscordClient
	{
	public:
		Client(const std::string& appID, const std::string& appSecret);
		Client(Client&) = delete;
		~Client();

		bool Connect();
		void Disconnect();

		void GetSelectedVoiceChannel(std::function<void(VoiceChannelInfo&&)> getJoinResponse);

		void SubscribeOnVoiceChannelJoin(std::function<void(GenericChannelInfo&&)> voiceJoinEvent);
		void UnsubscribeOnVoiceChannelJoin();

		void SubscribeOnVoiceChannelSpeak(
			const std::vector<ChannelID>& channelID,
			std::function<void(SpeakingEventInfo&&)> speakingStartEvent,
			std::function<void(SpeakingEventInfo&&)> speakingEndEvent);
		void UnsubscribeOnVoiceChannelSpeak();

		void SubscribeOnVoiceChannelEnter(
			const std::vector<ChannelID>& channelID,
			std::function<void(VoiceUserInfo&&)> voiceEnterEvent);
		void UnsubscribeOnVoiceChannelEnter();

		void SubscribeOnVoiceChannelExit(
			const std::vector<ChannelID>& channelID,
			std::function<void(VoiceUserInfo&&)> voiceExitEvent);
		void UnsubscribeOnVoiceChannelExit();

		void Update();
		void UpdateAsync();
	private:
		Pipe m_pipe;
		const std::string m_appID;
		const std::string m_appSecret;
		std::atomic<bool> m_finishUpdating;

		concurrency::task<void> m_asyncUpdateTask;

		concurrency::reader_writer_lock m_nonceToSelectedVoiceChannelResponsesDeleteLock;
		concurrency::concurrent_unordered_map<std::string, std::function<void(VoiceChannelInfo&&)>> m_nonceToSelectedVoiceChannelResponses;

		concurrency::reader_writer_lock m_voiceChannelJoinEventCreateLock;
		std::function<void(GenericChannelInfo&&)> m_voiceChannelJoinEvent;

		concurrency::reader_writer_lock m_speakingEventCreateLock;
		std::vector<ChannelID> m_speakingEventChannelIDList;
		std::function<void(SpeakingEventInfo&&)> m_speakingStartEvent;
		std::function<void(SpeakingEventInfo&&)> m_speakingEndEvent;

		concurrency::reader_writer_lock m_voiceChannelEnterEventCreateLock;
		std::vector<ChannelID> m_enterEventChannelIDList;
		std::function<void(VoiceUserInfo&&)> m_voiceChannelEnterEvent;

		concurrency::reader_writer_lock m_voiceChannelExitEventCreateLock;
		std::vector<ChannelID> m_exitEventChannelIDList;
		std::function<void(VoiceUserInfo&&)> m_voiceChannelExitEvent;
	};
}