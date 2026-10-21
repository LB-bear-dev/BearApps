#pragma once
#include <optional>

namespace DiscordBear
{
	struct OAuthParams
	{
		std::string token;
		unsigned long expiry;
		std::string refreshToken;
	};

	bool RegisterAppToDiscord(const std::string& appID);
	std::optional<OAuthParams> GetAuthToken(const std::string& appID, const std::string& clientSecret, const std::string& code);
}