/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "geom_transformation.h"
#include "geom_vector4d.h"
#include "geom_quaternion.h"


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constructors
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Geom::Transformation::Transformation() :
	m_xaxis(1.0, 0.0, 0.0, 0.0),
	m_yaxis(0.0, 1.0, 0.0, 0.0),
	m_zaxis(0.0, 0.0, 1.0, 0.0),
	m_origin(0.0, 0.0, 0.0, 1.0)
{
}

Geom::Transformation::Transformation(const Vector3d& origin) :
	m_xaxis(1.0, 0.0, 0.0, 0.0),
	m_yaxis(0.0, 1.0, 0.0, 0.0),
	m_zaxis(0.0, 0.0, 1.0, 0.0),
	m_origin(origin.m_x, origin.m_y, origin.m_z, 1.0)
{
}

Geom::Transformation::Transformation(const Vector3d& origin, const Vector3d& zaxis) :
    m_zaxis(zaxis.m_x, zaxis.m_y, zaxis.m_z, 0.0),
	m_origin(origin.m_x, origin.m_y, origin.m_z, (treal)(1.0))
{
    m_zaxis.normalize_self();
	if (fabs(zaxis.m_z) < (treal)(0.9999995)) {
		//m_xaxis = Vector3d::Z_AXIS.cross(zaxis);
		m_xaxis.m_x = -zaxis.m_y;
		m_xaxis.m_y = zaxis.m_x;
		m_xaxis.m_z = 0.0;
	}
	else {
		//m_xaxis = Vector3d::Y_AXIS.cross(zaxis);
		m_xaxis.m_x = zaxis.m_z;
		m_xaxis.m_y = 0.0;
		m_xaxis.m_z = -zaxis.m_x;
	}
	m_xaxis.normalize_self();
	m_xaxis.m_w = 0.0;
    m_yaxis = m_zaxis.cross(m_xaxis);
    m_yaxis.normalize_self();
	m_yaxis.m_w = 0.0;
}

Geom::Transformation::Transformation(const Vector3d& xaxis, const Vector3d& yaxis, const Vector3d& zaxis) :
	m_xaxis(xaxis.m_x, xaxis.m_y, xaxis.m_z, 0.0),
	m_yaxis(yaxis.m_x, yaxis.m_y, yaxis.m_z, 0.0),
	m_zaxis(zaxis.m_x, zaxis.m_y, zaxis.m_z, 0.0),
	m_origin(0.0, 0.0, 0.0, 1.0)
{
}

Geom::Transformation::Transformation(const Vector3d& xaxis, const Vector3d& yaxis, const Vector3d& zaxis, const Vector3d& origin) :
    m_xaxis(xaxis.m_x, xaxis.m_y, xaxis.m_z, 0.0),
	m_yaxis(yaxis.m_x, yaxis.m_y, yaxis.m_z, 0.0),
	m_zaxis(zaxis.m_x, zaxis.m_y, zaxis.m_z, 0.0),
	m_origin(origin.m_x, origin.m_y, origin.m_z, 1.0)
{
}

Geom::Transformation::Transformation(const Vector4d& xaxis, const Vector4d& yaxis, const Vector4d& zaxis, const Vector4d& origin) :
	m_xaxis(xaxis),
	m_yaxis(yaxis),
	m_zaxis(zaxis),
	m_origin(origin)
{
}

Geom::Transformation::Transformation(const Transformation& other) :
    m_xaxis(other.m_xaxis), m_yaxis(other.m_yaxis), m_zaxis(other.m_zaxis), m_origin(other.m_origin)
{
}

Geom::Transformation::Transformation(const Quaternion& rotation, Geom::Vector3d position) :
    m_origin(position.m_x, position.m_y, position.m_z, 1.0)
{
    treal x2 = (treal)(2.0) * rotation.m_q1 * rotation.m_q1;
    treal y2 = (treal)(2.0) * rotation.m_q2 * rotation.m_q2;
    treal z2 = (treal)(2.0) * rotation.m_q3 * rotation.m_q3;

    treal xy = (treal)(2.0) * rotation.m_q1 * rotation.m_q2;
    treal xz = (treal)(2.0) * rotation.m_q1 * rotation.m_q3;
    treal xw = (treal)(2.0) * rotation.m_q1 * rotation.m_q0;
    treal yz = (treal)(2.0) * rotation.m_q2 * rotation.m_q3;
    treal yw = (treal)(2.0) * rotation.m_q2 * rotation.m_q0;
    treal zw = (treal)(2.0) * rotation.m_q3 * rotation.m_q0;

    m_xaxis = Geom::Vector4d((treal)(1.0) - y2 - z2, xy + zw, xz - yw, 0.0);
    m_yaxis = Geom::Vector4d(xy - zw, (treal)(1.0) - x2 - z2, yz + xw, 0.0);
    m_zaxis = Geom::Vector4d(xz + yw, yz - xw, (treal)(1.0) - x2 - y2, 0.0);
}

