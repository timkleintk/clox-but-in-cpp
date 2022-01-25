#pragma once
#include "memory.h"

template <typename T>
struct Blob
{
	Blob() = default;
	Blob(const Blob& other) = delete;
	Blob(Blob&& other);
	Blob& operator=(const Blob& other) = delete;
	Blob& operator=(Blob&& other);
	virtual ~Blob();

	void write(T entry);

	T& operator[](size_t index);
	const T& operator[](size_t index) const;

	size_t size() const { return m_count; }

private:
	size_t m_count{0};
	size_t m_capacity{0};
	T* m_ptr{nullptr}; // nts: better name
};


// -----------------------------------------------------------------

template <typename T>
Blob<T>::Blob(Blob&& other)
{
	m_count = other.m_count;
	m_capacity = other.m_capacity;
	m_ptr = other.m_ptr;
	other.m_ptr = nullptr;
}

template <typename T>
Blob<T>& Blob<T>::operator=(Blob&& other)
{
	m_count = other.m_count;
	m_capacity = other.m_capacity;
	m_ptr = other.m_ptr;
	other.m_ptr = nullptr;
	return *this;
}

template <typename T>
Blob<T>::~Blob()
{
	FREE_ARRAY(T, m_ptr, m_capacity);
}

template <typename T>
void Blob<T>::write(T entry)
{
	if (m_capacity < m_count + 1)
	{
		const size_t oldCapacity = m_capacity;
		m_capacity = GROW_CAPACITY(oldCapacity);
		m_ptr = GROW_ARRAY(T, m_ptr, oldCapacity, m_capacity);
	}
	m_ptr[m_count++] = entry;
}

template <typename T>
T& Blob<T>::operator[](size_t index)
{ return m_ptr[index]; }

template <typename T>
const T& Blob<T>::operator[](size_t index) const
{ return m_ptr[index]; }
