#pragma once

#if defined(CHARACTERCONTROLCORE)
#define CHARACTERCONTROLCOREDLLINTERFACE   __declspec( dllexport )
#else
#define CHARACTERCONTROLCOREDLLINTERFACE   __declspec( dllimport )
#endif

#include <filesystem>
#include <vector>

namespace CharacterControlCore
{
	struct ImageInfo
	{
		ImageInfo(const std::string& _imageName): imageName ( _imageName ) {}

		std::string imageName;
	};

	struct CharacterInfo
	{
		std::string name;
		std::string ID;
		std::vector<ImageInfo> imageInfos;
	};

	using CharacterInfoList = std::vector<CharacterControlCore::CharacterInfo>;
	class ControlCore
	{
	public:
		virtual ~ControlCore() {}
		virtual void Update() = 0;
		virtual const std::filesystem::path& GetContentRoot() = 0;
		virtual const CharacterInfoList& GetCharacterList() = 0;
	};

	using ControlCorePtr = std::shared_ptr<ControlCore>;
	CHARACTERCONTROLCOREDLLINTERFACE ControlCorePtr GetCharacterControl(std::string controlFile);
}