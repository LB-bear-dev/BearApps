#pragma once
#include "Character.h"

namespace CharacterControlOBS
{
	class DiscordComponent : public Component
	{
	public:
		DiscordComponent(std::string ID) : m_ID(ID)
		{
		}

		virtual void DeclareAttributes(AttributeMap& map) override;
		virtual void UpdateAttributes(const AttributeMap& map) override;
	private:

		std::string m_ID;
	};
}