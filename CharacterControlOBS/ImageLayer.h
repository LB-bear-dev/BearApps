#pragma once
#include "Coord.h"
#include "WatchedImage.h"
#include <map>

namespace CharacterControlRender
{
	class ImageLayer
	{
	public:
		ImageLayer();

		void CreateChildLayer(STRIDR layerName);
		void CreateSlot(STRIDR slot);

		void AddImageLibraryToLayer(PATHR name);
		void AddImageLibraryToLayer(PATHR name, STRIDR overrideName);

		void Activate();
		void Deactivate();

		void Render(gs_effect_t* effect) const;

		ImageLayer* GetLayerByName(STRIDR name);

		Coord GetMaxResolution() const;

		void SetSlot(STRIDR slot, STRIDR name);

		const Coord& GetTranslation() const;
		void SetTranslation(const Coord& translation);

		const Coord& GetScale() const;
		void SetScale(const Coord& scale);

		float GetRotation() const;
		void SetRotation(float rotation);

	private:
		const ImageLayer* GetNextActiveChildLayer(uint32_t& startIndex) const;
		const gs_image_file4* GetImage() const;

		STRID m_layerName;

		struct SlotName
		{
			STRID slot;
			STRID name;
		};
		std::vector<SlotName> m_slotNames;

		STRID m_activeImageName;

		bool m_active;

		Coord m_translation;
		Coord m_scale;
		float m_rotation;

		CharacterImageLibrary m_imageLibraries;
		std::vector<ImageLayer> m_childLayersByZOrder;
	};
}