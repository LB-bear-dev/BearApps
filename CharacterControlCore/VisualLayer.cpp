#include "PCH.h"
#include "VisualLayer.h"


const std::string& CharacterControlCore::VisualLayerNameBuilder::Update(const Attributes& attributes) const
{
	for (const VisualLayerCondition& nameConditon : m_nameConditions)
	{
		if (nameConditon.m_condition.Evaluate(attributes))
		{
			return nameConditon.m_name;
		}
	}

	return m_defaultName;
}

void CharacterControlCore::VisualLayer::Update(const Attributes& attributes)
{
	m_name = "";

	for (const VisualLayerNameBuilder& namePartBuilder : m_namePartBuilders)
	{
		m_name += namePartBuilder.Update(attributes);
	}

	for (auto& childLayer : m_childLayers)
	{
		childLayer.second.Update(attributes);
	}
}

const std::string& CharacterControlCore::VisualLayer::GetCurrentImageName()
{
	return m_name;
}
