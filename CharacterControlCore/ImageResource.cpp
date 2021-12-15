#include "PCH.h"
#include "ImageResource.h"
#pragma optimize("", off)

void CharacterControlCore::to_json(json& resourceJson, const ImageResource& resource)
{
	resourceJson = json({ "Source", resource.GetFilename() });
}

void CharacterControlCore::from_json(const json& resourceJson, ImageResource& resource)
{
	json sourceJson = resourceJson.at("Source");
	sourceJson.get_to(resource.filename);
}

void CharacterControlCore::to_json(json& resourcesJson, const ImageResources& resources)
{
	for (auto& [key, value] : resources.m_resources)
	{
		resourcesJson.push_back({ key, json(value)});
	}
}

void CharacterControlCore::from_json(const json& resourcesJson, ImageResources& resources)
{
	for (auto& [key, value] : resourcesJson.items())
	{
		resources.m_resources.emplace(key, value.get<ImageResource>());
	}
}
