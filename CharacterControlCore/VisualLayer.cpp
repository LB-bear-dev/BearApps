#include "PCH.h"
#include "VisualLayer.h"


const std::string CharacterControlCore::VisualLayerNameBuilder::Update(const Attributes& attributes) const
{
	for (const VisualLayerCondition& nameConditon : m_nameConditions)
	{
		if (nameConditon.m_condition.Evaluate(attributes))
		{
			return nameConditon.m_name;
		}
	}

	return "";
}

void CharacterControlCore::VisualLayer::Update(const Attributes& attributes)
{
	m_name = "";

	bool first = true;
	for (const VisualLayerNameBuilder& namePartBuilder : m_namePartBuilders)
	{
		const std::string& namePart = namePartBuilder.Update(attributes);
		if (!namePart.empty())
		{
			m_name += (first ? "" : "_") + namePart;
		}
		first = false;
	}

	for (auto& childLayer : m_childLayers)
	{
		childLayer.second.Update(attributes);
	}
}

const std::string& CharacterControlCore::VisualLayer::GetCurrentImageName() const
{
	return m_name;
}

void CharacterControlCore::to_json(json& nameBuilderJson, const VisualLayerNameBuilder& nameBuilder)
{
}

void CharacterControlCore::from_json(const json& nameBuilderJson, VisualLayerNameBuilder& nameBuilder)
{
	for (auto& [key, conditionJson] : nameBuilderJson.items())
	{
		VisualLayerCondition condition;
		condition.m_name = key;
		condition.m_condition = conditionJson.get<Condition>();
		nameBuilder.m_nameConditions.push_back(condition);
	}
}

void CharacterControlCore::to_json(json& visualLayerJson, const VisualLayer& visualLayer)
{
}

void CharacterControlCore::from_json(const json& visualLayerJson, VisualLayer& visualLayer)
{
	const auto& nameBuilderListJson = visualLayerJson.find("NameBuilder");
	if (nameBuilderListJson != visualLayerJson.end())
	{
		for (auto& nameBuilderElement : *nameBuilderListJson)
		{
			visualLayer.m_namePartBuilders.push_back(nameBuilderElement.get<VisualLayerNameBuilder>());
		}
	}

	const auto& childLayersJson = visualLayerJson.find("ChildLayers");
	if (childLayersJson != visualLayerJson.end())
	{
		for (auto& [key, childLayerJson] : childLayersJson->items())
		{
			visualLayer.m_childLayers.emplace(key, childLayerJson.get<VisualLayer>());
		}
	}
}
