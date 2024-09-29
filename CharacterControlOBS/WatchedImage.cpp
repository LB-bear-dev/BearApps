#include "PCH.h"
#include "WatchedImage.h"
#include <obs-module.h>
#include "Messaging.h"
#include "Psd/Psd.h"
#include "Psd/PsdPlatform.h"
#include "Psd/PsdMallocAllocator.h"
#include "Psd/PsdNativeFile.h"
#include "Psd/PsdDocument.h"
#include "Psd/PsdParseDocument.h"
#include "Psd/PsdParseLayerMaskSection.h"
#include "Psd/PsdParseImageDataSection.h"
#include "Psd/PsdParseImageResourcesSection.h"
#include "Psd/PsdLayer.h"
#include "Psd/PsdChannel.h"
#include "Psd/PsdChannelType.h"
#include "Psd/PsdLayerMask.h"
#include "Psd/PsdVectorMask.h"
#include "Psd/PsdLayerMaskSection.h"
#include "Psd/PsdImageDataSection.h"
#include "Psd/PsdLayerCanvasCopy.h"
#include "Psd/PsdInterleave.h"

#pragma optimize("", off)

std::unordered_map<PATH, std::weak_ptr<CharacterControlRender::WatchedFileImageLibrary>> CharacterControlRender::CharacterImageLibrary::s_imageLibraries;
namespace
{
	void UnloadImage( gs_image_file4* image )
	{
		if ( image != nullptr )
		{
			obs_enter_graphics();
			gs_image_file4_free( image );
			obs_leave_graphics();
		}
	}

	bool LoadImage( const char* fileName, gs_image_file4* image )
	{
		UnloadImage( image );

		if ( fileName != nullptr && strlen(fileName) >= 0 )
		{
			debug_( "loading texture '%s'", fileName );
			gs_image_file4_init( image, fileName, GS_IMAGE_ALPHA_PREMULTIPLY );

			obs_enter_graphics();
			gs_image_file4_init_texture( image );
			obs_leave_graphics();

			if ( !image->image3.image2.image.loaded )
			{
				warn_( "failed to load texture '%s'", fileName );
				return false;
			}
		}
	}

	std::string GetLayerName(const psd::Layer& layer)
	{
		std::string layerName;
		if (layer.utf16Name)
		{
			char name[255];
			size_t unused;
			wcstombs_s(&unused, name, (wchar_t*)layer.utf16Name, 255);
			name[254] = '\0';
			layerName = name;
		}
		else
		{
			layerName = layer.name.c_str();
		}

		error_("%s\n", layerName.c_str());

		return layerName;
	}

	bool LoadPsd(const char* fileName, CharacterControlRender::FileImageLibrary& imageLibrary)
	{
		wchar_t wideFilename[MAX_PATH];
		size_t unused;
		mbstowcs_s(&unused, wideFilename, fileName, MAX_PATH);

		psd::MallocAllocator allocator;
		psd::NativeFile file(&allocator);

		// try opening the file. if it fails, bail out.
		if (!file.OpenRead(wideFilename))
		{
			error_("Cannot open file %s.\n", fileName);
			return false;
		}

		psd::Document* document = psd::CreateDocument(&file, &allocator);
		if (!document)
		{
			OutputDebugStringA("Cannot create document.\n");
			file.Close();
			return 1;
		}

		if (document->bitsPerChannel != 8)
		{
			file.Close();
			return 1;
		}
		
		if (psd::LayerMaskSection* layerMaskSection = psd::ParseLayerMaskSection(document, &file, &allocator))
		{
			for (unsigned int layerIndex = 0; layerIndex < layerMaskSection->layerCount; ++layerIndex)
			{
				psd::Layer& layer = layerMaskSection->layers[layerIndex];
				ExtractLayer(document, &file, &allocator, &layer);

				auto layerImage4 = std::make_unique<gs_image_file4>();
				layerImage4.get()->space = gs_color_space::GS_CS_SRGB;
				layerImage4.get()->image3.alpha_mode = gs_image_alpha_mode::GS_IMAGE_ALPHA_PREMULTIPLY;

				gs_image_file& layerImage = layerImage4.get()->image3.image2.image;
				layerImage.loaded = false;
				layerImage.is_animated_gif = false;
				layerImage.format = gs_color_format::GS_RGBA;
				layerImage.is_animated_gif = false;
				layerImage.cx = document->width;
				layerImage.cy = document->height;

				uint8_t* canvasData[4] = { nullptr };
				int realChannelCount = 4;

				for (unsigned int channelIndex = 0; channelIndex < layer.channelCount; ++channelIndex)
				{
					psd::Channel& channel = layer.channels[channelIndex];

					if (channel.type <= 3)
					{
						int channelType = channel.type < 0 ? 3 : channel.type;
						canvasData[channelType] = static_cast<uint8_t*>(allocator.Allocate(sizeof(uint8_t) * document->width * document->height, 16u));
						memset(canvasData[channelType], 0u, sizeof(uint8_t) * document->width * document->height);

						psd::imageUtil::CopyLayerData(static_cast<const uint8_t*>(channel.data), canvasData[channelType], layer.left, layer.top, layer.right, layer.bottom, document->width, document->height);
						//++realChannelCount;
					}
				}
				layerImage4.get()->image3.image2.mem_usage = document->width * document->height * layer.channelCount * sizeof(uint8_t);

				layerImage.texture_data = (uint8_t*)bmalloc(layerImage4.get()->image3.image2.mem_usage);

				psd::imageUtil::InterleaveRGBA(canvasData[0], canvasData[1], canvasData[2], canvasData[3], layerImage.texture_data, document->width, document->height);

				allocator.Free(canvasData[0]);
				allocator.Free(canvasData[1]);
				allocator.Free(canvasData[2]);
				allocator.Free(canvasData[3]);

				layerImage.loaded = true;
				obs_enter_graphics();
				gs_image_file4_init_texture(layerImage4.get());
				obs_leave_graphics();

				imageLibrary.emplace(GetLayerName(layer), std::move(layerImage4));
			}

			DestroyLayerMaskSection(layerMaskSection, &allocator);
		}

		DestroyDocument(document, &allocator);
		file.Close();
		return true;
	}
}

