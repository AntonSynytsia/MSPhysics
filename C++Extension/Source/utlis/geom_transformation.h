/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef GEOM_TRANSFORMATION_H
#define GEOM_TRANSFORMATION_H

#include "geom_vector4d.h"

Geom::Transformation operator * (const Geom::Transformation& tA, const Geom::Transformation& tB);

class Geom::Transformation
{
public:
    // Variables
    Vector4d m_xaxis, m_yaxis, m_zaxis, m_origin;

    // Constructors
    Transformation();
    Transformation(const Vector3d& origin, const Vector3d& zaxis);
    Transformation(const Vector3d& origin);
	Transformation(const Vector3d& xaxis, const Vector3d& yaxis, const Vector3d& zaxis);
    Transformation(const Vector3d& xaxis, const Vector3d& yaxis, const Vector3d& zaxis, const Vector3d& origin);
	Transformation(const Vector4d& xaxis, const Vector4d& yaxis, const Vector4d& zaxis, const Vector4d& origin);

    Transformation(const Transformation& other);
    Transformation(const Quaternion& rotation, Geom::Vector3d position);
    Transformation(const treal* matrix);
    Transformation(const Geom::Vector3d& origin, const Geom::Vector3d& normal, treal angle);

    // Operators
	friend Geom::Transformation (::operator *) (const Geom::Transformation& tA, const Geom::Transformation& tB);

    Transformation& operator = (const Transformation& other);
    Vector4d& operator [] (const int index);
    const Vector4d& operator [] (const int index) const;

    // Functions
    Transformation inverse() const;
    Vector3d get_scale() const;
    Transformation& set_scale(const Vector3d& scale);
    Transformation& normalize_self();
    Transformation normalize() const;
    Transformation& scale_axes_self(const Vector3d& scale);
    Transformation scale_axes(const Vector3d& scale) const;
    Transformation rotate_xaxis_to(const Vector3d& dir) const;
    Transformation rotate_yaxis_to(const Vector3d& dir) const;
    Transformation rotate_zaxis_to(const Vector3d& dir) const;
    Transformation uniform_normal_transition_to(const Transformation& other, treal ratio) const;
    Transformation uniform_transition_to(const Transformation& other, treal ratio) const;
    Vector3d transform_vector(const Vector3d& v) const;
    Vector3d rotate_vector(const Vector3d& v) const;
    Vector3d transform_vector2(const Vector3d& v) const;
    Vector3d rotate_vector2(const Vector3d& v) const;
    Vector3d transform_vector3(const Vector3d& v, treal det) const;
    Vector3d rotate_vector3(const Vector3d& v, treal det) const;

    treal get_determinant() const;
    void extract_w_factor();

    void get_normal_xaxis(Geom::Vector3d& res) const;
    void get_normal_yaxis(Geom::Vector3d& res) const;
    void get_normal_zaxis(Geom::Vector3d& res) const;
    void get_normal_origin(Geom::Vector3d& res) const;

    bool is_uniform() const;
    bool is_flipped() const;
    bool is_flat() const;

    void zero_out();
};

#endif /* GEOM_TRANSFORMATION_H */
