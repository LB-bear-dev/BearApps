#include "PCH.h"
#include "Condition.h"

#pragma optimize("", off)

namespace
{
	template <typename T>
	bool Equals(const T* attribute, const T& value)
	{
		return *attribute == value;
	}

	template <typename T>
	bool NotEqual(const T* attribute, const T& value)
	{
		return *attribute != value;
	}

	template <typename T>
	bool Less(const T* attribute, const T& value)
	{
		return *attribute < value;
	}

	template <typename T>
	bool Greater(const T* attribute, const T& value)
	{
		return *attribute > value;
	}

	template <typename T>
	bool LessEqual(const T* attribute, const T& value)
	{
		return *attribute <= value;
	}

	template <typename T>
	bool GreaterEqual(const T* attribute, const T& value)
	{
		return *attribute >= value;
	}

	bool Contains(const std::string* attribute, const std::string& value)
	{
		return attribute->find(value);
	}

	template<typename T>
	CharacterControlCore::Conditional<T>::ConditionalFn GetFn(const std::string& fnName)
	{
		if (fnName == "EQUAL")
		{
			return &Equals;
		}
		else if (fnName == "NOTEQUAL")
		{
			return &NotEqual;
		}
		else if (fnName == "LESS")
		{
			return &Less;
		}
		else if (fnName == "GREATER")
		{
			return &Greater;
		}
		else if (fnName == "LESSEQUAL")
		{
			return &LessEqual;
		}
		else if (fnName == "GREATEREQUAL")
		{
			return &GreaterEqual;
		}
		else
		{
			//make this a message, but we don't accept any other types here.
			assert(false);
			return nullptr;
		}
	}

	CharacterControlCore::Conditional<std::string>::ConditionalFn GetFnStr(const std::string& fnName)
	{
		if (fnName == "EQUAL")
		{
			return &Equals;
		}
		if (fnName == "CONTAINS")
		{
			return &Contains;
		}
		else
		{
			//make this a message, but we don't accept any other types here.
			assert(false);
			return nullptr;
		}
	}
}

bool CharacterControlCore::Condition::Evaluate(const Attributes& attributes) const
{
	bool conditionResult = true;
	for (auto& condition : m_conditions)
	{
		bool evaluatedCondition = condition->Evaluate(attributes);
		switch (condition->join)
		{
		case BooleanConditionJoiner::AND:
			conditionResult &= evaluatedCondition;
			break;
		case BooleanConditionJoiner::OR:
			conditionResult |= evaluatedCondition;
			break;
		}
	}

	return conditionResult;
}

void CharacterControlCore::to_json(json& j, const Condition& c)
{
}

void CharacterControlCore::from_json(const json& j, Condition& c)
{
	for (auto& conditionJson : j)
	{
		json varJson = conditionJson.at("var");
		json opJson = conditionJson.at("op");
		json valJson = conditionJson.at("val");
		auto joinJson = conditionJson.find("join");

		BooleanConditionJoiner joiner = BooleanConditionJoiner::AND;
		if (joinJson != conditionJson.end())
		{
			if (joinJson->get<std::string>() == "OR")
			{
				joiner = BooleanConditionJoiner::OR;
			}
			else if (joinJson->get<std::string>() != "AND")
			{
				//make this an error, but OR and AND should be the only accepted values here.
				assert(false);
			}
		}

		if (valJson.is_boolean())
		{
			Conditional<bool>::ConditionalFn op = GetFn<bool>(opJson.get<std::string>());
			c.m_conditions.emplace_back(std::make_shared<Conditional<bool>>(joiner, varJson.get<std::string>(), valJson.get<bool>(), op));
		}
		else if (valJson.is_number())
		{
			Conditional<int>::ConditionalFn op = GetFn<int>(opJson.get<std::string>());
			c.m_conditions.emplace_back(std::make_shared<Conditional<int>>(joiner, varJson.get<std::string>(), valJson.get<int>(), op));
		}
		else if (valJson.is_number_float())
		{
			Conditional<float>::ConditionalFn op = GetFn<float>(opJson.get<std::string>());
			c.m_conditions.emplace_back(std::make_shared<Conditional<float>>(joiner, varJson.get<std::string>(), valJson.get<float>(), op));
		}
		else if (valJson.is_string())
		{
			Conditional<std::string>::ConditionalFn op = GetFnStr(opJson.get<std::string>());
			c.m_conditions.emplace_back(std::make_shared<Conditional<std::string>>(joiner, varJson.get<std::string>(), valJson.get<std::string>(), op));
		}
	}
}
