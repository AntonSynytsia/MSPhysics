/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "ruby_util.h"


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE RU::SU_SKETCHUP;
VALUE RU::SU_GEOM;
VALUE RU::SU_CAMERA;
VALUE RU::SU_COLOR;
VALUE RU::SU_POINT3D;
VALUE RU::SU_VECTOR3D;
VALUE RU::SU_BOUNDING_BOX;
VALUE RU::SU_POLYGON_MESH;
VALUE RU::SU_TRANSFORMATION;
VALUE RU::SU_FACE;
VALUE RU::SU_EDGE;
VALUE RU::SU_COMPONENT_INSTANCE;
VALUE RU::SU_COMPONENT_DEFINITION;
VALUE RU::SU_GROUP;
VALUE RU::SU_ENTITIES;
VALUE RU::SU_ENTITY;
VALUE RU::SU_DRAWING_ELEMENT;
VALUE RU::SU_CONSTRUCTION_POINT;
VALUE RU::SU_CONSTRUCTION_LINE;
VALUE RU::SU_TEXT;
VALUE RU::SU_LANGUAGE_HANDLER;
VALUE RU::SU_MATERIAL;

VALUE RU::SU_FACE_POINT_UNKNOWN;
VALUE RU::SU_FACE_POINT_INSIDE;
VALUE RU::SU_FACE_POINT_ON_VERTEX;
VALUE RU::SU_FACE_POINT_ON_EDGE;
VALUE RU::SU_FACE_POINT_ON_FACE;
VALUE RU::SU_FACE_POINT_ON_OUTSIDE;
VALUE RU::SU_FACE_POINT_NOT_ON_PLANE;

VALUE RU::SU_GL_POINTS;
VALUE RU::SU_GL_LINES;
VALUE RU::SU_GL_LINE_STRIP;
VALUE RU::SU_GL_LINE_LOOP;
VALUE RU::SU_GL_TRIANGLES;
VALUE RU::SU_GL_TRIANGLE_STRIP;
VALUE RU::SU_GL_TRIANGLE_FAN;
VALUE RU::SU_GL_QUADS;
VALUE RU::SU_GL_QUAD_STRIP;
VALUE RU::SU_GL_POLYGON;

ID RU::INTERN_TO_A;
ID RU::INTERN_TO_I;
ID RU::INTERN_TO_F;
ID RU::INTERN_TO_S;
ID RU::INTERN_PUTS;
ID RU::INTERN_INSPECT;
ID RU::INTERN_BACKTRACE;
ID RU::INTERN_CALL;
ID RU::INTERN_PACK;
ID RU::INTERN_UNPACK;
ID RU::INTERN_KEY;
ID RU::INTERN_KEYS;
ID RU::INTERN_INDEX;
ID RU::INTERN_CLASS;
ID RU::INTERN_MESSAGE;

ID RU::INTERN_X;
ID RU::INTERN_Y;
ID RU::INTERN_Z;

ID RU::INTERN_XAXIS;
ID RU::INTERN_YAXIS;
ID RU::INTERN_ZAXIS;

ID RU::INTERN_RED;
ID RU::INTERN_SRED;
ID RU::INTERN_GREEN;
ID RU::INTERN_SGREEN;
ID RU::INTERN_BLUE;
ID RU::INTERN_SBLUE;
ID RU::INTERN_ALPHA;
ID RU::INTERN_SALPHA;

ID RU::INTERN_SDRAWING_COLOR;
ID RU::INTERN_SLINE_WIDTH;
ID RU::INTERN_SLINE_STIPPLE;
ID RU::INTERN_DRAW;
ID RU::INTERN_DRAW2D;
ID RU::INTERN_SCREEN_COORDS;
ID RU::INTERN_CORNER;
ID RU::INTERN_CAMERA;
ID RU::INTERN_ASPECT_RATIO;
ID RU::INTERN_SASPECT_RATIO;
ID RU::INTERN_FOCAL_LENGTH;
ID RU::INTERN_SFOCAL_LENGTH;
ID RU::INTERN_FOV;
ID RU::INTERN_SFOV;
ID RU::INTERN_IMAGE_WIDTH;
ID RU::INTERN_SIMAGE_WIDTH;
ID RU::INTERN_EYE;
ID RU::INTERN_DIRECTION;
ID RU::INTERN_UP;
ID RU::INTERN_HEIGHT;
ID RU::INTERN_SHEIGHT;
ID RU::INTERN_SPERSPECTIVE;
ID RU::INTERN_TPERSPECTIVE;
ID RU::INTERN_BOUNDS;
ID RU::INTERN_ADD;
ID RU::INTERN_CENTER;
ID RU::INTERN_VPWIDTH;
ID RU::INTERN_VPHEIGHT;
ID RU::INTERN_PICKRAY;
ID RU::INTERN_LENGTH;
ID RU::INTERN_SIZE;
ID RU::INTERN_AT;
ID RU::INTERN_ENTITYID;
ID RU::INTERN_START;
ID RU::INTERN_SSTART;
ID RU::INTERN_END;
ID RU::INTERN_SEND;
ID RU::INTERN_POSITION;
ID RU::INTERN_POSITION_MATERIAL;
ID RU::INTERN_MESH;
ID RU::INTERN_LOOPS;
ID RU::INTERN_POLYGONS;
ID RU::INTERN_POLYGON_POINTS_AT;
ID RU::INTERN_COUNT_POLYGONS;
ID RU::INTERN_COUNT_POINTS;
ID RU::INTERN_ADD_POINT;
ID RU::INTERN_POINTS;
ID RU::INTERN_POINT_AT;
ID RU::INTERN_POLYGON_AT;
ID RU::INTERN_UV_AT;
ID RU::INTERN_SET_UV;
ID RU::INTERN_POINT_INDEX;
ID RU::INTERN_RAYTEST;
ID RU::INTERN_NORMAL;
ID RU::INTERN_NAME;
ID RU::INTERN_SNAME;
ID RU::INTERN_LAYER;
ID RU::INTERN_SLAYER;
ID RU::INTERN_MATERIAL;
ID RU::INTERN_SMATERIAL;
ID RU::INTERN_BACK_MATERIAL;
ID RU::INTERN_SBACK_MATERIAL;
ID RU::INTERN_TEXTURE;
ID RU::INTERN_TRANSFORMATION;
ID RU::INTERN_STRANSFORMATION;
ID RU::INTERN_EMOVE;
ID RU::INTERN_TCASTS_SHADOWS;
ID RU::INTERN_SCASTS_SHADOWS;
ID RU::INTERN_TRECEIVES_SHADOWS;
ID RU::INTERN_SRECEIVES_SHADOWS;
ID RU::INTERN_TVISIBLE;
ID RU::INTERN_SVISIBLE;
ID RU::INTERN_TSMOOTH;
ID RU::INTERN_SSMOOTH;
ID RU::INTERN_TSOFT;
ID RU::INTERN_SSOFT;
ID RU::INTERN_STIPPLE;
ID RU::INTERN_SSTIPPLE;
ID RU::INTERN_EERASE;
ID RU::INTERN_ACTIVE_MODEL;
ID RU::INTERN_ACTIVE_VIEW;
ID RU::INTERN_IS_64BIT;
ID RU::INTERN_VERSION;
ID RU::INTERN_COMMON_EDGE;
ID RU::INTERN_COMMON_FACE;

