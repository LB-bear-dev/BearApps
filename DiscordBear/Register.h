#pragma once

namespace DiscordBear
{
	struct OAuthParams
	{
		std::string token;
		unsigned long expiry;
		std::string refreshToken;
	};

	bool RegisterAppToDiscord(const std::string& appID);
	OAuthParams GetAuthToken(const std::string& appID, const std::string& clientSecret, const std::string& code);
}