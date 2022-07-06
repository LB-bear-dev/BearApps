#pragma once
#include "Character.h"
#pragma optimize("", off)
namespace CharacterControlCore
{
	struct ThreadSafeCharacter
	{
		ThreadSafeCharacter(Character&& c):
			character(std::forward<Character>(c)){}

		std::shared_mutex mutex;
		Character character;
	};
	using ThreadSafeCharacterMap = std::map<std::string, ThreadSafeCharacter>;

	class CharacterMapInstance
	{
	private:
	public:
		CharacterMapInstance(
			ThreadSafeCharacterMap* container,
			std::shared_mutex* containerMutex):
			m_container(container),
			m_containerMutex(containerMutex){}

		~CharacterMapInstance()
		{
			m_containerMutex->unlock();
		}

		ThreadSafeCharacterMap* operator->()
		{
			return m_container;
		}

	private:
		ThreadSafeCharacterMap* m_container;
		std::shared_mutex* m_containerMutex;
	};

	class CharacterMapElementInstance
	{
	public:
		CharacterMapElementInstance(
			const Character* element,
			std::shared_mutex* containerMutex,
			std::shared_mutex* elementMutex) :
			m_element(element),
			m_containerMutex(containerMutex),
			m_elementMutex(elementMutex){}

		~CharacterMapElementInstance()
		{
			m_containerMutex->unlock_shared();
			m_elementMutex->unlock_shared();
		}

		bool Valid()
		{
			return m_element && m_containerMutex && m_elementMutex;
		}

		const Character* operator->() const
		{
			return m_element;
		}

	private:
		const Character* m_element;
		std::shared_mutex* m_containerMutex;
		std::shared_mutex* m_elementMutex;
	};

	class CharacterMapElementInstanceMutable
	{
	public:
		CharacterMapElementInstanceMutable(
			Character* element,
			std::shared_mutex* containerMutex,
			std::shared_mutex* elementMutex) :
			m_element(element),
			m_containerMutex(containerMutex),
			m_elementMutex(elementMutex){}

		~CharacterMapElementInstanceMutable()
		{
			m_containerMutex->unlock_shared();
			m_elementMutex->unlock();
		}

		bool Valid()
		{
			return m_element && m_containerMutex && m_elementMutex;
		}

		Character* operator->() const
		{
			return m_element;
		}

	private:
		Character* m_element;
		std::shared_mutex* m_containerMutex;
		std::shared_mutex* m_elementMutex;
	};

	class CharacterMap
	{
	private:
		using Vector = Concurrency::concurrent_vector<std::string>;

	public:
		//Get full access to the container, blocks all threads but you get to do whatever you want
		CharacterMapInstance GetRawContainerBlocking()
		{
			m_containerMutex.lock();
			m_updated.clear();
			return CharacterMapInstance(&m_container, &m_containerMutex);
		}

		CharacterMapElementInstance GetCharacter(const std::string& key)
		{
			m_containerMutex.lock_shared();
			auto element = m_container.find(key);
			bool exists = element != m_container.end();

			if (exists)
			{
				element->second.mutex.lock_shared();

				//Does not return container lock until the element instance has been released
				return CharacterMapElementInstance(&element->second.character, &m_containerMutex, &element->second.mutex);
			}

			m_containerMutex.unlock_shared();
			return CharacterMapElementInstance(nullptr, nullptr, nullptr);
		}

		CharacterMapElementInstanceMutable GetCharacterMutable(const std::string& key)
		{
			m_containerMutex.lock_shared();
			auto element = m_container.find(key);
			bool exists = element != m_container.end();

			if (exists)
			{
				m_updated.push_back(element->first);
				element->second.mutex.lock();

				//Does not return container lock until the element instance has been released
				return CharacterMapElementInstanceMutable(&element->second.character, &m_containerMutex, &element->second.mutex);
			}

			m_containerMutex.unlock_shared();
			return CharacterMapElementInstanceMutable(nullptr, nullptr, nullptr);
		}

		Vector GetUpdatedCharacters()
		{
			std::scoped_lock(m_containerMutex);
			Vector updatedCopy = m_updated;
			m_updated.clear();
			return updatedCopy;
		}

	private:
		ThreadSafeCharacterMap m_container;
		Vector m_updated;
		std::shared_mutex m_containerMutex;
	};
}