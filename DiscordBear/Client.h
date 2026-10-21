#pragma once
#include "interface.h"
#include "Pipe.h"

namespace DiscordBear
{
	class Client final : public DiscordClient
	{
	public:
		Client(const std::string& appID, const std::string& appSecret);
		Client(Client&) = delete;
		~Client() override;

		bool Connect() override;
		void Disconnect() override;

		void GetSelectedVoiceChannel(std::function<void(VoiceChannelInfo&&)> GetJoinResponse) override;

		void SubscribeOnVoiceChannelJoin(std::function<void(GenericChannelInfo&&)> VoiceJoinEvent) override;
		void UnsubscribeOnVoiceChannelJoin() override;

		void SubscribeOnVoiceChannelSpeak(const std::vector<ChannelID>& channelID,
			std::function<void(SpeakingEventInfo&&)> speakingStartEvent,
			std::function<void(SpeakingEventInfo&&)> speakingEndEvent) override;
		void UnsubscribeOnVoiceChannelSpeak() override;

		void SubscribeOnVoiceChannelEnter(const std::vector<ChannelID>& channelID, std::function<void(VoiceUserInfo&&)> voiceEnterEvent) override;
		void UnsubscribeOnVoiceChannelEnter() override;

		void SubscribeOnVoiceChannelExit(const std::vector<ChannelID>& channelID, std::function<void(VoiceUserInfo&&)> voiceExitEvent) override;
		void UnsubscribeOnVoiceChannelExit() override;

		void Update() override;
		void UpdateAsync() override;

	private:
		// Runs the pipe/handshake/authorize/authenticate sequence, no worker or reconnect bookkeeping.
		bool Establish();
		// AUTHENTICATEs with the cached token, refreshing it first if it has expired, so discord doesn't prompt the user.
		bool AuthenticateWithCachedToken();
		bool Reconnect();
		void RestoreSubscriptions();
		bool SendSubscription(const char* eventName, const ChannelID* channelID);
		// Signals the UpdateAsync worker to finish and waits for it, so the pipe isn't closed from under it.
		void StopUpdating();
		void HandleMessage(json& responseJson);

		Pipe m_pipe;
		const std::string m_appID;
		const std::string m_appSecret;
		std::atomic<bool> m_finishUpdating;
		// True between a successful Connect() and Disconnect(), the update worker reconnects a lost pipe only then.
		std::atomic<bool> m_autoReconnect;
		// Lets callbacks, which run on the worker, call Disconnect() without waiting on themselves.
		std::atomic<std::thread::id> m_workerThreadId;

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