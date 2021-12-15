#include "PCH.h"
#include "CharacterCreator.h"

namespace
{
	std::filesystem::path GetPathFromRootFilename(const std::string& id, const std::filesystem::path& rootFilename)
	{
		std::filesystem::path path = rootFilename.parent_path();
		path /= id;
		path /= "characterconfig.json";
		return path;
	}

	std::filesystem::path GetPathFromRoot(const std::string& id, const CharacterControlCore::WatchedJson& rootFile)
	{
		return GetPathFromRootFilename(id, rootFile.GetPath());
	}
}

CharacterControlCore::CharacterCreator::CharacterCreator(std::string& filename):
	m_rootFile(filename)
{
}

void CharacterControlCore::CharacterCreator::Update()
{
	m_rootFile.Update();
}

CharacterControlCore::Character CharacterControlCore::CharacterCreator::GetCharacter(std::string id)
{
	return Character(GetPathFromRoot(id, m_rootFile));
}
