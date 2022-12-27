/*
DiscordBear_OBS
Copyright (C) <Year> <Developer> <Email Address>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <graphics/image-file.h>
#include <string>
#include <CharacterControlCore/Interface.h>
#include <unordered_map>
#include <utility>

#include "plugin-macros.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)

namespace
{
	void UnloadImage ( gs_image_file4* image )
	{
		obs_enter_graphics ();
		gs_image_file4_free ( image );
		obs_leave_graphics ();
	}

	void LoadImage (std::string fileName, gs_image_file4* image)
	{
		UnloadImage ( image );

		if (!fileName.empty()) 
		{
			debug("loading texture '%s'", fileName.c_str());
			gs_image_file4_init(image, fileName.c_str(), GS_IMAGE_ALPHA_PREMULTIPLY);

			obs_enter_graphics();
			gs_image_file4_init_texture(image);
			obs_leave_graphics();

			if (!image->image3.image2.image.loaded) 
			{
				warn("failed to load texture '%s'", fileName.c_str());
			}
		}
	}
}

namespace
{
	struct DiscordImage
	{
		CharacterControlCore::ControlCorePtr characterControl;
		std::unordered_map<size_t, gs_image_file4> images;

		gs_image_file4* currentImage;
	};

	const char* GetName(void* unused)
	{
		UNUSED_PARAMETER(unused);
		return obs_module_text("DiscordBear");
	}

	void* Create(obs_data_t* settings, obs_source_t* source)
	{
		UNUSED_PARAMETER ( settings );
		UNUSED_PARAMETER ( source );
		DiscordImage* image = new DiscordImage;
		try
		{
			image->characterControl = CharacterControlCore::GetCharacterControl ( "C:\\Users\\chris\\OneDrive\\Documents\\OBSCharacterSettings\\globalSettings.json" );
		}
		catch ( ... ){}

		image->currentImage = nullptr;
		return image;
	}

	void Destroy(void *data)
	{
		DiscordImage* image = (DiscordImage*)data;
		delete image;
	}

	void Update(void *data, obs_data_t *settings) 
	{
		DiscordImage* image = (DiscordImage*)data;
		image->characterControl->Update();
		UNUSED_PARAMETER(settings);
		UNUSED_PARAMETER(data);
	}

	uint32_t GetWidth(void *data)
	{
		DiscordImage* image4 = (DiscordImage*)data;
		if ( image4->currentImage != nullptr )
		{
			gs_image_file* image = &image4->currentImage->image3.image2.image;

			return image->cx;
		}

		return 0;
	}

	uint32_t GetHeight(void *data)
	{
		DiscordImage* image4 = (DiscordImage*)data;
		if ( image4->currentImage != nullptr )
		{
			gs_image_file* image = &image4->currentImage->image3.image2.image;

			return image->cy;
		}

		return 0;
	}

	void Render(void* data, gs_effect_t* effect)
	{
		DiscordImage* image4 = (DiscordImage*)data;
		if ( image4->currentImage == nullptr )
		{
			return ;
		}

		gs_image_file* image = &image4->currentImage->image3.image2.image;
		gs_texture_t* texture = image->texture;
		if ( texture == nullptr )
		{
			return;
		}

		bool previous = gs_framebuffer_srgb_enabled();
		gs_enable_framebuffer_srgb(true);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

		gs_eparam_t* param = gs_effect_get_param_by_name(effect, "image");
		gs_effect_set_texture_srgb(param, texture);

		gs_draw_sprite(texture, 0, image->cx, image->cy);

		gs_blend_state_pop();

		gs_enable_framebuffer_srgb(previous);
	}

	void Tick( void* data, float seconds )
	{
		UNUSED_PARAMETER( seconds );

		DiscordImage* image = (DiscordImage*)data;
		image->characterControl->Update();
		auto& characterList = image->characterControl->GetCharacterList();
		auto foundCharacter = std::find_if( characterList.begin(), characterList.end(), []( const CharacterControlCore::CharacterInfo& info )
		{
			return info.ID == "186199041414004739";
		} );

		if ( foundCharacter != characterList.end() )
		{
			std::hash<std::string> stringHash;
			size_t hash = stringHash( foundCharacter->imageInfos[0].imageName );

			auto foundImage = image->images.find( hash );
			if ( foundImage == image->images.end() )
			{
				LoadImage( foundCharacter->imageInfos[0].imageName, &image->images[hash] );
			}

			image->currentImage = &image->images[hash];
		}
		else
		{
			image->currentImage = nullptr;
		}
	}

	static struct obs_source_info sourceInfo = {
		.id="DiscordBear",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
		.get_name = GetName,
		.create = Create,
		.destroy = Destroy,
		.get_width = GetWidth,
		.get_height = GetHeight,
		.update = Update,
		//get_defaults = pluginUpdate,
		//show = image_source_show,
		//hide = image_source_hide,
		.video_tick = Tick,
		.video_render = Render,
		//missing_files = image_source_missingfiles,
		//get_properties = image_source_properties,
		//icon_type = OBS_ICON_TYPE_IMAGE,
		//activate = image_source_activate,
		//video_get_color_space = image_source_get_color_space,
	};
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "plugin loaded successfully (version %s)",
	     PLUGIN_VERSION);

	obs_register_source(&sourceInfo);

	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "plugin unloaded");
}
