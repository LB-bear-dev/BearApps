#pragma once
#include <nlohmann/json.hpp>
namespace CharacterControlCore
{
	class ImageResource;
	class ImageResources;
	void to_json(json& attributesJson, const ImageResource& attributes);
	void from_json(const json& attributesJson, ImageResource& attributes);
	void to_json(json& attributesJson, const ImageResources& attributes);
	void from_json(const json& attributesJson, ImageResources& attributes);

	class ImageResource
	{
	public:
		const std::string& GetFilename() const { return filename; }

	private:
		std::string filename;

	public:
		friend void to_json(json& attributesJson, const ImageResource& attributes);
		friend void from_json(const json& attributesJson, ImageResource& attributes);
	};

	class ImageResources
	{
	public:
		std::optional<const ImageResource*> GetResource(const std::string& resourceName) const
		{
			auto foundResource = m_resources.find(resourceName);
			if (foundResource != m_resources.end())
			{
				return &foundResource->second;
			}

			return {};
		}

	private:
		std::unordered_map<std::string, ImageResource> m_resources;

	public:
		friend void to_json(json& attributesJson, const ImageResources& attributes);
		friend void from_json(const json& attributesJson, ImageResources& attributes);
	};

}