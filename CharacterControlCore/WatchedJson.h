#pragma once
#include "ThreadSafe.h"
namespace CharacterControlCore
{
	class WatchedJson
	{
	public:
		WatchedJson(std::filesystem::path filename);
		bool Update();

		std::optional<json> Get();
		std::filesystem::path GetPath() const;

	private:
		std::filesystem::path m_filename;
		std::filesystem::file_time_type m_lastChangedTime;
	};
}