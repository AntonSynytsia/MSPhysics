/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "geom_vector4d.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constructors
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Geom::Vector4d::Vector4d() :
    Vector3d(),
    m_w(0.0)
{
}

Geom::Vector4d::Vector4d(treal value) :
    Vector3d(value),
    m_w(0.0)
{
}

Geom::Vector4d::Vector4d(const Vector4d& other) :
    Vector3d(other),
    m_w(other.m_w)
{
}

Geom::Vector4d::Vector4d(treal x, treal y, treal z, treal w) :
    Vector3d(x, y, z),
    m_w(w)
{
}

Geom::Vector4d::Vector4d(const treal* values) :
    Vector3d(values),
    m_w(values[3])
{
}

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Operators
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Geom::Vector4d& Geom::Vector4d::operator=(const Vector4d& other) {
    if (this != &other) {
        m_x = other.m_x;
        m_y = other.m_y;
        m_z = other.m_z;
        m_w = other.m_w;
    }
    return *this;
}

Geom::Vector4d& Geom::Vector4d::operator=(const Vector3d& other) {
    if (this != &other) {
        m_x = other.m_x;
        m_y = other.m_y;
        m_z = other.m_z;
    }
    return *this;
}