const std::vector<std::string>& CharacterControlRender::WatchedFileImageLibrary::GetImageNames() const
{
	return m_imageNames;
}

CharacterControlRender::Coord CharacterControlRender::WatchedFileImageLibrary::GetMaxResolution() const
{
	return m_maxResolution;
}

bool CharacterControlRender::WatchedFileImageLibrary::UpdateInternal()
{
	bool success = false;
	if (GetPath().extension() == ".psd")
	{
		success = LoadPsd(GetPath().string().c_str(), *GetMutable());
	}
	else
	{
		const std::string& name = !m_overrideName.empty() ? m_overrideName : GetPath().stem().string();
		auto& img = GetMutable()->emplace(name, std::make_unique<gs_image_file4>()).first->second;
		success = LoadImage(this->GetPath().string().c_str(), img.get());
	}

	// update valid image names
	m_imageNames.clear();
	if (const FileImageLibrary* library = GetMutable())
	{
		if (library != nullptr)
		{
			for (auto& image : *library)
			{
				if (image.second != nullptr)
				{
					m_imageNames.push_back(image.first);
					m_maxResolution.X = std::max(m_maxResolution.X, (int)image.second->image3.image2.image.cx);
					m_maxResolution.Y = std::max(m_maxResolution.Y, (int)image.second->image3.image2.image.cx);
				}
			}
		}
	}

	return success;
}

void CharacterControlRender::WatchedFileImageLibrary::DisposeInternal()
{
	if (GetMutable() != nullptr && !GetMutable()->empty())
	{
		for (auto& [filename, image]: *GetMutable())
		{
			UnloadImage(image.get());
		}
	}
}

void CharacterControlRender::CharacterImageLibrary::AddImageLibrary(PATHR path)
{
	// Already exists
	for (auto& imgLib : m_imageLibraries)
	{
		if (imgLib != nullptr && imgLib->GetPath() == path)
		{
			return;
		}
	}

	// If this already exists in the global library, don't load it again
	auto library = s_imageLibraries.find(path);
	if (library != s_imageLibraries.end() && !library->second.expired())
	{
		if (auto Ptr = library->second.lock())
		{
			m_imageLibraries.push_back(Ptr);
		}
	}
	else
	{
		// Load, put in this image and put in global library
		auto Ptr = std::make_shared<WatchedFileImageLibrary>(path);
		m_imageLibraries.push_back(Ptr);
		s_imageLibraries[path] = Ptr;
	}
}

void CharacterControlRender::CharacterImageLibrary::AddImageLibrary(PATHR path, STRIDR overrideName)
{
	// Already exists
	for (auto& imgLib : m_imageLibraries)
	{
		if (imgLib != nullptr && imgLib->GetPath() == path)
		{
			return;
		}
	}

	// If this already exists in the global library, don't load it again
	auto library = s_imageLibraries.find(path);
	if (library != s_imageLibraries.end() && !library->second.expired())
	{
		if (auto Ptr = library->second.lock())
		{
			m_imageLibraries.push_back(Ptr);
		}
	}
	else
	{
		// Load, put in this image and put in global library
		auto Ptr = std::make_shared<WatchedFileImageLibrary>(path, overrideName);
		m_imageLibraries.push_back(Ptr);
		s_imageLibraries[path] = Ptr;
	}
}

std::vector<std::string> CharacterControlRender::CharacterImageLibrary::GetValidImageNames() const
{
	std::vector<std::string> list;

	for (auto& imageLibrary : m_imageLibraries)
	{
		list.insert(list.end(), imageLibrary->GetImageNames().begin(), imageLibrary->GetImageNames().end());
	}

	return list;
}

gs_image_file4* CharacterControlRender::CharacterImageLibrary::GetImageFromName(STRIDR name) const
{
	for (auto& imageLibrary : m_imageLibraries)
	{
		if (const FileImageLibrary* ImageFile = imageLibrary->Get())
		{
			auto foundIt = ImageFile->find(name);
			if (foundIt != ImageFile->end())
			{
				return foundIt->second.get();
			}
		}
	}

	return nullptr;
}

CharacterControlRender::Coord CharacterControlRender::CharacterImageLibrary::GetMaxResolution() const
{
	Coord size(0,0);
	for (auto& libraryPtr : m_imageLibraries)
	{
		if (const WatchedFileImageLibrary* library = libraryPtr.get())
		{
			Coord maxRes = library->GetMaxResolution();
			size.X = std::max(size.X, maxRes.X);
			size.Y = std::max(size.Y, maxRes.Y);
		}
	}
	
	return size;
}
