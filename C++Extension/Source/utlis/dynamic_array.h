/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "common.h"

template <class T>
class DynamicArray {
public:

    T* m_data;
    unsigned int m_capacity;
    unsigned int m_size;

    void grow();

    DynamicArray();
    DynamicArray(unsigned int init_capacity);
    DynamicArray(const DynamicArray<T>& other);
    DynamicArray<T>& operator=(const DynamicArray<T>& other);
    ~DynamicArray();

	unsigned int size_in_bytes() const;
    unsigned int size() const;
    void clear(); // preserves space
    void reset(); // frees space
    bool empty() const;
	unsigned int find(const T& item) const; // Returns size() if not found
	unsigned int find2(T item) const; // Returns size() if not found
    bool contains(const T& item) const;
    bool contains2(T item) const;
    void append(const T& item);
    void append2(T item);
	unsigned int get_append_index();
    T pop();
	void pop2(T& out);

    // Warning: the ordering is not preserved
    void remove_at(unsigned int index);
    void remove_first(const T& item);
    void remove_first2(T item);
	void remove_all(const T& item);
	void remove_all2(T item);

	T* pat(unsigned int index) const;
    T& operator[](unsigned int index); // does not perform boundary checks
    const T& operator[](unsigned int index) const; // does not perform boundary checks
};


template <class T>
DynamicArray<T>::DynamicArray()
    : m_capacity(1), m_size(0)
{
    m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));
}

template <class T>
DynamicArray<T>::DynamicArray(unsigned int init_capacity)
    : m_capacity(init_capacity), m_size(0)
{
    if (m_capacity == 0) m_capacity = 1;
    m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));
}

template <class T>
DynamicArray<T>::DynamicArray(const DynamicArray<T>& other)
    : m_capacity(other.m_capacity), m_size(other.m_size)
{
	// Allocate
    m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));

	// Deep copy
	unsigned int i = 0;
	for (; i < m_size; ++i)
		new (m_data + i) T(other.m_data[i]);
}

template <class T>
DynamicArray<T>& DynamicArray<T>::operator=(const DynamicArray<T>& other) {
    if (&other != this) {
		unsigned int i;

		// Destruct
		for (i = 0; i < m_size; ++i)
			(m_data + i)->~T();

		// Deallocate
		free(m_data);

		// Assign new info
		m_capacity = other.m_capacity;
		m_size = other.m_size;

		// Allocate
		m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));

		// Deep copy
		for (i = 0; i < m_size; ++i)
			new (m_data + i) T(other.m_data[i]);
    }
    return *this;
}

template <class T>
DynamicArray<T>::~DynamicArray() {
	// Destruct
	unsigned int i = 0;
	for (; i < m_size; ++i)
		(m_data + i)->~T();

	// Deallocate
	free(m_data);
}

template <class T>
void DynamicArray<T>::grow() {
    if (m_size == m_capacity) {
        m_capacity <<= 1;
        T* orig_data = m_data;

        m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));
        memcpy(m_data, orig_data, sizeof(T) * m_size);

        free(orig_data);
    }
}

template <class T>
unsigned int DynamicArray<T>::size_in_bytes() const {
	return sizeof(T) * m_size;
}

template <class T>
unsigned int DynamicArray<T>::size() const {
    return m_size;
}

template <class T>
void DynamicArray<T>::clear() {
	// Destruct
	unsigned int i;
	for (i = 0; i < m_size; ++i)
		(m_data + i)->~T();

	// Reset size
    m_size = 0;
}

template <class T>
void DynamicArray<T>::reset() {
	clear();

    m_capacity = 1;
    free(m_data);
    m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));
}

template <class T>
bool DynamicArray<T>::empty() const {
    return m_size == 0;
}

template <class T>
unsigned int DynamicArray<T>::find(const T& item) const {
	unsigned int i = 0;
	for (; i < m_size; ++i)
		if (m_data[i] == item) break;
	return i;
}

template <class T>
unsigned int DynamicArray<T>::find2(T item) const {
	unsigned int i = 0;
	for (; i < m_size; ++i)
		if (m_data[i] == item) break;
	return i;
}

template <class T>
bool DynamicArray<T>::contains(const T& item) const {
    unsigned int i = 0;
    for (; i < m_size; ++i)
        if (m_data[i] == item)
            return true;
	return false;
}

template <class T>
bool DynamicArray<T>::contains2(T item) const {
    unsigned int i = 0;
    for (; i < m_size; ++i)
        if (m_data[i] == item)
            return true;
}

template <class T>
void DynamicArray<T>::append(const T& item) {
    grow();

	new (m_data + m_size) T(item);

    ++m_size;
}

template <class T>
void DynamicArray<T>::append2(T item) {
    grow();

	new (m_data + m_size) T(item);

    ++m_size;
}

template <class T>
T DynamicArray<T>::pop() {
    --m_size;

	T v(m_data[m_size]);

	(m_data + m_size)->~T();

	return v;
}

template <class T>
void DynamicArray<T>::pop2(T& out) {
	--m_size;
	out = m_data[m_size];
	(m_data + m_size)->~T();
}

template <class T>
void DynamicArray<T>::remove_at(unsigned int index) {
	(m_data + index)->~T();
    --m_size;
    if (index != m_size) {
		memcpy(m_data + index, m_data + m_size, sizeof(T));
    }
}

template <class T>
void DynamicArray<T>::remove_first(const T& item) {
    unsigned int i = 0;
	while (i < m_size) {
		if (m_data[i] == item) {
			remove_at(i);
		}
		else
			++i;
	}
}

template <class T>
void DynamicArray<T>::remove_first2(T item) {
    unsigned int i = 0;
	while (i < m_size) {
		if (m_data[i] == item) {
			remove_at(i);
			return;
		}
		else
			++i;
	}
}

template <class T>
void DynamicArray<T>::remove_all(const T& item) {
    unsigned int i = 0;
    while (i < m_size) {
        if (m_data[i] == item) {
            remove_at(i);
        }
        else
            ++i;
    }
}

template <class T>
void DynamicArray<T>::remove_all2(T item) {
    unsigned int i = 0;
    while (i < m_size) {
        if (m_data[i] == item) {
            remove_at(i);
        }
        else
            ++i;
    }
}

template <class T>
unsigned int DynamicArray<T>::get_append_index() {
    grow(); // update size if necessary

	new (m_data + m_size) T;

    return m_size++;
}

template <class T>
T* DynamicArray<T>::pat(unsigned int index) const {
	return m_data + index;
}

template <class T>
T& DynamicArray<T>::operator[](unsigned int index) {
    return m_data[index];
}

template <class T>
const T& DynamicArray<T>::operator[](unsigned int index) const {
    return m_data[index];
}

#endif  /* DYNAMIC_ARRAY_H */