ID RU::INTERN_ENTITIES;
ID RU::INTERN_ACTIVE_ENTITIES;
ID RU::INTERN_INSTANCES;
ID RU::INTERN_DEFINITIONS;
ID RU::INTERN_MATERIALS;
ID RU::INTERN_LAYERS;
ID RU::INTERN_STYLES;
ID RU::INTERN_PAGES;

ID RU::INTERN_DEFINITION;
ID RU::INTERN_PARENT;
ID RU::INTERN_MODEL;
ID RU::INTERN_TGROUP;
ID RU::INTERN_TVALID;
ID RU::INTERN_VERTICES;
ID RU::INTERN_TRANSFORM;
ID RU::INTERN_TRANSFORM_SELF;
ID RU::INTERN_TRANSFORM_ENTITIES;
ID RU::INTERN_TRANSFORM_BY_VECTORS;
ID RU::INTERN_ADD_POLYGON;
ID RU::INTERN_ALL_CONNECTED;
ID RU::INTERN_DISTANCE;
ID RU::INTERN_ADD_INSTANCE;
ID RU::INTERN_ADD_FACES_FROM_MESH;
ID RU::INTERN_FILL_FROM_MESH;
ID RU::INTERN_ADD_GROUP;
ID RU::INTERN_ADD_EDGES;
ID RU::INTERN_ADD_FACE;
ID RU::INTERN_ADD_CPOINT;
ID RU::INTERN_ADD_CLINE;
ID RU::INTERN_ADD_LINE;
ID RU::INTERN_CLOSE_ACTIVE;
ID RU::INTERN_THAS_LEADER;
ID RU::INTERN_EXPLODE;
ID RU::INTERN_ADD_NOTE;
ID RU::INTERN_SET_TEXT;
ID RU::INTERN_CLASSIFY_POINT;
ID RU::INTERN_FACES;
ID RU::INTERN_TPARALLEL;
ID RU::INTERN_PLANE;
ID RU::INTERN_TON_PLANE;
ID RU::INTERN_MIN;
ID RU::INTERN_MAX;
ID RU::INTERN_MAKE_UNIQUE;
ID RU::INTERN_COUNT;
ID RU::INTERN_EDGES;
ID RU::INTERN_INTERSECT_WITH;
ID RU::INTERN_INVALIDATE_BOUNDS;

ID RU::INTERN_OP_SQUARE_BRACKETS;
ID RU::INTERN_OP_ASTERISKS;
ID RU::INTERN_OP_ADD;
ID RU::INTERN_OP_SUBTRACT;

int RU::su_version;
bool RU::su_b64bit;


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE RU::to_value(const char* c_str) {
    VALUE v_str = rb_str_new_cstr(c_str);
#ifdef HAVE_RUBY_ENCODING_H
    rb_enc_associate_index(v_str, rb_utf8_encindex());
#endif
    return v_str;
}

VALUE RU::to_value(const char* c_str, unsigned int c_str_len) {
    VALUE v_str = rb_str_new(c_str, c_str_len);
#ifdef HAVE_RUBY_ENCODING_H
    rb_enc_associate_index(v_str, rb_utf8_encindex());
#endif
    return v_str;
}

VALUE RU::to_value(const wchar_t* wc_str) {
#ifdef HAVE_RUBY_ENCODING_H
    unsigned int wc_str_len = static_cast<unsigned int>(wcslen(wc_str));
    unsigned int c_len = 0;
    unsigned int i, j, k;
    VALUE v_wchr, v_str;
    rb_encoding* enc = rb_utf8_encoding();
    for (i = 0; i < wc_str_len; ++i) {
        v_wchr = rb_enc_uint_chr(wc_str[i], enc);
        c_len += static_cast<unsigned int>(RSTRING_LEN(v_wchr));
    }
	char* c_str = reinterpret_cast<char*>(malloc(sizeof(char) * c_len));
    j = 0;
    for (i = 0; i < wc_str_len; ++i) {
        v_wchr = rb_enc_uint_chr(wc_str[i], enc);
        k = static_cast<unsigned int>(RSTRING_LEN(v_wchr));
        memcpy(c_str + j, RSTRING_PTR(v_wchr), k);
        j += k;
    }
    v_str = rb_str_new(c_str, c_len);
    rb_enc_associate_index(v_str, rb_utf8_encindex());
    free(c_str);
    return v_str;
#else
    unsigned int wc_str_len = static_cast<unsigned int>(wcslen(wc_str));
	char* c_str = reinterpret_cast<char*>(malloc(sizeof(char) * wc_str_len));
    unsigned int i;
    for (i = 0; i < wc_str_len; ++i)
        c_str[i] = static_cast<char>(wc_str[i]);
    VALUE v_str = rb_str_new(c_str, wc_str_len);
    free(c_str);
    return v_str;
#endif
}

