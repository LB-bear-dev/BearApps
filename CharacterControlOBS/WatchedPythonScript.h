#pragma once
#include <CharacterControlCore/WatchedFile.h>
#define PY_SSIZE_T_CLEAN
// pyconfig.h switches to the debug Python ABI (Py_DEBUG, python312_d.lib) whenever _DEBUG is set.
// Only the release Python is installed, so include it as if this were a release build.
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

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