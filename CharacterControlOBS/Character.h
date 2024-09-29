#pragma once
#include <obs-module.h>
#include "ImageLayer.h"
#include "WatchedImage.h"
#include "WatchedPythonScript.h"
#include "Attribute.h"

namespace CharacterControlOBS
{
	using AttributeMap = std::unordered_map<std::string, std::unique_ptr<Attribute<int>>>;
	class Component
	{
	public:
		virtual void DeclareAttributes(AttributeMap& map) = 0;
		virtual void UpdateAttributes(const AttributeMap& map) = 0;
	};

	class Action
	{
		virtual void Execute(CharacterControlRender::ImageLayer& m_imageRoot) = 0;
	};

	class Character
	{
	public:
		Character(STRIDR characterName);

		void AddComponent(std::unique_ptr<Component>&& component);

		void SetupImageControlScript(const std::filesystem::path& filename);

		void UpdateAttributes();

		void UpdateRenderState();

		const Attribute<int>* GetIntAttribute(STRIDR attrName) const;

		void Render(gs_effect_t* effect) const;

		CharacterControlRender::ImageLayer& GetImageRoot();
	private:
		std::vector<std::unique_ptr<Component>> m_components;

		AttributeMap m_intAttributes;

		CharacterControlRender::ImageLayer m_imageRoot;

		std::unique_ptr<CharacterControlScript::WatchedPythonScript> m_imageControlScript;
	};
}