VALUE RU::to_value(const wchar_t* wc_str, unsigned int wc_str_len) {
#ifdef HAVE_RUBY_ENCODING_H
    unsigned int c_len = 0;
    unsigned int i, j, k;
    VALUE v_wchr, v_str;
    rb_encoding* enc = rb_utf8_encoding();
    for (i = 0; i < wc_str_len; ++i) {
        v_wchr = rb_enc_uint_chr(wc_str[i], enc);
        c_len += static_cast<unsigned int>(RSTRING_LEN(v_wchr));
    }
	char* c_str = reinterpret_cast<char*>(malloc(sizeof(char) * c_len));
    j = 0;
    for (i = 0; i < wc_str_len; ++i) {
        v_wchr = rb_enc_uint_chr(wc_str[i], enc);
        k = static_cast<unsigned int>(RSTRING_LEN(v_wchr));
        memcpy(c_str + j, RSTRING_PTR(v_wchr), k);
        j += k;
    }
    v_str = rb_str_new(c_str, c_len);
    rb_enc_associate_index(v_str, rb_utf8_encindex());
	free(c_str);
    return v_str;
#else
	char* c_str = reinterpret_cast<char*>(malloc(sizeof(char) * wc_str_len));
    unsigned int i;
    for (i = 0; i < wc_str_len; ++i)
        c_str[i] = static_cast<char>(wc_str[i]);
    VALUE v_str = rb_str_new(c_str, wc_str_len);
	free(c_str);
    return v_str;
#endif
}

char* RU::value_to_c_str(VALUE value) {
    return StringValuePtr(value);
}

// Note: the returned string must be freed after use
wchar_t* RU::value_to_wc_str(VALUE value) {
#ifdef HAVE_RUBY_ENCODING_H
    VALUE v_str = StringValue(value);
    rb_encoding* enc = rb_enc_get(v_str);
    char* begin = RSTRING_PTR(v_str);
    char* end = RSTRING_END(v_str);
    unsigned int wc_str_len = static_cast<unsigned int>(rb_enc_strlen(begin, end, enc));
    wchar_t* res_str = new wchar_t[wc_str_len + 1];
    unsigned int i;
    char* last = begin;
    char* next;
    for (i = 0; i < wc_str_len; ++i) {
        next = rb_enc_nth(begin, end, i + 1, enc);
        res_str[i] = static_cast<wchar_t>(rb_enc_codepoint(last, next, enc));
        last = next;
    }
    res_str[wc_str_len] = 0;
    return res_str;
#else
    VALUE v_str = StringValue(value);
    char* c_str = RSTRING_PTR(v_str);
    unsigned int c_len = static_cast<unsigned int>(RSTRING_LEN(v_str));
    wchar_t* res_str = new wchar_t[c_len + 1];
    unsigned int i;
    for (i = 0; i < c_len; ++i)
        res_str[i] = c_str[i];
    res_str[c_len] = 0;
    return res_str;
#endif
}

VALUE RU::vector_to_value(const Geom::Vector3d& vector) {
    VALUE argv[3];
    argv[0] = rb_float_new(vector.m_x);
    argv[1] = rb_float_new(vector.m_y);
    argv[2] = rb_float_new(vector.m_z);
    return rb_class_new_instance(3, argv, SU_VECTOR3D);
}

VALUE RU::vector_to_value2(const Geom::Vector3d& vector, treal scale) {
    VALUE argv[3];
    argv[0] = rb_float_new(vector.m_x * scale);
    argv[1] = rb_float_new(vector.m_y * scale);
    argv[2] = rb_float_new(vector.m_z * scale);
    return rb_class_new_instance(3, argv, SU_VECTOR3D);
}

VALUE RU::point_to_value(const Geom::Vector3d& point) {
    VALUE argv[3];
    argv[0] = rb_float_new(point.m_x);
    argv[1] = rb_float_new(point.m_y);
    argv[2] = rb_float_new(point.m_z);
    return rb_class_new_instance(3, argv, SU_POINT3D);
}

VALUE RU::point_to_value2(const Geom::Vector3d& point, treal scale) {
    VALUE argv[3];
    argv[0] = rb_float_new(point.m_x * scale);
    argv[1] = rb_float_new(point.m_y * scale);
    argv[2] = rb_float_new(point.m_z * scale);
    return rb_class_new_instance(3, argv, SU_POINT3D);
}

VALUE RU::transformation_to_value(const Geom::Transformation& transformation) {
    VALUE v_matrix = rb_ary_new2(16);

    rb_ary_store(v_matrix, 0, rb_float_new(transformation.m_xaxis.m_x));
    rb_ary_store(v_matrix, 1, rb_float_new(transformation.m_xaxis.m_y));
    rb_ary_store(v_matrix, 2, rb_float_new(transformation.m_xaxis.m_z));
    rb_ary_store(v_matrix, 3, rb_float_new(transformation.m_xaxis.m_w));

    rb_ary_store(v_matrix, 4, rb_float_new(transformation.m_yaxis.m_x));
    rb_ary_store(v_matrix, 5, rb_float_new(transformation.m_yaxis.m_y));
    rb_ary_store(v_matrix, 6, rb_float_new(transformation.m_yaxis.m_z));
    rb_ary_store(v_matrix, 7, rb_float_new(transformation.m_yaxis.m_w));

    rb_ary_store(v_matrix, 8, rb_float_new(transformation.m_zaxis.m_x));
    rb_ary_store(v_matrix, 9, rb_float_new(transformation.m_zaxis.m_y));
    rb_ary_store(v_matrix, 10, rb_float_new(transformation.m_zaxis.m_z));
    rb_ary_store(v_matrix, 11, rb_float_new(transformation.m_zaxis.m_w));

    rb_ary_store(v_matrix, 12, rb_float_new(transformation.m_origin.m_x));
    rb_ary_store(v_matrix, 13, rb_float_new(transformation.m_origin.m_y));
    rb_ary_store(v_matrix, 14, rb_float_new(transformation.m_origin.m_z));
    rb_ary_store(v_matrix, 15, rb_float_new(transformation.m_origin.m_w));

    return rb_class_new_instance(1, &v_matrix, SU_TRANSFORMATION);
}

