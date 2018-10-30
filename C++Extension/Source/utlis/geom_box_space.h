/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef GEOM_BOX_SPACE_H
#define GEOM_BOX_SPACE_H

#include "geom.h"
#include "geom_bounding_box.h"
#include "fast_queue.h"

template <class T>
class Geom::BoxSpace
{
public:
    // Structures
    struct Item {
        T m_data;
        Geom::BoundingBox m_bb;
    };

	struct Node {
		unsigned int m_head;
		unsigned int m_tail;
		unsigned int m_next;
		Geom::BoundingBox m_bb;
	};

    // Callbacks

    // This callback is triggered on every update and is used for syncing the bounding boxes of all items.
    typedef void (*BoxUpdateCallback) (T item, BoundingBox& bb, void* user_data);

    // This callback is triggered when an item is determined overlapping a bounding box.
    // Return true to furthen overlap checks; false to abort.
    typedef bool (*BoxOverlapCallback) (T item, void* user_data);

    // This callback is triggered when two items are determined overlapping.
    // Return true to furthen overlap checks; false to abort.
    typedef bool (*ItemOverlapCallback) (T item1, T item2, void* user_data);

    typedef void (*GetItemsCallback) (Item* items_out, void* user_data);

    // Variables

    Item* m_items;
    Node* m_nodes;

    unsigned int m_min_items_per_node;
    unsigned int m_num_items;
    unsigned int m_num_nodes;
    unsigned int m_items_capacity;
    unsigned int m_nodes_capacity;

    // Functions

    inline void partition_aspects(unsigned int node_index, const char axis);

    BoxSpace(unsigned int min_items_per_node);

    BoxSpace(const BoxSpace<T>& other);
    BoxSpace<T>& operator=(const BoxSpace<T>& other);
    virtual ~BoxSpace();

    void set_items(
        unsigned int num_items,
        GetItemsCallback get_items_callback,
        void* user_data,
        bool powers_of_two_capacity);

	void add_item(const T& item);

    void get_bounds(Geom::BoundingBox& box_out) const;

    void update(
        BoxUpdateCallback box_update_callback,
        treal margin,
        void* user_data);

    // Triggers the callback for every two, different, overlapping items.
    // There are no redundancies!
    void overlap_self(
        ItemOverlapCallback overlap_callback,
        FastQueue<Pair>& cq,
        void* user_data) const;

    // Triggers the callback for every two, overlapping items, provided:
    //  - the items are from different box spaces
    // There are no redundancies!
    void overlap_with(
        const BoxSpace<T>& other,
        ItemOverlapCallback overlap_callback,
        FastQueue<Pair>& cq,
        void* user_data) const;

    // Triggers the callback for every item overlapping the box.
    // There are no redundancies!
    void overlap_with(
        const Geom::BoundingBox& box,
        BoxOverlapCallback overlap_callback,
        FastQueue<unsigned int>& cq,
        void* user_data) const;

	void overlap_with(
		const Geom::Vector3d& point,
		const Geom::Vector3d& vector,
		BoxOverlapCallback overlap_callback,
		FastQueue<unsigned int>& cq,
		void* user_data) const;
};

template <class T>
Geom::BoxSpace<T>::BoxSpace(unsigned int min_items_per_node) :
	m_num_items(0),
	m_num_nodes(1),
	m_items_capacity(2),
	m_nodes_capacity(2) // must be greater than 1
{
	if (min_items_per_node < 1)
		m_min_items_per_node = 1;
	else
		m_min_items_per_node = min_items_per_node;

	m_items = nullptr;
	m_nodes = reinterpret_cast<Node*>(malloc(sizeof(Node) * m_nodes_capacity));

	m_nodes[0].m_head = 0;
	m_nodes[0].m_tail = 0;
	m_nodes[0].m_next = 0;
	m_nodes[0].m_bb.clear();
}

template <class T>
Geom::BoxSpace<T>::BoxSpace(const BoxSpace<T>& other) :
	m_min_items_per_node(other.m_min_items_per_node),
	m_num_items(other.m_num_items),
	m_num_nodes(other.m_num_nodes),
	m_items_capacity(other.m_items_capacity),
	m_nodes_capacity(other.m_nodes_capacity)
{
	if (other.m_items) {
		m_items = reinterpret_cast<Item*>(malloc(sizeof(Item) * m_items_capacity));
		memcpy(m_items, other.m_items, sizeof(Item) * m_num_items);
	}
	else {
		m_items = nullptr;
	}
	if (other.m_nodes) {
		m_nodes = reinterpret_cast<Node*>(malloc(sizeof(Node) * m_nodes_capacity));
		memcpy(m_nodes, other.m_nodes, sizeof(Node) * m_num_nodes);
	}
	else {
		m_nodes = nullptr;
	}
}

