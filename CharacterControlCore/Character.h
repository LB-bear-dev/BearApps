#pragma once
#include "WatchedJson.h"
#include "Attributes.h"
#include "ImageResource.h"
namespace CharacterControlCore
{
	class Character
	{
	public:
		void Update();
	private:
		friend class CharacterCreator;
		Character(std::filesystem::path filename);
		WatchedJson m_characterJson;
		Attributes m_attributes;
		ImageResources m_imageResources;

		void Parse();
		
		friend void to_json(json& j, const Character& c);
		friend void from_json(const json& j, Character& c);
	};
}