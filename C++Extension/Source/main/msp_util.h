/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#pragma once

#include "utils/ruby_prep.h"
#include "utils/common.h"

#ifndef _NEWTON_USE_DOUBLE
#define _NEWTON_USE_DOUBLE
#endif

#include "Newton.h"
#include "dVector.h"
#include "dMatrix.h"
#include "dQuaternion.h"

#define M_SPI               (dFloat)(3.141592653589793)
#define M_EPSILON           (dFloat)(1.0e-5f)
#define M_EPSILON2          (dFloat)(1.0e-3f)
#define M_INCH_TO_METER     (dFloat)(0.0254)
#define M_INCH2_TO_METER2   (dFloat)(0.0254 * 0.0254)
#define M_INCH3_TO_METER3   (dFloat)(0.0254 * 0.0254 * 0.0254)
#define M_INCH4_TO_METER4   (dFloat)(0.0254 * 0.0254 * 0.0254 * 0.0254)
#define M_METER_TO_INCH     (dFloat)(1.0 / 0.0254)
#define M_METER2_TO_INCH2   (dFloat)(1.0 / (0.0254 * 0.0254))
#define M_METER3_TO_INCH3   (dFloat)(1.0 / (0.0254 * 0.0254 * 0.0254))
#define M_METER4_TO_INCH4   (dFloat)(1.0 / (0.0254 * 0.0254 * 0.0254 * 0.0254))
#define M_DEG_TO_RAD        (dFloat)(3.141592653589793 / 180.0)
#define M_RAD_TO_DEG        (dFloat)(180.0 / 3.141592653589793)

namespace Util {
    // Constants
    extern const dVector X_AXIS;
    extern const dVector Y_AXIS;
    extern const dVector Z_AXIS;
    extern const dVector ORIGIN;

    // Variables
    extern VALUE SU_SKETCHUP;
    extern VALUE SU_GEOM;
    extern VALUE SU_COLOR;
    extern VALUE SU_POINT3D;
    extern VALUE SU_VECTOR3D;
    extern VALUE SU_TRANSFORMATION;

    extern ID INTERN_NEW;
    extern ID INTERN_TO_A;
    extern ID INTERN_X;
    extern ID INTERN_Y;
    extern ID INTERN_Z;

    extern ID INTERN_RED;
    extern ID INTERN_SRED;
    extern ID INTERN_GREEN;
    extern ID INTERN_SGREEN;
    extern ID INTERN_BLUE;
    extern ID INTERN_SBLUE;
    extern ID INTERN_ALPHA;
    extern ID INTERN_SALPHA;

    extern ID INTERN_PUTS;
    extern ID INTERN_INSPECT;
    extern ID INTERN_BACKTRACE;
    extern ID INTERN_CALL;
    extern ID INTERN_PACK;
    extern ID INTERN_UNPACK;

    extern ID INTERN_XAXIS;
    extern ID INTERN_YAXIS;
    extern ID INTERN_ZAXIS;

    extern ID INTERN_SDRAWING_COLOR;
    extern ID INTERN_SLINE_WIDTH;
    extern ID INTERN_SLINE_STIPPLE;
    extern ID INTERN_DRAW;
    extern ID INTERN_DRAW2D;
    extern ID INTERN_SCREEN_COORDS;
    extern ID INTERN_CORNER;
    extern ID INTERN_CAMERA;
    extern ID INTERN_FOCAL_LENGTH;
    extern ID INTERN_EYE;
    extern ID INTERN_ADD;
    extern ID INTERN_CENTER;
    extern ID INTERN_VPWIDTH;
    extern ID INTERN_VPHEIGHT;

    extern ID INTERN_ACTIVE_MODEL;
    extern ID INTERN_ACTIVE_VIEW;

    extern bool s_validate_objects;

    // Functions
    double min_double(double a, double b);
    double max_double(double a, double b);
    double clamp_double(double val, double min_val, double max_val);

    int clamp_int(int val, int min_val, int max_val);
    unsigned int clamp_uint(unsigned int val, unsigned int min_val, unsigned int max_val);
    int max_int(int val1, int val2);

    dFloat min_dFloat(dFloat a, dFloat b);
    dFloat max_dFloat(dFloat a, dFloat b);
    dFloat clamp_dFloat(dFloat val, dFloat min_val, dFloat max_val);

    inline VALUE to_value(bool value) {
        return value ? Qtrue : Qfalse;
    }

    inline VALUE to_value(char value) {
        return rb_int2inum(value);
    }

    inline VALUE to_value(unsigned char value) {
        return rb_uint2inum(value);
    }

    inline VALUE to_value(short value) {
        return rb_int2inum(value);
    }