Geom::Transformation::Transformation(const treal* matrix) :
    m_xaxis(matrix[0], matrix[1], matrix[2], matrix[3]),
    m_yaxis(matrix[4], matrix[5], matrix[6], matrix[7]),
    m_zaxis(matrix[8], matrix[9], matrix[10], matrix[11]),
    m_origin(matrix[12], matrix[13], matrix[14], matrix[15])
{
}

Geom::Transformation::Transformation(const Geom::Vector3d& origin, const Geom::Vector3d& normal, treal angle)
{
    angle *= (treal)(0.5);
    treal q0 = cos(angle);
    treal sin_ang = sin(angle);
    treal q1 = normal.m_x * sin_ang;
    treal q2 = normal.m_y * sin_ang;
    treal q3 = normal.m_z * sin_ang;

    treal x2 = (treal)(2.0) * q1 * q1;
    treal y2 = (treal)(2.0) * q2 * q2;
    treal z2 = (treal)(2.0) * q3 * q3;

    treal xy = (treal)(2.0) * q1 * q2;
    treal xz = (treal)(2.0) * q1 * q3;
    treal xw = (treal)(2.0) * q1 * q0;
    treal yz = (treal)(2.0) * q2 * q3;
    treal yw = (treal)(2.0) * q2 * q0;
    treal zw = (treal)(2.0) * q3 * q0;

    m_xaxis = Geom::Vector4d((treal)(1.0) - y2 - z2, xy + zw, xz - yw, 0.0);
    m_yaxis = Geom::Vector4d(xy - zw, (treal)(1.0) - x2 - z2, yz + xw, 0.0);
    m_zaxis = Geom::Vector4d(xz + yw, yz - xw, (treal)(1.0) - x2 - y2, 0.0);

    m_origin.m_w = (treal)(1.0);
    m_origin = transform_vector(origin.reverse());
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Operators
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Geom::Transformation& Geom::Transformation::operator = (const Transformation& other) {
    if (this != &other) {
        m_xaxis = other.m_xaxis;
        m_yaxis = other.m_yaxis;
        m_zaxis = other.m_zaxis;
        m_origin = other.m_origin;
    }
    return *this;
}

Geom::Vector4d& Geom::Transformation::operator [] (const int index) {
    return (&m_xaxis)[index];
}

const Geom::Vector4d& Geom::Transformation::operator [] (const int index) const {
    return (&m_xaxis)[index];
}

Geom::Transformation operator * (const Geom::Transformation& tA, const Geom::Transformation& tB) {
    return Geom::Transformation(
		Geom::Vector4d(
            tA[0][0] * tB[0][0] + tA[0][1] * tB[1][0] + tA[0][2] * tB[2][0] + tA[0][3] * tB[3][0],
            tA[0][0] * tB[0][1] + tA[0][1] * tB[1][1] + tA[0][2] * tB[2][1] + tA[0][3] * tB[3][1],
            tA[0][0] * tB[0][2] + tA[0][1] * tB[1][2] + tA[0][2] * tB[2][2] + tA[0][3] * tB[3][2],
            tA[0][0] * tB[0][3] + tA[0][1] * tB[1][3] + tA[0][2] * tB[2][3] + tA[0][3] * tB[3][3]),
		Geom::Vector4d(
            tA[1][0] * tB[0][0] + tA[1][1] * tB[1][0] + tA[1][2] * tB[2][0] + tA[1][3] * tB[3][0],
            tA[1][0] * tB[0][1] + tA[1][1] * tB[1][1] + tA[1][2] * tB[2][1] + tA[1][3] * tB[3][1],
            tA[1][0] * tB[0][2] + tA[1][1] * tB[1][2] + tA[1][2] * tB[2][2] + tA[1][3] * tB[3][2],
            tA[1][0] * tB[0][3] + tA[1][1] * tB[1][3] + tA[1][2] * tB[2][3] + tA[1][3] * tB[3][3]),
		Geom::Vector4d(
            tA[2][0] * tB[0][0] + tA[2][1] * tB[1][0] + tA[2][2] * tB[2][0] + tA[2][3] * tB[3][0],
            tA[2][0] * tB[0][1] + tA[2][1] * tB[1][1] + tA[2][2] * tB[2][1] + tA[2][3] * tB[3][1],
            tA[2][0] * tB[0][2] + tA[2][1] * tB[1][2] + tA[2][2] * tB[2][2] + tA[2][3] * tB[3][2],
            tA[2][0] * tB[0][3] + tA[2][1] * tB[1][3] + tA[2][2] * tB[2][3] + tA[2][3] * tB[3][3]),
		Geom::Vector4d(
            tA[3][0] * tB[0][0] + tA[3][1] * tB[1][0] + tA[3][2] * tB[2][0] + tA[3][3] * tB[3][0],
            tA[3][0] * tB[0][1] + tA[3][1] * tB[1][1] + tA[3][2] * tB[2][1] + tA[3][3] * tB[3][1],
            tA[3][0] * tB[0][2] + tA[3][1] * tB[1][2] + tA[3][2] * tB[2][2] + tA[3][3] * tB[3][2],
            tA[3][0] * tB[0][3] + tA[3][1] * tB[1][3] + tA[3][2] * tB[2][3] + tA[3][3] * tB[3][3]));
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Geom::Transformation Geom::Transformation::inverse() const {
    treal inv[16], det;
    int i;

    inv[0] = m_yaxis.m_y  * m_zaxis.m_z * m_origin.m_w -
             m_yaxis.m_y  * m_zaxis.m_w * m_origin.m_z -
             m_zaxis.m_y  * m_yaxis.m_z  * m_origin.m_w +
             m_zaxis.m_y  * m_yaxis.m_w  * m_origin.m_z +
             m_origin.m_y * m_yaxis.m_z  * m_zaxis.m_w -
             m_origin.m_y * m_yaxis.m_w  * m_zaxis.m_z;

    inv[4] = -m_yaxis.m_x  * m_zaxis.m_z * m_origin.m_w +
              m_yaxis.m_x  * m_zaxis.m_w * m_origin.m_z +
              m_zaxis.m_x  * m_yaxis.m_z  * m_origin.m_w -
              m_zaxis.m_x  * m_yaxis.m_w  * m_origin.m_z -
              m_origin.m_x * m_yaxis.m_z  * m_zaxis.m_w +
              m_origin.m_x * m_yaxis.m_w  * m_zaxis.m_z;

    inv[8] = m_yaxis.m_x  * m_zaxis.m_y * m_origin.m_w -
             m_yaxis.m_x  * m_zaxis.m_w * m_origin.m_y -
             m_zaxis.m_x  * m_yaxis.m_y * m_origin.m_w +
             m_zaxis.m_x  * m_yaxis.m_w * m_origin.m_y +
             m_origin.m_x * m_yaxis.m_y * m_zaxis.m_w -
             m_origin.m_x * m_yaxis.m_w * m_zaxis.m_y;

    inv[12] = -m_yaxis.m_x  * m_zaxis.m_y * m_origin.m_z +
               m_yaxis.m_x  * m_zaxis.m_z * m_origin.m_y +
               m_zaxis.m_x  * m_yaxis.m_y * m_origin.m_z -
               m_zaxis.m_x  * m_yaxis.m_z * m_origin.m_y -
               m_origin.m_x * m_yaxis.m_y * m_zaxis.m_z +
               m_origin.m_x * m_yaxis.m_z * m_zaxis.m_y;

    inv[1] = -m_xaxis.m_y  * m_zaxis.m_z * m_origin.m_w +
              m_xaxis.m_y  * m_zaxis.m_w * m_origin.m_z +
              m_zaxis.m_y  * m_xaxis.m_z * m_origin.m_w -
              m_zaxis.m_y  * m_xaxis.m_w * m_origin.m_z -
              m_origin.m_y * m_xaxis.m_z * m_zaxis.m_w +
              m_origin.m_y * m_xaxis.m_w * m_zaxis.m_z;

    inv[5] = m_xaxis.m_x  * m_zaxis.m_z * m_origin.m_w -
             m_xaxis.m_x  * m_zaxis.m_w * m_origin.m_z -
             m_zaxis.m_x  * m_xaxis.m_z * m_origin.m_w +
             m_zaxis.m_x  * m_xaxis.m_w * m_origin.m_z +
             m_origin.m_x * m_xaxis.m_z * m_zaxis.m_w -
             m_origin.m_x * m_xaxis.m_w * m_zaxis.m_z;

    inv[9] = -m_xaxis.m_x  * m_zaxis.m_y * m_origin.m_w +
              m_xaxis.m_x  * m_zaxis.m_w * m_origin.m_y +
              m_zaxis.m_x  * m_xaxis.m_y * m_origin.m_w -
              m_zaxis.m_x  * m_xaxis.m_w * m_origin.m_y -
              m_origin.m_x * m_xaxis.m_y * m_zaxis.m_w +
              m_origin.m_x * m_xaxis.m_w * m_zaxis.m_y;

    inv[13] = m_xaxis.m_x  * m_zaxis.m_y * m_origin.m_z -
              m_xaxis.m_x  * m_zaxis.m_z * m_origin.m_y -
              m_zaxis.m_x  * m_xaxis.m_y * m_origin.m_z +
              m_zaxis.m_x  * m_xaxis.m_z * m_origin.m_y +
              m_origin.m_x * m_xaxis.m_y * m_zaxis.m_z -
              m_origin.m_x * m_xaxis.m_z * m_zaxis.m_y;

    inv[2] = m_xaxis.m_y  * m_yaxis.m_z * m_origin.m_w -
             m_xaxis.m_y  * m_yaxis.m_w * m_origin.m_z -
             m_yaxis.m_y  * m_xaxis.m_z * m_origin.m_w +
             m_yaxis.m_y  * m_xaxis.m_w * m_origin.m_z +
             m_origin.m_y * m_xaxis.m_z * m_yaxis.m_w -
             m_origin.m_y * m_xaxis.m_w * m_yaxis.m_z;

    inv[6] = -m_xaxis.m_x  * m_yaxis.m_z * m_origin.m_w +
              m_xaxis.m_x  * m_yaxis.m_w * m_origin.m_z +
              m_yaxis.m_x  * m_xaxis.m_z * m_origin.m_w -
              m_yaxis.m_x  * m_xaxis.m_w * m_origin.m_z -
              m_origin.m_x * m_xaxis.m_z * m_yaxis.m_w +
              m_origin.m_x * m_xaxis.m_w * m_yaxis.m_z;

    inv[10] = m_xaxis.m_x  * m_yaxis.m_y * m_origin.m_w -
              m_xaxis.m_x  * m_yaxis.m_w * m_origin.m_y -
              m_yaxis.m_x  * m_xaxis.m_y * m_origin.m_w +
              m_yaxis.m_x  * m_xaxis.m_w * m_origin.m_y +
              m_origin.m_x * m_xaxis.m_y * m_yaxis.m_w -
              m_origin.m_x * m_xaxis.m_w * m_yaxis.m_y;

    inv[14] = -m_xaxis.m_x  * m_yaxis.m_y * m_origin.m_z +
               m_xaxis.m_x  * m_yaxis.m_z * m_origin.m_y +
               m_yaxis.m_x  * m_xaxis.m_y * m_origin.m_z -
               m_yaxis.m_x  * m_xaxis.m_z * m_origin.m_y -
               m_origin.m_x * m_xaxis.m_y * m_yaxis.m_z +
               m_origin.m_x * m_xaxis.m_z * m_yaxis.m_y;

    inv[3] = -m_xaxis.m_y * m_yaxis.m_z * m_zaxis.m_w +
              m_xaxis.m_y * m_yaxis.m_w * m_zaxis.m_z +
              m_yaxis.m_y * m_xaxis.m_z * m_zaxis.m_w -
              m_yaxis.m_y * m_xaxis.m_w * m_zaxis.m_z -
              m_zaxis.m_y * m_xaxis.m_z * m_yaxis.m_w +
              m_zaxis.m_y * m_xaxis.m_w * m_yaxis.m_z;

    inv[7] = m_xaxis.m_x * m_yaxis.m_z * m_zaxis.m_w -
             m_xaxis.m_x * m_yaxis.m_w * m_zaxis.m_z -
             m_yaxis.m_x * m_xaxis.m_z * m_zaxis.m_w +
             m_yaxis.m_x * m_xaxis.m_w * m_zaxis.m_z +
             m_zaxis.m_x * m_xaxis.m_z * m_yaxis.m_w -
             m_zaxis.m_x * m_xaxis.m_w * m_yaxis.m_z;

    inv[11] = -m_xaxis.m_x * m_yaxis.m_y * m_zaxis.m_w +
               m_xaxis.m_x * m_yaxis.m_w * m_zaxis.m_y +
               m_yaxis.m_x * m_xaxis.m_y * m_zaxis.m_w -
               m_yaxis.m_x * m_xaxis.m_w * m_zaxis.m_y -
               m_zaxis.m_x * m_xaxis.m_y * m_yaxis.m_w +
               m_zaxis.m_x * m_xaxis.m_w * m_yaxis.m_y;

    inv[15] = m_xaxis.m_x * m_yaxis.m_y * m_zaxis.m_z -
              m_xaxis.m_x * m_yaxis.m_z * m_zaxis.m_y -
              m_yaxis.m_x * m_xaxis.m_y * m_zaxis.m_z +
              m_yaxis.m_x * m_xaxis.m_z * m_zaxis.m_y +
              m_zaxis.m_x * m_xaxis.m_y * m_yaxis.m_z -
              m_zaxis.m_x * m_xaxis.m_z * m_yaxis.m_y;

    det = m_xaxis.m_x * inv[0] + m_xaxis.m_y * inv[4] + m_xaxis.m_z * inv[8] + m_xaxis.m_w * inv[12];

    if (fabs(det) > M_EPSILON)
        det = (treal)(1.0) / det;
    else
        det = (treal)(0.0);

    for (i = 0; i < 16; i++)
        inv[i] *= det;

    return Transformation(inv);
}

Geom::Vector3d Geom::Transformation::get_scale() const {
    if (fabs(m_origin.m_w) > M_EPSILON) {
        treal inv_scale = (treal)(1.0) / m_origin.m_w;
        return Vector3d(m_xaxis.get_length() * inv_scale, m_yaxis.get_length() * inv_scale, m_zaxis.get_length() * inv_scale);
    }
    else
        return Vector3d(m_xaxis.get_length(), m_yaxis.get_length(), m_zaxis.get_length());
}

Geom::Transformation& Geom::Transformation::set_scale(const Vector3d& scale) {
    m_xaxis.set_length(scale.m_x);
    m_yaxis.set_length(scale.m_y);
    m_zaxis.set_length(scale.m_z);

    if (fabs(m_origin.m_w) > M_EPSILON)
        m_origin.scale_self((treal)(1.0) / m_origin.m_w);
    m_origin.m_w = (treal)(1.0);

    return *this;
}

Geom::Transformation& Geom::Transformation::normalize_self() {
    m_xaxis.normalize_self();
    m_yaxis.normalize_self();
    m_zaxis.normalize_self();

    if (fabs(m_origin.m_w) > M_EPSILON)
        m_origin.scale_self((treal)(1.0) / m_origin.m_w);

	m_xaxis.m_w = 0.0;
	m_yaxis.m_w = 0.0;
	m_zaxis.m_w = 0.0;
    m_origin.m_w = (treal)(1.0);

    return *this;
}

Geom::Transformation Geom::Transformation::normalize() const {
    Geom::Vector4d origin(m_origin);

    if (fabs(m_origin.m_w) > M_EPSILON)
        origin.scale_self((treal)(1.0) / m_origin.m_w);
    origin.m_w = (treal)(1.0);

    return Transformation(m_xaxis.normalize(), m_yaxis.normalize(), m_zaxis.normalize(), origin);
}

Geom::Transformation& Geom::Transformation::scale_axes_self(const Vector3d& scale) {
    m_xaxis.scale_self(scale.m_x);
    m_yaxis.scale_self(scale.m_y);
    m_zaxis.scale_self(scale.m_z);

    return *this;
}

Geom::Transformation Geom::Transformation::scale_axes(const Vector3d& scale) const {
    return Transformation(m_xaxis.scale(scale.m_x), m_yaxis.scale(scale.m_y), m_zaxis.scale(scale.m_z), m_origin);
}

Geom::Transformation Geom::Transformation::rotate_xaxis_to(const Vector3d& dir) const {
    // Determine the cross product between the z-axis of matrix 1 and the given dir.
    Vector3d dir1(m_xaxis);
    dir1.normalize_self();
    Vector3d dir2(dir);
    dir2.normalize_self();
    treal cos_theta = dir1.dot(dir2);
    Vector3d normal;
    if (cos_theta > (treal)(0.9999995))
        return Transformation(*this);
    else if (cos_theta < (treal)(-0.9999995))
        normal = m_yaxis;
    else
        normal = dir1.cross(dir2);
    //Vector3d normal(dir1.cross(dir2));
    normal.normalize_self();
    // Develop two temporary matrices.
    Vector3d yaxis1(normal.cross(dir1));
    yaxis1.normalize_self();
    Vector3d yaxis2(normal.cross(dir2));
    yaxis2.normalize_self();
    Transformation t1(dir1, yaxis1, normal);
    Transformation t2(dir2, yaxis2, normal);
    // Unrotate matrix with respect to t1.
    // Then rotate the result with respect to t2.
    Transformation result(((*this) * t1.inverse()) * t2);
    result.m_origin = m_origin;

    return result;
}

Geom::Transformation Geom::Transformation::rotate_yaxis_to(const Vector3d& dir) const {
    // Determine the cross product between the z-axis of matrix 1 and the given dir.
    Vector3d dir1(m_yaxis);
    dir1.normalize_self();
    Vector3d dir2(dir);
    dir2.normalize_self();
    treal cos_theta = dir1.dot(dir2);
    Vector3d normal;
    if (cos_theta > (treal)(0.9999995))
        return Transformation(*this);
    else if (cos_theta < (treal)(-0.9999995))
        normal = m_zaxis;
    else
        normal = dir1.cross(dir2);
    //Vector3d normal(dir1.cross(dir2));
    normal.normalize_self();
    // Develop two temporary matrices.
    Vector3d yaxis1(normal.cross(dir1));
    yaxis1.normalize_self();
    Vector3d yaxis2(normal.cross(dir2));
    yaxis2.normalize_self();
    Transformation t1(dir1, yaxis1, normal);
    Transformation t2(dir2, yaxis2, normal);
    // Unrotate matrix with respect to t1.
    // Then rotate the result with respect to t2.
    Transformation result(((*this) * t1.inverse()) * t2);
    result.m_origin = m_origin;

    return result;
}

Geom::Transformation Geom::Transformation::rotate_zaxis_to(const Vector3d& dir) const {
    // Determine the cross product between the z-axis of matrix 1 and the given dir.
    Vector3d dir1(m_zaxis);
    dir1.normalize_self();
    Vector3d dir2(dir);
    dir2.normalize_self();
    treal cos_theta = dir1.dot(dir2);
    Vector3d normal;
    if (cos_theta > (treal)(0.9999995))
        return Transformation(*this);
    else if (cos_theta < (treal)(-0.9999995))
        normal = m_xaxis;
    else
        normal = dir1.cross(dir2);
    //Vector3d normal(dir1.cross(dir2));
    normal.normalize_self();
    // Develop two temporary matrices.
    Vector3d yaxis1(normal.cross(dir1));
    yaxis1.normalize_self();
    Vector3d yaxis2(normal.cross(dir2));
    yaxis2.normalize_self();
    Transformation t1(dir1, yaxis1, normal);
    Transformation t2(dir2, yaxis2, normal);
    // Unrotate matrix with respect to t1.
    // Then rotate the result with respect to t2.
    Transformation result(((*this) * t1.inverse()) * t2);
    result.m_origin = m_origin;

    return result;
}

Geom::Transformation Geom::Transformation::uniform_normal_transition_to(const Transformation& other, treal ratio) const {
    // Assuming both transformations are unform, normal, and not flipped.
    Geom::Transformation other_inv(other.inverse());

    // Compute the normal at which the zaxis should rotate
    treal zcos_theta = m_zaxis.dot(other.m_zaxis);
    Vector3d normal(m_xaxis);
    if (fabs(zcos_theta) < (treal)(0.9999995))
        normal = m_zaxis.cross(other.m_zaxis);
    normal.normalize_self();

    // Calculate the new xaxis when this matrix is rotated to match the other matrix's zaxis, along the normal.
    treal ztheta = acos(Geom::clamp_treal(zcos_theta, (treal)(-1.0), (treal)(1.0)));
    Geom::Vector3d rot_xaxis(m_xaxis.rotate(normal, ztheta));

    // Compute the angle between rot_xaxis and other.m_xaxis
    // We'll do this by first transforming rot_xaxis relative to other matrix.
    Geom::Vector3d loc_rot_xaxis(other_inv.rotate_vector(rot_xaxis));

    // Now compute angle between loc_rot_xaxis and X_AXIS
    treal xtheta = acos(Geom::clamp_treal(loc_rot_xaxis.m_x, (treal)(-1.0), (treal)(1.0)));
    if (loc_rot_xaxis.m_y < 0.0) xtheta = -xtheta;

    // With the angles determined, rotate this matrix a specific ratio to the other matrix.
    Geom::Vector3d new_zaxis(m_zaxis.rotate(normal, ztheta * ratio));
    Geom::Vector3d new_xaxis(m_xaxis.rotate(normal, ztheta * ratio));
    new_xaxis = new_xaxis.rotate(new_zaxis, -xtheta * ratio);
    Geom::Vector3d new_yaxis(new_zaxis.cross(new_xaxis));

    // Transition origin and return transformation
    return Geom::Transformation(new_xaxis, new_yaxis, new_zaxis, m_origin.transition_to(other.m_origin, ratio));
}

Geom::Transformation Geom::Transformation::uniform_transition_to(const Transformation& other, treal ratio) const {
    // Assuming both transformations are unform and not flipped.
    Geom::Transformation other_inv(other.inverse());

    // Get axes scales
    Geom::Vector3d t1_scale(get_scale());
    Geom::Vector3d t2_scale(other.get_scale());

    // Extract origin scales
    Geom::Vector3d t1_origin(m_origin);
    Geom::Vector3d t2_origin(other.m_origin);

    if (fabs(m_origin.m_w) > M_EPSILON)
        t1_origin.scale_self((treal)(1.0) / m_origin.m_w);

    if (fabs(other.m_origin.m_w) > M_EPSILON)
        t2_origin.scale_self((treal)(1.0) / other.m_origin.m_w);

    // Compute the normal at which the zaxis should rotate
    Geom::Vector3d t1_normal_zaxis(fabs(t1_scale.m_z) > M_EPSILON ? m_zaxis.scale((treal)(1.0) / t1_scale.m_z) : m_zaxis);
    Geom::Vector3d t2_normal_zaxis(fabs(t2_scale.m_z) > M_EPSILON ? other.m_zaxis.scale((treal)(1.0) / t2_scale.m_z) : other.m_zaxis);
    treal zcos_theta = t1_normal_zaxis.dot(t2_normal_zaxis);
    Vector3d normal(m_xaxis);
    if (fabs(zcos_theta) < (treal)(0.9999995))
        normal = t1_normal_zaxis.cross(t2_normal_zaxis);
    normal.normalize_self();

    // Calculate the new xaxis when this matrix is rotated to match the other matrix's zaxis, along the normal.
    treal ztheta = acos(Geom::clamp_treal(zcos_theta, (treal)(-1.0), (treal)(1.0)));
    Geom::Vector3d rot_xaxis(m_xaxis.rotate(normal, ztheta));

    // Compute the angle between rot_xaxis and other.m_xaxis
    // We'll do this by first transforming rot_xaxis relative to other matrix.
    Geom::Vector3d loc_rot_xaxis(other_inv.rotate_vector(rot_xaxis));

    // Now compute angle between loc_rot_xaxis and X_AXIS
    loc_rot_xaxis.m_x /= t2_scale.m_x;
    loc_rot_xaxis.m_y /= t2_scale.m_y;
    treal hypotenuse = sqrt(loc_rot_xaxis.m_x * loc_rot_xaxis.m_x + loc_rot_xaxis.m_y * loc_rot_xaxis.m_y);
    treal xtheta;
    if (hypotenuse > M_EPSILON)
        xtheta = acos(Geom::clamp_treal(loc_rot_xaxis.m_x / hypotenuse, (treal)(-1.0), (treal)(1.0)));
    else
        xtheta = (treal)(0.0);
    if (loc_rot_xaxis.m_y < (treal)(0.0)) xtheta = -xtheta;

    // With the angles determined, rotate this matrix a specific ratio to the other matrix.
    Geom::Vector3d new_zaxis(t1_normal_zaxis.rotate(normal, ztheta * ratio));
    Geom::Vector3d new_xaxis(m_xaxis.rotate(normal, ztheta * ratio));
    new_xaxis = new_xaxis.rotate(new_zaxis, -xtheta * ratio);
    Geom::Vector3d new_yaxis(new_zaxis.cross(new_xaxis));

    // Transition axes scale
    Geom::Vector3d new_scale(t1_scale.transition_to(t2_scale, ratio));
    new_xaxis.set_length(new_scale.m_x);
    new_yaxis.set_length(new_scale.m_y);
    new_zaxis.set_length(new_scale.m_z);

    // Transition origin and return transformation
    return Geom::Transformation(new_xaxis, new_yaxis, new_zaxis, t1_origin.transition_to(t2_origin, ratio));
}

Geom::Vector3d Geom::Transformation::transform_vector(const Vector3d& v) const {
    treal det;
    if (fabs(m_origin.m_w) > M_EPSILON)
        det = (treal)(1.0) / m_origin.m_w;
    else
        det = (treal)(0.0);
    return Vector3d(
        (v.m_x * m_xaxis.m_x + v.m_y * m_yaxis.m_x + v.m_z * m_zaxis.m_x + m_origin.m_x) * det,
        (v.m_x * m_xaxis.m_y + v.m_y * m_yaxis.m_y + v.m_z * m_zaxis.m_y + m_origin.m_y) * det,
        (v.m_x * m_xaxis.m_z + v.m_y * m_yaxis.m_z + v.m_z * m_zaxis.m_z + m_origin.m_z) * det);
}

Geom::Vector3d Geom::Transformation::rotate_vector(const Vector3d& v) const {
    treal det;
    if (fabs(m_origin.m_w) > M_EPSILON)
        det = (treal)(1.0) / m_origin.m_w;
    else
        det = (treal)(0.0);
    return Vector3d(
        (v.m_x * m_xaxis.m_x + v.m_y * m_yaxis.m_x + v.m_z * m_zaxis.m_x) * det,
        (v.m_x * m_xaxis.m_y + v.m_y * m_yaxis.m_y + v.m_z * m_zaxis.m_y) * det,
        (v.m_x * m_xaxis.m_z + v.m_y * m_yaxis.m_z + v.m_z * m_zaxis.m_z) * det);
}

Geom::Vector3d Geom::Transformation::transform_vector2(const Vector3d& v) const {
    return Vector3d(
        (v.m_x * m_xaxis.m_x + v.m_y * m_yaxis.m_x + v.m_z * m_zaxis.m_x + m_origin.m_x),
        (v.m_x * m_xaxis.m_y + v.m_y * m_yaxis.m_y + v.m_z * m_zaxis.m_y + m_origin.m_y),
        (v.m_x * m_xaxis.m_z + v.m_y * m_yaxis.m_z + v.m_z * m_zaxis.m_z + m_origin.m_z));
}

Geom::Vector3d Geom::Transformation::rotate_vector2(const Vector3d& v) const {
    return Vector3d(
        (v.m_x * m_xaxis.m_x + v.m_y * m_yaxis.m_x + v.m_z * m_zaxis.m_x),
        (v.m_x * m_xaxis.m_y + v.m_y * m_yaxis.m_y + v.m_z * m_zaxis.m_y),
        (v.m_x * m_xaxis.m_z + v.m_y * m_yaxis.m_z + v.m_z * m_zaxis.m_z));
}

Geom::Vector3d Geom::Transformation::transform_vector3(const Vector3d& v, treal det) const {
    return Vector3d(
        (v.m_x * m_xaxis.m_x + v.m_y * m_yaxis.m_x + v.m_z * m_zaxis.m_x + m_origin.m_x) * det,
        (v.m_x * m_xaxis.m_y + v.m_y * m_yaxis.m_y + v.m_z * m_zaxis.m_y + m_origin.m_y) * det,
        (v.m_x * m_xaxis.m_z + v.m_y * m_yaxis.m_z + v.m_z * m_zaxis.m_z + m_origin.m_z) * det);
}

Geom::Vector3d Geom::Transformation::rotate_vector3(const Vector3d& v, treal det) const {
    return Vector3d(
        (v.m_x * m_xaxis.m_x + v.m_y * m_yaxis.m_x + v.m_z * m_zaxis.m_x) * det,
        (v.m_x * m_xaxis.m_y + v.m_y * m_yaxis.m_y + v.m_z * m_zaxis.m_y) * det,
        (v.m_x * m_xaxis.m_z + v.m_y * m_yaxis.m_z + v.m_z * m_zaxis.m_z) * det);
}

treal Geom::Transformation::get_determinant() const {
    if (fabs(m_origin.m_w) > M_EPSILON)
        return (treal)(1.0) / m_origin.m_w;
    else
        return (treal)(0.0);
}

void Geom::Transformation::extract_w_factor() {
    if (fabs(m_origin.m_w) > M_EPSILON) {
        treal det = (treal)(1.0) / m_origin.m_w;
        m_xaxis.scale_self(det);
        m_yaxis.scale_self(det);
        m_zaxis.scale_self(det);
        m_origin.scale_self(det);
        m_origin.m_w = 1.0;
    }
}

void Geom::Transformation::get_normal_xaxis(Geom::Vector3d& res) const {
    res = m_xaxis.normalize();
}

void Geom::Transformation::get_normal_yaxis(Geom::Vector3d& res) const {
    res = m_yaxis.normalize();
}

void Geom::Transformation::get_normal_zaxis(Geom::Vector3d& res) const {
    res = m_zaxis.normalize();
}

void Geom::Transformation::get_normal_origin(Geom::Vector3d& res) const {
    if (fabs(m_origin.m_w) > M_EPSILON) {
        treal det = (treal)(1.0) / m_origin.m_w;
        res = m_origin.scale(det);
    }
    else
        res = m_origin;
}

bool Geom::Transformation::is_uniform() const {
    return fabs(m_xaxis.dot(m_yaxis)) < M_EPSILON2 && fabs(m_xaxis.dot(m_zaxis)) < M_EPSILON2 && fabs(m_yaxis.dot(m_zaxis)) < M_EPSILON2;
}

bool Geom::Transformation::is_flipped() const {
    return (m_xaxis.cross(m_yaxis)).dot(m_zaxis) < (treal)(0.0);
}

bool Geom::Transformation::is_flat() const {
    return m_xaxis.get_length() < M_EPSILON || m_yaxis.get_length() < M_EPSILON || m_zaxis.get_length() < M_EPSILON;
}

void Geom::Transformation::zero_out() {
    m_xaxis.zero_out();
    m_yaxis.zero_out();
    m_zaxis.zero_out();
    m_origin.zero_out();

	m_xaxis.m_w = 0.0;
	m_yaxis.m_w = 0.0;
	m_zaxis.m_w = 0.0;
	m_origin.m_w = 1.0;
}
