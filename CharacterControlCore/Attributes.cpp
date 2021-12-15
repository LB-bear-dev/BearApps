#include "PCH.h"
#include "Attributes.h"

void CharacterControlCore::to_json(json& json, const Attributes& attributes)
{

}

void CharacterControlCore::from_json(const json& attributesJson, Attributes& attributes)
{
	for (auto& [key, value] : attributesJson.items())
	{
		if (value.is_boolean())
		{
			attributes.CreateAttribute(key, value.get<bool>());
		}
		else if (value.is_number())
		{
			attributes.CreateAttribute(key, value.get<int>());
		}
		else if (value.is_number_float())
		{
			attributes.CreateAttribute(key, value.get<float>());
		}
		else if (value.is_string())
		{
			attributes.CreateAttribute(key, value.get<std::string>());
		}
	}
}