template <class T>
Geom::BoxSpace<T>& Geom::BoxSpace<T>::operator=(const BoxSpace<T>& other) {
	if (this != &other) {
		m_nodes = other.m_nodes;

		if (m_items)
			free(m_items);
		if (m_nodes)
			free(m_nodes);

		m_min_items_per_node = other.m_min_items_per_node;
		m_num_items = other.m_num_items;
		m_num_nodes = other.m_num_nodes;
		m_items_capacity = other.m_items_capacity;
		m_nodes_capacity = other.m_nodes_capacity;

		if (other.m_items) {
			m_items = reinterpret_cast<Item*>(malloc(sizeof(Item) * m_items_capacity));
			memcpy(m_items, other.m_items, sizeof(Item) * m_num_items);
		}
		else {
			m_items = nullptr;
		}
		if (other.m_nodes) {
			m_nodes = reinterpret_cast<Node*>(malloc(sizeof(Node) * m_nodes_capacity));
			memcpy(m_nodes, other.m_nodes, sizeof(Node) * m_num_nodes);
		}
		else {
			m_nodes = nullptr;
		}
	}
	return *this;
}

template <class T>
Geom::BoxSpace<T>::~BoxSpace() {
	if (m_items)
		free(m_items);
	if (m_nodes)
		free(m_nodes);
}

template <class T>
void Geom::BoxSpace<T>::set_items(
	unsigned int num_items,
	GetItemsCallback get_items_callback,
	void* user_data,
	bool powers_of_two_capacity)
{
	m_num_items = num_items;

	if (m_items_capacity < m_num_items) {
		if (powers_of_two_capacity) {
			m_items_capacity = 2;
			while (m_items_capacity < m_num_items)
				m_items_capacity <<= 1;
		}
		else {
			m_items_capacity = m_num_items;
		}
		if (m_items) {
			free(m_items);
			m_items = nullptr;
		}
	}

	if (m_items == nullptr) {
		m_items = reinterpret_cast<Item*>(malloc(sizeof(Item) * m_items_capacity));
	}

	m_nodes[0].m_tail = m_num_items;

	get_items_callback(m_items, user_data);
}

template <class T>
void Geom::BoxSpace<T>::add_item(const T& item) {
	if (m_num_items == m_items_capacity) {
		m_items_capacity = 2;
		while (m_items_capacity <= m_num_items)
			m_items_capacity <<= 1;

		if (m_items) {
			Item* new_items = reinterpret_cast<Item*>(malloc(sizeof(Item) * m_items_capacity));
			memcpy(new_items, m_items, sizeof(Item) * m_num_items);
			free(m_items);
			m_items = new_items;
		}
	}
	
	if (m_items == nullptr) {
		m_items = reinterpret_cast<Item*>(malloc(sizeof(Item) * m_items_capacity));
	}

	m_items[m_num_items].m_data = item;
	++m_num_items;
	++m_nodes[0].m_tail;
}

