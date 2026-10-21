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

	bool IDIsActiveInDiscord(const std::string& ID);

	// Must run from obs_module_unload, at process exit the update worker is already gone and the client would wait on it forever.
	void ShutdownDiscord();
}