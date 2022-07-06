#pragma once
#include "Attributes.h"
#include "Condition.h"
#include "ImageResource.h"
#include "WatchedJson.h"

namespace CharacterControlCore
{
	struct VisualLayerCondition
	{
		VisualLayerCondition() {}
		Condition m_condition;
		std::string m_name;
	};

	class VisualLayerNameBuilder
	{
	public:
		const std::string Update(const Attributes& attributes) const;
	private:
		std::vector<VisualLayerCondition> m_nameConditions;

	public:
		friend void to_json(json& attributesJson, const VisualLayerNameBuilder& attributes);
		friend void from_json(const json& attributesJson, VisualLayerNameBuilder& attributes);
	};

	class VisualLayer
	{
	public:
		void Update(const Attributes& attributes);
		const std::string& GetCurrentImageName() const;
	private:
		std::string m_name;
		std::unordered_map<std::string, VisualLayer> m_childLayers;
		std::vector<VisualLayerNameBuilder> m_namePartBuilders;

	public:
		friend void to_json(json& attributesJson, const VisualLayer& attributes);
		friend void from_json(const json& attributesJson, VisualLayer& attributes);
	};
}