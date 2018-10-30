/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef FAST_QUEUE_H
#define FAST_QUEUE_H

#include "common.h"

template <class T>
class FastQueue {
protected:
    T* m_data;
    unsigned int m_head;
    unsigned int m_tail;
    unsigned int m_capacity;

    void grow();

public:
    FastQueue();
    FastQueue(unsigned int init_capacity);
    FastQueue(const FastQueue<T>& other);
    FastQueue<T>& operator=(const FastQueue<T>& other);
    ~FastQueue();

    unsigned int size() const;
    bool empty() const;
    void clear(); // preserves space
    void reset(); // frees space

    void enqueue(const T& item);
    void enqueue2(T item);

    void dequeue(T& item_out); // does not check if empty
    T dequeue2(); // does not check if empty
};

template <class T>
FastQueue<T>::FastQueue()
    : m_head(0), m_tail(0), m_capacity(2)
{
    m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));
}

template <class T>
FastQueue<T>::FastQueue(unsigned int init_capacity)
    : m_head(0), m_tail(0), m_capacity(init_capacity)
{
    if (m_capacity < 2) m_capacity = 2;
    m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));
}

template <class T>
FastQueue<T>::FastQueue(const FastQueue<T>& other)
    : m_head(other.m_head), m_tail(other.m_tail), m_capacity(other.m_capacity)
{
    m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));

	// Deep copy
	unsigned int i;
	if (m_head > m_tail) {
		for (i = 0; i < m_tail; ++i)
			new (m_data + i) T(other.m_data[i]);
		for (i = m_head; i < m_capacity; ++i)
			new (m_data + i) T(other.m_data[i]);
	}
	else {
		for (i = m_head; i < m_tail; ++i)
			new (m_data + i) T(other.m_data[i]);
	}
}

template <class T>
FastQueue<T>& FastQueue<T>::operator=(const FastQueue<T>& other) {
    // Check for self-assignment
    if (this != &other) {
		unsigned int i;

		// Destruct
		if (m_head > m_tail) {
			for (i = 0; i < m_tail; ++i)
				(m_data + i)->~T();
			for (i = m_head; i < m_capacity; ++i)
				(m_data + i)->~T();
		}
		else {
			for (i = m_head; i < m_tail; ++i)
				(m_data + i)->~T();
		}

        // Dispose
        free(m_data);

        // Assign new data
        m_head = other.m_head;
        m_tail = other.m_tail;
        m_capacity = other.m_capacity;

		// Allocate
        m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));
        
		// Deep copy
		if (m_head > m_tail) {
			for (i = 0; i < m_tail; ++i)
				new (m_data + i) T(other.m_data[i]);
			for (i = m_head; i < m_capacity; ++i)
				new (m_data + i) T(other.m_data[i]);
		}
		else {
			for (i = m_head; i < m_tail; ++i)
				new (m_data + i) T(other.m_data[i]);
		}
    }
    return *this;
}

template <class T>
FastQueue<T>::~FastQueue() {
	unsigned int i;

	// Destruct
	if (m_head > m_tail) {
		for (i = 0; i < m_tail; ++i)
			(m_data + i)->~T();
		for (i = m_head; i < m_capacity; ++i)
			(m_data + i)->~T();
	}
	else {
		for (i = m_head; i < m_tail; ++i)
			(m_data + i)->~T();
	}

	// Dispose
    free(m_data);
}

template <class T>
void FastQueue<T>::grow() {
    unsigned int next = m_tail + 1;

    // Grow if full
    if (next == m_head || (next == m_capacity && m_head == 0)) {
        unsigned int new_cap = m_capacity << 1;
        T* new_data = reinterpret_cast<T*>(malloc(sizeof(T) * new_cap));

		// Deep copy isn't necessary here
        if (m_head > m_tail) {
            memcpy(new_data, m_data + m_head, (m_capacity - m_head) * sizeof(T));
            memcpy(new_data + (m_capacity - m_head), m_data, m_tail * sizeof(T));
            m_tail += m_capacity - m_head;
        }
        else {
            memcpy(new_data, m_data + m_head, (m_tail - m_head) * sizeof(T));
            m_tail -= m_head;
        }

        free(m_data);

        m_head = 0;
        m_capacity = new_cap;
        m_data = new_data;
    }
}

template <class T>
unsigned int FastQueue<T>::size() const {
    if (m_head > m_tail)
        return m_tail + m_capacity - m_head;
    else
        return m_tail - m_head;
}

template <class T>
bool FastQueue<T>::empty() const {
    return m_head == m_tail;
}

template <class T>
void FastQueue<T>::clear() {
	// Destruct
	unsigned int i;
	if (m_head > m_tail) {
		for (i = 0; i < m_tail; ++i)
			(m_data + i)->~T();
		for (i = m_head; i < m_capacity; ++i)
			(m_data + i)->~T();
	}
	else {
		for (i = m_head; i < m_tail; ++i)
			(m_data + i)->~T();
	}

    m_head = 0;
    m_tail = 0;
}

template <class T>
void FastQueue<T>::reset() {
	clear();

    m_capacity = 2;
    free(m_data);
    m_data = reinterpret_cast<T*>(malloc(sizeof(T) * m_capacity));
}

template <class T>
void FastQueue<T>::enqueue(const T& item) {
    grow(); // update size if necessary

	new (m_data + m_tail) T(item);

    ++m_tail;
    if (m_tail == m_capacity) m_tail = 0;
}

template <class T>
void FastQueue<T>::enqueue2(T item) {
    grow(); // update size if necessary

	new (m_data + m_tail) T(item);

    ++m_tail;
    if (m_tail == m_capacity) m_tail = 0;
}

template <class T>
void FastQueue<T>::dequeue(T& item_out) {
    item_out = m_data[m_head];

	(m_data + m_head)->~T();

    ++m_head;
    if (m_head == m_capacity) m_head = 0;
}

template <class T>
T FastQueue<T>::dequeue2() {
	T k(m_data[m_head]);

	(m_data + m_head)->~T();

    ++m_head;
    if (m_head == m_capacity) m_head = 0;

    return k;
}

#endif  /* FAST_QUEUE_H */
