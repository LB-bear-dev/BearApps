#pragma once
#include "WatchedJson.h"
#include "ImageResource.h"
#include "VisualLayer.h"
namespace CharacterControlCore
{
	class Character
	{
	public:
		Character( std::filesystem::path filename );
		void Update();

		const Attributes& GetAttributes() const;
		Attributes& GetAttributesMutable();
		const VisualLayer& GetVisualLayerRoot() const;
		const ImageResources& GetImageResources() const;
	private:
		WatchedJson m_characterJson;
		VisualLayer m_visualLayerRoot;
		ImageResources m_imageResources;

		void Parse();
	};
}