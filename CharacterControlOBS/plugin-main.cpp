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
#include <util/dstr.h>
#include <string>
#include <CharacterControlCore/Interface.h>
#include "DiscordCharacterInfo.h"
#include <utility>
#include <shlobj_core.h>

#include "plugin-macros.h"
#include "WatchedImage.h"

#pragma optimize("", off)

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

namespace
{
	std::optional<std::filesystem::path> GetGlobalSettingsPath()
	{
		try
		{
			char documentsFolder[MAX_PATH] = { 0 };
			HRESULT result = SHGetFolderPathA( NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, documentsFolder );

			if ( SUCCEEDED( result ) )
			{
				return std::make_optional( std::filesystem::path( documentsFolder ) / "OBSCharacterSettings" / "globalSettings.json" );
			}
		}
		catch ( ... )
		{
		}

		return {};
	}

	void CreateCharacterControl( obs_data_t* settings, CharacterControlOBS::DiscordCharacterInfo*& characterInfo )
	{
		if ( characterInfo != nullptr )
		{
			delete characterInfo;
		}

		if ( const char* id = obs_data_get_string( settings, "discordID" ) )
		{
			std::optional<std::filesystem::path> path = GetGlobalSettingsPath();
			if ( path.has_value() )
			{
				characterInfo = new CharacterControlOBS::DiscordCharacterInfo( id, path->string().c_str() );
			}
		}
	}


	const char* GetName(void* unused)
	{
		UNUSED_PARAMETER(unused);
		return obs_module_text("DiscordBear");
	}

	void* Create(obs_data_t* settings, obs_source_t* source)
	{
		if ( source == nullptr || settings == nullptr )
		{
			return nullptr;
		}

		UNUSED_PARAMETER ( source );

		CharacterControlOBS::DiscordCharacterInfo* characterInfo = nullptr;
		CreateCharacterControl( settings, characterInfo );

		return characterInfo;
	}

	void Destroy(void *data)
	{
		if ( data == nullptr )
		{
			return;
		}

		CharacterControlOBS::DiscordCharacterInfo* characterInfo = (CharacterControlOBS::DiscordCharacterInfo*)data;
		delete characterInfo;
	}

	uint32_t GetWidth(void *data)
	{
		if ( data == nullptr )
		{
			return 100;
		}

		CharacterControlOBS::DiscordCharacterInfo* characterInfo = (CharacterControlOBS::DiscordCharacterInfo*)data;
		if ( const CharacterControlOBS::WatchedImage* currentImage = characterInfo->GetCurrentImage() )
		{
			const gs_image_file* image = &currentImage->Get()->image3.image2.image;

			return image->cx;
		}

		return 100;
	}

	uint32_t GetHeight(void *data)
	{
		if ( data == nullptr )
		{
			return 100;
		}

		CharacterControlOBS::DiscordCharacterInfo* characterInfo = (CharacterControlOBS::DiscordCharacterInfo*)data;
		if ( const CharacterControlOBS::WatchedImage* currentImage = characterInfo->GetCurrentImage() )
		{
			const gs_image_file* image = &currentImage->Get()->image3.image2.image;

			return image->cy;
		}

		return 100;
	}

	void GetDefaults( obs_data_t* settings )
	{
		if ( settings == nullptr )
		{
			return;
		}

		obs_data_set_default_string( settings, "discordID", "0");
	}

	obs_properties_t* GetProperties( void* data )
	{
		obs_properties_t* settings = obs_properties_create();

		if ( data == nullptr )
		{
			return settings;
		}

		obs_properties_add_text( settings, "discordID", obs_module_text( "Discord ID" ), OBS_TEXT_DEFAULT);

		return settings;
	}

	void Update( void* data, obs_data_t* settings )
	{
		if ( data == nullptr || settings == nullptr )
		{
			return;
		}

		CharacterControlOBS::DiscordCharacterInfo* characterInfo = (CharacterControlOBS::DiscordCharacterInfo*)data;

		const char* id = obs_data_get_string( settings, "discordID" );
		if ( id != nullptr && strcmp(id, characterInfo->GetID()) != 0 )
		{
			CreateCharacterControl( settings, characterInfo );
		}
	}

	void Tick( void* data, float seconds )
	{
		UNUSED_PARAMETER( seconds );

		if ( data == nullptr )
		{
			return;
		}

		CharacterControlOBS::DiscordCharacterInfo* characterInfo = (CharacterControlOBS::DiscordCharacterInfo*)data;
		characterInfo->Update();

	}

	void Render(void* data, gs_effect_t* effect)
	{
		if ( data == nullptr || effect == nullptr )
		{
			return;
		}

		CharacterControlOBS::DiscordCharacterInfo* characterInfo = (CharacterControlOBS::DiscordCharacterInfo*)data;
		if ( const CharacterControlOBS::WatchedImage* currentImage = characterInfo->GetCurrentImage() )
		{
			const gs_image_file* image = &currentImage->Get()->image3.image2.image;

			if ( gs_texture_t* texture = image->texture )
			{
				bool previous = gs_framebuffer_srgb_enabled();
				gs_enable_framebuffer_srgb( true );

				gs_blend_state_push();
				gs_blend_function( GS_BLEND_ONE, GS_BLEND_INVSRCALPHA );

				gs_eparam_t* param = gs_effect_get_param_by_name( effect, "image" );
				gs_effect_set_texture_srgb( param, texture );

				gs_draw_sprite( texture, 0, image->cx, image->cy );

				gs_blend_state_pop();

				gs_enable_framebuffer_srgb( previous );
			}
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
		.get_defaults = GetDefaults,
		.get_properties = GetProperties,
		.update = Update,
		//show = image_source_show,
		//hide = image_source_hide,
		.video_tick = Tick,
		.video_render = Render,
		//missing_files = image_source_missingfiles,
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
