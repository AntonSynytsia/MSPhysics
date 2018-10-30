/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef GEOM_VECTOR4D_H
#define GEOM_VECTOR4D_H

#include "geom_vector3d.h"

class Geom::Vector4d : public Vector3d
{
public:
    // Variables
    treal m_w;

    // Constructors
    Vector4d();
    Vector4d(treal value);
    Vector4d(const Vector4d& other);
    Vector4d(treal x, treal y, treal z, treal w);
    Vector4d(const treal* values);

    // Operators
    Vector4d& operator=(const Vector4d& other);
    Vector4d& operator=(const Vector3d& other);
};

#endif /* GEOM_VECTOR4D_H */
