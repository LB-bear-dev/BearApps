#include "PCH.h"
#include "WatchedJson.h"

CharacterControlCore::WatchedJson::WatchedJson(std::filesystem::path filename) :
	m_filename(filename), m_lastChangedTime()
{
}

bool CharacterControlCore::WatchedJson::Update()
{
	if (std::filesystem::exists(m_filename) && std::filesystem::last_write_time(m_filename) != m_lastChangedTime)
	{
		return true;
	}

	return false;
}

std::optional<json> CharacterControlCore::WatchedJson::Get()
{
	try
	{
		json jsonFile;
		std::ifstream characterControlMasterFileStream(m_filename);
		characterControlMasterFileStream >> jsonFile;
		characterControlMasterFileStream.close();

		m_lastChangedTime = std::filesystem::last_write_time(m_filename);

		return std::move(jsonFile);
	}
	catch (...)
	{
	}
	return {};
}

const std::filesystem::path& CharacterControlCore::WatchedJson::GetPath() const
{
	return m_filename;
}
