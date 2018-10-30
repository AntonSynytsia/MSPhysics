/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "geom_quaternion.h"
#include "geom_transformation.h"
#include "geom_vector3d.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const Geom::Quaternion::QUAT_INDEX Geom::Quaternion::QIndex[3] = {Y_INDEX, Z_INDEX, X_INDEX};


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constructors
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Geom::Quaternion::Quaternion() :
    m_q0(1.0),
    m_q1(0.0),
    m_q2(0.0),
    m_q3(0.0)
{
}

Geom::Quaternion::Quaternion(treal q0, treal q1, treal q2, treal q3) :
    m_q0(q0),
    m_q1(q1),
    m_q2(q2),
    m_q3(q3)
{
}

Geom::Quaternion::Quaternion(const Quaternion& other) :
    m_q0(other.m_q0),
    m_q1(other.m_q1),
    m_q2(other.m_q2),
    m_q3(other.m_q3)
{
}

Geom::Quaternion::Quaternion(const Transformation& matrix)
{
    treal trace = matrix[0][0] + matrix[1][1] + matrix[2][2];

    if (trace > (treal)(0.0)) {
        trace = sqrt(trace + (treal)(1.0));
        m_q0 = (treal)(0.5) * trace;
        trace = (treal)(0.5) / trace;
        m_q1 = (matrix[1][2] - matrix[2][1]) * trace;
        m_q2 = (matrix[2][0] - matrix[0][2]) * trace;
        m_q3 = (matrix[0][1] - matrix[1][0]) * trace;

    } else {
        QUAT_INDEX i = X_INDEX;
        if (matrix[Y_INDEX][Y_INDEX] > matrix[X_INDEX][X_INDEX]) {
            i = Y_INDEX;
        }
        if (matrix[Z_INDEX][Z_INDEX] > matrix[i][i]) {
            i = Z_INDEX;
        }
        QUAT_INDEX j = QIndex [i];
        QUAT_INDEX k = QIndex [j];

        trace = (treal)(1.0) + matrix[i][i] - matrix[j][j] - matrix[k][k];
        trace = sqrt(trace);

        treal* const ptr = &m_q1;
        ptr[i] = (treal)(0.5) * trace;
        trace = (treal)(0.5) / trace;
        m_q0 = (matrix[j][k] - matrix[k][j]) * trace;
        ptr[j] = (matrix[i][j] + matrix[j][i]) * trace;
        ptr[k] = (matrix[i][k] + matrix[k][i]) * trace;
    }
}

Geom::Quaternion::Quaternion(const Vector3d& unit_axis, treal angle)
{
    angle *= (treal)(0.5);
    m_q0 = cos(angle);
    treal sin_ang = sin(angle);

    m_q1 = unit_axis.m_x * sin_ang;
    m_q2 = unit_axis.m_y * sin_ang;
    m_q3 = unit_axis.m_z * sin_ang;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Operators
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


Geom::Quaternion& Geom::Quaternion::operator = (const Quaternion& other) {
    if (this != &other) {
        m_q0 = other.m_q0;
        m_q1 = other.m_q1;
        m_q2 = other.m_q2;
        m_q3 = other.m_q3;
    }
    return *this;
}

Geom::Quaternion operator + (const Geom::Quaternion& lhs, const Geom::Quaternion& rhs) {
    return Geom::Quaternion(lhs.m_q0 + rhs.m_q0, lhs.m_q1 + rhs.m_q1, lhs.m_q2 + rhs.m_q2, lhs.m_q3 + rhs.m_q3);
}

Geom::Quaternion operator - (const Geom::Quaternion& lhs, const Geom::Quaternion& rhs) {
    return Geom::Quaternion(lhs.m_q0 - rhs.m_q0, lhs.m_q1 - rhs.m_q1, lhs.m_q2 - rhs.m_q2, lhs.m_q3 - rhs.m_q3);
}

Geom::Quaternion operator * (const Geom::Quaternion& lhs, const Geom::Quaternion& rhs) {
    return Geom::Quaternion(
        rhs.m_q0 * lhs.m_q0 - rhs.m_q1 * lhs.m_q1 - rhs.m_q2 * lhs.m_q2 - rhs.m_q3 * lhs.m_q3,
        rhs.m_q1 * lhs.m_q0 + rhs.m_q0 * lhs.m_q1 - rhs.m_q3 * lhs.m_q2 + rhs.m_q2 * lhs.m_q3,
        rhs.m_q2 * lhs.m_q0 + rhs.m_q3 * lhs.m_q1 + rhs.m_q0 * lhs.m_q2 - rhs.m_q1 * lhs.m_q3,
        rhs.m_q3 * lhs.m_q0 - rhs.m_q2 * lhs.m_q1 + rhs.m_q1 * lhs.m_q2 + rhs.m_q0 * lhs.m_q3);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


Geom::Quaternion Geom::Quaternion::scale(treal s) const {
    return Quaternion(m_q0 * s, m_q1 * s, m_q2 * s, m_q3 * s);
}

Geom::Quaternion& Geom::Quaternion::scale_self(treal s) {
    m_q0 *= s;
    m_q1 *= s;
    m_q2 *= s;
    m_q3 *= s;
    return *this;
}

Geom::Quaternion Geom::Quaternion::normalize() const {
    treal len_sq = m_q0 * m_q0 + m_q1 * m_q1 + m_q2 * m_q2 + m_q3 * m_q3;
    treal s;
    if (len_sq > M_EPSILON_SQ)
        s = (treal)(1.0) / sqrt(len_sq);
    else
        s = (treal)(0.0);
    return scale(s);
}

Geom::Quaternion& Geom::Quaternion::normalize_self() {
    treal len_sq = m_q0 * m_q0 + m_q1 * m_q1 + m_q2 * m_q2 + m_q3 * m_q3;
    treal s;
    if (len_sq > M_EPSILON_SQ)
        s = (treal)(1.0) / sqrt(len_sq);
    else
        s = (treal)(0.0);
    return scale_self(s);
}

Geom::Quaternion Geom::Quaternion::inverse() const {
    return Quaternion(m_q0, -m_q1, -m_q2, -m_q3);
}

treal Geom::Quaternion::get_length() const {
    return sqrt(m_q0 * m_q0 + m_q1 * m_q1 + m_q2 * m_q2 + m_q3 * m_q3);
}
