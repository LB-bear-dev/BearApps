#include "PCH.h"
#include "ActiveCharacters.h"
#include "Coord.h"
#include "Character.h"
#include "Messaging.h"
#include <obs-module.h>
#include "WatchedImage.h"
#include <QtWidgets\QMenu.h>
#include "DiscordComponent.h"

#pragma optimize("", off)

namespace
{
	struct NamedCharacterDisplayInfo
	{
		CharacterControlRender::Coord size;
		std::string name;
	};


	const char* GetName(void* unused)
	{
		UNUSED_PARAMETER(unused);
		return obs_module_text("NamedCharacterSlot");
	}

	void* Create(obs_data_t* settings, obs_source_t* source)
	{
		if (source == nullptr || settings == nullptr)
		{
			return nullptr;
		}

		UNUSED_PARAMETER(source);

		NamedCharacterDisplayInfo* namedCharacterInfo = new NamedCharacterDisplayInfo();
		namedCharacterInfo->name = obs_data_get_string(settings, "Name");
		//CharacterControlOBS::ActiveCharacters::Get().MakeActiveCharacter(namedCharacterInfo->name, CharacterControlOBS::Character(namedCharacterInfo->name, CharacterControlOBS::GetGlobalSettingsPath().value_or(""), obs_data_get_string(settings, "Path to action script")));

		if (CharacterControlOBS::Character* character = CharacterControlOBS::ActiveCharacters::Get().GetActiveCharacter(namedCharacterInfo->name))
		{
			character->AddComponent(std::make_unique<CharacterControlOBS::DiscordComponent>(CharacterControlOBS::DiscordComponent(namedCharacterInfo->name)));
		}
		namedCharacterInfo->size.X = 100;
		namedCharacterInfo->size.Y = 100;

		return namedCharacterInfo;
	}

	void Destroy(void* data)
	{
		if (data == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;
		delete characterInfo;
	}

	uint32_t GetWidth(void* data)
	{
		if (data == nullptr)
		{
			return 100;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;

		return characterInfo->size.X;
	}

	uint32_t GetHeight(void* data)
	{
		if (data == nullptr)
		{
			return 100;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;

		return characterInfo->size.Y;
	}

	void GetDefaults(obs_data_t* settings)
	{
		if (settings == nullptr)
		{
			return;
		}

		obs_data_set_default_string(settings, "Name", "");
	}

	obs_properties_t* GetProperties(void* data)
	{
		obs_properties_t* settings = obs_properties_create();

		if (data == nullptr)
		{
			return settings;
		}

		obs_properties_add_text(settings, "Name", obs_module_text("Character Name"), OBS_TEXT_DEFAULT);
		obs_properties_add_path(settings, "Path to action script", obs_module_text("Action Script"), OBS_PATH_FILE, "*.py", CharacterControlOBS::GetGlobalSettingsPath().value_or("").string().c_str());

		return settings;
	}

	void Update(void* data, obs_data_t* settings)
	{
		if (data == nullptr || settings == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;

		CharacterControlOBS::ActiveCharacters::Get().RemoveActiveCharacter(characterInfo->name);

		if (const char* name = obs_data_get_string(settings, "Name"))
		{
			characterInfo->name = name;
		}

		//CharacterControlOBS::ActiveCharacters::Get().MakeActiveCharacter(characterInfo->name, CharacterControlOBS::Character(characterInfo->name, CharacterControlOBS::GetGlobalSettingsPath().value_or(""), obs_data_get_string(settings, "Path to action script")));

		if (CharacterControlOBS::Character* character = CharacterControlOBS::ActiveCharacters::Get().GetActiveCharacter(characterInfo->name))
		{
			character->AddComponent(std::make_unique<CharacterControlOBS::DiscordComponent>(CharacterControlOBS::DiscordComponent(characterInfo->name)));
		}
	}

	void Tick(void* data, float seconds)
	{
		UNUSED_PARAMETER(seconds);

		if (data == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;

		if (CharacterControlOBS::Character* character = CharacterControlOBS::ActiveCharacters::Get().GetActiveCharacter(characterInfo->name))
		{
			character->UpdateAttributes();
			character->UpdateRenderState();
		}
	}

	void Render(void* data, gs_effect_t* effect)
	{
		if (data == nullptr || effect == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;

		if (const CharacterControlOBS::Character* character = CharacterControlOBS::ActiveCharacters::Get().GetActiveCharacter(characterInfo->name))
		{
			character->Render(effect);
		}
	}

	void MouseClick(void* data, const struct obs_mouse_event* event,
		int32_t type, bool mouse_up, uint32_t click_count)
	{
		if (data == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;

		if (mouse_up)
		{
			QMenu popup(nullptr);

			popup.addAction("Add");
			popup.exec(QCursor::pos());
			if (CharacterControlOBS::Character* character = CharacterControlOBS::ActiveCharacters::Get().GetActiveCharacter(characterInfo->name))
			{
				character->GetImageRoot().GetLayerByName("default")->SetSlot(STRID("SpeakStatus"), STRID("SILENT"));
			}
		}
	}

	void Focus(void* data, bool focus)
	{
		if (data == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;

		if (focus)
		{
			if (CharacterControlOBS::Character* character = CharacterControlOBS::ActiveCharacters::Get().GetActiveCharacter(characterInfo->name))
			{
				character->GetImageRoot().GetLayerByName("default")->SetSlot("SpeakStatus", "TALK");
			}
		}
	}
}

obs_source_info namedCharacterSourceInfo = {
	.id = "NamedCharacterSlot",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB | OBS_SOURCE_INTERACTION,
	.get_name = GetName,
	.create = Create,
	.destroy = Destroy,
	.get_width = GetWidth,
	.get_height = GetHeight,
	.get_defaults = GetDefaults,
	.get_properties = GetProperties,
	.update = Update,
	//show = ,
	//hide = ,
	.video_tick = Tick,
	.video_render = Render,
	//missing_files = ,
	.mouse_click = MouseClick,
	.focus = Focus,
	.icon_type = OBS_ICON_TYPE_CUSTOM,
	//activate = ,
	//video_get_color_space = ,
};