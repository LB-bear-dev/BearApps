#pragma once
#include "../CharacterControlCore/Interface.h"
#include "Component.h"
#include "CharacterMap.h"
namespace CharacterControlCore
{
	class CharacterInfoInternal : public CharacterInfo
	{
	public:
		CharacterInfoInternal( const std::filesystem::path& rootPath, const Character& element );

		const char* Name() const;
		const ImageInfoList& CurrentImageStack() const;
		const ImageInfoList& ImageLibrary() const;

		ImageInfoList m_imageStack;

	private:
		std::string m_name;
		ImageInfoList m_imageLibrary;
	};
	using CharacterInfoList = std::unordered_map<size_t, CharacterInfoInternal>;

	using ComponentList = std::vector<std::unique_ptr<InputComponent>>;

	class CharacterControl : public ControlCore
	{
	public:
		CharacterControl( const char* filename);
		void Update();

		const CharacterControlCore::CharacterInfo* GetCharacter( size_t m_ID ) const;
	private:
		void UpdateCharacterInfo( size_t characterID, Character& element );

		WatchedJson m_root;
		CharacterMap m_activeCharacters;
		CharacterInfoList m_characterList;
		ComponentList m_components;
	};
}