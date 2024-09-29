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
	};

	const char* GetName(void* unused)
	{
		UNUSED_PARAMETER(unused);
		return obs_module_text("DiscordChar");
	}

	void SetupCharacter(obs_data_t* settings, NamedCharacterDisplayInfo* namedCharacterInfo)
	{
		if (settings == nullptr || namedCharacterInfo == nullptr)
		{
			return;
		}

		ActiveCharacters::Get().MakeActiveCharacter(namedCharacterInfo->name, Character(namedCharacterInfo->name));

		if (Character* character = ActiveCharacters::Get().GetActiveCharacter(namedCharacterInfo->name))
		{
			character->AddComponent(std::make_unique<DiscordComponent>(DiscordComponent(namedCharacterInfo->ID)));
			character->DeclareAttributes();
			if (ImageLayer* defaultLayer = character->GetImageRoot().GetLayerByName("default"))
			{
				defaultLayer->CreateSlot("SpeakingState");
				defaultLayer->AddImageLibraryToLayer(obs_data_get_string(settings, "NotSpeakingImg"), "TALK");
				defaultLayer->AddImageLibraryToLayer(obs_data_get_string(settings, "SpeakingImg"), "SILENT");
			}

			namedCharacterInfo->size = character->GetImageRoot().GetMaxResolution();
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

		SetupCharacter(settings, characterInfo);

		return characterInfo;
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
		obs_properties_add_text(settings, "ID", obs_module_text("Discord ID"), OBS_TEXT_DEFAULT);
		obs_properties_add_path(settings, "NotSpeakingImg", obs_module_text("Image when speaking"), OBS_PATH_FILE, "*.*", GetGlobalSettingsPath().value_or("").string().c_str());
		obs_properties_add_path(settings, "SpeakingImg", obs_module_text("Image when not speaking"), OBS_PATH_FILE, "*.*", GetGlobalSettingsPath().value_or("").string().c_str());

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

		SetupCharacter(settings, characterInfo);
	}

	void Tick(void* data, float seconds)
	{
		UNUSED_PARAMETER(seconds);

		if (data == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;

		if (Character* character = ActiveCharacters::Get().GetActiveCharacter(characterInfo->name))
		{
			character->UpdateAttributes();
			
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
	}

	void Render(void* data, gs_effect_t* effect)
	{
		if (data == nullptr || effect == nullptr)
		{
			return;
		}

		NamedCharacterDisplayInfo* characterInfo = (NamedCharacterDisplayInfo*)data;

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
	//show = ,
	//hide = ,
	.video_tick = Tick,
	.video_render = Render,
	.icon_type = OBS_ICON_TYPE_CUSTOM,
};