VALUE RU::transformation_to_value2(const Geom::Transformation& transformation, treal scale) {
    VALUE v_matrix = rb_ary_new2(16);

    rb_ary_store(v_matrix, 0, rb_float_new(transformation.m_xaxis.m_x));
    rb_ary_store(v_matrix, 1, rb_float_new(transformation.m_xaxis.m_y));
    rb_ary_store(v_matrix, 2, rb_float_new(transformation.m_xaxis.m_z));
    rb_ary_store(v_matrix, 3, rb_float_new(transformation.m_xaxis.m_w));

    rb_ary_store(v_matrix, 4, rb_float_new(transformation.m_yaxis.m_x));
    rb_ary_store(v_matrix, 5, rb_float_new(transformation.m_yaxis.m_y));
    rb_ary_store(v_matrix, 6, rb_float_new(transformation.m_yaxis.m_z));
    rb_ary_store(v_matrix, 7, rb_float_new(transformation.m_yaxis.m_w));

    rb_ary_store(v_matrix, 8, rb_float_new(transformation.m_zaxis.m_x));
    rb_ary_store(v_matrix, 9, rb_float_new(transformation.m_zaxis.m_y));
    rb_ary_store(v_matrix, 10, rb_float_new(transformation.m_zaxis.m_z));
    rb_ary_store(v_matrix, 11, rb_float_new(transformation.m_zaxis.m_w));

    rb_ary_store(v_matrix, 12, rb_float_new(transformation.m_origin.m_x * scale));
    rb_ary_store(v_matrix, 13, rb_float_new(transformation.m_origin.m_y * scale));
    rb_ary_store(v_matrix, 14, rb_float_new(transformation.m_origin.m_z * scale));
    rb_ary_store(v_matrix, 15, rb_float_new(transformation.m_origin.m_w));

    return rb_class_new_instance(1, &v_matrix, SU_TRANSFORMATION);
}

VALUE RU::color_to_value(const Geom::Color& color) {
    VALUE argv[4];
    argv[0] = rb_int2inum(color.m_r);
    argv[1] = rb_int2inum(color.m_g);
    argv[2] = rb_int2inum(color.m_b);
    argv[3] = rb_int2inum(color.m_a);

    return rb_class_new_instance(4, argv, SU_COLOR);
}

VALUE RU::bb_to_value(const Geom::BoundingBox& bb) {
    VALUE v_bb = rb_class_new_instance(0, nullptr, SU_BOUNDING_BOX);
    if (bb.is_valid()) {
        rb_funcall(v_bb, INTERN_ADD, 1, point_to_value(bb.m_min));
        rb_funcall(v_bb, INTERN_ADD, 1, point_to_value(bb.m_max));
    }
    return v_bb;
}

VALUE RU::bb_to_value2(const Geom::BoundingBox& bb, treal scale) {
    VALUE v_bb = rb_class_new_instance(0, nullptr, SU_BOUNDING_BOX);
    if (bb.is_valid()) {
        rb_funcall(v_bb, INTERN_ADD, 1, point_to_value2(bb.m_min, scale));
        rb_funcall(v_bb, INTERN_ADD, 1, point_to_value2(bb.m_max, scale));
    }
    return v_bb;
}

void RU::varry_to_vector(const VALUE* varry, Geom::Vector3d& vector_out) {
    vector_out.m_x = static_cast<treal>(rb_num2dbl(varry[0]));
    vector_out.m_y = static_cast<treal>(rb_num2dbl(varry[1]));
    vector_out.m_z = static_cast<treal>(rb_num2dbl(varry[2]));
}

void RU::varry_to_vector2(const VALUE* varry, Geom::Vector3d& vector_out, double scale) {
    vector_out.m_x = static_cast<treal>(rb_num2dbl(varry[0]) * scale);
    vector_out.m_y = static_cast<treal>(rb_num2dbl(varry[1]) * scale);
    vector_out.m_z = static_cast<treal>(rb_num2dbl(varry[2]) * scale);
}

void RU::value_to_vector(VALUE value, Geom::Vector3d& vector_out) {
    vector_out.m_x = static_cast<treal>(rb_num2dbl(rb_funcall(value, INTERN_X, 0)));
    vector_out.m_y = static_cast<treal>(rb_num2dbl(rb_funcall(value, INTERN_Y, 0)));
    vector_out.m_z = static_cast<treal>(rb_num2dbl(rb_funcall(value, INTERN_Z, 0)));
}

void RU::value_to_vector2(VALUE value, Geom::Vector3d& vector_out, double scale) {
    vector_out.m_x = static_cast<treal>(rb_num2dbl(rb_funcall(value, INTERN_X, 0)) * scale);
    vector_out.m_y = static_cast<treal>(rb_num2dbl(rb_funcall(value, INTERN_Y, 0)) * scale);
    vector_out.m_z = static_cast<treal>(rb_num2dbl(rb_funcall(value, INTERN_Z, 0)) * scale);
}