template <class T>
void Geom::BoxSpace<T>::partition_aspects(unsigned int node_index, const char axis) {
	Item t;
	unsigned int i, m1, m2;
	Node node;

	memcpy(&node, m_nodes + node_index, sizeof(Node));
	
	assert(node.m_bb.m_min[axis] < node.m_bb.m_max[axis]);

	// Compute centre
	treal centre = (node.m_bb.m_min[axis] + node.m_bb.m_max[axis]) * (treal)(0.5);
	// Partition
	// - all boxes with maximums less than centre go left
	// - all boxes with minimums greater than centre go right
	// - all other boxes become a part of the middle node

	// Move all aspects with maximums less than or equal to centre to the left
	m1 = node.m_head;
	i = node.m_tail - 1;
	while (m1 < i) {
		// Find a box with maximum greater than centre
		while (m1 < i && m_items[m1].m_bb.m_max[axis] <= centre) ++m1;
		if (m1 == i) break;
		// Find a box with maximum less than or equal to centre
		while (m1 < i && m_items[i].m_bb.m_max[axis] > centre) --i;
		if (m1 == i) break;
		// Swap
		t = m_items[i];
		m_items[i] = m_items[m1];
		m_items[m1] = t;
	}

	// Move all aspects with minimums less than centre to left (but to right of m1) and all others to right
	m2 = m1;
	i = node.m_tail - 1;
	while (m2 < i) {
		// Find a box with minimum greater than centre
		while (m2 < i && m_items[m2].m_bb.m_min[axis] <= centre) ++m2;
		if (m2 == i) break;
		// Find a box with minimum less than or equal to centre
		while (m2 < i && m_items[i].m_bb.m_min[axis] > centre) --i;
		if (m2 == i) break;
		// Swap
		t = m_items[i];
		m_items[i] = m_items[m2];
		m_items[m2] = t;
	}

	// Make this node a leaf node if left and right nodes are empty
	if (m1 != node.m_head || m2 != node.m_tail) {
		Node* znode;

		// Allocate space for nodes if necessary
		if (m_num_nodes + 3 > m_nodes_capacity) {
			znode = m_nodes;
			m_nodes_capacity <<= 1;
			m_nodes = reinterpret_cast<Node*>(malloc(sizeof(Node) * m_nodes_capacity));
			memcpy(m_nodes, znode, sizeof(Node) * m_num_nodes);
			free(znode);
		}

		m_nodes[node_index].m_next = m_num_nodes;

		znode = m_nodes + m_num_nodes;
		znode->m_head = node.m_head;
		znode->m_tail = m1;
		znode->m_next = 0;
		znode->m_bb.clear();
		for (i = node.m_head; i < m1; ++i)
			znode->m_bb.add(m_items[i].m_bb);

		++znode;
		znode->m_head = m1;
		znode->m_tail = m2;
		znode->m_next = 0;
		znode->m_bb.clear();
		for (i = m1; i < m2; ++i)
			znode->m_bb.add(m_items[i].m_bb);

		++znode;
		znode->m_head = m2;
		znode->m_tail = node.m_tail;
		znode->m_next = 0;
		znode->m_bb.clear();
		for (i = m2; i < node.m_tail; ++i)
			znode->m_bb.add(m_items[i].m_bb);

		m_num_nodes += 3;
	}
}

template <class T>
void Geom::BoxSpace<T>::get_bounds(Geom::BoundingBox& box_out) const {
	box_out = m_nodes[0].m_bb;
}

template <class T>
void Geom::BoxSpace<T>::update(
	BoxUpdateCallback box_update_callback,
	treal margin,
	void* user_data)
{
	Item* item;
	Geom::Vector3d diff;
	unsigned int i;
	//char axis;

	m_num_nodes = 1;
	m_nodes[0].m_next = 0;

	// Sync item bounding boxes
	m_nodes[0].m_bb.clear();
	for (i = 0; i < m_num_items; ++i) {
		item = m_items + i;
		box_update_callback(item->m_data, item->m_bb, user_data);
		item->m_bb.pad_out(margin);
		m_nodes[0].m_bb.add(item->m_bb);
	}

	// Partition until any two of the ranges are zero
	i = 0;
	while (i < m_num_nodes) {
		// Divide <=> a node has more than a minimum, required number of nodes
		if (m_nodes[i].m_tail - m_nodes[i].m_head > m_min_items_per_node) {
			// Find an axis with the greatest min/max difference
			diff = m_nodes[i].m_bb.m_max - m_nodes[i].m_bb.m_min;
			if (diff.m_x > diff.m_y) {
				if (diff.m_x > diff.m_z) {
					if (diff.m_x > M_EPSILON2)
						partition_aspects(i, 0);
				}
				else if (diff.m_z > M_EPSILON2) {
					partition_aspects(i, 2);
				}
			}
			else if (diff.m_y > diff.m_z) {
				if (diff.m_y > M_EPSILON2)
					partition_aspects(i, 1);
			}
			else if (diff.m_z > M_EPSILON2) {
				partition_aspects(i, 2);
			}
		}
		++i;
	}
}

