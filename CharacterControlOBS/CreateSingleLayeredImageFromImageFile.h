#pragma once

#include "ImageLayer.h"

namespace CharacterControlRender
{
	void CreateSingleLayeredImageFromImageFile(const std::filesystem::path& path, ImageLayer& root);
}