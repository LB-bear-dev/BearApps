#include "WatchedImage.h"
#include <obs-module.h>
#include "Messaging.h"

namespace
{
	void UnloadImage( gs_image_file4* image )
	{
		if ( image != nullptr )
		{
			obs_enter_graphics();
			gs_image_file4_free( image );
			obs_leave_graphics();
		}
	}

	void LoadImage( const char* fileName, gs_image_file4* image )
	{
		UnloadImage( image );

		if ( fileName != nullptr && strlen(fileName) >= 0 )
		{
			debug( "loading texture '%s'", fileName );
			gs_image_file4_init( image, fileName, GS_IMAGE_ALPHA_PREMULTIPLY );

			obs_enter_graphics();
			gs_image_file4_init_texture( image );
			obs_leave_graphics();

			if ( !image->image3.image2.image.loaded )
			{
				warn( "failed to load texture '%s'", fileName );
			}
		}
	}
}

bool CharacterControlOBS::WatchedImage::UpdateInternal()
{ 
	LoadImage( this->GetPath().string().c_str(), GetMutable());

	return true;
}

void CharacterControlOBS::WatchedImage::DisposeInternal()
{
	UnloadImage( GetMutable() );
}
