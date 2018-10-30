/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "geom.h"
#include "geom_vector3d.h"


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

int Geom::min_int(int a, int b) {
    return b ^ ((a ^ b) & -(a < b));
}

int Geom::max_int(int a, int b) {
    return a ^ ((a ^ b) & -(a < b));
}

int Geom::clamp_int(int val, int min_val, int max_val) {
    return min_int(max_int(val, min_val), max_val);
}

long long Geom::min_ll(long long a, long long b) {
    return b ^ ((a ^ b) & -(a < b));
}

long long Geom::max_ll(long long a, long long b) {
    return a ^ ((a ^ b) & -(a < b));
}

long long Geom::clamp_ll(long long val, long long min_val, long long max_val) {
    return min_ll(max_ll(val, min_val), max_val);
}

float Geom::min_float(float a, float b) {
    _mm_store_ss(&a, _mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
    return a;
}

float Geom::max_float(float a, float b) {
    _mm_store_ss(&a, _mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
    return a;
}

float Geom::clamp_float(float val, float min_val, float max_val) {
    _mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(min_val)), _mm_set_ss(max_val)));
    return val;
}

double Geom::min_double(double a, double b) {
    _mm_store_sd(&a, _mm_min_sd(_mm_set_sd(a), _mm_set_sd(b)));
    return a;
}

double Geom::max_double(double a, double b) {
    _mm_store_sd(&a, _mm_max_sd(_mm_set_sd(a), _mm_set_sd(b)));
    return a;
}

double Geom::clamp_double(double val, double min_val, double max_val) {
    _mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val), _mm_set_sd(min_val)), _mm_set_sd(max_val)));
    return val;
}

treal Geom::min_treal(treal a, treal b) {
#ifdef M_USE_DOUBLE
    _mm_store_sd(&a, _mm_min_sd(_mm_set_sd(a), _mm_set_sd(b)));
#else
    _mm_store_ss(&a, _mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
#endif
    return a;
}

treal Geom::max_treal(treal a, treal b) {
#ifdef M_USE_DOUBLE
    _mm_store_sd(&a, _mm_max_sd(_mm_set_sd(a), _mm_set_sd(b)));
#else
    _mm_store_ss(&a, _mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
#endif
    return a;
}

treal Geom::clamp_treal(treal val, treal min_val, treal max_val) {
#ifdef M_USE_DOUBLE
    _mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val), _mm_set_sd(min_val)), _mm_set_sd(max_val)));
#else
    _mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(min_val)), _mm_set_ss(max_val)));
#endif
    return val;
}

void Geom::min_float2(float& a, float b) {
    _mm_store_ss(&a, _mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

void Geom::max_float2(float& a, float b) {
    _mm_store_ss(&a, _mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

void Geom::clamp_float2(float& val, float min_val, float max_val) {
    _mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(min_val)), _mm_set_ss(max_val)));
}

void Geom::min_double2(double& a, double b) {
    _mm_store_sd(&a, _mm_min_sd(_mm_set_sd(a), _mm_set_sd(b)));
}

void Geom::max_double2(double& a, double b) {
    _mm_store_sd(&a, _mm_max_sd(_mm_set_sd(a), _mm_set_sd(b)));
}

void Geom::clamp_double2(double& val, double min_val, double max_val) {
    _mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val), _mm_set_sd(min_val)), _mm_set_sd(max_val)));
}

void Geom::min_treal2(treal& a, treal b) {
#ifdef M_USE_DOUBLE
    _mm_store_sd(&a, _mm_min_sd(_mm_set_sd(a), _mm_set_sd(b)));
#else
    _mm_store_ss(&a, _mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
#endif
}

void Geom::max_treal2(treal& a, treal b) {
#ifdef M_USE_DOUBLE
    _mm_store_sd(&a, _mm_max_sd(_mm_set_sd(a), _mm_set_sd(b)));
#else
    _mm_store_ss(&a, _mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
#endif
}

void Geom::clamp_treal2(treal& val, treal min_val, treal max_val) {
#ifdef M_USE_DOUBLE
    _mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val), _mm_set_sd(min_val)), _mm_set_sd(max_val)));