template <class T>
void Geom::BoxSpace<T>::overlap_self(
	ItemOverlapCallback overlap_callback,
	FastQueue<Pair>& cq,
	void* user_data) const
{
	unsigned int a, b, i, j, k, l;
	Pair pair;

	cq.clear();

	pair.m_a = 0;
	pair.m_b = 0;
	cq.enqueue(pair);

	while (!cq.empty()) {
		cq.dequeue(pair);
		a = pair.m_a;
		b = pair.m_b;

		if (a == b) {
			k = m_nodes[a].m_next;

			if (k == 0) {
				l = m_nodes[a].m_tail;
				if (l != 0)
					for (i = m_nodes[a].m_head; i < l - 1; ++i)
						for (j = i + 1; j < l; ++j)
							if (m_items[i].m_bb.overlaps_with(m_items[j].m_bb)) {
								assert(m_items[i].m_bb.is_valid());
								assert(m_items[j].m_bb.is_valid());
								if (!overlap_callback(m_items[i].m_data, m_items[j].m_data, user_data))
									return;
							}
			}
			else {
				pair.m_a = k;
				pair.m_b = k;
				cq.enqueue(pair);

				pair.m_a = k + 1;
				pair.m_b = k + 1;
				cq.enqueue(pair);

				pair.m_a = k + 2;
				pair.m_b = k + 2;
				cq.enqueue(pair);

				if (m_nodes[k].m_bb.overlaps_with(m_nodes[k + 1].m_bb)) {
					pair.m_a = k;
					pair.m_b = k + 1;
					cq.enqueue(pair);
				}

				if (m_nodes[k + 1].m_bb.overlaps_with(m_nodes[k + 2].m_bb)) {
					pair.m_a = k + 1;
					pair.m_b = k + 2;
					cq.enqueue(pair);
				}

			}
		}
		else {
			k = m_nodes[a].m_next;
			l = m_nodes[b].m_next;

			if (k == 0 && l == 0) {
				for (i = m_nodes[a].m_head; i < m_nodes[a].m_tail; ++i)
					for (j = m_nodes[b].m_head; j < m_nodes[b].m_tail; ++j)
						if (m_items[i].m_bb.overlaps_with(m_items[j].m_bb)) {
							assert(m_items[i].m_bb.is_valid());
							assert(m_items[j].m_bb.is_valid());
							if (!overlap_callback(m_items[i].m_data, m_items[j].m_data, user_data))
								return;
						}
			}
			else if (k == 0) {
				if (m_nodes[a].m_bb.overlaps_with(m_nodes[l].m_bb)) {
					pair.m_a = a;
					pair.m_b = l;
					cq.enqueue(pair);
				}

				if (m_nodes[a].m_bb.overlaps_with(m_nodes[l + 1].m_bb)) {
					pair.m_a = a;
					pair.m_b = l + 1;
					cq.enqueue(pair);
				}

				if (m_nodes[a].m_bb.overlaps_with(m_nodes[l + 2].m_bb)) {
					pair.m_a = a;
					pair.m_b = l + 2;
					cq.enqueue(pair);
				}
			}
			else if (l == 0) {
				if (m_nodes[k].m_bb.overlaps_with(m_nodes[b].m_bb)) {
					pair.m_a = k;
					pair.m_b = b;
					cq.enqueue(pair);
				}

				if (m_nodes[k + 1].m_bb.overlaps_with(m_nodes[b].m_bb)) {
					pair.m_a = k + 1;
					pair.m_b = b;
					cq.enqueue(pair);
				}

				if (m_nodes[k + 2].m_bb.overlaps_with(m_nodes[b].m_bb)) {
					pair.m_a = k + 2;
					pair.m_b = b;
					cq.enqueue(pair);
				}
			}
			else {
				if (m_nodes[k].m_bb.overlaps_with(m_nodes[b].m_bb)) {
					if (m_nodes[k].m_bb.overlaps_with(m_nodes[l].m_bb)) {
						pair.m_a = k;
						pair.m_b = l;
						cq.enqueue(pair);
					}

					if (m_nodes[k].m_bb.overlaps_with(m_nodes[l + 1].m_bb)) {
						pair.m_a = k;
						pair.m_b = l + 1;
						cq.enqueue(pair);
					}

					if (m_nodes[k].m_bb.overlaps_with(m_nodes[l + 2].m_bb)) {
						pair.m_a = k;
						pair.m_b = l + 2;
						cq.enqueue(pair);
					}
				}

				if (m_nodes[k + 1].m_bb.overlaps_with(m_nodes[b].m_bb)) {
					if (m_nodes[k + 1].m_bb.overlaps_with(m_nodes[l].m_bb)) {
						pair.m_a = k + 1;
						pair.m_b = l;
						cq.enqueue(pair);
					}

					if (m_nodes[k + 1].m_bb.overlaps_with(m_nodes[l + 1].m_bb)) {
						pair.m_a = k + 1;
						pair.m_b = l + 1;
						cq.enqueue(pair);
					}

					if (m_nodes[k + 1].m_bb.overlaps_with(m_nodes[l + 2].m_bb)) {
						pair.m_a = k + 1;
						pair.m_b = l + 2;
						cq.enqueue(pair);
					}
				}

				if (m_nodes[k + 2].m_bb.overlaps_with(m_nodes[b].m_bb)) {
					if (m_nodes[k + 2].m_bb.overlaps_with(m_nodes[l].m_bb)) {
						pair.m_a = k + 2;
						pair.m_b = l;
						cq.enqueue(pair);
					}

					if (m_nodes[k + 2].m_bb.overlaps_with(m_nodes[l + 1].m_bb)) {
						pair.m_a = k + 2;
						pair.m_b = l + 1;
						cq.enqueue(pair);
					}

					if (m_nodes[k + 2].m_bb.overlaps_with(m_nodes[l + 2].m_bb)) {
						pair.m_a = k + 2;
						pair.m_b = l + 2;
						cq.enqueue(pair);
					}
				}
			}
		}
	}
}

