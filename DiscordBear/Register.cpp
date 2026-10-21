#include "PCH.h"
#include "Register.h"
#include "Log.h"
#include <cpprest/http_client.h>

bool DiscordBear::RegisterAppToDiscord(const std::string& appID)
{
	LOG(LogSeverity::Info) << "Registering app with client ID " << appID << " to discord";

	char exeFilePath[MAX_PATH];
	DWORD exeLen = GetModuleFileNameA(nullptr, exeFilePath, MAX_PATH);

	std::stringstream protocolName;
	protocolName << "discord-" << appID;

	std::stringstream protocolDescription;
	protocolDescription << "URL:Run game " << appID << " protocol";

	std::stringstream keyName;
	keyName << "Software\\Classes\\" << protocolName.str();

	HKEY key;
	auto status = RegCreateKeyExA(HKEY_CURRENT_USER, keyName.str().c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr);
	if (status != ERROR_SUCCESS)
	{
		LOG(LogSeverity::Severe) << "Could not write registry key " << keyName.str() << " so we can't authenticate";
		return false;
	}

	struct KeyGuard
	{
		HKEY key;
		~KeyGuard() { RegCloseKey(key); }
	} keyGuard{key};

	// Registry calls return Win32 error codes (positive), so FAILED() would never trip on them.
	const auto succeeded = [](LSTATUS result, const char* what)
		{
			if (result != ERROR_SUCCESS)
			{
				LOG(LogSeverity::Severe) << "Could not set " << what << ", error " << result << " so we can't authenticate";
				return false;
			}

			return true;
		};

	const std::string description = protocolDescription.str();
	if (!succeeded(RegSetKeyValueA(key, nullptr, nullptr, REG_SZ, description.c_str(), (DWORD)description.length() + 1), "description"))
	{
		return false;
	}

	char urlProtocol = 0;
	if (!succeeded(RegSetKeyValueA(key, nullptr, "URL Protocol", REG_SZ, &urlProtocol, sizeof(urlProtocol)), "URL Protocol"))
	{
		return false;
	}

	if (!succeeded(RegSetKeyValueA(key, "DefaultIcon", nullptr, REG_SZ, exeFilePath, (exeLen + 1) * sizeof(char)), "Default Icon"))
	{
		return false;
	}

	if (!succeeded(RegSetKeyValueA(key, "shell\\open\\command", nullptr, REG_SZ, exeFilePath, (exeLen + 1) * sizeof(char)), "open command"))
	{
		return false;
	}

	LOG(LogSeverity::Info) << "Registered app with client ID " << appID << " to discord";
	return true;
}

std::optional<DiscordBear::OAuthParams> DiscordBear::GetAuthToken(const std::string& appID, const std::string& clientSecret, const std::string& code)
{
	try
	{
		web::http::client::http_client client(U("https://discord.com/api/oauth2/"));

		utf8stringstream oauthString;
		oauthString << "grant_type=authorization_code&client_id=" << appID << "&code=" << code << "&client_secret=" << clientSecret;

		// The request body holds the client secret and auth code, never log it.
		LOG(LogSeverity::Info) << "Exchanging authorization code for client ID " << appID;

		auto httpResponse = client.request(web::http::methods::POST, "token", oauthString.str(), "application/x-www-form-urlencoded").get();
		json parsedResponse = json::parse(httpResponse.extract_utf8string().get(), nullptr, false);

		if (httpResponse.status_code() != web::http::status_codes::OK || parsedResponse.is_discarded() || !parsedResponse.contains("access_token"))
		{
			LOG(LogSeverity::Severe) << "Token exchange failed, HTTP status " << httpResponse.status_code();
			return std::nullopt;
		}

		return OAuthParams{parsedResponse.value("access_token", ""), parsedResponse.value("expires_in", 0ul), parsedResponse.value("refresh_token", "")};
	}
	catch (const std::exception& e)
	{
		LOG(LogSeverity::Severe) << "Token exchange threw an exception: " << e.what();
		return std::nullopt;
	}
}