    inline VALUE to_value(unsigned short value) {
        return rb_uint2inum(value);
    }

    inline VALUE to_value(int value) {
        return rb_int2inum(value);
    }

    inline VALUE to_value(unsigned int value) {
        return rb_uint2inum(value);
    }

    inline VALUE to_value(long value) {
        return rb_int2inum(value);
    }

    inline VALUE to_value(unsigned long value) {
        return rb_uint2inum(value);
    }

    inline VALUE to_value(long long value) {
        return rb_ll2inum(value);
    }

    inline VALUE to_value(unsigned long long value) {
        return rb_ull2inum(value);
    }

    inline VALUE to_value(float value) {
        return rb_float_new(value);
    }

    inline VALUE to_value(double value) {
        return rb_float_new(value);
    }

    VALUE to_value(const char* value);
    VALUE to_value(const char* value, unsigned int length);

    VALUE to_value(const wchar_t* value);
    VALUE to_value(const wchar_t* value, unsigned int length);

    VALUE vector_to_value(const dVector& value);
    VALUE point_to_value(const dVector& value);
    VALUE matrix_to_value(const dMatrix& value);
    VALUE color_to_value(const dVector& value, dFloat alpha);

    inline bool value_to_bool(VALUE value) {
        return RTEST(value);
    }

    inline char value_to_char(VALUE value) {
        return static_cast<char>(NUM2INT(value));
    }

    inline unsigned char value_to_uchar(VALUE value) {
        return static_cast<unsigned char>(NUM2UINT(value));
    }

    inline short value_to_short(VALUE value) {
        return static_cast<short>(NUM2INT(value));
    }

    inline unsigned short value_to_ushort(VALUE value) {
        return static_cast<unsigned short>(NUM2UINT(value));
    }

    inline int value_to_int(VALUE value) {
        return NUM2INT(value);
    }

    inline unsigned int value_to_uint(VALUE value) {
        return NUM2UINT(value);
    }

    inline long value_to_long(VALUE value) {
        return NUM2LONG(value);
    }

    inline unsigned long value_to_ulong(VALUE value) {
        return NUM2ULONG(value);
    }

    inline long long value_to_ll(VALUE value) {
        return NUM2LL(value);
    }

    inline unsigned long long value_to_ull(VALUE value) {
        return rb_num2ull(value);
    }

    inline float value_to_float(VALUE value) {
        return static_cast<float>(rb_num2dbl(value));
    }

    inline double value_to_double(VALUE value) {
        return rb_num2dbl(value);
    }

    inline dFloat value_to_dFloat(VALUE value) {
        return static_cast<dFloat>(rb_num2dbl(value));
    }

    char* value_to_c_str(VALUE value);
    wchar_t* value_to_c_str2(VALUE value);

    inline unsigned int get_string_length(VALUE value) {
        return static_cast<unsigned int>(RSTRING_LEN(StringValue(value)));
    }

    dVector value_to_vector(VALUE value);
    dVector value_to_point(VALUE value);
    dMatrix value_to_matrix(VALUE value);
    dVector value_to_color(VALUE value);

    dFloat get_vector_magnitude(const dVector& vector);
    dFloat get_vector_magnitude2(const dVector& vector);
    void set_vector_magnitude(dVector& vector, dFloat magnitude);
    void normalize_vector(dVector& vector);
    void scale_vector(dVector& vector, dFloat scale);
    void zero_out_vector(dVector& vector);
    bool vectors_identical(const dVector& a, const dVector& b);
    bool is_vector_valid(const dVector& vector);

    bool is_matrix_uniform(const dMatrix& matrix);
    bool is_matrix_flat(const dMatrix& matrix);
    bool is_matrix_flipped(const dMatrix& matrix);
    dVector get_matrix_scale(const dMatrix& matrix);
    void set_matrix_scale(dMatrix& matrix, const dVector& scale);
    void extract_matrix_scale(dMatrix& matrix);
    void matrix_from_pin_dir(const dVector& pos, const dVector& dir, dMatrix& matrix_out);
    void rotate_matrix_to_dir(const dMatrix& matrix, const dVector& dir, dMatrix& matrix_out);
    void rotate_matrix_to_dir2(dMatrix& matrix, const dVector& dir);
    dVector rotate_vector(const dVector& vector, const dVector& normal, const dFloat& angle);
    void correct_to_expected_matrix(dMatrix& matrix, const dVector& velocity, const dVector& omega, dFloat timestep);

    VALUE call_proc(VALUE v_proc);
    VALUE rescue_proc(VALUE v_args, VALUE v_exception);

    template<typename T>
    inline bool is_number(T number) {
        return number == number;
    }

    // Main
    void init_ruby();
};