template <class T>
void Geom::BoxSpace<T>::overlap_with(
	const BoxSpace<T>& other,
	ItemOverlapCallback overlap_callback,
	FastQueue<Pair>& cq,
	void* user_data) const
{
	unsigned int a, b, i, j, k, l;
	Pair pair;

	cq.clear();

	if (m_nodes[0].m_bb.overlaps_with(other.m_nodes[0].m_bb)) {
		pair.m_a = 0;
		pair.m_b = 0;
		cq.enqueue(pair);
	}

	while (!cq.empty()) {
		cq.dequeue(pair);
		a = pair.m_a;
		b = pair.m_b;

		k = m_nodes[a].m_next;
		l = other.m_nodes[b].m_next;

		if (k != 0 && l != 0) {
			if (m_nodes[k].m_bb.overlaps_with(other.m_nodes[b].m_bb)) {
				if (m_nodes[k].m_bb.overlaps_with(other.m_nodes[l].m_bb)) {
					pair.m_a = k;
					pair.m_b = l;
					cq.enqueue(pair);
				}

				if (m_nodes[k].m_bb.overlaps_with(other.m_nodes[l + 1].m_bb)) {
					pair.m_a = k;
					pair.m_b = l + 1;
					cq.enqueue(pair);
				}

				if (m_nodes[k].m_bb.overlaps_with(other.m_nodes[l + 2].m_bb)) {
					pair.m_a = k;
					pair.m_b = l + 2;
					cq.enqueue(pair);
				}
			}

			if (m_nodes[k + 1].m_bb.overlaps_with(other.m_nodes[b].m_bb)) {
				if (m_nodes[k + 1].m_bb.overlaps_with(other.m_nodes[l].m_bb)) {
					pair.m_a = k + 1;
					pair.m_b = l;
					cq.enqueue(pair);
				}

				if (m_nodes[k + 1].m_bb.overlaps_with(other.m_nodes[l + 1].m_bb)) {
					pair.m_a = k + 1;
					pair.m_b = l + 1;
					cq.enqueue(pair);
				}

				if (m_nodes[k + 1].m_bb.overlaps_with(other.m_nodes[l + 2].m_bb)) {
					pair.m_a = k + 1;
					pair.m_b = l + 2;
					cq.enqueue(pair);
				}
			}


			if (m_nodes[k + 2].m_bb.overlaps_with(other.m_nodes[b].m_bb)) {
				if (m_nodes[k + 2].m_bb.overlaps_with(other.m_nodes[l].m_bb)) {
					pair.m_a = k + 2;
					pair.m_b = l;
					cq.enqueue(pair);
				}

				if (m_nodes[k + 2].m_bb.overlaps_with(other.m_nodes[l + 1].m_bb)) {
					pair.m_a = k + 2;
					pair.m_b = l + 1;
					cq.enqueue(pair);
				}

				if (m_nodes[k + 2].m_bb.overlaps_with(other.m_nodes[l + 2].m_bb)) {
					pair.m_a = k + 2;
					pair.m_b = l + 2;
					cq.enqueue(pair);
				}
			}
		}
		else if (k != 0) {
			if (m_nodes[k].m_bb.overlaps_with(other.m_nodes[b].m_bb)) {
				pair.m_a = k;
				pair.m_b = b;
				cq.enqueue(pair);
			}

			if (m_nodes[k + 1].m_bb.overlaps_with(other.m_nodes[b].m_bb)) {
				pair.m_a = k + 1;
				pair.m_b = b;
				cq.enqueue(pair);
			}

			if (m_nodes[k + 2].m_bb.overlaps_with(other.m_nodes[b].m_bb)) {
				pair.m_a = k + 2;
				pair.m_b = b;
				cq.enqueue(pair);
			}
		}
		else if (l != 0) {
			if (m_nodes[a].m_bb.overlaps_with(other.m_nodes[l].m_bb)) {
				pair.m_a = a;
				pair.m_b = l;
				cq.enqueue(pair);
			}

			if (m_nodes[a].m_bb.overlaps_with(other.m_nodes[l + 1].m_bb)) {
				pair.m_a = a;
				pair.m_b = l + 1;
				cq.enqueue(pair);
			}

			if (m_nodes[a].m_bb.overlaps_with(other.m_nodes[l + 2].m_bb)) {
				pair.m_a = a;
				pair.m_b = l + 2;
				cq.enqueue(pair);
			}
		}
		else {
			for (i = m_nodes[a].m_head; i < m_nodes[a].m_tail; ++i) {
				for (j = other.m_nodes[b].m_head; j < other.m_nodes[b].m_tail; ++j) {
					if (m_items[i].m_bb.overlaps_with(other.m_items[j].m_bb)) {
						assert(m_items[i].m_bb.is_valid());
						assert(other.m_items[j].m_bb.is_valid());
						if (!overlap_callback(m_items[i].m_data, other.m_items[j].m_data, user_data))
							return;
					}
				}
			}
		}
	}
}

