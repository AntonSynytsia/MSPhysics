/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "geom_color.h"


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constructors
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Geom::Color::Color()
    : m_r(0), m_g(0), m_b(0), m_a(0)
{
}

Geom::Color::Color(const Color& other)
    : m_r(other.m_r), m_g(other.m_g), m_b(other.m_b), m_a(other.m_a)
{
}

Geom::Color::Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
    : m_r(r), m_g(g), m_b(b), m_a(a)
{
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Operators
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Geom::Color& Geom::Color::operator=(const Color& other) {
    if (this != &other) {
        m_r = other.m_r;
        m_g = other.m_g;
        m_b = other.m_b;
        m_a = other.m_a;
    }
    return *this;
}

bool Geom::operator==(const Geom::Color& lhs, const Geom::Color& rhs) {
    return (lhs.m_r == rhs.m_r && lhs.m_g == rhs.m_g && lhs.m_b == rhs.m_b && lhs.m_a == rhs.m_a);
}

bool Geom::operator!=(const Geom::Color& lhs, const Geom::Color& rhs) {
    return !(lhs == rhs);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Geom::Color Geom::Color::transition_to(const Geom::Color& other, treal ratio) {
    return Geom::Color(
        m_r + static_cast<unsigned char>(static_cast<treal>(other.m_r - m_r) * ratio),
        m_g + static_cast<unsigned char>(static_cast<treal>(other.m_g - m_g) * ratio),
        m_b + static_cast<unsigned char>(static_cast<treal>(other.m_b - m_b) * ratio),
        m_a + static_cast<unsigned char>(static_cast<treal>(other.m_a - m_a) * ratio));
}
