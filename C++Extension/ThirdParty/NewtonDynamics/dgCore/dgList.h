/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __dgList__
#define __dgList__

#include "dgStdafx.h"
#include "dgRef.h"
#include "dgDebug.h"
#include "dgMemory.h"



template<class T>
class dgList 
{
	public:
	class dgListNode
	{
		DG_CLASS_ALLOCATOR(allocator)

		dgListNode (dgListNode* const prev, dgListNode* const next) 
			:m_info () 
		{
			m_prev = prev;
			m_next = next;
			if (m_prev) {
				m_prev->m_next = this;
			}
			if (m_next) {
				m_next->m_prev = this;
			}
		}

		dgListNode (const T &info, dgListNode* const prev, dgListNode* const next) 
			:m_info (info) 
		{
			m_prev = prev;
			m_next = next;
			if (m_prev) {
				m_prev->m_next = this;
			}
			if (m_next) {
				m_next->m_prev = this;
			}
		}

		virtual ~dgListNode()
		{
		}

		void Unlink ()
		{
			if (m_prev) {
				m_prev->m_next = m_next;
			}

			if (m_next) {
				m_next->m_prev = m_prev;
			}
			m_prev = NULL;
			m_next = NULL;
		}

		void AddLast(dgListNode* const node) 
		{
			m_next = node;
			node->m_prev = this;
		}

		void AddFirst(dgListNode* const node) 
		{
			m_prev = node;
			node->m_next = this;
		}

		public:
		T& GetInfo()
		{
			return m_info;
		}

		dgListNode *GetNext() const
		{
			return m_next;
		}

		dgListNode *GetPrev() const
		{
			return m_prev;
		}

		private:
		T m_info;
		dgListNode *m_next;
		dgListNode *m_prev;
		friend class dgList<T>;

	};

	class Iterator
	{
		public:
		Iterator (const dgList<T> &me)
		{
			m_ptr = NULL;
			m_list = (dgList *)&me;
		}

		~Iterator ()
		{
		}

		operator dgInt32() const
		{
			return m_ptr != NULL;
		}

		bool operator== (const Iterator &target) const
		{
			return (m_ptr == target.m_ptr) && (m_list == target.m_list);
		}

		void Begin()
		{
			m_ptr = m_list->GetFirst();
		}

		void End()
		{
			m_ptr = m_list->GetLast();
		}

		void Set (dgListNode* const node)
		{
			m_ptr = node;
		}

		void operator++ ()
		{
			dgAssert (m_ptr);
			m_ptr = m_ptr->m_next();
		}

		void operator++ (dgInt32)
		{
			dgAssert (m_ptr);
			m_ptr = m_ptr->GetNext();
		}

		void operator-- () 
		{
			dgAssert (m_ptr);
			m_ptr = m_ptr->GetPrev();
		}

		void operator-- (dgInt32) 
		{
			dgAssert (m_ptr);
			m_ptr = m_ptr->GetPrev();
		}

		T &operator* () const
		{
			return m_ptr->GetInfo();
		}

		dgListNode *GetNode() const
		{
			return m_ptr;
		}

		private:
		dgList *m_list;
		dgListNode *m_ptr;
	};

	// ***********************************************************
	// member functions
	// ***********************************************************
	public:
	DG_CLASS_ALLOCATOR(allocator)

//	dgList ();
	dgList (dgMemoryAllocator* const allocator);
	virtual ~dgList ();

	dgMemoryAllocator* GetAllocator () const;
	void SetAllocator (dgMemoryAllocator* const allocator);

	operator dgInt32() const;
	dgInt32 GetCount() const;
	dgListNode *GetLast() const;
	dgListNode *GetFirst() const;
	dgListNode *Append ();
	dgListNode *Append (dgListNode* const node);
	dgListNode *Append (const T &element);
	dgListNode *Addtop ();
	dgListNode *Addtop (dgListNode* const node);
	dgListNode *Addtop (const T &element);
	
	void RotateToEnd (dgListNode* const node);
	void RotateToBegin (dgListNode* const node);
	void InsertAfter (dgListNode* const root, dgListNode* const node);
	void InsertBefore (dgListNode* const root, dgListNode* const node);


	dgListNode *Find (const T &element) const;
	dgListNode *GetNodeFromInfo (T &m_info) const;
	void Remove (dgListNode* const node);
	void Remove (const T &element);
	void RemoveAll ();

	void Merge (dgList<T>& list);
	void Unlink (dgListNode* const node);
	bool SanityCheck () const;


