#pragma once
#include "WatchedFile.h"
namespace CharacterControlCore
{
	class WatchedJson final : public WatchedFile<json>
	{
	public:
		WatchedJson( std::filesystem::path filename ) : WatchedFile<json>( filename )	
		{
			Update();
		}
	private:
		bool UpdateInternal() override;
	};
}