#include "PCH.h"
#include "Register.h"
#include "Log.h"
#include <cpprest/http_client.h>
#include <dpapi.h>
#include <vector>

#pragma comment(lib, "crypt32.lib")

using namespace DiscordBear;

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

namespace
{
	const char* TokenKeyName = "Software\\BearApps\\DiscordBear";

	long long UnixNow()
	{
		return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	// Posts to discord's token endpoint, both the authorization code and the refresh token grants go through here.
	std::optional<DiscordBear::OAuthParams> RequestToken(const utf8string& requestBody, const char* what)
	{
		try
		{
			web::http::client::http_client client(U("https://discord.com/api/oauth2/"));

			auto httpResponse = client.request(web::http::methods::POST, "token", requestBody, "application/x-www-form-urlencoded").get();
			json parsedResponse = json::parse(httpResponse.extract_utf8string().get(), nullptr, false);

			if (httpResponse.status_code() != web::http::status_codes::OK || parsedResponse.is_discarded() || !parsedResponse.contains("access_token"))
			{
				LOG(LogSeverity::Severe) << what << " failed, HTTP status " << httpResponse.status_code();
				return std::nullopt;
			}

			return DiscordBear::OAuthParams{parsedResponse.value("access_token", ""), UnixNow() + parsedResponse.value("expires_in", 0ll), parsedResponse.value("refresh_token", "")};
		}
		catch (const std::exception& e)
		{
			LOG(LogSeverity::Severe) << what << " threw an exception: " << e.what();
			return std::nullopt;
		}
	}
}

std::optional<DiscordBear::OAuthParams> DiscordBear::GetAuthToken(const std::string& appID, const std::string& clientSecret, const std::string& code)
{
	utf8stringstream oauthString;
	oauthString << "grant_type=authorization_code&client_id=" << appID << "&code=" << code << "&client_secret=" << clientSecret;

	// The request body holds the client secret and auth code, never log it.
	LOG(LogSeverity::Info) << "Exchanging authorization code for client ID " << appID;

	return RequestToken(oauthString.str(), "Token exchange");
}

std::optional<DiscordBear::OAuthParams> DiscordBear::RefreshAuthToken(const std::string& appID, const std::string& clientSecret, const std::string& refreshToken)
{
	utf8stringstream oauthString;
	oauthString << "grant_type=refresh_token&client_id=" << appID << "&refresh_token=" << refreshToken << "&client_secret=" << clientSecret;

	// The request body holds the client secret and refresh token, never log it.
	LOG(LogSeverity::Info) << "Refreshing auth token for client ID " << appID;

	return RequestToken(oauthString.str(), "Token refresh");
}

bool DiscordBear::TokenStillValid(const OAuthParams& params)
{
	return params.expiresAt - UnixNow() > 60;
}

std::optional<DiscordBear::OAuthParams> DiscordBear::LoadCachedToken(const std::string& appID)
{
	DWORD size = 0;
	if (RegGetValueA(HKEY_CURRENT_USER, TokenKeyName, appID.c_str(), RRF_RT_REG_BINARY, nullptr, nullptr, &size) != ERROR_SUCCESS || size == 0)
	{
		return std::nullopt;
	}

	std::vector<BYTE> encrypted(size);
	if (RegGetValueA(HKEY_CURRENT_USER, TokenKeyName, appID.c_str(), RRF_RT_REG_BINARY, nullptr, encrypted.data(), &size) != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	DATA_BLOB in{size, encrypted.data()};
	DATA_BLOB out{};
	if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out))
	{
		LOG(LogSeverity::NonSevere) << "Could not decrypt the cached token for client ID " << appID << ", error " << GetLastError();
		return std::nullopt;
	}

	json parsed = json::parse(std::string(reinterpret_cast<const char*>(out.pbData), out.cbData), nullptr, false);
	SecureZeroMemory(out.pbData, out.cbData);
	LocalFree(out.pbData);

	if (parsed.is_discarded() || !parsed.contains("access_token") || !parsed.contains("refresh_token"))
	{
		LOG(LogSeverity::NonSevere) << "Cached token for client ID " << appID << " is malformed, ignoring it";
		return std::nullopt;
	}

	return OAuthParams{parsed.value("access_token", ""), parsed.value("expires_at", 0ll), parsed.value("refresh_token", "")};
}

bool DiscordBear::SaveCachedToken(const std::string& appID, const OAuthParams& params)
{
	std::string plain = json{{"access_token", params.token}, {"expires_at", params.expiresAt}, {"refresh_token", params.refreshToken}}.dump();

	DATA_BLOB in{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE*>(plain.data())};
	DATA_BLOB out{};
	const BOOL encrypted = CryptProtectData(&in, L"DiscordBear token", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out);
	SecureZeroMemory(plain.data(), plain.size());

	if (!encrypted)
	{
		LOG(LogSeverity::NonSevere) << "Could not encrypt the token for client ID " << appID << ", error " << GetLastError();
		return false;
	}

	HKEY key;
	LSTATUS status = RegCreateKeyExA(HKEY_CURRENT_USER, TokenKeyName, 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr);
	if (status == ERROR_SUCCESS)
	{
		status = RegSetValueExA(key, appID.c_str(), 0, REG_BINARY, out.pbData, out.cbData);
		RegCloseKey(key);
	}
	LocalFree(out.pbData);

	if (status != ERROR_SUCCESS)
	{
		LOG(LogSeverity::NonSevere) << "Could not cache the token for client ID " << appID << ", error " << status;
		return false;
	}

	return true;
}

void DiscordBear::ClearCachedToken(const std::string& appID)
{
	RegDeleteKeyValueA(HKEY_CURRENT_USER, TokenKeyName, appID.c_str());
}
