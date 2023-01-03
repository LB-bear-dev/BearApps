#pragma once
#include <CharacterControlCore/WatchedFile.h>
#include <graphics/image-file.h>

namespace CharacterControlOBS
{
	class WatchedImage: public CharacterControlCore::WatchedFile<gs_image_file4>
	{
	public:
		WatchedImage( std::filesystem::path filename ): WatchedFile<gs_image_file4>( filename )
		{
			Update();
		}

	private:
		bool UpdateInternal() override;
		void DisposeInternal() override;
	};
}