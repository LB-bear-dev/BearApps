#pragma once

namespace CharacterControlCore
{
	template <typename T>
	struct Attribute
	{
		T value;
		T defaultValue;
	};

	class Attributes
	{
	public:
		template<typename T>
		void CreateAttribute(const std::string& key, T defaultValue);

		template<typename T>
		const T* GetAttribute(const std::string& key) const;

		template<typename T>
		T* GetAttribute(const std::string& key);
	private:
		std::unordered_map<std::string, Attribute<bool>> m_attributesBool;
		std::unordered_map<std::string, Attribute<int>> m_attributesInt;
		std::unordered_map<std::string, Attribute<float>> m_attributesFloat;
		std::unordered_map<std::string, Attribute<std::string>> m_attributesString;
	};

	template<typename T>
	inline void Attributes::CreateAttribute(const std::string& key, T defaultValue)
	{
		
	}

	template<>
	inline void Attributes::CreateAttribute(const std::string& key, bool defaultValue)
	{
		auto& attribute = m_attributesBool[key];
		attribute.value = defaultValue;
		attribute.defaultValue = defaultValue;
	}

	template<>
	inline void Attributes::CreateAttribute(const std::string& key, int defaultValue)
	{
		auto& attribute = m_attributesInt[key];
		attribute.value = defaultValue;
		attribute.defaultValue = defaultValue;
	}

	template<>
	inline void Attributes::CreateAttribute(const std::string& key, float defaultValue)
	{
		auto& attribute = m_attributesFloat[key];
		attribute.value = defaultValue;
		attribute.defaultValue = defaultValue;
	}

	template<>
	inline void Attributes::CreateAttribute(const std::string& key, std::string defaultValue)
	{
		auto& attribute = m_attributesString[key];
		attribute.value = defaultValue;
		attribute.defaultValue = defaultValue;
	}

	template<typename T>
	inline T* Attributes::GetAttribute(const std::string& key) 
	{
		
		return nullptr;
	}

	template<>
	inline bool* Attributes::GetAttribute<bool>(const std::string& key) 
	{
		auto foundName = m_attributesBool.find(key);
		if (foundName == m_attributesBool.end())
		{
			return nullptr;
		}
		return &foundName->second.value;
	}

	template<>
	inline int* Attributes::GetAttribute<int>(const std::string& key) 
	{
		auto foundName = m_attributesInt.find(key);
		if (foundName == m_attributesInt.end())
		{
			return nullptr;
		}
		return &foundName->second.value;
	}

	template<>
	inline float* Attributes::GetAttribute<float>(const std::string& key) 
	{
		auto foundName = m_attributesFloat.find(key);
		if (foundName == m_attributesFloat.end())
		{
			return nullptr;
		}
		return &foundName->second.value;
	}

	template<>
	inline std::string* Attributes::GetAttribute<std::string>(const std::string& key) 
	{
		auto foundName = m_attributesString.find(key);
		if (foundName == m_attributesString.end())
		{
			return nullptr;
		}
		return &foundName->second.value;
	}

	template<typename T>
	inline const T* Attributes::GetAttribute(const std::string& key) const
	{
		return const_cast<Attributes*>(this)->GetAttribute<T>(key);
	}

	void to_json(json& attributesJson, const Attributes& attributes);
	void from_json(const json& attributesJson, Attributes& attributes);
}