#include "PCH.h"
#include "Interface.h"
#include "Client.h"
#include "Log.h"

void DiscordBear::SetLogFn(LogFnDefinition LogFnDefinition)
{
	LogFn = LogFnDefinition;
}

std::unique_ptr<DiscordBear::DiscordClient> DiscordBear::GetDiscordClient(const std::string& appID, const std::string& appSecret)
{
	return std::make_unique<DiscordBear::Client>(appID, appSecret);
}