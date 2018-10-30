/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef RUBY_UTIL_H
#define RUBY_UTIL_H

#include "ruby_prep.h"
#include "common.h"

#include "geom.h"
#include "geom_color.h"
#include "geom_vector3d.h"
#include "geom_transformation.h"
#include "geom_quaternion.h"
#include "geom_bounding_box.h"
#include "geom_box_space.h"

#include "dynamic_array.h"
#include "fast_queue.h"

namespace RU {
    // Variables
    extern VALUE SU_SKETCHUP;
    extern VALUE SU_GEOM;
    extern VALUE SU_CAMERA;
    extern VALUE SU_COLOR;
    extern VALUE SU_POINT3D;
    extern VALUE SU_VECTOR3D;
    extern VALUE SU_BOUNDING_BOX;
    extern VALUE SU_POLYGON_MESH;
    extern VALUE SU_TRANSFORMATION;
    extern VALUE SU_FACE;
    extern VALUE SU_EDGE;
    extern VALUE SU_COMPONENT_INSTANCE;
    extern VALUE SU_COMPONENT_DEFINITION;
    extern VALUE SU_GROUP;
	extern VALUE SU_ENTITIES;
	extern VALUE SU_ENTITY;
	extern VALUE SU_DRAWING_ELEMENT;
    extern VALUE SU_CONSTRUCTION_POINT;
    extern VALUE SU_CONSTRUCTION_LINE;
    extern VALUE SU_TEXT;
    extern VALUE SU_LANGUAGE_HANDLER;
	extern VALUE SU_MATERIAL;

    extern VALUE SU_FACE_POINT_UNKNOWN;
    extern VALUE SU_FACE_POINT_INSIDE;
    extern VALUE SU_FACE_POINT_ON_VERTEX;
    extern VALUE SU_FACE_POINT_ON_EDGE;
    extern VALUE SU_FACE_POINT_ON_FACE;
    extern VALUE SU_FACE_POINT_ON_OUTSIDE;
    extern VALUE SU_FACE_POINT_NOT_ON_PLANE;

    extern VALUE SU_GL_POINTS;
    extern VALUE SU_GL_LINES;
    extern VALUE SU_GL_LINE_STRIP;
    extern VALUE SU_GL_LINE_LOOP;
    extern VALUE SU_GL_TRIANGLES;
    extern VALUE SU_GL_TRIANGLE_STRIP;
    extern VALUE SU_GL_TRIANGLE_FAN;
    extern VALUE SU_GL_QUADS;
    extern VALUE SU_GL_QUAD_STRIP;
    extern VALUE SU_GL_POLYGON;

    extern ID INTERN_TO_A;
    extern ID INTERN_TO_I;
    extern ID INTERN_TO_F;
    extern ID INTERN_TO_S;
    extern ID INTERN_PUTS;
    extern ID INTERN_INSPECT;
    extern ID INTERN_BACKTRACE;
    extern ID INTERN_CALL;
    extern ID INTERN_PACK;
    extern ID INTERN_UNPACK;
    extern ID INTERN_KEY;
    extern ID INTERN_KEYS;
    extern ID INTERN_INDEX;
    extern ID INTERN_CLASS;
    extern ID INTERN_MESSAGE;

    extern ID INTERN_X;
    extern ID INTERN_Y;
    extern ID INTERN_Z;

    extern ID INTERN_XAXIS;
    extern ID INTERN_YAXIS;
    extern ID INTERN_ZAXIS;

    extern ID INTERN_RED;
    extern ID INTERN_SRED;
    extern ID INTERN_GREEN;
    extern ID INTERN_SGREEN;
    extern ID INTERN_BLUE;
    extern ID INTERN_SBLUE;
    extern ID INTERN_ALPHA;
    extern ID INTERN_SALPHA;

