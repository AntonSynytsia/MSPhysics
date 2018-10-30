/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "geom_vector3d.h"


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constructors
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Geom::Vector3d::Vector3d()
    : m_x(0.0), m_y(0.0), m_z(0.0)
{
}

Geom::Vector3d::Vector3d(treal value)
    : m_x(value), m_y(value), m_z(value)
{
}

Geom::Vector3d::Vector3d(const Vector3d& other)
    : m_x(other.m_x), m_y(other.m_y), m_z(other.m_z)
{
}

Geom::Vector3d::Vector3d(treal x, treal y, treal z)
    : m_x(x), m_y(y), m_z(z)
{
}

Geom::Vector3d::Vector3d(const treal* values) :
    m_x(values[0]), m_y(values[1]), m_z(values[2])
{
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Operators
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

bool operator == (const Geom::Vector3d& lhs, const Geom::Vector3d& rhs) {
    return (fabs(lhs.m_x - rhs.m_x) < M_EPSILON &&
        fabs(lhs.m_y - rhs.m_y) < M_EPSILON &&
        fabs(lhs.m_z - rhs.m_z) < M_EPSILON);
}

bool operator != (const Geom::Vector3d& lhs, const Geom::Vector3d& rhs) {
    return !(lhs == rhs);
}

Geom::Vector3d operator + (Geom::Vector3d lhs, const Geom::Vector3d& rhs) {
    lhs.m_x += rhs.m_x;
    lhs.m_y += rhs.m_y;
    lhs.m_z += rhs.m_z;
    return lhs;
}

Geom::Vector3d operator - (Geom::Vector3d lhs, const Geom::Vector3d& rhs) {
    lhs.m_x -= rhs.m_x;
    lhs.m_y -= rhs.m_y;
    lhs.m_z -= rhs.m_z;
    return lhs;
}

Geom::Vector3d& Geom::Vector3d::operator =(const Vector3d& other) {
    if (this != &other) {
        m_x = other.m_x;
        m_y = other.m_y;
        m_z = other.m_z;
    }
    return *this;
}

treal& Geom::Vector3d::operator [] (const int index) {
    return (&m_x)[index];
}

const treal& Geom::Vector3d::operator [] (const int index) const {
    return (&m_x)[index];
}

Geom::Vector3d& Geom::Vector3d::operator += (const Vector3d& other) {
    m_x += other.m_x;
    m_y += other.m_y;
    m_z += other.m_z;
    return *this;
}

Geom::Vector3d& Geom::Vector3d::operator -= (const Vector3d& other) {
    m_x -= other.m_x;
    m_y -= other.m_y;
    m_z -= other.m_z;
    return *this;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

treal Geom::Vector3d::get_length_squared() const {
    return m_x * m_x + m_y * m_y + m_z * m_z;
}

treal Geom::Vector3d::get_length() const {
    return sqrt(m_x * m_x + m_y * m_y + m_z * m_z);
}

Geom::Vector3d& Geom::Vector3d::set_length(treal length) {
    treal mag_sq = m_x * m_x + m_y * m_y + m_z * m_z;
    if (mag_sq > M_EPSILON_SQ) {
        treal inv_des_mag = length / sqrt(mag_sq);
        m_x *= inv_des_mag;
        m_y *= inv_des_mag;
        m_z *= inv_des_mag;
    }
    return *this;
}

Geom::Vector3d& Geom::Vector3d::normalize_self() {
    return set_length(1.0);
}

Geom::Vector3d Geom::Vector3d::normalize() const {
    treal mag_sq = m_x * m_x + m_y * m_y + m_z * m_z;
    if (mag_sq > M_EPSILON_SQ) {
        treal inv_mag = (treal)(1.0) / sqrt(mag_sq);
        return Vector3d(m_x * inv_mag, m_y * inv_mag, m_z * inv_mag);
    }
    else
        return Vector3d(*this);
}

Geom::Vector3d& Geom::Vector3d::scale_self(treal s) {
    m_x *= s;
    m_y *= s;
    m_z *= s;
    return *this;
}

Geom::Vector3d Geom::Vector3d::scale(treal s) const {
    return Vector3d(m_x * s, m_y * s, m_z * s);
}

Geom::Vector3d& Geom::Vector3d::reverse_self() {
    m_x = -m_x;
    m_y = -m_y;
    m_z = -m_z;
    return *this;
}

Geom::Vector3d Geom::Vector3d::reverse() const {
    return Vector3d(-m_x, -m_y, -m_z);
}

void Geom::Vector3d::zero_out() {
    m_x = 0.0;
    m_y = 0.0;
    m_z = 0.0;
}

void Geom::Vector3d::set_all(treal value) {
    m_x = value;
    m_y = value;
    m_z = value;
}

treal Geom::Vector3d::dot(const Vector3d& other) const {
    return m_x * other.m_x + m_y * other.m_y + m_z * other.m_z;
}

Geom::Vector3d Geom::Vector3d::cross(const Vector3d& other) const {
    return Vector3d(
		m_y * other.m_z - m_z * other.m_y,
		m_z * other.m_x - m_x * other.m_z,
		m_x * other.m_y - m_y * other.m_x);
}

Geom::Vector3d Geom::Vector3d::product(const Vector3d& other) const {
    return Vector3d(m_x * other.m_x, m_y * other.m_y, m_z * other.m_z);
}

Geom::Vector3d& Geom::Vector3d::product_self(const Vector3d& other) {
    m_x *= other.m_x;
    m_y *= other.m_y;
    m_z *= other.m_z;
    return *this;
}

Geom::Vector3d Geom::Vector3d::transition_to(const Vector3d& other, treal ratio) const {
    return Vector3d(m_x + (other.m_x - m_x) * ratio, m_y + (other.m_y - m_y) * ratio, m_z + (other.m_z - m_z) * ratio);
}

Geom::Vector3d Geom::Vector3d::rotate_to(const Vector3d& other, treal ratio) const {
    treal len1 = get_length();
    treal len2 = other.get_length();
    if (len1 > M_EPSILON && len2 > M_EPSILON) {
        Vector3d v1normal(scale((treal)(1.0) / len1));
        Vector3d v2normal(other.scale((treal)(1.0) / len2));
        treal cos_theta = Geom::clamp_treal(v1normal.dot(v2normal), (treal)(-1.0), (treal)(1.0));
        if (cos_theta < (treal)(-0.9999995)) {
            Geom::Vector3d side;
			if (fabs(v1normal.m_z) < (treal)(0.9999995)) {
				//side = v1normal.cross(Z_AXIS);
				side.m_x = v1normal.m_y;
				side.m_y = -v1normal.m_x;
				side.m_z = 0.0;
			}
			else {
				//side = v1normal.cross(X_AXIS);
				side.m_x = 0.0;
				side.m_y = v1normal.m_z;
				side.m_z = -v1normal.m_y;
			}
            return rotate(side, M_SPI * ratio);
        }
        else if (cos_theta < (treal)(0.9999995))
            return rotate(v1normal.cross(v2normal).normalize(), acos(cos_theta) * ratio);
    }
    return Geom::Vector3d(*this);
}

Geom::Vector3d Geom::Vector3d::rotate_and_scale_to(const Vector3d& other, treal ratio) const {
    treal len1 = get_length();
    treal len2 = other.get_length();
    Vector3d result(m_x, m_y, m_z);
    if (len1 > M_EPSILON && len2 > M_EPSILON) {
        Vector3d v1normal(scale((treal)(1.0) / len1));
        Vector3d v2normal(other.scale((treal)(1.0) / len2));
        treal cos_theta = Geom::clamp_treal(v1normal.dot(v2normal), (treal)(-1.0), (treal)(1.0));
        if (cos_theta < (treal)(-0.9999995)) {
            Geom::Vector3d side;
			if (fabs(v1normal.m_z) < (treal)(0.9999995)) {
				//side = v1normal.cross(Z_AXIS);
				side.m_x = v1normal.m_y;
				side.m_y = -v1normal.m_x;
				side.m_z = 0.0;
			}
			else {
				//side = v1normal.cross(X_AXIS);
				side.m_x = 0.0;
				side.m_y = v1normal.m_z;
				side.m_z = -v1normal.m_y;
			}
            result = rotate(side, M_SPI * ratio);
        }
        else if (cos_theta < (treal)(0.9999995))
            result = rotate(v1normal.cross(v2normal).normalize(), acos(cos_theta) * ratio);
    }
    result.set_length(len1 + (len2 - len1) * ratio);
    return result;
}

Geom::Vector3d Geom::Vector3d::rotate(const Vector3d& normal, treal angle) const {
    // Rodrigues' rotation formula: https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
    treal cos_ang = cos(angle);
    treal sin_ang = sin(angle);
    treal cos_ang_rev = (treal)(1.0) - cos_ang;

    Vector3d k_cross_v(normal.cross(*this));
    treal k_dot_v = dot(normal);

    return Vector3d(
        m_x * cos_ang + k_cross_v.m_x * sin_ang + normal.m_x * k_dot_v * cos_ang_rev,
        m_y * cos_ang + k_cross_v.m_y * sin_ang + normal.m_y * k_dot_v * cos_ang_rev,
        m_z * cos_ang + k_cross_v.m_z * sin_ang + normal.m_z * k_dot_v * cos_ang_rev);
}

treal Geom::Vector3d::angle_between(const Vector3d& other) const {
    treal cos_theta = normalize().dot(other.normalize());
    return acos(Geom::clamp_treal(cos_theta, (treal)(-1.0), (treal)(1.0)));
}

treal Geom::Vector3d::angle_between(const Vector3d& other, const Vector3d& normal) const {
    Geom::Vector3d zaxis(normal.normalize());
    Geom::Vector3d xaxis;
	if (fabs(zaxis.m_z) < (treal)(0.9999995)) {
		//xaxis = Z_AXIS.cross(zaxis);
		xaxis.m_x = -zaxis.m_y;
		xaxis.m_y = zaxis.m_x;
		xaxis.m_z = 0.0;
	}
	else {
		//xaxis = Y_AXIS.cross(zaxis);
		xaxis.m_x = zaxis.m_z;
		xaxis.m_y = 0;
		xaxis.m_z = -zaxis.m_x;
	}
    xaxis.normalize_self();
    Geom::Vector3d yaxis(zaxis.cross(xaxis));
    yaxis.normalize_self();
    treal x1 = dot(xaxis);
    treal y1 = dot(yaxis);
    treal mag1 = sqrt(x1 * x1 + y1 * y1);
    if (mag1 < M_EPSILON)
        return (treal)(0.0);
    treal x2 = other.dot(xaxis);
    treal y2 = other.dot(yaxis);
    treal mag2 = sqrt(x2 * x2 + y2 * y2);
    if (mag2 < M_EPSILON)
        return (treal)(0.0);
    // Compute the first angle and make it range from -PI to PI
    treal theta1 = acos(Geom::clamp_treal(x1 / mag1, (treal)(-1.0), (treal)(1.0)));
    if (y1 < (treal)(0.0)) theta1 = -theta1;
    // Compute the second angle and make it range from -PI to PI
    treal theta2 = acos(Geom::clamp_treal(x2 / mag2, (treal)(-1.0), (treal)(1.0)));
    if (y2 < (treal)(0.0)) theta2 = -theta2;
    // Find the difference and make it range from -PI to PI
    treal theta = theta2 - theta1;
    if (theta > M_SPI)
        theta -= M_SPI2;
    else if (theta < -M_SPI)
        theta += M_SPI2;
    return theta;
}

bool Geom::Vector3d::is_valid() const {
    return m_x == m_x && m_y == m_y && m_z == m_z;
}

bool Geom::Vector3d::is_parallel_to(const Vector3d& other) const {
    Vector3d v1(normalize());
    Vector3d v2(other.normalize());
    return v1 == v2 || v1 == v2.reverse();
}

bool Geom::Vector3d::is_same_direction_as(const Vector3d& other) const {
    return normalize() == other.normalize();
}

bool Geom::Vector3d::is_aniparallel_to(const Vector3d& other) const {
    return normalize() == other.normalize().reverse();
}
