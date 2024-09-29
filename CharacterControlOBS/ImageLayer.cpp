#include "PCH.h"
#include "ImageLayer.h"

using namespace CharacterControlRender;

CharacterControlRender::ImageLayer::ImageLayer() : m_layerName(""), m_active(true), m_parentOffset({0,0})
{
}

void CharacterControlRender::ImageLayer::CreateChildLayer(STRIDR name, Coord parentOffset)
{
	ImageLayer child;
	child.m_layerName = name;
	child.m_parentOffset = parentOffset;
	m_childLayersByZOrder.push_back(child);
}

void CharacterControlRender::ImageLayer::CreateSlot(STRIDR slot)
{
	m_slotNames.push_back({ slot, "" });
	for (ImageLayer& layer : m_childLayersByZOrder)
	{
		layer.CreateSlot(slot);
	}
}

void CharacterControlRender::ImageLayer::AddImageLibraryToLayer(PATHR path)
{
	m_imageLibraries.AddImageLibrary(path);
}

void CharacterControlRender::ImageLayer::AddImageLibraryToLayer(PATHR path, STRIDR overrideName)
{
	m_imageLibraries.AddImageLibrary(path, overrideName);
}

void CharacterControlRender::ImageLayer::Activate()
{
	m_active = true;

	for (ImageLayer& layer : m_childLayersByZOrder)
	{
		layer.Activate();
	}
}

void CharacterControlRender::ImageLayer::Deactivate()
{
	m_active = false;

	for (ImageLayer& layer : m_childLayersByZOrder)
	{
		layer.Deactivate();
	}
}

void CharacterControlRender::ImageLayer::Render(gs_effect_t* effect) const
{
	if (!m_active)
	{
		return;
	}

	auto image4 = GetImage();
	if (image4)
	{
		const gs_image_file* image = &image4->image3.image2.image;

		if (gs_texture_t* texture = image->texture)
		{
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
	}

	uint32_t index = 0;
	while (auto layer = GetNextActiveChildLayer(index))
	{
		layer->Render(effect);
	}
}

CharacterControlRender::ImageLayer* CharacterControlRender::ImageLayer::GetLayerByName(STRIDR name)
{
	auto foundLayer = std::find_if(m_childLayersByZOrder.begin(), m_childLayersByZOrder.end(), [&name](ImageLayer& layer)
		{
			return layer.m_layerName == name;
		});

	if (foundLayer == m_childLayersByZOrder.end())
	{
		return nullptr;
	}

	return &*foundLayer;
}

Coord CharacterControlRender::ImageLayer::GetMaxResolution() const
{
	Coord maxRes = m_imageLibraries.GetMaxResolution();

	for (auto& childLayer : m_childLayersByZOrder)
	{
		Coord childMaxRes = childLayer.GetMaxResolution();
		maxRes.X = std::max(maxRes.X, childMaxRes.X);
		maxRes.Y = std::max(maxRes.Y, childMaxRes.Y);
	}

	return maxRes;
}

void CharacterControlRender::ImageLayer::SetSlot(STRIDR slot, STRIDR name)
{
	for (auto& slotName : m_slotNames)
	{
		if (slotName.slot == slot)
		{
			slotName.name = name;
		}
	}

	auto imageNames = m_imageLibraries.GetValidImageNames();
	m_activeImageName.clear();

	for (auto& slotName : m_slotNames)
	{
		for (auto index = imageNames.begin(); index != imageNames.end(); )
		{
			if (index->find(slotName.name))
			{
				imageNames.erase(index);
			}
			else
			{
				++index;
			}
		}

		if (!imageNames.empty())
		{
			m_activeImageName = imageNames[0];
		}
	}

	for (ImageLayer& layer : m_childLayersByZOrder)
	{
		layer.SetSlot(slot, name);
	}
}

const gs_image_file4* CharacterControlRender::ImageLayer::GetImage() const
{
	return m_imageLibraries.GetImageFromName(m_activeImageName);
}

const CharacterControlRender::ImageLayer* CharacterControlRender::ImageLayer::GetNextActiveChildLayer(uint32_t& childIndex) const
{
	for (; childIndex < m_childLayersByZOrder.size(); ++childIndex)
	{
		if (m_childLayersByZOrder[childIndex].m_active == true)
		{
			return &m_childLayersByZOrder[childIndex++];
		}
	}

	return nullptr;
}
