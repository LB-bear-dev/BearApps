#include "PCH.h"
#include "Client.h"
#include "Log.h"
#include "Pipe.h"
#include "Message.h"
#include "Register.h"

#pragma optimize("", off)

using namespace DiscordBear;

DiscordBear::Client::Client(const std::string& appID, const std::string& appSecret) : m_appID(appID), m_appSecret(appSecret), m_asyncUpdateTask([]() {}) {}

DiscordBear::Client::~Client()
{
	LOG(LogSeverity::NonSevere) << "Discord client not disconnected before client was destroyed, disconnecting now.";
	Disconnect();
}

std::string GenerateNonce()
{
	static unsigned long long count = 0;
	std::stringstream nonceStream;
	nonceStream << std::chrono::high_resolution_clock::now().time_since_epoch().count() << count++;

	return nonceStream.str();
}

bool Handshake(Pipe& pipe, const std::string& appID)
{
	LOG(LogSeverity::Info) << "Sending handshake for client ID " << appID << " to discord";

	if (!pipe.WriteMessage(
		Message(Message::Opcode::Handshake,
			json
			{
				{"v", "1"},
				{"client_id", appID.c_str()}
			}.dump())))
	{
		LOG(LogSeverity::Severe) << "Could not write handshake message";
		return false;
	}

	Message handshakeResponse = pipe.ReadMessage();

	if (!handshakeResponse.IsValid())
	{
		LOG(LogSeverity::Severe) << "Discord client could not read handshake response";
		return false;
	}

	json handshakeResponseJson = json::parse(handshakeResponse.GetMessage());
	if (handshakeResponseJson["cmd"] != "DISPATCH" || handshakeResponseJson["evt"] != "READY")
	{
		LOG(LogSeverity::Severe) << "Discord client did not respond with ready response";
		return false;
	}

	LOG(LogSeverity::Info) << "Sending handshake for client ID " << appID << " succeeded";
	return true;
}

bool Authorize(Pipe& pipe, const std::string& appID, std::string& authorizationCode)
{
	LOG(LogSeverity::Info) << "Sending authorization for client ID " << appID << " to discord";

	std::string nonce = GenerateNonce();

	if (!pipe.WriteMessage(
		Message(Message::Opcode::Frame,
			json
			{
				{"nonce", nonce},
				{"cmd", "AUTHORIZE"},
				{"args",
				{
					{"client_id", appID.c_str()},
					{"scopes", {"rpc"}}
				}}
			}.dump())))
	{
		LOG(LogSeverity::Severe) << "Could not write authorize message";
		return false;
	}

	Message response = pipe.ReadMessage();

	if (!response.IsValid())
	{
		LOG(LogSeverity::Severe) << "Discord client could not read authorize response";
		return false;
	}

	json responseJson = json::parse(response.GetMessage());
	if (responseJson["nonce"] != nonce)
	{
		LOG(LogSeverity::Severe) << "nonce does not match, this is bad";
		return false;
	}

	authorizationCode = responseJson["data"]["code"];

	return true;
}

bool Authenticate(Pipe& pipe, const std::string& authorizationCode, const std::string& appID)
{
	std::string nonce = GenerateNonce();

	if (!pipe.WriteMessage(
		Message(Message::Opcode::Frame,
			json
			{
				{"nonce", nonce},
				{"cmd", "AUTHENTICATE"},
				{"args",
				{
					{"access_token", authorizationCode}
				}}
			}.dump())))
	{
		LOG(LogSeverity::Severe) << "Could not write authenticate message";
		return false;
	}

	Message response = pipe.ReadMessage();

	if (!response.IsValid())
	{
		LOG(LogSeverity::Severe) << "Discord client could not read authenticate response";
		return false;
	}

	json responseJson = json::parse(response.GetMessage());
	if (responseJson["nonce"] != nonce)
	{
		LOG(LogSeverity::Severe) << "nonce does not match, this is bad";
		return false;
	}

	return true;
}

