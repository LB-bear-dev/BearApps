#include "PCH.h"
#include "ImageLayer.h"

using namespace CharacterControlRender;

CharacterControlRender::ImageLayer::ImageLayer() : m_layerName(""), m_active(true), m_translation({0, 0}), m_scale({1, 1}), m_rotation(0)
{}

void CharacterControlRender::ImageLayer::CreateChildLayer(STRIDR name)
{
	ImageLayer child;
	child.m_layerName = name;
	m_childLayersByZOrder.push_back(child);
}

void CharacterControlRender::ImageLayer::CreateSlot(STRIDR slot)
{
	m_slotNames.push_back({slot, ""});
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
}

void CharacterControlRender::ImageLayer::Deactivate()
{
	m_active = false;
}

void CharacterControlRender::ImageLayer::Render(gs_effect_t* effect) const
{
	if (!m_active)
	{
		return;
	}
	gs_matrix_push();
	gs_matrix_translate3f(m_translation.X, m_translation.Y, 0);

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
	gs_matrix_pop();
}

CharacterControlRender::ImageLayer* CharacterControlRender::ImageLayer::GetLayerByName(STRIDR name)
{
	auto foundLayer = std::find_if(
		m_childLayersByZOrder.begin(),
		m_childLayersByZOrder.end(),
		[&name](ImageLayer& layer)
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
	bool changed = false;
	for (auto& slotName : m_slotNames)
	{
		if (slotName.slot == slot && slotName.name != name)
		{
			slotName.name = name;
			changed = true;
		}
	}

	// Layers without this slot keep the image they already have
	if (changed)
	{
		RefreshActiveImageFromSlots();
	}

	for (ImageLayer& layer : m_childLayersByZOrder)
	{
		layer.SetSlot(slot, name);
	}
}

// Picks the first image whose name contains every slot's current value
void CharacterControlRender::ImageLayer::RefreshActiveImageFromSlots()
{
	m_activeImageName.clear();

	for (const std::string& imageName : m_imageLibraries.GetValidImageNames())
	{
		bool matchesAllSlots = std::all_of(
			m_slotNames.begin(),
			m_slotNames.end(),
			[&imageName](const SlotName& slotName)
				{
					return imageName.find(slotName.name) != std::string::npos;
				});

		if (matchesAllSlots)
		{
			m_activeImageName = imageName;
			return;
		}
	}
}

const Coord& CharacterControlRender::ImageLayer::GetTranslation() const
{
	return m_translation;
}

void CharacterControlRender::ImageLayer::SetTranslation(const Coord& translation)
{
	m_translation = translation;
}

const Coord& CharacterControlRender::ImageLayer::GetScale() const
{
	return m_scale;
}

void CharacterControlRender::ImageLayer::SetScale(const Coord& scale)
{
	m_scale = scale;
}

float CharacterControlRender::ImageLayer::GetRotation() const
{
	return m_rotation;
}

void CharacterControlRender::ImageLayer::SetRotation(float rotation)
{
	m_rotation = rotation;
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