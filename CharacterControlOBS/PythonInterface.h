#pragma once
#include "ActiveCharacters.h"

namespace CharacterControlScript
{
	class PythonInterface final
	{
	public:
		~PythonInterface()
		{
			Py_FinalizeEx();
		}

		static PythonInterface& Get()
		{
			if (s_singleton == nullptr)
			{
				s_singleton = std::unique_ptr<PythonInterface>(new PythonInterface());
			}

			return *s_singleton;
		}
	private:
		PythonInterface()
		{
			PyConfig config;
			PyConfig_InitPythonConfig(&config);
			auto pathstr = CharacterControlOBS::GetGlobalSettingsPath().value_or("").string();
			std::wstring widestr = std::wstring(pathstr.begin(), pathstr.end());

			PyConfig_SetString(&config, &config.pythonpath_env,
				widestr.c_str());
			Py_InitializeFromConfig(&config);


			/* Setup the __main__ module for us to use */
			auto main_module = PyImport_ImportModule("__main__");
			auto main_dict = PyModule_GetDict(main_module);

			/* Fetch the sys module */
			auto sys_module = PyImport_ImportModule("sys");
			auto sys_dict = PyModule_GetDict(sys_module);

			/* Attach the sys module into the __main__ namespace */
			PyDict_SetItemString(main_dict, "sys", sys_module);
		}
		static std::unique_ptr<PythonInterface> s_singleton;
	};
}