template <class T>
void Geom::BoxSpace<T>::overlap_with(
	const Geom::BoundingBox& box,
	BoxOverlapCallback overlap_callback,
	FastQueue<unsigned int>& cq,
	void* user_data) const
{
	unsigned int i, j;

	cq.clear();
	if (m_nodes[0].m_bb.overlaps_with(box))
		cq.enqueue2(0);

	while (!cq.empty()) {
		i = cq.dequeue2();
		if (m_nodes[i].m_bb.is_within(box)) {
			for (j = m_nodes[i].m_head; j < m_nodes[i].m_tail; ++j) {
				assert(m_items[j].m_bb.is_valid());
				if (!overlap_callback(m_items[j].m_data, user_data))
					return;
			}
		}
		else if (m_nodes[i].m_next == 0) {
			for (j = m_nodes[i].m_head; j < m_nodes[i].m_tail; ++j)
				if (m_items[j].m_bb.overlaps_with(box)) {
					assert(m_items[j].m_bb.is_valid());
					if (!overlap_callback(m_items[j].m_data, user_data))
						return;
				}
		}
		else {
			j = m_nodes[i].m_next;

			if (m_nodes[j].m_bb.overlaps_with(box))
				cq.enqueue2(j);
			if (m_nodes[j + 1].m_bb.overlaps_with(box))
				cq.enqueue2(j + 1);
			if (m_nodes[j + 2].m_bb.overlaps_with(box))
				cq.enqueue2(j + 2);
		}
	}
}

template <class T>
void Geom::BoxSpace<T>::overlap_with(
	const Geom::Vector3d& point,
	const Geom::Vector3d& vector,
	BoxOverlapCallback overlap_callback,
	FastQueue<unsigned int>& cq,
	void* user_data) const
{
	unsigned int i, j;

	cq.clear();
	if (m_nodes[0].m_bb.intersects_ray(point, vector))
		cq.enqueue2(0);

	while (!cq.empty()) {
		i = cq.dequeue2();
		if (m_nodes[i].m_next == 0) {
			for (j = m_nodes[i].m_head; j < m_nodes[i].m_tail; ++j)
				if (m_items[j].m_bb.intersects_ray(point, vector)) {
					assert(m_items[j].m_bb.is_valid());
					if (!overlap_callback(m_items[j].m_data, user_data))
						return;
				}
		}
		else {
			j = m_nodes[i].m_next;

			if (m_nodes[j].m_bb.intersects_ray(point, vector))
				cq.enqueue2(j);
			if (m_nodes[j + 1].m_bb.intersects_ray(point, vector))
				cq.enqueue2(j + 1);
			if (m_nodes[j + 2].m_bb.intersects_ray(point, vector))
				cq.enqueue2(j + 2);
		}
	}
}

#endif /* GEOM_BOX_SPACE_H */