    extern ID INTERN_SDRAWING_COLOR;
    extern ID INTERN_SLINE_WIDTH;
    extern ID INTERN_SLINE_STIPPLE;
    extern ID INTERN_DRAW;
    extern ID INTERN_DRAW2D;
    extern ID INTERN_SCREEN_COORDS;
    extern ID INTERN_CORNER;
    extern ID INTERN_CAMERA;
    extern ID INTERN_ASPECT_RATIO;
    extern ID INTERN_SASPECT_RATIO;
    extern ID INTERN_FOCAL_LENGTH;
    extern ID INTERN_SFOCAL_LENGTH;
    extern ID INTERN_FOV;
    extern ID INTERN_SFOV;
    extern ID INTERN_IMAGE_WIDTH;
    extern ID INTERN_SIMAGE_WIDTH;
    extern ID INTERN_EYE;
    extern ID INTERN_DIRECTION;
    extern ID INTERN_UP;
    extern ID INTERN_HEIGHT;
    extern ID INTERN_SHEIGHT;
    extern ID INTERN_SPERSPECTIVE;
    extern ID INTERN_TPERSPECTIVE;
    extern ID INTERN_BOUNDS;
    extern ID INTERN_ADD;
    extern ID INTERN_CENTER;
    extern ID INTERN_VPWIDTH;
    extern ID INTERN_VPHEIGHT;
    extern ID INTERN_PICKRAY;
    extern ID INTERN_LENGTH;
    extern ID INTERN_SIZE;
    extern ID INTERN_AT;
    extern ID INTERN_ENTITYID;
    extern ID INTERN_START;
    extern ID INTERN_SSTART;
    extern ID INTERN_END;
    extern ID INTERN_SEND;
    extern ID INTERN_POSITION;
	extern ID INTERN_POSITION_MATERIAL;
    extern ID INTERN_MESH;
    extern ID INTERN_LOOPS;
    extern ID INTERN_POLYGONS;
    extern ID INTERN_POLYGON_POINTS_AT;
    extern ID INTERN_COUNT_POLYGONS;
    extern ID INTERN_COUNT_POINTS;
    extern ID INTERN_ADD_POINT;
    extern ID INTERN_POINTS;
    extern ID INTERN_POINT_AT;
    extern ID INTERN_POLYGON_AT;
	extern ID INTERN_UV_AT;
	extern ID INTERN_SET_UV;
	extern ID INTERN_POINT_INDEX;
    extern ID INTERN_RAYTEST;
    extern ID INTERN_NORMAL;
    extern ID INTERN_NAME;
    extern ID INTERN_SNAME;
    extern ID INTERN_LAYER;
    extern ID INTERN_SLAYER;
    extern ID INTERN_MATERIAL;
    extern ID INTERN_SMATERIAL;
    extern ID INTERN_BACK_MATERIAL;
    extern ID INTERN_SBACK_MATERIAL;
	extern ID INTERN_TEXTURE;
    extern ID INTERN_TRANSFORMATION;
    extern ID INTERN_STRANSFORMATION;
	extern ID INTERN_EMOVE;
    extern ID INTERN_TCASTS_SHADOWS;
    extern ID INTERN_SCASTS_SHADOWS;
    extern ID INTERN_TRECEIVES_SHADOWS;
    extern ID INTERN_SRECEIVES_SHADOWS;
    extern ID INTERN_TVISIBLE;
    extern ID INTERN_SVISIBLE;
    extern ID INTERN_TSMOOTH;
    extern ID INTERN_SSMOOTH;
    extern ID INTERN_TSOFT;
    extern ID INTERN_SSOFT;
    extern ID INTERN_STIPPLE;
    extern ID INTERN_SSTIPPLE;
    extern ID INTERN_EERASE;
    extern ID INTERN_ACTIVE_MODEL;
    extern ID INTERN_ACTIVE_VIEW;
    extern ID INTERN_IS_64BIT;
    extern ID INTERN_VERSION;
	extern ID INTERN_COMMON_EDGE;
	extern ID INTERN_COMMON_FACE;

    extern ID INTERN_ENTITIES;
    extern ID INTERN_ACTIVE_ENTITIES;
    extern ID INTERN_INSTANCES;
    extern ID INTERN_DEFINITIONS;
    extern ID INTERN_MATERIALS;
    extern ID INTERN_LAYERS;
    extern ID INTERN_STYLES;
    extern ID INTERN_PAGES;