void RU::value_to_transformation(VALUE value, Geom::Transformation& tra_out) {
    if (rb_obj_is_kind_of(value, SU_TRANSFORMATION) == Qfalse)
        value = rb_class_new_instance(1, &value, SU_TRANSFORMATION);
    VALUE v_matrix = rb_funcall(value, INTERN_TO_A, 0);

    tra_out.m_xaxis.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 0)));
    tra_out.m_xaxis.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 1)));
    tra_out.m_xaxis.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 2)));
    tra_out.m_xaxis.m_w = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 3)));

    tra_out.m_yaxis.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 4)));
    tra_out.m_yaxis.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 5)));
    tra_out.m_yaxis.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 6)));
    tra_out.m_yaxis.m_w = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 7)));

    tra_out.m_zaxis.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 8)));
    tra_out.m_zaxis.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 9)));
    tra_out.m_zaxis.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 10)));
    tra_out.m_zaxis.m_w = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 11)));

    tra_out.m_origin.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 12)));
    tra_out.m_origin.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 13)));
    tra_out.m_origin.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 14)));
    tra_out.m_origin.m_w = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 15)));
}

void RU::value_to_transformation2(VALUE value, Geom::Transformation& tra_out, double scale) {
    if (rb_obj_is_kind_of(value, SU_TRANSFORMATION) == Qfalse)
        value = rb_class_new_instance(1, &value, SU_TRANSFORMATION);
    VALUE v_matrix = rb_funcall(value, INTERN_TO_A, 0);

    tra_out.m_xaxis.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 0)));
    tra_out.m_xaxis.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 1)));
    tra_out.m_xaxis.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 2)));
    tra_out.m_xaxis.m_w = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 3)));

    tra_out.m_yaxis.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 4)));
    tra_out.m_yaxis.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 5)));
    tra_out.m_yaxis.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 6)));
    tra_out.m_yaxis.m_w = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 7)));

    tra_out.m_zaxis.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 8)));
    tra_out.m_zaxis.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 9)));
    tra_out.m_zaxis.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 10)));
    tra_out.m_zaxis.m_w = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 11)));

    tra_out.m_origin.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 12)) * scale);
    tra_out.m_origin.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 13)) * scale);
    tra_out.m_origin.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 14)) * scale);
    tra_out.m_origin.m_w = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 15)));
}

void RU::value_to_transformation3(VALUE value, Geom::Transformation& tra_out) {
    if (rb_obj_is_kind_of(value, SU_TRANSFORMATION) == Qfalse)
        value = rb_class_new_instance(1, &value, SU_TRANSFORMATION);
    VALUE v_matrix = rb_funcall(value, INTERN_TO_A, 0);

    Geom::Transformation ttra;

    ttra.m_xaxis.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 0)));
    ttra.m_xaxis.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 1)));
    ttra.m_xaxis.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 2)));
    ttra.m_xaxis.m_w = (treal)(0.0);

    ttra.m_yaxis.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 4)));
    ttra.m_yaxis.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 5)));
    ttra.m_yaxis.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 6)));
    ttra.m_yaxis.m_w = (treal)(0.0);

    ttra.m_zaxis.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 8)));
    ttra.m_zaxis.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 9)));
    ttra.m_zaxis.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 10)));
    ttra.m_zaxis.m_w = (treal)(0.0);

    ttra.m_origin.m_x = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 12)));
    ttra.m_origin.m_y = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 13)));
    ttra.m_origin.m_z = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 14)));
    ttra.m_origin.m_w = (treal)(1.0);

    // Extract global scale
    treal gs = static_cast<treal>(rb_num2dbl(rb_ary_entry(v_matrix, 15)));
    if (fabs(gs) > M_EPSILON) {
        treal inv_wscale = (treal)(1.0) / gs;
        ttra.m_xaxis.scale_self(inv_wscale);
        ttra.m_yaxis.scale_self(inv_wscale);
        ttra.m_zaxis.scale_self(inv_wscale);
        ttra.m_origin.scale_self(inv_wscale);
    }

    // Validate
    if (!ttra.is_uniform())
        rb_raise(rb_eTypeError, "Non-uniform transformations are not allowed!");
    if (ttra.is_flat())
        rb_raise(rb_eTypeError, "Transformations with zero-lengthed axes are not allowed!");

    // Overwrite
    tra_out = ttra;
}

void RU::value_to_bb(VALUE value, Geom::BoundingBox& bb_out) {
    if (rb_obj_is_kind_of(value, SU_BOUNDING_BOX) == Qfalse) {
        VALUE cname = rb_class_name(SU_BOUNDING_BOX);
        rb_raise(rb_eTypeError, "Expected %s", RSTRING_PTR(cname));
    }
    value_to_vector(rb_funcall(value, INTERN_MIN, 0), bb_out.m_min);
    value_to_vector(rb_funcall(value, INTERN_MAX, 0), bb_out.m_max);
}

void RU::value_to_bb2(VALUE value, Geom::BoundingBox& bb_out, double scale) {
    if (rb_obj_is_kind_of(value, SU_BOUNDING_BOX) == Qfalse) {
        VALUE cname = rb_class_name(SU_BOUNDING_BOX);
        rb_raise(rb_eTypeError, "Expected %s", RSTRING_PTR(cname));
    }
    value_to_vector2(rb_funcall(value, INTERN_MIN, 0), bb_out.m_min, scale);
    value_to_vector2(rb_funcall(value, INTERN_MAX, 0), bb_out.m_max, scale);
}

void RU::value_to_color(VALUE value, Geom::Color& color_out) {
    if (rb_obj_is_kind_of(value, SU_COLOR) == Qfalse)
        value = rb_class_new_instance(1, &value, SU_COLOR);
    color_out.m_r = static_cast<unsigned char>(NUM2INT(rb_funcall(value, INTERN_RED, 0)));
    color_out.m_g = static_cast<unsigned char>(NUM2INT(rb_funcall(value, INTERN_GREEN, 0)));
    color_out.m_b = static_cast<unsigned char>(NUM2INT(rb_funcall(value, INTERN_BLUE, 0)));
    color_out.m_a = static_cast<unsigned char>(NUM2INT(rb_funcall(value, INTERN_ALPHA, 0)));
}