bool DiscordBear::Client::Connect()
{
	LOG(LogSeverity::Info) << "Connecting discord client";

	if (!RegisterAppToDiscord(m_appID))
	{
		return false;
	}

	if (!m_pipe.Connect())
	{
		LOG(LogSeverity::Severe) << "Could not connect to discord pipe, client is not connected.";
		return false;
	}

	if (!Handshake(m_pipe, m_appID))
	{
		return false;
	}

	std::string authorizationCode;
	if (!Authorize(m_pipe, m_appID, authorizationCode))
	{
		return false;
	}

	OAuthParams params = GetAuthToken(m_appID, m_appSecret.c_str(), authorizationCode.c_str());

	if (!Authenticate(m_pipe, params.token, m_appID))
	{
		return false;
	}

	LOG(LogSeverity::Info) << "Discord client connected";
	return true;
}

void DiscordBear::Client::Disconnect()
{
	LOG(LogSeverity::Info) << "Disconnecting discord client";

	m_asyncUpdateTask.wait();

	m_pipe.Disconnect();
}

void DiscordBear::Client::GetSelectedVoiceChannel(std::function<void(VoiceChannelInfo&&)> response)
{
	std::string nonce = GenerateNonce();

	if (!m_pipe.WriteMessage(
		Message(Message::Opcode::Frame,
			json
			{
				{"nonce", nonce},
				{"cmd", "GET_SELECTED_VOICE_CHANNEL"}
			}.dump())))
	{
		LOG(LogSeverity::Severe) << "Could not write GET_SELECTED_VOICE_CHANNEL message";
		return;
	}

	concurrency::reader_writer_lock::scoped_lock_read lock(m_nonceToSelectedVoiceChannelResponsesDeleteLock);
	m_nonceToSelectedVoiceChannelResponses[nonce] = response;
}

void DiscordBear::Client::SubscribeOnVoiceChannelJoin(std::function<void(GenericChannelInfo&&)> voiceJoinEvent)
{
	if (!m_pipe.WriteMessage(
		Message(Message::Opcode::Frame,
			json
			{
				{"nonce", GenerateNonce()},
				{"cmd", "SUBSCRIBE"},
				{"evt", "VOICE_CHANNEL_SELECT"}
			}.dump())))
	{
		LOG(LogSeverity::Severe) << "Could not write GET_SELECTED_VOICE_CHANNEL message";
		return;
	}

	concurrency::reader_writer_lock::scoped_lock lock(m_voiceChannelJoinEventCreateLock);
	m_voiceChannelJoinEvent = voiceJoinEvent;
}

void DiscordBear::Client::UnsubscribeOnVoiceChannelJoin()
{
	{
		concurrency::reader_writer_lock::scoped_lock_read lock(m_voiceChannelJoinEventCreateLock);
		if (m_voiceChannelJoinEvent == nullptr)
		{
			return;
		}
	}

	if (!m_pipe.WriteMessage(
		Message(Message::Opcode::Frame,
			json
			{
				{"nonce", GenerateNonce()},
				{"cmd", "UNSUBSCRIBE"},
				{"evt", "VOICE_CHANNEL_SELECT"}
			}.dump())))
	{
		LOG(LogSeverity::Severe) << "Could not write GET_SELECTED_VOICE_CHANNEL message";
		return;
	}

	concurrency::reader_writer_lock::scoped_lock lock(m_voiceChannelJoinEventCreateLock);
	m_voiceChannelJoinEvent = nullptr;
}

