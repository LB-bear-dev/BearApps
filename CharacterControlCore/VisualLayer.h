#pragma once
#include "Attributes.h"
#include "Condition.h"
#include "ImageResource.h"
#include "WatchedJson.h"

namespace CharacterControlCore
{
	struct VisualLayerCondition
	{
		Condition m_condition;
		std::string m_name;
	};

	class VisualLayerNameBuilder
	{
	public:
		const std::string& Update(const Attributes& attributes) const;
	private:
		std::vector<VisualLayerCondition> m_nameConditions;
		std::string m_defaultName;
	};

	class VisualLayer
	{
	public:
		void Update(const Attributes& attributes);
		const std::string& GetCurrentImageName();
	private:
		std::string m_name;
		std::unordered_map<std::string, VisualLayer> m_childLayers;
		std::vector<VisualLayerNameBuilder> m_namePartBuilders;
	};
}