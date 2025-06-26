#include "PCH.h"
#include "ActiveCharacters.h"
#include "Coord.h"
#include "Character.h"
#include "Messaging.h"
#include <obs-module.h>
#include "WatchedImage.h"
#include <QtWidgets\QMenu.h>
#include "DiscordComponent.h"
#include "WatchedImage.h"

using namespace CharacterControlOBS;
using namespace CharacterControlRender;

namespace
{
	struct NamedCharacterDisplayInfo
	{
		CharacterControlRender::Coord size;
		STR name;
		STR ID;
		STR scriptFile;
		bool active;
	};

	const char* GetName(void* unused)
	{
		UNUSED_PARAMETER(unused);
		return obs_module_text("Scripted Discord Char");
	}

	void SetupCharacter(NamedCharacterDisplayInfo* namedCharacterInfo)
	{
		if (namedCharacterInfo == nullptr)
		{
			return;
		}

		if (Character* character = ActiveCharacters::Get().GetActiveCharacter(namedCharacterInfo->name))
		{
			character->AddComponent(std::make_unique<DiscordComponent>(DiscordComponent(namedCharacterInfo->ID)));
		}
	}

	void* Create(obs_data_t* settings, obs_source_t* source)
	{
		if (source == nullptr || settings == nullptr)
		{
			return nullptr;
		}

		UNUSED_PARAMETER(source);

		NamedCharacterDisplayInfo* characterInfo = new NamedCharacterDisplayInfo();
		characterInfo->name = obs_data_get_string(settings, "Name");
		characterInfo->ID = obs_data_get_string(settings, "ID");
		characterInfo->scriptFile = obs_data_get_string(settings, "ScriptFile");
		characterInfo->active = obs_source_active(source);
		SetupCharacter(characterInfo);

		return characterInfo;
	}

	void Destroy(void* data)
	{
		if (data == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;
		ActiveCharacters::Get().RemoveActiveCharacter(characterInfo->name);
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

		obs_properties_add_text(settings, "Name", obs_module_text("Name"), OBS_TEXT_DEFAULT);
		obs_properties_add_text(settings, "ID", obs_module_text("Discord ID"), OBS_TEXT_DEFAULT);
		obs_properties_add_path(settings, "ScriptFile", obs_module_text("Script File"), OBS_PATH_FILE, "*.py", GetGlobalSettingsPath().value_or("").string().c_str());

		return settings;
	}

	void Update(void* data, obs_data_t* settings)
	{
		if (data == nullptr || settings == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;
		ActiveCharacters::Get().RemoveActiveCharacter(characterInfo->name);

		characterInfo->name = obs_data_get_string(settings, "Name");
		characterInfo->ID = obs_data_get_string(settings, "ID");
		characterInfo->scriptFile = obs_data_get_string(settings, "ScriptFile");

		SetupCharacter(characterInfo);
	}

	void Activate(void* data)
	{
		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;
		SetupCharacter(characterInfo);
		characterInfo->active = true;
	}

	void Deactivate(void* data)
	{
		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;
		ActiveCharacters::Get().RemoveActiveCharacter(characterInfo->name);
		characterInfo->active = false;
	}

	void Show(void* data)
	{
		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;
		SetupCharacter(characterInfo);
		characterInfo->active = true;
	}

	void Hide(void* data)
	{
		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;
		ActiveCharacters::Get().RemoveActiveCharacter(characterInfo->name);
		characterInfo->active = false;
	}

	void Tick(void* data, float seconds)
	{
		UNUSED_PARAMETER(seconds);

		if (data == nullptr)
		{
			return;
		}
		 
		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;
		if (!characterInfo->active)
		{
			return;
		}

		if (Character* character = ActiveCharacters::Get().GetActiveCharacter(characterInfo->name))
		{
			character->UpdateAttributes();
			character->UpdateRenderState();
			
			if (const Attribute<int>* attr = character->GetIntAttribute("Talking"))
			{
				character->GetImageRoot().SetSlot("SpeakingState", attr->GetValue() == 0 ? "SILENT" : "TALK");
			}

			if (const Attribute<int>* attr = character->GetIntAttribute("InCall"))
			{
				if (attr->GetValue() > 0)
				{
					character->GetImageRoot().Activate();
				}
				else
				{
					character->GetImageRoot().Deactivate();
				}
			}
		}
		else
		{
			SetupCharacter(characterInfo);
		}
	}

	void Render(void* data, gs_effect_t* effect)
	{
		if (data == nullptr || effect == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;
		
		if (!characterInfo->active)
		{
			return;
		}

		if (const Character* character = ActiveCharacters::Get().GetActiveCharacter(characterInfo->name))
		{
			character->Render(effect);
		}
	}
}

obs_source_info scriptedDiscordCharacterSourceInfo = {
	.id = "ScriptedDiscordCharacter",
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
	.activate = Activate,
	.deactivate = Deactivate,
	.show = Show,
	.hide = Hide,
	.video_tick = Tick,
	.video_render = Render,
	.icon_type = OBS_ICON_TYPE_CUSTOM,
};