void DiscordBear::Client::SubscribeOnVoiceChannelSpeak(
	const std::vector<ChannelID>& channelIDList,
	std::function<void(SpeakingEventInfo&&)> speakingStartEvent,
	std::function<void(SpeakingEventInfo&&)> speakingEndEvent)
{
	{
		concurrency::reader_writer_lock::scoped_lock_read lock(m_speakingEventCreateLock);
		if (!m_speakingEventChannelIDList.empty())
		{
			LOG(LogSeverity::Severe) << "Already subscribed to Voice channel speaking event, unsubscribe and resubscribe to change which channels you want to subscribe to.";
			return;
		}
	}

	for (auto& channelID : channelIDList)
	{
		if (!m_pipe.WriteMessage(
			Message(Message::Opcode::Frame,
				json
				{
					{"nonce", GenerateNonce()},
					{"cmd", "SUBSCRIBE"},
					{"evt", "SPEAKING_START"},
					{"args",
					{
						{"channel_id", channelID}
					}}
				}.dump())))
		{
			LOG(LogSeverity::Severe) << "Could not write SPEAKING_START message";
			return;
		}

		if (!m_pipe.WriteMessage(
			Message(Message::Opcode::Frame,
				json
				{
					{"nonce", GenerateNonce()},
					{"cmd", "SUBSCRIBE"},
					{"evt", "SPEAKING_STOP"},
					{"args",
					{
						{"channel_id", channelID}
					}}
				}.dump())))
		{
			LOG(LogSeverity::Severe) << "Could not write SPEAKING_STOP message";
			return;
		}
	}

	concurrency::reader_writer_lock::scoped_lock lock(m_speakingEventCreateLock);
	m_speakingEventChannelIDList = channelIDList;
	m_speakingStartEvent = speakingStartEvent;
	m_speakingEndEvent = speakingEndEvent;
}

void DiscordBear::Client::UnsubscribeOnVoiceChannelSpeak()
{
	{
		concurrency::reader_writer_lock::scoped_lock_read lock(m_speakingEventCreateLock);
		if (m_speakingEventChannelIDList.empty())
		{
			return;
		}
	}

	for (auto& channelID : m_speakingEventChannelIDList)
	{
		if (!m_pipe.WriteMessage(
			Message(Message::Opcode::Frame,
				json
				{
					{"nonce", GenerateNonce()},
					{"cmd", "UNSUBSCRIBE"},
					{"evt", "SPEAKING_START"},
					{"args",
					{
						{"channel_id", channelID}
					}}
				}.dump())))
		{
			LOG(LogSeverity::Severe) << "Could not write SPEAKING_START message";
			return;
		}

		if (!m_pipe.WriteMessage(
			Message(Message::Opcode::Frame,
				json
				{
					{"nonce", GenerateNonce()},
					{"cmd", "UNSUBSCRIBE"},
					{"evt", "SPEAKING_STOP"},
					{"args",
					{
						{"channel_id", channelID}
					}}
				}.dump())))
		{
			LOG(LogSeverity::Severe) << "Could not write SPEAKING_STOP message";
			return;
		}
	}

	concurrency::reader_writer_lock::scoped_lock lock(m_speakingEventCreateLock);
	m_speakingEventChannelIDList.clear();
	m_speakingStartEvent = nullptr;
	m_speakingEndEvent = nullptr;
}

void DiscordBear::Client::SubscribeOnVoiceChannelEnter(
	const std::vector<ChannelID>& channelIDList,
	std::function<void(VoiceUserInfo&&)> voiceEnterEvent)
{
	{
		concurrency::reader_writer_lock::scoped_lock_read lock(m_voiceChannelEnterEventCreateLock);
		if (m_voiceChannelEnterEvent != nullptr)
		{
			LOG(LogSeverity::Severe) << "Already subscribed to Voice channel enter event, unsubscribe and resubscribe to change which channels you want to subscribe to.";
			return;
		}
	}

	for (auto& channelID : channelIDList)
	{
		if (!m_pipe.WriteMessage(
			Message(Message::Opcode::Frame,
				json
				{
					{"nonce", GenerateNonce()},
					{"cmd", "SUBSCRIBE"},
					{"evt", "VOICE_STATE_CREATE"},
					{"args",
					{
						{"channel_id", channelID}
					}}
				}.dump())))
		{
			LOG(LogSeverity::Severe) << "Could not write VOICE_STATE_CREATE message";
			return;
		}
	}

	concurrency::reader_writer_lock::scoped_lock lock(m_voiceChannelEnterEventCreateLock);
	m_enterEventChannelIDList = channelIDList;
	m_voiceChannelEnterEvent = voiceEnterEvent;
}