VALUE RU::call_proc(VALUE v_proc) {
    if (rb_obj_is_kind_of(v_proc, rb_cProc) == Qfalse)
        rb_raise(rb_eTypeError, "Expected a Proc object!");
    return rb_funcall(v_proc, INTERN_CALL, 0);
}

VALUE RU::rescue_proc(VALUE v_args, VALUE v_exception) {
    VALUE v_message(rb_funcall(v_exception, RU::INTERN_INSPECT, 0));
    rb_funcall(rb_stdout, INTERN_PUTS, 1, v_message);
    VALUE v_backtrace(rb_funcall(v_exception, INTERN_BACKTRACE, 0));
    rb_funcall(rb_stdout, INTERN_PUTS, 1, rb_ary_entry(v_backtrace, 0));
    return Qnil;
}

VALUE RU::array_delete_first(VALUE v_array, VALUE v_element) {
    if (rb_obj_is_kind_of(v_array, rb_cArray) == Qfalse)
        rb_raise(rb_eTypeError, "Expected an Array object!");
    unsigned int size = static_cast<unsigned int>(RARRAY_LEN(v_array));
    for (unsigned int i = 0; i < size; ++i) {
        if (rb_ary_entry(v_array, i) == v_element) {
            rb_ary_delete_at(v_array, i);
            return RU::to_value(i);
        }
    }
    return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void RU::init_ruby() {
    SU_SKETCHUP = rb_const_get(rb_cObject, rb_intern("Sketchup"));
    SU_COLOR = rb_const_get_at(SU_SKETCHUP, rb_intern("Color"));
    SU_GROUP = rb_const_get_at(SU_SKETCHUP, rb_intern("Group"));
	SU_ENTITIES = rb_const_get_at(SU_SKETCHUP, rb_intern("Entities"));
	SU_ENTITY = rb_const_get_at(SU_SKETCHUP, rb_intern("Entity"));
	SU_DRAWING_ELEMENT = rb_const_get_at(SU_SKETCHUP, rb_intern("Drawingelement"));
    SU_COMPONENT_INSTANCE = rb_const_get_at(SU_SKETCHUP, rb_intern("ComponentInstance"));
    SU_COMPONENT_DEFINITION = rb_const_get_at(SU_SKETCHUP, rb_intern("ComponentDefinition"));
    SU_CAMERA = rb_const_get_at(SU_SKETCHUP, rb_intern("Camera"));
    SU_TEXT = rb_const_get_at(SU_SKETCHUP, rb_intern("Text"));
    SU_FACE = rb_const_get_at(SU_SKETCHUP, rb_intern("Face"));
    SU_EDGE = rb_const_get_at(SU_SKETCHUP, rb_intern("Edge"));
    SU_CONSTRUCTION_POINT = rb_const_get_at(SU_SKETCHUP, rb_intern("ConstructionPoint"));
    SU_CONSTRUCTION_LINE = rb_const_get_at(SU_SKETCHUP, rb_intern("ConstructionLine"));
	SU_MATERIAL = rb_const_get_at(SU_SKETCHUP, rb_intern("Material"));

    SU_LANGUAGE_HANDLER = rb_const_get(rb_cObject, rb_intern("LanguageHandler"));

    SU_GEOM = rb_const_get(rb_cObject, rb_intern("Geom"));
    SU_POINT3D = rb_const_get_at(SU_GEOM, rb_intern("Point3d"));
    SU_VECTOR3D = rb_const_get_at(SU_GEOM, rb_intern("Vector3d"));
    SU_TRANSFORMATION = rb_const_get_at(SU_GEOM, rb_intern("Transformation"));
    SU_BOUNDING_BOX = rb_const_get_at(SU_GEOM, rb_intern("BoundingBox"));
    SU_POLYGON_MESH = rb_const_get_at(SU_GEOM, rb_intern("PolygonMesh"));

    INTERN_TO_A = rb_intern("to_a");
    INTERN_TO_I = rb_intern("to_i");
    INTERN_TO_F = rb_intern("to_f");
    INTERN_TO_S = rb_intern("to_s");
    INTERN_PUTS = rb_intern("puts");
    INTERN_INSPECT = rb_intern("inspect");
    INTERN_BACKTRACE = rb_intern("backtrace");
    INTERN_CALL = rb_intern("call");
    INTERN_PACK = rb_intern("pack");
    INTERN_UNPACK = rb_intern("unpack");
    INTERN_KEY = rb_intern("key");
    INTERN_KEYS = rb_intern("keys");
    INTERN_INDEX = rb_intern("index");
    INTERN_CLASS = rb_intern("class");
    INTERN_MESSAGE = rb_intern("message");

    INTERN_X = rb_intern("x");
    INTERN_Y = rb_intern("y");
    INTERN_Z = rb_intern("z");

    INTERN_XAXIS = rb_intern("xaxis");
    INTERN_YAXIS = rb_intern("yaxis");
    INTERN_ZAXIS = rb_intern("zaxis");

    INTERN_RED = rb_intern("red");
    INTERN_SRED = rb_intern("red=");
    INTERN_GREEN = rb_intern("green");
    INTERN_SGREEN = rb_intern("green=");
    INTERN_BLUE = rb_intern("blue");
    INTERN_SBLUE = rb_intern("blue=");
    INTERN_ALPHA = rb_intern("alpha");
    INTERN_SALPHA = rb_intern("alpha=");

    INTERN_SDRAWING_COLOR = rb_intern("drawing_color=");
    INTERN_SLINE_WIDTH = rb_intern("line_width=");
    INTERN_SLINE_STIPPLE = rb_intern("line_stipple=");
    INTERN_DRAW = rb_intern("draw");
    INTERN_DRAW2D = rb_intern("draw2d");
    INTERN_SCREEN_COORDS = rb_intern("screen_coords");
    INTERN_CORNER = rb_intern("corner");
    INTERN_CAMERA = rb_intern("camera");
    INTERN_ASPECT_RATIO = rb_intern("aspect_ratio");
    INTERN_SASPECT_RATIO = rb_intern("aspect_ratio=");
    INTERN_FOCAL_LENGTH = rb_intern("focal_length");
    INTERN_SFOCAL_LENGTH = rb_intern("focal_length=");
    INTERN_FOV = rb_intern("fov");
    INTERN_SFOV = rb_intern("fov=");
    INTERN_IMAGE_WIDTH = rb_intern("image_width");
    INTERN_SIMAGE_WIDTH = rb_intern("image_width=");
    INTERN_EYE = rb_intern("eye");
    INTERN_DIRECTION = rb_intern("direction");
    INTERN_UP = rb_intern("up");
    INTERN_HEIGHT = rb_intern("height");
    INTERN_SHEIGHT = rb_intern("height=");
    INTERN_SPERSPECTIVE = rb_intern("perspective=");
    INTERN_TPERSPECTIVE = rb_intern("perspective?");
    INTERN_BOUNDS = rb_intern("bounds");
    INTERN_ADD = rb_intern("add");
    INTERN_CENTER = rb_intern("center");
    INTERN_VPWIDTH = rb_intern("vpwidth");
    INTERN_VPHEIGHT = rb_intern("vpheight");
    INTERN_PICKRAY = rb_intern("pickray");
    INTERN_LENGTH = rb_intern("length");
    INTERN_SIZE = rb_intern("size");
    INTERN_AT = rb_intern("at");
    INTERN_ENTITYID = rb_intern("entityID");
    INTERN_START = rb_intern("start");
    INTERN_SSTART = rb_intern("start=");
    INTERN_END = rb_intern("end");
    INTERN_SEND = rb_intern("end=");
    INTERN_POSITION = rb_intern("position");
	INTERN_POSITION_MATERIAL = rb_intern("position_material");
    INTERN_MESH = rb_intern("mesh");
    INTERN_LOOPS = rb_intern("loops");
    INTERN_POLYGONS = rb_intern("polygons");
    INTERN_POLYGON_POINTS_AT = rb_intern("polygon_points_at");
    INTERN_COUNT_POLYGONS = rb_intern("count_polygons");
    INTERN_COUNT_POINTS = rb_intern("count_points");
    INTERN_ADD_POINT = rb_intern("add_point");
    INTERN_POINTS = rb_intern("points");
    INTERN_POINT_AT = rb_intern("point_at");
    INTERN_POLYGON_AT = rb_intern("polygon_at");
	INTERN_UV_AT = rb_intern("uv_at");
	INTERN_SET_UV = rb_intern("set_uv");
	INTERN_POINT_INDEX = rb_intern("point_index");
    INTERN_RAYTEST = rb_intern("raytest");
    INTERN_NORMAL = rb_intern("normal");
    INTERN_NAME = rb_intern("name");
    INTERN_SNAME = rb_intern("name=");
    INTERN_LAYER = rb_intern("layer");
    INTERN_SLAYER = rb_intern("layer=");
    INTERN_MATERIAL = rb_intern("material");
    INTERN_SMATERIAL = rb_intern("material=");
    INTERN_BACK_MATERIAL = rb_intern("back_material");
    INTERN_SBACK_MATERIAL = rb_intern("back_material=");
	INTERN_TEXTURE = rb_intern("texture");
    INTERN_TRANSFORMATION = rb_intern("transformation");
    INTERN_STRANSFORMATION = rb_intern("transformation=");
	INTERN_EMOVE = rb_intern("move!");
    INTERN_TCASTS_SHADOWS = rb_intern("casts_shadows?");
    INTERN_SCASTS_SHADOWS = rb_intern("casts_shadows=");
    INTERN_TRECEIVES_SHADOWS = rb_intern("receives_shadows?");
    INTERN_SRECEIVES_SHADOWS = rb_intern("receives_shadows=");
    INTERN_TVISIBLE = rb_intern("visible?");
    INTERN_SVISIBLE = rb_intern("visible=");
    INTERN_TSMOOTH = rb_intern("smooth?");
    INTERN_SSMOOTH = rb_intern("smooth=");
    INTERN_TSOFT = rb_intern("soft?");
    INTERN_SSOFT = rb_intern("soft=");
    INTERN_STIPPLE = rb_intern("stipple");
    INTERN_SSTIPPLE = rb_intern("stipple=");
    INTERN_EERASE = rb_intern("erase!");
    INTERN_ACTIVE_MODEL = rb_intern("active_model");
    INTERN_ACTIVE_VIEW = rb_intern("active_view");
    INTERN_IS_64BIT = rb_intern("is_64bit?");
    INTERN_VERSION = rb_intern("version");
	INTERN_COMMON_EDGE = rb_intern("common_edge");
	INTERN_COMMON_FACE = rb_intern("common_face");

    INTERN_ENTITIES = rb_intern("entities");
    INTERN_ACTIVE_ENTITIES = rb_intern("active_entities");
    INTERN_INSTANCES = rb_intern("instances");
    INTERN_DEFINITIONS = rb_intern("definitions");
    INTERN_MATERIALS = rb_intern("materials");
    INTERN_LAYERS = rb_intern("layers");
    INTERN_STYLES = rb_intern("styles");
    INTERN_PAGES = rb_intern("pages");

    INTERN_DEFINITION = rb_intern("definition");
    INTERN_PARENT = rb_intern("parent");
    INTERN_MODEL = rb_intern("model");
    INTERN_TGROUP = rb_intern("group?");
    INTERN_TVALID = rb_intern("valid?");
    INTERN_VERTICES = rb_intern("vertices");
    INTERN_TRANSFORM = rb_intern("transform");
    INTERN_TRANSFORM_SELF = rb_intern("transform!");
	INTERN_TRANSFORM_ENTITIES = rb_intern("transform_entities");
	INTERN_TRANSFORM_BY_VECTORS = rb_intern("transform_by_vectors");
    INTERN_ADD_POLYGON = rb_intern("add_polygon");
    INTERN_ALL_CONNECTED = rb_intern("all_connected");
    INTERN_DISTANCE = rb_intern("distance");
    INTERN_ADD_INSTANCE = rb_intern("add_instance");
    INTERN_ADD_FACES_FROM_MESH = rb_intern("add_faces_from_mesh");
    INTERN_FILL_FROM_MESH = rb_intern("fill_from_mesh");
    INTERN_ADD_GROUP = rb_intern("add_group");
    INTERN_ADD_EDGES = rb_intern("add_edges");
    INTERN_ADD_FACE = rb_intern("add_face");
    INTERN_ADD_CPOINT = rb_intern("add_cpoint");
    INTERN_ADD_CLINE = rb_intern("add_cline");
	INTERN_ADD_LINE = rb_intern("add_line");
    INTERN_CLOSE_ACTIVE = rb_intern("close_active");
    INTERN_THAS_LEADER = rb_intern("has_leader?");
    INTERN_EXPLODE = rb_intern("explode");
    INTERN_ADD_NOTE = rb_intern("add_note");
    INTERN_SET_TEXT = rb_intern("set_text");
    INTERN_CLASSIFY_POINT = rb_intern("classify_point");
    INTERN_FACES = rb_intern("faces");
    INTERN_TPARALLEL = rb_intern("parallel?");
    INTERN_PLANE = rb_intern("plane");
    INTERN_TON_PLANE = rb_intern("on_plane?");
    INTERN_MIN = rb_intern("min");
    INTERN_MAX = rb_intern("max");
    INTERN_MAKE_UNIQUE = rb_intern("make_unique");
    INTERN_COUNT = rb_intern("count");
    INTERN_EDGES = rb_intern("edges");
    INTERN_INTERSECT_WITH = rb_intern("intersect_with");
	INTERN_INVALIDATE_BOUNDS = rb_intern("invalidate_bounds");

    INTERN_OP_SQUARE_BRACKETS = rb_intern("[]");
    INTERN_OP_ASTERISKS = rb_intern("*");
    INTERN_OP_ADD = rb_intern("+");
    INTERN_OP_SUBTRACT = rb_intern("-");

    // Obtain SU version for the future use
    VALUE v_str_version = rb_funcall(SU_SKETCHUP, INTERN_VERSION, 0);
    VALUE v_int_version = rb_funcall(v_str_version, INTERN_TO_I, 0);
    su_version = FIX2INT(v_int_version);

    // Determine if SU is 64bit
    su_b64bit = (rb_respond_to(SU_SKETCHUP, INTERN_IS_64BIT) == 1 && rb_funcall(SU_SKETCHUP, INTERN_IS_64BIT, 0) == Qtrue);

    if (su_version > 8) {
        SU_FACE_POINT_UNKNOWN = rb_const_get_at(SU_FACE, rb_intern("PointUnknown"));
        SU_FACE_POINT_INSIDE = rb_const_get_at(SU_FACE, rb_intern("PointInside"));
        SU_FACE_POINT_ON_VERTEX = rb_const_get_at(SU_FACE, rb_intern("PointOnVertex"));
        SU_FACE_POINT_ON_EDGE = rb_const_get_at(SU_FACE, rb_intern("PointOnEdge"));
        SU_FACE_POINT_ON_FACE = rb_const_get_at(SU_FACE, rb_intern("PointOnFace"));
        SU_FACE_POINT_ON_OUTSIDE = rb_const_get_at(SU_FACE, rb_intern("PointOutside"));
        SU_FACE_POINT_NOT_ON_PLANE = rb_const_get_at(SU_FACE, rb_intern("PointNotOnPlane"));
    }
    else {
        SU_FACE_POINT_UNKNOWN = INT2FIX(0);
        SU_FACE_POINT_INSIDE = INT2FIX(1);
        SU_FACE_POINT_ON_VERTEX = INT2FIX(2);
        SU_FACE_POINT_ON_EDGE = INT2FIX(4);
        SU_FACE_POINT_ON_FACE = INT2FIX(8);
        SU_FACE_POINT_ON_OUTSIDE = INT2FIX(16);
        SU_FACE_POINT_NOT_ON_PLANE = INT2FIX(32);
    }

    SU_GL_POINTS = rb_const_get(rb_cObject, rb_intern("GL_POINTS"));
    SU_GL_LINES = rb_const_get(rb_cObject, rb_intern("GL_LINES"));
    SU_GL_LINE_STRIP = rb_const_get(rb_cObject, rb_intern("GL_LINE_STRIP"));
    SU_GL_LINE_LOOP = rb_const_get(rb_cObject, rb_intern("GL_LINE_LOOP"));
    SU_GL_TRIANGLES = rb_const_get(rb_cObject, rb_intern("GL_TRIANGLES"));
    SU_GL_TRIANGLE_STRIP = rb_const_get(rb_cObject, rb_intern("GL_TRIANGLE_STRIP"));
    SU_GL_TRIANGLE_FAN = rb_const_get(rb_cObject, rb_intern("GL_TRIANGLE_FAN"));
    SU_GL_QUADS = rb_const_get(rb_cObject, rb_intern("GL_QUADS"));
    SU_GL_QUAD_STRIP = rb_const_get(rb_cObject, rb_intern("GL_QUAD_STRIP"));
    SU_GL_POLYGON = rb_const_get(rb_cObject, rb_intern("GL_POLYGON"));
}
