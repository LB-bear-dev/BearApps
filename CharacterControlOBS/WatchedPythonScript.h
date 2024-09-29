#pragma once
#include <CharacterControlCore/WatchedFile.h>
#define PY_SSIZE_T_CLEAN
#include <Python.h>

namespace CharacterControlScript
{
	class WatchedPythonScript : public CharacterControlCore::WatchedFile<PyObject*>
	{
	public:
		WatchedPythonScript(const std::filesystem::path& filename) :
			CharacterControlCore::WatchedFile<PyObject*>(filename)
		{
			Update();
		}

		PyObject* Call(std::string fnName, PyObject* args);

	private:
		bool UpdateInternal() override;
		void DisposeInternal() override;
	};
}