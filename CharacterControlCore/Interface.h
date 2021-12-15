#pragma once

#if defined(CHARACTERCONTROLCORE)
#define DLLINTERFACE   __declspec( dllexport )
#else
#define DLLINTERFACE   __declspec( dllimport )
#endif

namespace CharacterControlCore
{
	class ControlCore
	{
	public:
		virtual ~ControlCore() {}
		virtual void Update() = 0;
	};

	using ControlCorePtr = std::unique_ptr<ControlCore>;
	DLLINTERFACE ControlCorePtr GetCharacterControl(std::string controlFile);
}