	// ***********************************************************
	// member variables
	// ***********************************************************
	private:
	dgInt32 m_count;
	dgListNode *m_first;
	dgListNode *m_last;
	dgMemoryAllocator* m_allocator;

//	static dgInt32 m_size;
//	static dgMemoryAllocator* m_staticAllocator;
	friend class dgListNode;
};

/*
template<class T>
dgList<T>::dgList ()
{
	m_count = 0;
	m_first = NULL;
	m_last = NULL;
	m_allocator = NULL;
}
*/

template<class T>
dgList<T>::dgList (dgMemoryAllocator* const allocator)
	:m_count(0)
	,m_first(NULL)
	,m_last(NULL)
	,m_allocator(allocator)
{
}


template<class T>
dgList<T>::~dgList () 
{
	RemoveAll ();
}

template<class T>
void dgList<T>::SetAllocator (dgMemoryAllocator* const allocator)
{
	if ((m_count == 0) && (m_allocator == NULL)) {
		m_allocator = allocator;
	}
}

template<class T>
dgMemoryAllocator* dgList<T>::GetAllocator () const
{
	return m_allocator;
}


template<class T>
dgInt32 dgList<T>::GetCount() const
{
	return m_count;
}

template<class T>
dgList<T>::operator dgInt32() const
{
	return m_first != NULL;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::GetFirst() const
{
	return m_first;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::GetLast() const
{
	return m_last;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::Append (dgListNode* const node)
{
	dgAssert (node->m_next == NULL);
	dgAssert (node->m_prev == NULL);
	m_count	++;
	if (m_first == NULL) {
		m_last = node;
		m_first = node;
	} else {
		m_last->AddLast (node);
		m_last = node;
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif
	return m_last;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::Append ()
{
	m_count	++;
	if (m_first == NULL) {
		m_first = new (m_allocator) dgListNode(NULL, NULL);
		m_last = m_first;
	} else {
		m_last = new (m_allocator) dgListNode(m_last, NULL);
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif
	return m_last;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::Append (const T &element)
{
	m_count	++;
	if (m_first == NULL) {
		m_first = new (m_allocator) dgListNode(element, NULL, NULL);
		m_last = m_first;
	} else {
		m_last = new (m_allocator) dgListNode(element, m_last, NULL);
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif

	return m_last;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::Addtop (dgListNode* const node)
{
	dgAssert (node->m_next == NULL);
	dgAssert (node->m_prev == NULL);
	m_count	++;
	if (m_last == NULL) {
		m_last = node;
		m_first = node;
	} else {
		m_first->AddFirst(node);
		m_first = node;
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif
	return m_first;
}


template<class T>
typename dgList<T>::dgListNode *dgList<T>::Addtop ()
{
	m_count	++;
	if (m_last == NULL) {
		m_last = new (m_allocator) dgListNode(NULL, NULL);
		m_first = m_last;
	} else {
		m_first = new (m_allocator) dgListNode(NULL, m_first);
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif
	return m_first;
}


template<class T>
typename dgList<T>::dgListNode *dgList<T>::Addtop (const T &element)
{
	m_count	++;
	if (m_last == NULL) {
		m_last = new (m_allocator) dgListNode(element, NULL, NULL);
		m_first = m_last;
	} else {
		m_first = new (m_allocator) dgListNode(element, NULL, m_first);
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif
	return m_first;
}

template<class T>
void dgList<T>::InsertAfter (dgListNode* const root, dgListNode* const node)
{
	dgAssert (root);
	dgAssert (node != root);
	
	if (root->m_next != node) {
		if (node == m_first) {
			m_first = node->m_next;
		}
		if (node == m_last) {
			m_last = node->m_prev;
		}
		node->Unlink ();
		
		node->m_prev = root;
		node->m_next = root->m_next;
		if (root->m_next) {
			root->m_next->m_prev = node;
		} 
		root->m_next = node;

		if (node->m_next == NULL) {
			m_last = node;
		}

		dgAssert (m_last);
		dgAssert (!m_last->m_next);
		dgAssert (m_first);
		dgAssert (!m_first->m_prev);
		dgAssert (SanityCheck ());
	}
}


template<class T>
void dgList<T>::InsertBefore (dgListNode* const root, dgListNode* const node)
{
	dgAssert (root);
	dgAssert (node != root);
	
	if (root->m_prev != node) {
		if (node == m_last) {
			m_last = node->m_prev;
		}
		if (node == m_first) {
			m_first = node->m_next;
		}
		node->Unlink ();
		
		node->m_next = root;
		node->m_prev = root->m_prev;
		if (root->m_prev) {
			root->m_prev->m_next = node;
		} 
		root->m_prev = node;

		if (node->m_prev == NULL) {
			m_first = node;
		}

		dgAssert (m_first);
		dgAssert (!m_first->m_prev);
		dgAssert (m_last);
		dgAssert (!m_last->m_next);
		dgAssert (SanityCheck ());
	}
}


template<class T>
void dgList<T>::RotateToEnd (dgListNode* const node)
{
	if (node != m_last) {
		if (m_last != m_first) {
			if (node == m_first) {
				m_first = m_first->GetNext();
			}
			node->Unlink();
			m_last->AddLast(node);
			m_last = node; 
		}
	}

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif
}

template<class T>
void dgList<T>::RotateToBegin (dgListNode* const node)
{
	if (node != m_first) {
		if (m_last != m_first) {
			if (node == m_last) {
				m_last = m_last->GetPrev();
			}
			node->Unlink();
			m_first->AddFirst(node);
			m_first = node; 
		}
	}

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif
}


template<class T>
typename dgList<T>::dgListNode *dgList<T>::Find (const T &element) const
{
	dgListNode *node;
	for (node = m_first; node; node = node->GetNext()) {
		if (element	== node->m_info) {
			break;
		}
	}
	return node;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::GetNodeFromInfo (T &info) const
{
	dgListNode* const node = (dgListNode *) &info;
	dgInt64 offset = ((char*) &node->m_info) - ((char *) node);
	dgListNode* const retnode = (dgListNode *) (((char *) node) - offset);

	dgAssert (&retnode->GetInfo () == &info);
	return retnode;
}


template<class T> 
void dgList<T>::Remove (const T &element)
{
	dgListNode *const node = Find (element);
	if (node) {
		Remove (node);
	}
}

template<class T> 
void dgList<T>::Unlink (dgListNode* const node)
{
	dgAssert (node);

	m_count --;
	dgAssert (m_count >= 0);

	if (node == m_first) {
		m_first = m_first->GetNext();
	}
	if (node == m_last) {
		m_last = m_last->GetPrev();
	}
	node->Unlink();

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif
}

template<class T> 
void dgList<T>::Merge (dgList<T>& list)
{
	m_count += list.m_count;
	if (list.m_first) {
		list.m_first->m_prev = m_last; 
	}
	if (m_last) {
		m_last->m_next = list.m_first;
	}
	m_last = list.m_last;
	if (!m_first) {
		m_first = list.m_first;
	}

	list.m_count = 0;
	list.m_last = NULL;
	list.m_first = NULL;
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dgAssert (SanityCheck ());
#endif
}


template<class T>
void dgList<T>::Remove (dgListNode* const node)
{
	Unlink (node);
	delete node;
}


template<class T>
void dgList<T>::RemoveAll ()
{
	for (dgListNode *node = m_first; node; node = m_first) {
		m_count --;
		m_first = node->GetNext();
		node->Unlink();
		delete node;
	}
	dgAssert (m_count == 0);
	m_last = NULL;
	m_first = NULL;
}

template<class T>
bool dgList<T>::SanityCheck () const
{
	#ifdef _DEBUG
	dgInt32 tCount = 0;
	for (dgListNode * node = m_first; node; node = node->GetNext()) {
		tCount ++;
		if (node->GetPrev()) {
			dgAssert (node->GetPrev() != node->GetNext());
			if (node->GetPrev()->GetNext() != node) {
				dgAssert (0);
				return false; 
			}
		}
		if (node->GetNext()) {
			dgAssert (node->GetPrev() != node->GetNext());
			if (node->GetNext()->GetPrev() != node)	{
				dgAssert (0);
				return false;
			}
		}
	}
	if (tCount != m_count) {
		dgAssert (0);
		return false;
	}
	#endif
	return true;
}


//template<class T>
//void dgList<T>::SetAllocator (dgMemoryAllocator * allocator)
//{
//	m_allocator = allocator;
//}

//template<class T>
//dgMemoryAllocator* dgList<T>::GetAllocator () const
//{
//	return m_allocator;
//}
//template<class T> dgInt32 dgList <T>::m_size = 0;
//template<class T> dgMemoryAllocator* dgList<T>::m_staticAllocator = NULL;


#endif