#else
    _mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(min_val)), _mm_set_ss(max_val)));
#endif
}

double Geom::inv_sqrt(double x) {
    double xhalf = 0.5 * x;
    long long i = *(long long*)(&x);
    i = 0x5fe6ec85e7de30daLL - (i >> 1);
    x = *(double*)(&i);
    x = x * (1.5 - xhalf * x * x);
    return x;
}

float Geom::inv_sqrt(float x) {
    float xhalf = 0.5f * x;
    int i = *(int*)&x;
    i = 0x5f375a86 - (i >> 1);
    x = *(float*)&i;
    x = x * (1.5f - xhalf * x * x);
    return x;
}

unsigned int Geom::log2uint(unsigned int x) {
    unsigned int v = 0;
    while (x >>= 1) ++v;
    return v;
}

unsigned int Geom::pow2uint(unsigned int x) {
    return (unsigned int)(1) << x;
}

size_t Geom::round_up(size_t m, size_t x) {
	if (x < m)
		return m;
	else {
		size_t r = x % m;
		if (r != 0)
			return x + m - r;
		else
			return x;
	}
}

int Geom::intersect_segment_plane(const Geom::Vector3d& s1, const Geom::Vector3d& s2, const Geom::Vector3d& pl_point, const Geom::Vector3d& pl_normal, Geom::Vector3d& point_out) {
    Geom::Vector3d u(s2 - s1);
    Geom::Vector3d w(s1 - pl_point);
    treal d = u.dot(pl_normal);
    treal n = -w.dot(pl_normal);
    if (fabs(d) < M_EPSILON) { // segment is parallel to plane
        if (n == 0)
            return 2; // segment lies in plane
        else
            return 0; // no intersection
    }
    else {
        treal si = n / d;
        if (si < (treal)(0.0) || si > (treal)(1.0))
            return 0;
        else {
            point_out = s1 + u.scale(si);
            return 1;
        }
    }
}

int Geom::intersect_ray_plane(const Geom::Vector3d& ray_point, const Geom::Vector3d& ray_vector, const Geom::Vector3d& plane_point, const Geom::Vector3d& plane_normal, Geom::Vector3d& point_out) {
	treal d = ray_vector.dot(plane_normal);
	if (fabs(d) > M_EPSILON) {
		treal n = (plane_point - ray_point).dot(plane_normal);
		treal si = n / d;

		if (si < (treal)(0.0))
			return 0;
		else {
			point_out = ray_point + ray_vector.scale(si);
			return 1;
		}

	}
	else {
		return 0;
	}
}

void Geom::cpa_line_line(const Geom::Vector3d& pA, const Geom::Vector3d& vA, const Geom::Vector3d& pB, const Geom::Vector3d& vB, Geom::Vector3d& p1_out, Geom::Vector3d& p2_out) {
	Geom::Vector3d w(pA - pB);
	treal a = vA.get_length_squared();
	treal b = vA.dot(vB);
	treal c = vB.get_length_squared();
	treal d = vA.dot(w);
	treal e = vB.dot(w);
	treal f = a * c - b * b;
	treal sc, tc;
	if (f < M_EPSILON) {
		sc = 0.0;
		if (b > c)
			tc = d / b;
		else
			tc = e / c;
	}
	else {
		sc = (b * e - c * d) / f;
		tc = (a * e - b * d) / f;
	}

	p1_out = pA + vA.scale(sc);
	p2_out = pB + vB.scale(tc);
}

// cf. Appendix B of [Meyer et al 2002]
// http://www.geometry.caltech.edu/pubs/DMSB_III.pdf
treal Geom::cotan(const Geom::Vector3d& u, const Geom::Vector3d& v) {
	treal d = u.dot(v);

	treal sf = u.get_length_squared() * v.get_length_squared() - d * d;

	if (sf < M_EPSILON_SQ) {
		return (treal)(0.0);
	}
	else {
		return d / sqrt(sf);
	}
}
