#pragma once
#include "ThreadSafe.h"
#include <filesystem>

using namespace std::chrono_literals;
namespace CharacterControlCore
{
	template <typename T>
	class WatchedFile
	{
	public:
		WatchedFile( const std::filesystem::path& filename ): m_filename( filename ), m_lastChangedTime(), m_updateInterval(10s), m_lastUpdate(std::chrono::system_clock::duration::zero())
		{
		}

		WatchedFile( const WatchedFile& ) = delete;
		WatchedFile& operator=( const WatchedFile& ) = delete;

		WatchedFile(WatchedFile&& rhs) :
			m_filename(std::move(rhs.m_filename)),
			m_lastChangedTime(std::move(rhs.m_lastChangedTime)),
			m_updateInterval(std::move(rhs.m_updateInterval))
		{
			m_file = std::move(rhs.m_file);
		}

		~WatchedFile() 
		{
			DisposeInternal();
		}

		bool Update()
		{
			if (std::chrono::system_clock::now() - m_lastUpdate <= m_updateInterval)
			{
				return true;
			}

			m_lastUpdate = std::chrono::system_clock::now();

			if ( std::filesystem::exists( m_filename ) )
			{
				auto duration = std::filesystem::last_write_time(m_filename) - m_lastChangedTime;
				if (duration > std::chrono::system_clock::duration::zero())
				{
					DisposeInternal();
					m_file = std::make_unique<T>();
					if (!UpdateInternal())
					{
						m_file = nullptr;
						return false;
					}
					m_lastChangedTime = std::filesystem::last_write_time(m_filename);
					return true;
				}
			}

			return false;
		}

		void SetUpdateInterval( std::filesystem::file_time_type interval )
		{
			m_updateInterval = interval;
		}

		const std::filesystem::path& GetPath() const
		{
			return m_filename;
		}

		const T* Get() const
		{
			return m_file.get();
		}

	protected:
		virtual bool UpdateInternal() = 0;
		virtual void DisposeInternal() {}
		T* GetMutable() const
		{
			return m_file.get();
		}

	private:
		std::filesystem::path m_filename;
		std::filesystem::file_time_type m_lastChangedTime;
		std::chrono::system_clock::time_point m_lastUpdate;
		std::chrono::system_clock::duration m_updateInterval;
		std::unique_ptr<T> m_file;
	};
}
