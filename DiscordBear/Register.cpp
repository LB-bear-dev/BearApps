#include "PCH.h"
#include "Register.h"
#include "Log.h"
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

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

	LSTATUS result = RegSetKeyValueA(key, nullptr, nullptr, REG_SZ, protocolDescription.str().c_str(), (DWORD)protocolDescription.str().length());
	if (FAILED(result))
	{
		LOG(LogSeverity::Severe) << "Could not set description " << protocolDescription.str() << " so we can't authenticate";
		return false;
	}

	char urlProtocol = 0;
	result = RegSetKeyValueA(key, nullptr, "URL Protocol", REG_SZ, &urlProtocol, sizeof(urlProtocol));
	if (FAILED(result))
	{
		LOG(LogSeverity::Severe) << "Could not set URL Protocol " << protocolDescription.str() << " so we can't authenticate";
		return false;
	}

	result = RegSetKeyValueA(key, "DefaultIcon", nullptr, REG_SZ, exeFilePath, (exeLen + 1) * sizeof(char));
	if (FAILED(result))
	{
		LOG(LogSeverity::Severe) << "Could not set Default Icon " << protocolDescription.str() << " so we can't authenticate";
		return false;
	}

	result = RegSetKeyValueA(key, "shell\\open\\command", nullptr, REG_SZ, exeFilePath, (exeLen + 1) * sizeof(char));
	if (FAILED(result))
	{
		LOG(LogSeverity::Severe) << "Could not set open command " << protocolDescription.str() << " so we can't authenticate";
		return false;
	}
	
	LOG(LogSeverity::Info) << "Registered app with client ID " << appID << " to discord";
	RegCloseKey(key);
	return true;
}

DiscordBear::OAuthParams DiscordBear::GetAuthToken(const std::string& appID, const std::string& clientSecret, const std::string& code)
{
	web::http::client::http_client client(U("https://discord.com/api/oauth2/"));

	utf8stringstream oauthString;
	oauthString << "grant_type=authorization_code&client_id=" << appID << "&code=" << code << "&client_secret=" << clientSecret;

	LOG(LogSeverity::Info) << "Sending OAuth info: " << oauthString.str();

	utf8string response = client.request(web::http::methods::POST, "token", oauthString.str(), "application/x-www-form-urlencoded").get().extract_utf8string().get();
	json parsedResponse = json::parse(response);
	

	return { parsedResponse["access_token"], parsedResponse["expires_in"], parsedResponse["refresh_token"]};
}
