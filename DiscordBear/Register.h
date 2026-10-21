#pragma once
#include <optional>

namespace DiscordBear
{
	struct OAuthParams
	{
		std::string token;
		// Seconds since the unix epoch.
		long long expiresAt;
		std::string refreshToken;
	};

	bool RegisterAppToDiscord(const std::string& appID);
	std::optional<OAuthParams> GetAuthToken(const std::string& appID, const std::string& clientSecret, const std::string& code);
	std::optional<OAuthParams> RefreshAuthToken(const std::string& appID, const std::string& clientSecret, const std::string& refreshToken);
	// False once the token is within a minute of expiring, so it doesn't run out on its way to discord.
	bool TokenStillValid(const OAuthParams& params);

	// Tokens are kept per app ID under HKCU, encrypted with DPAPI so only this Windows user can read them.
	// Reusing them lets us AUTHENTICATE without discord showing its authorization prompt on every connect.
	std::optional<OAuthParams> LoadCachedToken(const std::string& appID);
	bool SaveCachedToken(const std::string& appID, const OAuthParams& params);
	void ClearCachedToken(const std::string& appID);
}
