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
#include "../DiscordBear/Interface.h"

using namespace CharacterControlOBS;
using namespace CharacterControlRender;

namespace
{
	struct NamedCharacterDisplayInfo
	{
		CharacterControlRender::Coord size;
		STR name;
		STR ID;
		STR NotSpeakingImg;
		STR SpeakingImg;
		bool active = false;
	};

	const char* GetName(void* unused)
	{
		UNUSED_PARAMETER(unused);
		return obs_module_text("Simple Discord Char");
	}

	void SetupCharacter(NamedCharacterDisplayInfo* namedCharacterInfo)
	{
		if (namedCharacterInfo == nullptr)
		{
			return;
		}

		ActiveCharacters::Get().MakeActiveCharacter(namedCharacterInfo->name, Character(namedCharacterInfo->name));

		if (Character* character = ActiveCharacters::Get().GetActiveCharacter(namedCharacterInfo->name))
		{
			character->AddComponent(std::make_unique<DiscordComponent>(DiscordComponent(namedCharacterInfo->ID)));
			if (ImageLayer* defaultLayer = character->GetImageRoot().GetLayerByName("default"))
			{
				defaultLayer->CreateSlot("SpeakingState");
				defaultLayer->AddImageLibraryToLayer(namedCharacterInfo->SpeakingImg, "TALK");
				defaultLayer->AddImageLibraryToLayer(namedCharacterInfo->NotSpeakingImg, "SILENT");
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
		characterInfo->NotSpeakingImg = obs_data_get_string(settings, "NotSpeakingImg");
		characterInfo->SpeakingImg = obs_data_get_string(settings, "SpeakingImg");
		characterInfo->active = obs_source_active(source);

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

		obs_properties_add_text(settings, "Name", obs_module_text("Character Name"), OBS_TEXT_DEFAULT);
		obs_properties_add_text(settings, "ID", obs_module_text("Discord ID"), OBS_TEXT_DEFAULT);
		obs_properties_add_path(settings, "SpeakingImg", obs_module_text("Image when speaking"), OBS_PATH_FILE, "*.*", GetGlobalSettingsPath().value_or("").string().c_str());
		obs_properties_add_path(settings, "NotSpeakingImg", obs_module_text("Image when not speaking"), OBS_PATH_FILE, "*.*", GetGlobalSettingsPath().value_or("").string().c_str());

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
		characterInfo->NotSpeakingImg = obs_data_get_string(settings, "NotSpeakingImg");
		characterInfo->SpeakingImg = obs_data_get_string(settings, "SpeakingImg");

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
		if (!characterInfo->active || !CharacterControlOBS::IDIsActiveInDiscord(characterInfo->ID))
		{
			if (Character* character = ActiveCharacters::Get().GetActiveCharacter(characterInfo->name))
			{
				ActiveCharacters::Get().RemoveActiveCharacter(characterInfo->name);
			}
			return;
		}

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

obs_source_info simpleDiscordCharacterSourceInfo = {
	.id = "SimpleDiscordCharacter",
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