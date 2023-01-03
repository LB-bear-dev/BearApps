#pragma once
namespace CharacterControlCore
{
	class CharacterMap;
	class InputComponent
	{
	public:
		InputComponent(CharacterMap& characters, const std::filesystem::path& path );
		virtual ~InputComponent();
		virtual void Update() = 0;

	protected:
		CharacterMap& m_activeCharacters;
		const std::filesystem::path& m_path;
	};
}