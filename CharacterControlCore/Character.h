#pragma once
#include "WatchedJson.h"
#include "Attributes.h"
#include "ImageResource.h"
#include "VisualLayer.h"
namespace CharacterControlCore
{
	class Character
	{
	public:
		void Update();

		const Attributes& GetAttributes() const;
		Attributes& GetAttributesMutable();
		const VisualLayer& GetVisualLayerRoot() const;
		const ImageResources& GetImageResources() const;
	private:
		friend class CharacterCreator;
		Character(std::filesystem::path filename);
		WatchedJson m_characterJson;
		Attributes m_attributes;
		VisualLayer m_visualLayerRoot;
		ImageResources m_imageResources;

		void Parse();
	};
}