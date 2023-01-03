#pragma once
#include  <shared_mutex>

namespace CharacterControlCore
{
	//wraps a class with a mutex, provides access to the class while taking in the mutex
	template<typename T>
	class ThreadSafe
	{
	public:
		ThreadSafe(
			const T& object,
			std::shared_mutex& mutex) :
			m_object(object),
			m_mutex(mutex)
		{
			m_mutex.lock_shared();
		}

		~ThreadSafe()
		{
			m_mutex.unlock_shared();
		}

		T* operator->()
		{
			return &m_object;
		}
	private:
		const T& m_object;
		std::shared_mutex& m_mutex;
	};

	template<typename T>
	class ThreadSafeMutable
	{
	public:
		ThreadSafeMutable(
			T& object,
			std::shared_mutex& mutex) :
			m_object(object),
			m_mutex(mutex) 
		{
			m_mutex.lock();
		}

		~ThreadSafeMutable()
		{
			m_mutex.unlock();
		}

		T* operator->()
		{
			return &m_object;
		}
	private:
		T& m_object;
		std::shared_mutex& m_mutex;
	};
}