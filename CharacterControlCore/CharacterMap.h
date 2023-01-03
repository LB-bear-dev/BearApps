#pragma once
#include "Character.h"
#pragma optimize("", off)
namespace CharacterControlCore
{
	class CharacterMap
	{
	private:
		using Vector = std::vector<size_t>;
		using Map = std::unordered_map<size_t, Character>;

	public:
		template< class... Args >
		Character* CreateCharacter( size_t key, Args&&... args )
		{
			auto container = m_container.try_emplace( key, std::forward<Args>(args)... );
			m_updated.push_back( key );

			return &container.first->second;
		}

		void RemoveCharacter( size_t key )
		{
			m_container.erase( key );
		}

		void Clear()
		{
			m_updated.clear();
			m_container.clear();
		}

		Character* GetCharacter( size_t key )
		{
			auto element = m_container.find(key);
			if ( element != m_container.end() )
			{
				return &element->second;
			}

			return nullptr;
		}

		Character* GetCharacterMutable( size_t key )
		{
			auto element = m_container.find( key );
			if ( element != m_container.end() )
			{
				m_updated.push_back( element->first );

				return &element->second;
			}

			return nullptr;
		}

		Vector GetUpdatedCharacters()
		{
			Vector updatedCopy = m_updated;
			m_updated.clear();
			return updatedCopy;
		}

	private:
		Map m_container;
		Vector m_updated;
	};
}