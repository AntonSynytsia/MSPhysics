/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef GEOM_VECTOR3D_H
#define GEOM_VECTOR3D_H

#include "geom.h"


// Global friend operator declaration
// https://stackoverflow.com/questions/2207219/how-do-i-define-friends-in-global-namespace-within-another-c-namespace

bool operator == (const Geom::Vector3d& lhs, const Geom::Vector3d& rhs);
bool operator != (const Geom::Vector3d& lhs, const Geom::Vector3d& rhs);

Geom::Vector3d operator + (Geom::Vector3d lhs, const Geom::Vector3d& rhs);
Geom::Vector3d operator - (Geom::Vector3d lhs, const Geom::Vector3d& rhs);

class Geom::Vector3d
{
public:
    // Variables
    treal m_x, m_y, m_z;

    // Constructors
    Vector3d();
    Vector3d(treal value);
    Vector3d(const Vector3d& other);
    Vector3d(treal x, treal y, treal z);
    Vector3d(const treal* values);

    // Operators
    friend bool (::operator ==) (const Geom::Vector3d& lhs, const Geom::Vector3d& rhs);
    friend bool (::operator !=) (const Geom::Vector3d& lhs, const Geom::Vector3d& rhs);

    friend Geom::Vector3d (::operator +) (Geom::Vector3d lhs, const Geom::Vector3d& rhs);
    friend Geom::Vector3d (::operator -) (Geom::Vector3d lhs, const Geom::Vector3d& rhs);

    Vector3d& operator = (const Vector3d& other);
    treal& operator [] (const int index);
    const treal& operator [] (const int index) const;

    Vector3d& operator += (const Vector3d& other);
    Vector3d& operator -= (const Vector3d& other);

    // Functions
    treal get_length_squared() const;
    treal get_length() const;
    Vector3d& set_length(treal length);
    Vector3d& normalize_self();
    Vector3d normalize() const;
    Vector3d& scale_self(treal s);
    Vector3d scale(treal s) const;
    Vector3d& reverse_self();
    Vector3d reverse() const;
    void zero_out();
    void set_all(treal value);
    treal dot(const Vector3d& other) const;
    Vector3d cross(const Vector3d& other) const;
    Vector3d product(const Vector3d& other) const;
    Vector3d& product_self(const Vector3d& other);
    Vector3d transition_to(const Vector3d& other, treal ratio) const;
    Vector3d rotate_to(const Vector3d& other, treal ratio) const;
    Vector3d rotate_and_scale_to(const Vector3d& other, treal ratio) const;
    Vector3d rotate(const Vector3d& normal, treal angle) const;
    treal angle_between(const Vector3d& other) const;
    treal angle_between(const Vector3d& other, const Vector3d& normal) const;

    bool is_valid() const;
    bool is_parallel_to(const Vector3d& other) const;
    bool is_same_direction_as(const Vector3d& other) const;
    bool is_aniparallel_to(const Vector3d& other) const;
};

#endif /* GEOM_VECTOR3D_H */
