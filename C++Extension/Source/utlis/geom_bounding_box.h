/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef GEOM_BOUNDING_BOX_H
#define GEOM_BOUNDING_BOX_H

#include "geom.h"
#include "geom_vector3d.h"

class Geom::BoundingBox
{
public:
    // Constants
    static const treal MIN_VALUE;
    static const treal MAX_VALUE;

    // Variables
    Vector3d m_min, m_max;

    // Constructors
    BoundingBox();
    BoundingBox(const BoundingBox& other);
    BoundingBox(const Geom::Vector3d& min, const Geom::Vector3d& max);

    // Operators
    BoundingBox& operator=(const BoundingBox& other);

    // Functions
    BoundingBox& add(const Vector3d& point);
    BoundingBox& add(const BoundingBox& other);
    BoundingBox& add(const Geom::Vector3d& min, const Geom::Vector3d& max);
    BoundingBox& scale_self(const treal& scale);
    BoundingBox scale(const treal& scale) const;
    BoundingBox& product_self(const Geom::Vector3d& scale);
    BoundingBox product(const Geom::Vector3d& scale) const;
    void intersect(const BoundingBox& other, Geom::BoundingBox& bb_out) const;
    bool is_point_inside(const Vector3d& point) const;
    bool overlaps_with(const BoundingBox& other) const;
    bool is_within(const BoundingBox& other) const;
	bool intersects_ray(const Geom::Vector3d& ray_point, const Geom::Vector3d& ray_vector) const;
    treal get_width() const;
    treal get_height() const;
    treal get_depth() const;
    treal get_diagonal() const;
    treal get_min_max_difference_at(unsigned int axis) const;
    treal get_min_max_sum_at(unsigned int axis) const;
    treal get_center_at(unsigned int axis) const;
    void get_center(Geom::Vector3d& center_out) const;
    void get_corner(unsigned int i, Geom::Vector3d& corner_out) const;
    void clear();
    void pad_out(treal value);
    bool is_valid() const;
    bool is_invalid() const;
};

#endif /* GEOM_BOUNDING_BOX_H */
