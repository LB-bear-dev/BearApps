#include "PCH.h"
#include "Character.h"
#include "Messaging.h"
#include "WatchedImage.h"
#include "PythonInterface.h"
#include "WatchedPythonScript.h"

#pragma optimize("", off)

using namespace CharacterControlOBS;

CharacterControlOBS::Character::Character(STRIDR characterName)
{
	m_imageRoot.CreateChildLayer("default", { 0, 0 });
}

void CharacterControlOBS::Character::AddComponent(std::unique_ptr<Component>&& component)
{
	m_components.emplace_back(std::move(component))->DeclareAttributes(m_intAttributes);
}

void CharacterControlOBS::Character::SetupImageControlScript(const std::filesystem::path& filename)
{
	m_imageControlScript = std::make_unique<CharacterControlScript::WatchedPythonScript>(filename);
}

void CharacterControlOBS::Character::UpdateAttributes()
{
	for (auto& component : m_components)
	{
		component->UpdateAttributes(m_intAttributes);
	}
}

void CharacterControlOBS::Character::UpdateRenderState()
{
	if (m_imageControlScript)
	{
		CharacterControlScript::PythonInterface::Get();
		m_imageControlScript->Update();

		PyObject* pArgs = PyTuple_New(1);
		PyObject* pDict = PyDict_New();
		if (pDict != nullptr && !Py_IsNone(pDict))
		{
			for (auto& intAttribute : m_intAttributes)
			{
				if (intAttribute.second != nullptr)
				{
					PyObject* pVal = PyLong_FromLong(intAttribute.second->GetValue());
					if (pVal != nullptr && !Py_IsNone(pVal))
					{
						PyDict_SetItemString(pDict, intAttribute.first.c_str(), pVal);
					}
				}
			}
		}
		PyTuple_SetItem(pArgs, 0, pDict);
	}	
}

const Attribute<int>* CharacterControlOBS::Character::GetIntAttribute(STRIDR attrName) const
{
	auto foundAttr = m_intAttributes.find(attrName);
	if (foundAttr != m_intAttributes.end())
	{
		return foundAttr->second.get();
	}

	return nullptr;
}

void CharacterControlOBS::Character::Render(gs_effect_t* effect) const
{
	m_imageRoot.Render(effect);
}

CharacterControlRender::ImageLayer& CharacterControlOBS::Character::GetImageRoot()
{
	return m_imageRoot;
}
