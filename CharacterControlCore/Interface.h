#pragma once

#if defined(CHARACTERCONTROLCORE)
#define CHARACTERCONTROLCOREDLLINTERFACE   __declspec( dllexport )
#else
#define CHARACTERCONTROLCOREDLLINTERFACE   __declspec( dllimport )
#endif

#include <vector>
#include <memory>
#include <string>

namespace CharacterControlCore
{
	struct ImageInfo
	{
		std::string filename;
	};
	using ImageInfoList = std::vector<ImageInfo>;

	class CharacterInfo
	{
	public:
		virtual const char* Name() const = 0;
		virtual const ImageInfoList& CurrentImageStack() const = 0;
		virtual const ImageInfoList& ImageLibrary() const = 0;
	};

	class ControlCore
	{
	public:
		virtual void Update() = 0;
		virtual const CharacterControlCore::CharacterInfo* GetCharacter( size_t m_ID ) const = 0;
	};

	using ControlCorePtr = std::shared_ptr<ControlCore>;
	CHARACTERCONTROLCOREDLLINTERFACE ControlCorePtr GetCharacterControl(const char* controlFile);
}