void DiscordBear::Client::UnsubscribeOnVoiceChannelEnter()
{
	{
		concurrency::reader_writer_lock::scoped_lock_read lock(m_voiceChannelEnterEventCreateLock);
		if (m_enterEventChannelIDList.empty())
		{
			return;
		}
	}

	for (auto& channelID : m_enterEventChannelIDList)
	{
		if (!m_pipe.WriteMessage(
			Message(Message::Opcode::Frame,
				json
				{
					{"nonce", GenerateNonce()},
					{"cmd", "UNSUBSCRIBE"},
					{"evt", "VOICE_STATE_CREATE"},
					{"args",
					{
						{"channel_id", channelID}
					}}
				}.dump())))
		{
			LOG(LogSeverity::Severe) << "Could not write VOICE_STATE_CREATE message";
			return;
		}
	}

	concurrency::reader_writer_lock::scoped_lock lock(m_voiceChannelEnterEventCreateLock);
	m_enterEventChannelIDList.clear();
	m_voiceChannelEnterEvent = nullptr;
}

void DiscordBear::Client::SubscribeOnVoiceChannelExit(
	const std::vector<ChannelID>& channelIDList,
	std::function<void(VoiceUserInfo&&)> voiceExitEvent)
{
	{
		concurrency::reader_writer_lock::scoped_lock_read lock(m_voiceChannelExitEventCreateLock);
		if (m_voiceChannelExitEvent != nullptr)
		{
			LOG(LogSeverity::Severe) << "Already subscribed to Voice channel exit event, unsubscribe and resubscribe to change which channels you want to subscribe to.";
			return;
		}
	}

	for (auto& channelID : channelIDList)
	{
		if (!m_pipe.WriteMessage(
			Message(Message::Opcode::Frame,
				json
				{
					{"nonce", GenerateNonce()},
					{"cmd", "SUBSCRIBE"},
					{"evt", "VOICE_STATE_DELETE"},
					{"args",
					{
						{"channel_id", channelID}
					}}
				}.dump())))
		{
			LOG(LogSeverity::Severe) << "Could not write VOICE_STATE_DELETE message";
			return;
		}
	}

	concurrency::reader_writer_lock::scoped_lock lock(m_voiceChannelExitEventCreateLock);
	m_exitEventChannelIDList = channelIDList;
	m_voiceChannelExitEvent = voiceExitEvent;
}
void DiscordBear::Client::UnsubscribeOnVoiceChannelExit()
{
	{
		concurrency::reader_writer_lock::scoped_lock_read lock(m_voiceChannelExitEventCreateLock);
		if (m_exitEventChannelIDList.empty())
		{
			return;
		}
	}

	for (auto& channelID : m_exitEventChannelIDList)
	{
		if (!m_pipe.WriteMessage(
			Message(Message::Opcode::Frame,
				json
				{
					{"nonce", GenerateNonce()},
					{"cmd", "UNSUBSCRIBE"},
					{"evt", "VOICE_STATE_DELETE"},
					{"args",
					{
						{"channel_id", channelID}
					}}
				}.dump())))
		{
			LOG(LogSeverity::Severe) << "Could not write VOICE_STATE_DELETE message";
			return;
		}
	}

	concurrency::reader_writer_lock::scoped_lock lock(m_voiceChannelExitEventCreateLock);
	m_exitEventChannelIDList.clear();
	m_voiceChannelExitEvent = nullptr;
}

