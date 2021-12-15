#pragma once
#include "WatchedJson.h"
#include "Attributes.h"
#include "ImageResource.h"
#include "Condition.h"
namespace CharacterControlCore
{
	struct VisualLayerCondition
	{
		Condition m_condition;
		std::string m_name;
	};

	class VisualLayerConditions
	{
	public:
		std::string Get();
	private:
		std::vector<VisualLayerCondition> m_conditions;
		std::string m_defaultName;
	};

	class VisualLayer
	{
	public:
		void Update();
		ImageResource GetCurrentImage();
	private:
		std::string m_name;
		std::unordered_map<std::string, VisualLayer> m_childLayers;
		std::vector<VisualLayerConditions> m_conditionsList;
	};
}