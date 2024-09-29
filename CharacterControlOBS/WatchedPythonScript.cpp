#include "PCH.h"
#include "WatchedPythonScript.h"
#include "PythonInterface.h"

#pragma optimize("", off)
std::unique_ptr<CharacterControlScript::PythonInterface> CharacterControlScript::PythonInterface::s_singleton = nullptr;


PyObject* CharacterControlScript::WatchedPythonScript::Call(std::string fnName, PyObject* args)
{
	PyObject* pVal = nullptr;
	if (GetMutable() != nullptr)
	{
		PyObject* pyFile = *GetMutable();
		if (pyFile != nullptr && !Py_IsNone(pyFile))
		{
			PyObject* pFunc = PyObject_GetAttrString(pyFile, fnName.c_str());
			if (pFunc != nullptr && PyCallable_Check(pFunc))
			{
				pVal = PyObject_CallObject(pFunc, args);
			}

			Py_DECREF(pFunc);
		}

	}
	return pVal;
}

bool CharacterControlScript::WatchedPythonScript::UpdateInternal()
{
	PythonInterface::Get();
	std::string rootPath = CharacterControlOBS::GetGlobalSettingsPath().value_or("").make_preferred().string();
	std::filesystem::path path = GetPath();
	path = path.make_preferred();
	size_t rootPathIndex = path.string().find(rootPath) + rootPath.length()+1;
	std::string filenameWithoutExtension = path.string().substr(rootPathIndex, path.string().find_first_of('.') - rootPathIndex);
	PyObject* pName = PyUnicode_FromString(filenameWithoutExtension.c_str());
	PyObject* pyFile = PyImport_GetModule(pName);
	if (pyFile == nullptr || Py_IsNone(pyFile))
	{
		try
		{
			PyObject* module = PyImport_Import(pName);
			*GetMutable() = module;
		}
		catch (...)
		{
			Py_DECREF(pName);
			return false;
		}
	}
	else
	{
		PyImport_ReloadModule(pyFile);
	}
	Py_DECREF(pName);
	return true;
}

void CharacterControlScript::WatchedPythonScript::DisposeInternal()
{
	if (GetMutable() != nullptr)
	{
		PyObject* pyFile = *GetMutable();
		if (pyFile != nullptr && !Py_IsNone(pyFile))
		{
			Py_DECREF(pyFile);
		}
	}
}
