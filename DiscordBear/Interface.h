#pragma once
#include "LogDefinitions.h"
#include <vector>
#include <functional>

#if defined(DISCORDBEAR)
#define DLLINTERFACE   __declspec( dllexport )
#else
#define DLLINTERFACE   __declspec( dllimport )
#endif

namespace DiscordBear
{
	using UserID = std::string;
	using AvatarID = std::string;
	using ServerID = std::string;
	using ChannelID = std::string;

	struct DiscordInfo
	{
		bool valid;
		DiscordInfo() :valid(false) {}
	};

	struct GenericUserInfo : public DiscordInfo
	{
		UserID userID;
		std::string globalUsername;
		std::string discriminator;
		AvatarID avatarID;
		bool isBot;
	};

	struct GenericChannelInfo : public DiscordInfo
	{
		ServerID serverID;
		ChannelID channelID;
	};

	struct SpeakingEventInfo : public DiscordInfo
	{
		UserID userID;
		ChannelID channelID;
	};

	struct VoiceUserInfo : public GenericUserInfo
	{
		std::string serverNickname;
		unsigned long volume;
		bool isMuted;
		bool isDeafened;
	};
	struct VoiceChannelInfo : public GenericChannelInfo
	{
		std::vector<VoiceUserInfo> voiceUserList;
	};

	class DiscordClient
	{
	public:
		DiscordClient() {}
		DiscordClient(DiscordClient&) = delete;
		virtual ~DiscordClient() {};

		virtual bool Connect() = 0;
		virtual void Disconnect() = 0;

		virtual void GetSelectedVoiceChannel(std::function<void(VoiceChannelInfo&&)>) = 0;

		virtual void SubscribeOnVoiceChannelJoin(std::function<void(GenericChannelInfo&&)>) = 0;
		virtual void UnsubscribeOnVoiceChannelJoin() = 0;

		virtual void SubscribeOnVoiceChannelSpeak(
			const std::vector<ChannelID>& channelID,
			std::function<void(SpeakingEventInfo&&)> speakingStartEvent,
			std::function<void(SpeakingEventInfo&&)> speakingEndEvent) = 0;
		virtual void UnsubscribeOnVoiceChannelSpeak() = 0;

		virtual void SubscribeOnVoiceChannelEnter(
			const std::vector<ChannelID>& channelID,
			std::function<void(VoiceUserInfo&&)> voiceEnterEvent) = 0;
		virtual void UnsubscribeOnVoiceChannelEnter() = 0;

		virtual void SubscribeOnVoiceChannelExit(
			const std::vector<ChannelID>& channelID,
			std::function<void(VoiceUserInfo&&)> voiceExitEvent) = 0;
		virtual void UnsubscribeOnVoiceChannelExit() = 0;

		virtual void Update() = 0;
		virtual void UpdateAsync() = 0;
	};

	DLLINTERFACE void SetLogFn(LogFnDefinition LogFnDefinition);

	using ClientPtr = std::unique_ptr<DiscordClient>;
	DLLINTERFACE ClientPtr GetDiscordClient(const std::string& appID, const std::string& appSecret);
}
