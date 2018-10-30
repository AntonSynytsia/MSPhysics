/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef GEOM_QUATERNION_H
#define GEOM_QUATERNION_H

#include "geom.h"

Geom::Quaternion operator + (const Geom::Quaternion& lhs, const Geom::Quaternion& rhs);
Geom::Quaternion operator - (const Geom::Quaternion& lhs, const Geom::Quaternion& rhs);
Geom::Quaternion operator * (const Geom::Quaternion& lhs, const Geom::Quaternion& rhs);

class Geom::Quaternion
{
public:

    // Enumerators
    enum QUAT_INDEX
    {
        X_INDEX = 0,
        Y_INDEX = 1,
        Z_INDEX = 2
    };

    // Constants
    static const QUAT_INDEX QIndex[3];

    // Variables
    treal m_q0, m_q1, m_q2, m_q3;

    // Constructors
    Quaternion();
    Quaternion(treal q0, treal q1, treal q2, treal q3);
    Quaternion(const Quaternion& other);
    Quaternion(const Transformation& matrix);
    Quaternion(const Vector3d& unit_axis, treal angle = 0.0);

    // Operators
	friend Geom::Quaternion (::operator +) (const Geom::Quaternion& lhs, const Geom::Quaternion& rhs);
	friend Geom::Quaternion (::operator -) (const Geom::Quaternion& lhs, const Geom::Quaternion& rhs);
	friend Geom::Quaternion (::operator *) (const Geom::Quaternion& lhs, const Geom::Quaternion& rhs);

    Quaternion& operator = (const Quaternion& other);

    // Functions
    Quaternion scale(treal s) const;
    Quaternion& scale_self(treal s);
    Quaternion normalize() const;
    Quaternion& normalize_self();
    Quaternion inverse() const;
    treal get_length() const;
};

#endif /* GEOM_QUATERNION_H */
