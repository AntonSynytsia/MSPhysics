/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef GEOM_COLOR_H
#define GEOM_COLOR_H

#include "geom.h"

class Geom::Color
{
public:
    // Variables
    unsigned char m_r, m_g, m_b, m_a;

    // Constructors
    Color();
    Color(const Color& other);
    Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

    // Operators
    Color& operator=(const Color& other);

    friend bool operator==(const Color& lhs, const Color& rhs);
    friend bool operator!=(const Color& lhs, const Color& rhs);

    // Functions
    Color transition_to(const Color& other, treal ratio);
};

#endif /* GEOM_COLOR_H */
