#pragma once
#include "WatchedImageLibrary.h"

namespace CharacterControlRender
{
	using FileImageLibrary = std::unordered_map<STRID, std::unique_ptr<gs_image_file4>>;


	class WatchedFileImageLibrary: public CharacterControlCore::WatchedFile<FileImageLibrary>
	{
	public:
		WatchedFileImageLibrary(PATHR filename) : CharacterControlCore::WatchedFile<FileImageLibrary>(filename), m_maxResolution{0,0}
		{
			Update();
		}

		WatchedFileImageLibrary(PATHR filename, STRIDR overrideName) : CharacterControlCore::WatchedFile<FileImageLibrary>(filename), m_overrideName(overrideName), m_maxResolution{0,0}
		{
			Update();
		}

		const std::vector<STRID>& GetImageNames() const;

		Coord GetMaxResolution() const;

	private:
		bool UpdateInternal() override;
		void DisposeInternal() override;
		std::vector<STRID> m_imageNames;
		STRID m_overrideName;

		Coord m_maxResolution;
	};

	class CharacterImageLibrary
	{
	public:
		void AddImageLibrary(PATHR filename);
		void AddImageLibrary(PATHR filename, STRIDR overrideName);
		std::vector<std::string> GetValidImageNames() const;
		gs_image_file4* GetImageFromName(STRIDR name) const;

		Coord GetMaxResolution() const;

	private:
		std::vector<std::shared_ptr<WatchedFileImageLibrary>> m_imageLibraries;
		static std::unordered_map<PATH, std::weak_ptr<CharacterControlRender::WatchedFileImageLibrary>> s_imageLibraries;
	};
}