    extern ID INTERN_DEFINITION;
    extern ID INTERN_PARENT;
    extern ID INTERN_MODEL;
    extern ID INTERN_TGROUP;
    extern ID INTERN_TVALID;
    extern ID INTERN_VERTICES;
    extern ID INTERN_TRANSFORM;
    extern ID INTERN_TRANSFORM_SELF;
	extern ID INTERN_TRANSFORM_ENTITIES;
	extern ID INTERN_TRANSFORM_BY_VECTORS;
    extern ID INTERN_ADD_POLYGON;
    extern ID INTERN_ALL_CONNECTED;
    extern ID INTERN_DISTANCE;
    extern ID INTERN_ADD_INSTANCE;
    extern ID INTERN_ADD_FACES_FROM_MESH;
    extern ID INTERN_FILL_FROM_MESH;
    extern ID INTERN_ADD_GROUP;
    extern ID INTERN_ADD_EDGES;
    extern ID INTERN_ADD_FACE;
    extern ID INTERN_ADD_CPOINT;
    extern ID INTERN_ADD_CLINE;
	extern ID INTERN_ADD_LINE;
    extern ID INTERN_CLOSE_ACTIVE;
    extern ID INTERN_THAS_LEADER;
    extern ID INTERN_EXPLODE;
    extern ID INTERN_ADD_NOTE;
    extern ID INTERN_SET_TEXT;
    extern ID INTERN_CLASSIFY_POINT;
    extern ID INTERN_FACES;
    extern ID INTERN_TPARALLEL;
    extern ID INTERN_PLANE;
    extern ID INTERN_TON_PLANE;
    extern ID INTERN_MIN;
    extern ID INTERN_MAX;
    extern ID INTERN_MAKE_UNIQUE;
    extern ID INTERN_COUNT;
    extern ID INTERN_EDGES;
    extern ID INTERN_INTERSECT_WITH;
	extern ID INTERN_INVALIDATE_BOUNDS;

    extern ID INTERN_OP_SQUARE_BRACKETS;
    extern ID INTERN_OP_ASTERISKS;
    extern ID INTERN_OP_ADD;
    extern ID INTERN_OP_SUBTRACT;

    extern int su_version;
    extern bool su_b64bit;

    // Functions
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

    VALUE to_value(const char* c_str);
    VALUE to_value(const char* c_str, unsigned int c_str_len);
    VALUE to_value(const wchar_t* wc_str);
    VALUE to_value(const wchar_t* wc_str, unsigned int wc_str_len);

    char* value_to_c_str(VALUE value);
    wchar_t* value_to_wc_str(VALUE value);


    inline bool value_to_bool(VALUE value) {
        return RTEST(value);
    }

    inline char value_to_char(VALUE value) {
        return static_cast<char>(NUM2CHR(value));
    }

    inline unsigned char value_to_uchar(VALUE value) {
        return static_cast<unsigned char>(NUM2CHR(value));
    }

    inline short value_to_short(VALUE value) {
        return static_cast<short>(NUM2INT(value));
    }

    inline unsigned short value_to_ushort(VALUE value) {
        return static_cast<unsigned short>(NUM2INT(value));
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
        return static_cast<float>(rb_num2dbl(rb_funcall(value, INTERN_TO_F, 0)));
    }

    inline double value_to_double(VALUE value) {
        return rb_num2dbl(rb_funcall(value, INTERN_TO_F, 0));
    }

    inline treal value_to_treal(VALUE value) {
        return static_cast<treal>(rb_num2dbl(rb_funcall(value, INTERN_TO_F, 0)));
    }

    VALUE vector_to_value(const Geom::Vector3d& vector);
    VALUE vector_to_value2(const Geom::Vector3d& vector, treal scale);
    VALUE point_to_value(const Geom::Vector3d& point);
    VALUE point_to_value2(const Geom::Vector3d& point, treal scale);
    VALUE transformation_to_value(const Geom::Transformation& transformation);
    VALUE transformation_to_value2(const Geom::Transformation& transformation, treal scale);
    VALUE color_to_value(const Geom::Color& color);
    VALUE bb_to_value(const Geom::BoundingBox& bb);
    VALUE bb_to_value2(const Geom::BoundingBox& bb, treal scale);

    void varry_to_vector(const VALUE* varry, Geom::Vector3d& vector_out);
    void varry_to_vector2(const VALUE* varry, Geom::Vector3d& vector_out, double scale);
    void value_to_vector(VALUE value, Geom::Vector3d& vector_out);
    void value_to_vector2(VALUE value, Geom::Vector3d& vector_out, double scale);
    void value_to_transformation(VALUE value, Geom::Transformation& tra_out);
    void value_to_transformation2(VALUE value, Geom::Transformation& tra_out, double scale);
    void value_to_transformation3(VALUE value, Geom::Transformation& tra_out);
    void value_to_bb(VALUE value, Geom::BoundingBox& bb_out);
    void value_to_bb2(VALUE value, Geom::BoundingBox& bb_out, double scale);
    void value_to_color(VALUE value, Geom::Color& color_out);

    VALUE call_proc(VALUE v_proc);
    VALUE rescue_proc(VALUE v_args, VALUE v_exception);
    VALUE array_delete_first(VALUE v_array, VALUE v_element);

    // Main
    void init_ruby();
}

#endif  /* RUBY_UTIL_H */

