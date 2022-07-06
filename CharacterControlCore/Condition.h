#pragma once
#include "Attributes.h"
namespace CharacterControlCore
{
	enum class BooleanConditionJoiner
	{
		AND,
		OR
	};

	struct ConditionalBase
	{
		ConditionalBase(BooleanConditionJoiner j) :join(j) {}
		virtual bool Evaluate(const Attributes& attributes) = 0;
		BooleanConditionJoiner join;
	};

	template <typename T>
	struct Conditional : public ConditionalBase
	{
		typedef bool (*ConditionalFn)(const T* attribute, const T& value);
		Conditional(BooleanConditionJoiner j, const std::string& v, const T& va, ConditionalFn o) : ConditionalBase(j), var(v), val(va), op(o) {};
		bool Evaluate(const Attributes& attributes);
		std::string var;
		T val;
		ConditionalFn op;
	};

	class Condition
	{
	public:
		bool Evaluate(const Attributes& attributes) const;

	private:
		std::vector<std::shared_ptr<ConditionalBase>> m_conditions;

		friend void to_json(json& j, const Condition& c);
		friend void from_json(const json& j, Condition& c);
	};

	template<typename T>
	inline bool Conditional<T>::Evaluate(const Attributes& attributes)
	{
		const T* attribute = attributes.GetAttribute<T>(var);
		if (attribute != nullptr)
		{
			return op(attribute, val);
		}

		//Attribute of appropriate type could not be found
		return false;
	}
}