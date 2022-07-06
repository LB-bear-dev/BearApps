#pragma once

#if defined(CHARACTERCONTROLCORE)
#define DLLINTERFACE   __declspec( dllexport )
#else
#define DLLINTERFACE   __declspec( dllimport )
#endif

#include <filesystem>
#include <vector>

namespace CharacterControlCore
{
	DLLINTERFACE std::size_t GetImageInfoHash(const std::string& characterID, const std::string& imageName);

	struct Vector2D
	{
		float x;
		float y;
	};

	struct ImageInfo
	{
		ImageInfo(): x(0), y(0), imageToken(0) {}
		ImageInfo(float _x, float _y, const std::string& characterID, const std::string& imageName): x(_x), y(_y), imageToken(GetImageInfoHash(characterID, imageName)) {}

		union
		{
			struct
			{
				float x;
				float y;
			};
			Vector2D position;
		};
		std::size_t imageToken;
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
	DLLINTERFACE ControlCorePtr GetCharacterControl(std::string controlFile);

	DLLINTERFACE void InitControlUI(CharacterControlCore::ControlCorePtr controlPtr);
}