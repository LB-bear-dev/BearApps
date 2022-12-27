#include "../DiscordBear/Interface.h"
#include "../CharacterControlCore/Interface.h"
#include <iostream>

//int main()
//{
//	//DiscordBear::SetLogFn([](LogSeverity severity, const char* message) { std::cout << message << std::endl; });
//	DiscordBear::ClientPtr client = DiscordBear::GetDiscordClient("889010164760461322", "vkfhch9vw8NNRnhVMPeQKoIb7exvCWOM");
//	client->Connect();
//
//	client->GetSelectedVoiceChannel([&client](DiscordBear::VoiceChannelInfo&& info)
//	{
//			if (info.valid)
//			{
//				std::cout << "Hey, it's " << info.voiceUserList[0].serverNickname << std::endl;
//			}
//			else
//			{
//				std::cout << "You're not in a channel" << std::endl;
//			}
//
//		client->UnsubscribeOnVoiceChannelSpeak();
//		client->SubscribeOnVoiceChannelSpeak({ info.channelID },
//		[](DiscordBear::SpeakingEventInfo&& speakingEvent)
//		{
//			if (speakingEvent.valid)
//			{
//				std::cout << "SPEAKING: " << speakingEvent.userID << std::endl;
//			}
//		},
//		[](DiscordBear::SpeakingEventInfo&& speakingEvent)
//		{
//			if (speakingEvent.valid)
//			{
//				std::cout << "STOPPED SPEAKING: " << speakingEvent.userID << std::endl;
//			}
//		});
//	});
//
//	client->SubscribeOnVoiceChannelJoin([&client](DiscordBear::GenericChannelInfo&& info)
//	{
//			if (info.valid)
//			{
//				std::cout << "User joined server: " << info.serverID << std::endl << "channel: " << info.channelID;
//			}
//
//		client->UnsubscribeOnVoiceChannelSpeak();
//		client->SubscribeOnVoiceChannelSpeak({ info.channelID },
//		[](DiscordBear::SpeakingEventInfo&& speakingEvent)
//		{
//			if (speakingEvent.valid)
//			{
//				std::cout << "SPEAKING: " << speakingEvent.userID << std::endl;
//			}
//		},
//		[](DiscordBear::SpeakingEventInfo&& speakingEvent)
//		{
//			if (speakingEvent.valid)
//			{
//				std::cout << "STOPPED SPEAKING: " << speakingEvent.userID << std::endl;
//			}
//		});
//	});
//
//	while (1)
//	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(32));
//		client->UpdateAsync();
//	}
//
//	client->Disconnect();
//	return 0;
//}

int main(int argc, char* argv[])
{
	//DiscordBear::SetLogFn([](LogSeverity severity, const char* message) { std::cout << message << std::endl; });
	CharacterControlCore::ControlCorePtr characterControl;
	try
	{
		characterControl = CharacterControlCore::GetCharacterControl("C:\\Users\\chris\\OneDrive\\Documents\\OBSCharacterSettings\\globalSettings.json");
	}
	catch (...)
	{

	}

	if (characterControl)
	{
		while(1)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds (32));
			characterControl->Update();
		}
	}
}