namespace
{
	bool GetGenericUserInfoFromJson(const json& userJson, GenericUserInfo& userInfo)
	{
		if (userJson.contains("id") &&
			userJson.contains("username") &&
			userJson.contains("discriminator") &&
			userJson.contains("avatar") &&
			userJson.contains("bot"))
		{
			userInfo.userID = userJson["id"];
			userInfo.globalUsername = userJson["username"];
			userInfo.discriminator = userJson["discriminator"];
			userInfo.avatarID = userJson["avatar"];
			userInfo.isBot = userJson["bot"];
			userInfo.valid = true;
		}
		else
		{
			userInfo.valid = false;
			LOG(LogSeverity::NonSevere) << "Expected generic user message to have params 'id', 'username', 'discriminator', 'avatar', 'bot' raw JSON:" << std::endl << userJson;
		}

		return userInfo.valid;
	}

	bool GetVoiceUserInfoFromJson(const json& userJson, VoiceUserInfo& userInfo)
	{
		if (GetGenericUserInfoFromJson(userJson["user"], userInfo))
		{
			if (userJson.contains("nick") &&
				userJson.contains("volume") &&
				userJson.contains("mute") &&
				userJson.contains("user"))
			{
				userInfo.serverNickname = userJson["nick"];
				userInfo.volume = userJson["volume"];
				userInfo.isMuted = userJson["mute"];
				userInfo.valid = true;
			}
			else
			{
				userInfo.valid = false;
				LOG(LogSeverity::NonSevere) << "Expected voice user message to have params 'nick', 'volume', 'mute' raw JSON:" << std::endl << userJson;
			}
		}

		return userInfo.valid;
	}

	bool GetGenericChannelInfoFromJson(const json& channelJson, GenericChannelInfo& channelInfo)
	{
		if (channelJson.contains("channel_id") &&
			channelJson.contains("guild_id"))
		{
			channelInfo.channelID = channelJson["channel_id"];
			channelInfo.serverID = channelJson["guild_id"];
			channelInfo.valid = true;
		}
		else
		{
			channelInfo.valid = false;
			LOG(LogSeverity::NonSevere) << "Expected voice channel message to have params 'channel_id', 'guild_id', raw JSON:" << std::endl << channelJson;
		}

		return channelInfo.valid;
	}

	bool GetVoiceChannelInfoFromJson(const json& channelJson, VoiceChannelInfo& channelInfo)
	{
		if (channelJson.contains("id") &&
			channelJson.contains("guild_id") &&
			channelJson.contains("voice_states"))
		{
			channelInfo.channelID = channelJson["id"];
			channelInfo.serverID = channelJson["guild_id"];
			json userListJson = channelJson["voice_states"];
			channelInfo.voiceUserList.reserve(userListJson.size());

			for (json voiceUserJson : userListJson)
			{
				VoiceUserInfo userInfo;
				if (GetVoiceUserInfoFromJson(voiceUserJson, userInfo))
				{
					channelInfo.voiceUserList.push_back(std::move(userInfo));
				}
			}

			channelInfo.valid = true;
		}
		else
		{
			channelInfo.valid = false;
			LOG(LogSeverity::NonSevere) << "Expected voice channel message to have params 'voice_states', 'id', 'guild_id', raw JSON : " << std::endl << channelJson;
		}

		return channelInfo.valid;
	}

	bool GetSpeakingEventInfoFromJson(const json& channelJson, SpeakingEventInfo& speakingInfo)
	{
		if (channelJson.contains("channel_id") &&
			channelJson.contains("user_id"))
		{
			speakingInfo.channelID = channelJson["channel_id"];
			speakingInfo.userID = channelJson["user_id"];
			speakingInfo.valid = true;
		}
		else
		{
			speakingInfo.valid = false;
			LOG(LogSeverity::NonSevere) << "Expected voice channel message to have params 'channel_id', 'user_id', raw JSON:" << std::endl << channelJson;
		}

		return speakingInfo.valid;
	}
}

void DiscordBear::Client::Update()
{
	while (m_pipe.HasMessage())
	{
		Message response = m_pipe.ReadMessage();

		if (!response.IsValid())
		{
			LOG(LogSeverity::Severe) << "Discord client had a message but we could not read it";
			return;
		}

		json responseJson = json::parse(response.GetMessage());

		if (responseJson.contains("cmd") &&
			responseJson.contains("data") &&
			responseJson.contains("nonce"))
		{
			std::string commandName = responseJson["cmd"];
			if (commandName == "GET_CHANNEL" || commandName == "GET_SELECTED_VOICE_CHANNEL")
			{
				std::string nonce = responseJson["nonce"];
				auto callback = m_nonceToSelectedVoiceChannelResponses[nonce];
				if (callback)
				{
					VoiceChannelInfo info;
					GetVoiceChannelInfoFromJson(responseJson["data"], info);
					callback(std::move(info));

					//deletion is unsafe, so if we do actually take care of this message we need to lock just when we are doing the delete.
					concurrency::reader_writer_lock::scoped_lock lock(m_nonceToSelectedVoiceChannelResponsesDeleteLock);
					m_nonceToSelectedVoiceChannelResponses.unsafe_erase(nonce);
				}
				else
				{
					LOG(LogSeverity::NonSevere) << "Found GET_CHANNEL message with nonce '" << nonce << "' but there was no response available";
				}
			}
			else if (commandName == "DISPATCH")
			{
				std::string eventName = responseJson["evt"];
				{
					concurrency::reader_writer_lock::scoped_lock lock(m_voiceChannelJoinEventCreateLock);
					if (eventName == "VOICE_CHANNEL_SELECT" && m_voiceChannelJoinEvent)
					{
						GenericChannelInfo channelInfo;
						GetGenericChannelInfoFromJson(responseJson["data"], channelInfo);
						m_voiceChannelJoinEvent(std::move(channelInfo));
					}
				}

				{
					concurrency::reader_writer_lock::scoped_lock lock(m_speakingEventCreateLock);
					if (eventName == "SPEAKING_START" && m_speakingStartEvent)
					{
						SpeakingEventInfo speakingInfo;
						GetSpeakingEventInfoFromJson(responseJson["data"], speakingInfo);
						m_speakingStartEvent(std::move(speakingInfo));
					}
					if (eventName == "SPEAKING_STOP" && m_speakingEndEvent)
					{
						SpeakingEventInfo speakingInfo;
						GetSpeakingEventInfoFromJson(responseJson["data"], speakingInfo);
						m_speakingEndEvent(std::move(speakingInfo));
					}
				}

				{
					concurrency::reader_writer_lock::scoped_lock lock(m_voiceChannelEnterEventCreateLock);
					if (eventName == "VOICE_STATE_CREATE" && m_voiceChannelEnterEvent)
					{
						VoiceUserInfo voiceUserInfo;
						GetVoiceUserInfoFromJson(responseJson["data"], voiceUserInfo);
						m_voiceChannelEnterEvent(std::move(voiceUserInfo));
					}
				}

				{
					concurrency::reader_writer_lock::scoped_lock lock(m_voiceChannelExitEventCreateLock);
					if (eventName == "VOICE_STATE_DELETE" && m_voiceChannelExitEvent)
					{
						VoiceUserInfo voiceUserInfo;
						GetVoiceUserInfoFromJson(responseJson["data"], voiceUserInfo);
						m_voiceChannelExitEvent(std::move(voiceUserInfo));
					}
				}
			}
			else
			{
				LOG(LogSeverity::Info) << "Message with command name '" << commandName << "' recieved but no callback registered to handle it";
			}
		}
		else
		{
			LOG(LogSeverity::NonSevere) << "Found message without 'cmd', 'data' or 'nonce' fields, these are required on messages. Raw JSON: " << std::endl << responseJson;
		}
	}
}

void DiscordBear::Client::UpdateAsync()
{
	Update ();
}
