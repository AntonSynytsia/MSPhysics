#include "msp_util.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////
*/

const dVector Util::X_AXIS(1.0f, 0.0f, 0.0f, 0.0f);
const dVector Util::Y_AXIS(0.0f, 1.0f, 0.0f, 0.0f);
const dVector Util::Z_AXIS(0.0f, 0.0f, 1.0f, 0.0f);
const dVector Util::ORIGIN(0.0f, 0.0f, 0.0f, 1.0f);


/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE Util::SU_SKETCHUP;
VALUE Util::SU_GEOM;
VALUE Util::SU_COLOR;
VALUE Util::SU_POINT3D;
VALUE Util::SU_VECTOR3D;
VALUE Util::SU_TRANSFORMATION;

ID Util::INTERN_NEW;
ID Util::INTERN_TO_A;

ID Util::INTERN_X;
ID Util::INTERN_Y;
ID Util::INTERN_Z;

ID Util::INTERN_RED;
ID Util::INTERN_SRED;
ID Util::INTERN_GREEN;
ID Util::INTERN_SGREEN;
ID Util::INTERN_BLUE;
ID Util::INTERN_SBLUE;
ID Util::INTERN_ALPHA;
ID Util::INTERN_SALPHA;

ID Util::INTERN_PUTS;
ID Util::INTERN_INSPECT;
ID Util::INTERN_BACKTRACE;
ID Util::INTERN_CALL;
ID Util::INTERN_PACK;
ID Util::INTERN_UNPACK;

ID Util::INTERN_XAXIS;
ID Util::INTERN_YAXIS;
ID Util::INTERN_ZAXIS;

ID Util::INTERN_SDRAWING_COLOR;
ID Util::INTERN_SLINE_WIDTH;
ID Util::INTERN_SLINE_STIPPLE;
ID Util::INTERN_DRAW;
ID Util::INTERN_DRAW2D;
ID Util::INTERN_SCREEN_COORDS;
ID Util::INTERN_CORNER;
ID Util::INTERN_CAMERA;
ID Util::INTERN_FOCAL_LENGTH;
ID Util::INTERN_EYE;
ID Util::INTERN_ADD;
ID Util::INTERN_CENTER;
ID Util::INTERN_VPWIDTH;
ID Util::INTERN_VPHEIGHT;

ID Util::INTERN_ACTIVE_MODEL;
ID Util::INTERN_ACTIVE_VIEW;

bool Util::s_validate_objects(true);


/*
 ///////////////////////////////////////////////////////////////////////////////
  Utility Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

float Util::min_float(float a, float b) {
	_mm_store_ss(&a, _mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
	return a;
}

float Util::max_float(float a, float b) {
	_mm_store_ss(&a, _mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
	return a;
}

float Util::clamp_float(float val, float min_val, float max_val) {
	_mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(min_val)), _mm_set_ss(max_val)));
	return val;
}

double Util::min_double(double a, double b) {
	_mm_store_sd(&a, _mm_min_sd(_mm_set_sd(a), _mm_set_sd(b)));
	return a;
}

double Util::max_double(double a, double b) {
	_mm_store_sd(&a, _mm_max_sd(_mm_set_sd(a), _mm_set_sd(b)));
	return a;
}

double Util::clamp_double(double val, double min_val, double max_val) {
	_mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val), _mm_set_sd(min_val)), _mm_set_sd(max_val)));
	return val;
}

int Util::clamp_int(int val, int min_val, int max_val) {
	if (val < min_val)
		return min_val;
	else if (val > max_val)
		return max_val;
	else
		return val;
}

unsigned int Util::clamp_uint(unsigned int val, unsigned int min_val, unsigned int max_val) {
	if (val < min_val)
		return min_val;
	else if (val > max_val)
		return max_val;
	else
		return val;
}

int Util::max_int(int val1, int val2) {
	if (val1 > val2)
		return val1;
	else
		return val2;
}

VALUE Util::to_value(const char* value) {
	VALUE v_string = rb_str_new2(value);
	#ifdef HAVE_RUBY_ENCODING_H
		static int enc_index = rb_enc_find_index("UTF-8");
		rb_enc_associate_index(v_string, enc_index);
	#endif
	return v_string;
}

VALUE Util::to_value(const char* value, unsigned int length) {
	VALUE v_string = rb_str_new(value, length);
	#ifdef HAVE_RUBY_ENCODING_H
		static int enc_index = rb_enc_find_index("UTF-8");
		rb_enc_associate_index(v_string, enc_index);
	#endif
	return v_string;
}

VALUE Util::to_value(const wchar_t* value) {
	unsigned int length = (unsigned int)wcslen(value);
	VALUE v_data = rb_ary_new2(length);
	for (unsigned int i = 0; i < length; ++i)
		rb_ary_store(v_data, i, INT2FIX(value[i]));
	return rb_funcall(v_data, INTERN_PACK, 1, rb_str_new2("U*"));
}

VALUE Util::to_value(const wchar_t* value, unsigned int length) {
	VALUE v_data = rb_ary_new2(length);
	for (unsigned int i = 0; i < length; ++i)
		rb_ary_store(v_data, i, INT2FIX(value[i]));
	return rb_funcall(v_data, INTERN_PACK, 1, rb_str_new2("U*"));
}

VALUE Util::vector_to_value(const dVector& value) {
	return rb_funcall(
		SU_VECTOR3D,
		INTERN_NEW,
		3,
		rb_float_new(value.m_x),
		rb_float_new(value.m_y),
		rb_float_new(value.m_z)
	);
}

VALUE Util::point_to_value(const dVector& value) {
	return rb_funcall(
		SU_POINT3D,
		INTERN_NEW,
		3,
		rb_float_new(value.m_x),
		rb_float_new(value.m_y),
		rb_float_new(value.m_z)
	);
}

VALUE Util::matrix_to_value(const dMatrix& value) {
	VALUE v_matrix = rb_ary_new2(16);

	rb_ary_store(v_matrix, 0, rb_float_new(value.m_front.m_x));
	rb_ary_store(v_matrix, 1, rb_float_new(value.m_front.m_y));
	rb_ary_store(v_matrix, 2, rb_float_new(value.m_front.m_z));
	rb_ary_store(v_matrix, 3, rb_float_new(value.m_front.m_w));

	rb_ary_store(v_matrix, 4, rb_float_new(value.m_up.m_x));
	rb_ary_store(v_matrix, 5, rb_float_new(value.m_up.m_y));
	rb_ary_store(v_matrix, 6, rb_float_new(value.m_up.m_z));
	rb_ary_store(v_matrix, 7, rb_float_new(value.m_up.m_w));

	rb_ary_store(v_matrix, 8, rb_float_new(value.m_right.m_x));
	rb_ary_store(v_matrix, 9, rb_float_new(value.m_right.m_y));
	rb_ary_store(v_matrix, 10, rb_float_new(value.m_right.m_z));
	rb_ary_store(v_matrix, 11, rb_float_new(value.m_right.m_w));

	rb_ary_store(v_matrix, 12, rb_float_new(value.m_posit.m_x));
	rb_ary_store(v_matrix, 13, rb_float_new(value.m_posit.m_y));
	rb_ary_store(v_matrix, 14, rb_float_new(value.m_posit.m_z));
	rb_ary_store(v_matrix, 15, rb_float_new(value.m_posit.m_w));

	return rb_funcall(SU_TRANSFORMATION, INTERN_NEW, 1, v_matrix);
}

VALUE Util::color_to_value(const dVector& value, dFloat alpha) {
	return rb_funcall(
		SU_COLOR,
		INTERN_NEW,
		4,
		rb_int2inum(static_cast<int>(value.m_x)),
		rb_int2inum(static_cast<int>(value.m_y)),
		rb_int2inum(static_cast<int>(value.m_z)),
		rb_float_new(alpha));
}

char* Util::value_to_c_str(VALUE value) {
	return RSTRING_PTR(StringValue(value));
}

wchar_t* Util::value_to_c_str2(VALUE value) {
	VALUE v_str = StringValue(value);
	VALUE v_data = rb_funcall(v_str, INTERN_UNPACK, 1, rb_str_new2("U*"));
	long len = RARRAY_LEN(v_data);
	wchar_t* text = new wchar_t[len+1];
	for (long i = 0; i < len; ++i)
		text[i] = (wchar_t)Util::value_to_int(rb_ary_entry(v_data, i));
	text[len] = '\0';
	return text;
}

dVector Util::value_to_vector(VALUE value) {
	return dVector(
		static_cast<dFloat>(rb_num2dbl(rb_funcall(value, INTERN_X, 0))),
		static_cast<dFloat>(rb_num2dbl(rb_funcall(value, INTERN_Y, 0))),
		static_cast<dFloat>(rb_num2dbl(rb_funcall(value, INTERN_Z, 0)))
		);
}

dVector Util::value_to_point(VALUE value) {
	return dVector(
		static_cast<dFloat>(rb_num2dbl(rb_funcall(value, INTERN_X, 0))),
		static_cast<dFloat>(rb_num2dbl(rb_funcall(value, INTERN_Y, 0))),
		static_cast<dFloat>(rb_num2dbl(rb_funcall(value, INTERN_Z, 0)))
	);
}

dMatrix Util::value_to_matrix(VALUE value) {
	if (rb_obj_is_kind_of(value, SU_TRANSFORMATION) == Qfalse)
		value = rb_funcall(SU_TRANSFORMATION, INTERN_NEW, 1, value);
	VALUE v_matrix_ary = rb_funcall(value, INTERN_TO_A, 0);
	dFloat ma[16];
	for (int i = 0; i < 16; ++i)
		ma[i] = static_cast<dFloat>(rb_num2dbl(rb_ary_entry(v_matrix_ary, i)));
	// Extract global scale
	if (dAbs(ma[15]) > M_EPSILON) {
		dFloat inv_wscale = 1.0f / ma[15];
		for (unsigned int i = 0; i < 15; ++i)
			ma[i] *= inv_wscale;
		ma[15] = 1.0f;
	}
	// Create matrix
	dMatrix matrix(ma);
	// Validate
	if (!is_matrix_uniform(matrix))
		rb_raise(rb_eTypeError, "Given matrix is not uniform. Some or all matrix axes are not perpendicular to each other.");
	if (is_matrix_flat(matrix))
		rb_raise(rb_eTypeError, "Given matrix has one or more axes scaled to zero. Flat matrices are not acceptable!");
	// Return
	return matrix;
}

dVector Util::value_to_color(VALUE value) {
	if (rb_obj_is_kind_of(value, SU_COLOR) == Qfalse)
		value = rb_funcall(SU_COLOR, INTERN_NEW, 1, value);
	return dVector(
		static_cast<dFloat>(NUM2INT(rb_funcall( value, INTERN_RED, 0 ))),
		static_cast<dFloat>(NUM2INT(rb_funcall( value, INTERN_GREEN, 0 ))),
		static_cast<dFloat>(NUM2INT(rb_funcall( value, INTERN_BLUE, 0 ))),
		static_cast<dFloat>(NUM2INT(rb_funcall( value, INTERN_ALPHA, 0 )))
	);
}

dFloat Util::get_vector_magnitude(const dVector& vector) {
	return dSqrt(vector.m_x * vector.m_x + vector.m_y * vector.m_y + vector.m_z * vector.m_z);
}

dFloat Util::get_vector_magnitude2(const dVector& vector) {
	return vector.m_x * vector.m_x + vector.m_y * vector.m_y + vector.m_z * vector.m_z;
}

void Util::set_vector_magnitude(dVector& vector, dFloat magnitude) {
	dFloat m = get_vector_magnitude(vector);
	if (m > M_EPSILON) {
		dFloat r = magnitude / m;
		vector.m_x *= r;
		vector.m_y *= r;
		vector.m_z *= r;
	}
	else
		rb_raise(rb_eTypeError, "Can't change magnitude of a zero lengthed vector!");
}

void Util::normalize_vector(dVector& vector) {
	dFloat m = get_vector_magnitude(vector);
	if (m > M_EPSILON) {
		dFloat r = 1.0f / m;
		vector.m_x *= r;
		vector.m_y *= r;
		vector.m_z *= r;
	}
	else
		rb_raise(rb_eTypeError, "Can't normalize a zero lengthed vector!");
}

void Util::scale_vector(dVector& vector, dFloat scale) {
	vector.m_x *= scale;
	vector.m_y *= scale;
	vector.m_z *= scale;
}

void Util::zero_out_vector(dVector& vector) {
	vector.m_x = 0.0f;
	vector.m_y = 0.0f;
	vector.m_z = 0.0f;
}

bool Util::vectors_identical(const dVector& a, const dVector& b) {
	return ((dAbs(a.m_x - b.m_x) < M_EPSILON && dAbs(a.m_y - b.m_y) < M_EPSILON && dAbs(a.m_z - b.m_z) < M_EPSILON));
}

bool Util::is_vector_valid(const dVector& vector) {
	return (vector.m_x == vector.m_x && vector.m_y == vector.m_y && vector.m_z == vector.m_z);
}

bool Util::is_matrix_uniform(const dMatrix& matrix) {
	return (dAbs(matrix.m_front.DotProduct3(matrix.m_up)) < M_EPSILON2 && dAbs(matrix.m_front.DotProduct3(matrix.m_right)) < M_EPSILON2 && dAbs(matrix.m_up.DotProduct3(matrix.m_right)) < M_EPSILON2);
}

bool Util::is_matrix_flat(const dMatrix& matrix) {
	return (get_vector_magnitude(matrix.m_front) < M_EPSILON || get_vector_magnitude(matrix.m_up) < M_EPSILON || get_vector_magnitude(matrix.m_right) < M_EPSILON);
}

bool Util::is_matrix_flipped(const dMatrix& matrix) {
	return ((matrix.m_front.CrossProduct(matrix.m_up)).DotProduct3(matrix.m_right) < 0);
}

dVector Util::get_matrix_scale(const dMatrix& matrix) {
	return dVector(
		get_vector_magnitude(matrix.m_front),
		get_vector_magnitude(matrix.m_up),
		get_vector_magnitude(matrix.m_right));
}

void Util::set_matrix_scale(dMatrix& matrix, const dVector& scale) {
	set_vector_magnitude(matrix.m_front, scale.m_x);
	set_vector_magnitude(matrix.m_up, scale.m_y);
	set_vector_magnitude(matrix.m_right, scale.m_z);
}

void Util::extract_matrix_scale(dMatrix& matrix) {
	normalize_vector(matrix.m_front);
	normalize_vector(matrix.m_up);
	normalize_vector(matrix.m_right);
}

void Util::matrix_from_pin_dir(const dVector& pos, const dVector& dir, dMatrix& matrix_out) {
	dVector zaxis(dir);
	Util::normalize_vector(zaxis);
	dVector xaxis;
	if (dAbs(zaxis.m_z) > 0.9999995f) {
		//xaxis = Y_AXIS.CrossProduct(zaxis);
		xaxis.m_x = zaxis.m_z;
		xaxis.m_y = 0.0f;
		xaxis.m_z = -zaxis.m_x;
	}
	else {
		//xaxis = Z_AXIS.CrossProduct(zaxis);
		xaxis.m_x = -zaxis.m_y;
		xaxis.m_y = zaxis.m_x;
		xaxis.m_z = 0.0f;
	}
	Util::normalize_vector(xaxis);
	dVector yaxis(zaxis.CrossProduct(xaxis));
	Util::normalize_vector(yaxis);
	matrix_out.m_front = xaxis;
	matrix_out.m_front.m_w = 0.0f;
	matrix_out.m_up = yaxis;
	matrix_out.m_up.m_w = 0.0f;
	matrix_out.m_right = zaxis;
	matrix_out.m_right.m_w = 0.0f;
	matrix_out.m_posit = pos;
	matrix_out.m_posit.m_w = 1.0f;
}

void Util::rotate_matrix_to_dir(const dMatrix& matrix, const dVector& dir, dMatrix& matrix_out) {
	// Determine the cross product between the z-axis of matrix 1 and the given dir.
	dVector dir1(matrix.m_right);
	Util::normalize_vector(dir1);
	dir1.m_w = 0.0f;
	dVector dir2(dir);
	Util::normalize_vector(dir2);
	dir2.m_w = 0.0f;
	dFloat cos_theta = dir1.DotProduct3(dir2);
	dVector normal;
	if (cos_theta > 0.9999995f) {
		matrix_out = matrix;
		return;
	}
	else if (cos_theta < -0.9999995f) {
		normal = matrix.m_front;
	}
	else {
		normal = dir1.CrossProduct(dir2);
	}
	//dVector normal(dir1.CrossProduct(dir2));
	Util::normalize_vector(normal);
	normal.m_w = 0.0f;
	// Develop two temporary matrices.
	dVector yaxis1(normal.CrossProduct(dir1));
	Util::normalize_vector(yaxis1);
	yaxis1.m_w = 0.0f;
	dVector yaxis2(normal.CrossProduct(dir2));
	Util::normalize_vector(yaxis2);
	yaxis2.m_w = 0.0f;
	dMatrix t1(dir1, yaxis1, normal, ORIGIN);
	dMatrix t2(dir2, yaxis2, normal, ORIGIN);
	// Unrotate matrix with respect to t1.
	// Then rotate the result with respect to t2.
	matrix_out = (matrix * t1.Inverse()) * t2;
	matrix_out.m_posit = matrix.m_posit;
}

void Util::rotate_matrix_to_dir2(dMatrix& matrix, const dVector& dir) {
	// Determine the cross product between the z-axis of matrix 1 and the given dir.
	dVector dir1(matrix.m_right);
	Util::normalize_vector(dir1);
	dir1.m_w = 0.0f;
	dVector dir2(dir);
	Util::normalize_vector(dir2);
	dir2.m_w = 0.0f;
	dFloat cos_theta = dir1.DotProduct3(dir2);
	dVector normal;
	if (cos_theta > 0.9999995f)
		return;
	if (cos_theta < -0.9999995f)
		normal = matrix.m_front;
	else
		normal = dir1.CrossProduct(dir2);
	//dVector normal(dir1.CrossProduct(dir2));
	Util::normalize_vector(normal);
	normal.m_w = 0.0f;
	// Develop two temporary matrices.
	dVector yaxis1(normal.CrossProduct(dir1));
	Util::normalize_vector(yaxis1);
	yaxis1.m_w = 0.0f;
	dVector yaxis2(normal.CrossProduct(dir2));
	Util::normalize_vector(yaxis2);
	yaxis2.m_w = 0.0f;
	dMatrix t1(dir1, yaxis1, normal, ORIGIN);
	dMatrix t2(dir2, yaxis2, normal, ORIGIN);
	// Unrotate matrix with respect to t1.
	// Then rotate the result with respect to t2.
	dMatrix rot_matrix((matrix * t1.Inverse()) * t2);
	matrix.m_front = rot_matrix.m_front;
	matrix.m_up = rot_matrix.m_up;
	matrix.m_right = rot_matrix.m_right;
}

dVector Util::rotate_vector(const dVector& vector, const dVector& normal, const dFloat& angle) {
	dMatrix rot_matrix;
	matrix_from_pin_dir(ORIGIN, normal, rot_matrix);
	dVector loc_vector(rot_matrix.UnrotateVector(vector));
	dFloat mag = dSqrt(loc_vector.m_x * loc_vector.m_x + loc_vector.m_y * loc_vector.m_y);
	if (mag < 1.0e-8f)
		return dVector(vector);
	dFloat theta = dAcos(Util::clamp_float(loc_vector.m_x/mag, -1.0f, 1.0f));
	if (loc_vector.m_y < 0)
		theta = -theta;
	theta += angle;
	loc_vector.m_x = mag * dCos(theta);
	loc_vector.m_y = mag * dSin(theta);
	return rot_matrix.RotateVector(loc_vector);
}

void Util::correct_to_expected_matrix(dMatrix& matrix, const dVector& velocity, const dVector& omega, dFloat timestep) {
	matrix.m_posit.m_x -= velocity.m_x * timestep;
	matrix.m_posit.m_y -= velocity.m_y * timestep;
	matrix.m_posit.m_z -= velocity.m_z * timestep;
}

VALUE Util::call_proc(VALUE v_proc) {
	if (rb_class_of(v_proc) != rb_cProc)
		rb_raise(rb_eTypeError, "Expected a Proc object!");
	return rb_funcall(v_proc, INTERN_CALL, 0);
}

VALUE Util::rescue_proc(VALUE v_args, VALUE v_exception) {
	VALUE v_message = rb_funcall(v_exception, INTERN_INSPECT, 0);
	rb_funcall(rb_stdout, INTERN_PUTS, 1, v_message);
	VALUE v_backtrace = rb_funcall(v_exception, INTERN_BACKTRACE, 0);
	rb_funcall(rb_stdout, INTERN_PUTS, 1, rb_ary_entry(v_backtrace, 0));
	return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void Util::init_ruby() {
	SU_SKETCHUP = rb_eval_string("::Sketchup");
	SU_GEOM = rb_eval_string("::Geom");
	SU_COLOR = rb_eval_string("::Sketchup::Color");
	SU_POINT3D = rb_eval_string("::Geom::Point3d");
	SU_VECTOR3D = rb_eval_string("::Geom::Vector3d");
	SU_TRANSFORMATION = rb_eval_string("::Geom::Transformation");

	INTERN_NEW = rb_intern("new");
	INTERN_TO_A = rb_intern("to_a");
	INTERN_X = rb_intern("x");
	INTERN_Y = rb_intern("y");
	INTERN_Z = rb_intern("z");

	INTERN_RED = rb_intern("red");
	INTERN_SRED = rb_intern("red=");
	INTERN_GREEN = rb_intern("green");
	INTERN_SGREEN = rb_intern("green=");
	INTERN_BLUE = rb_intern("blue");
	INTERN_SBLUE = rb_intern("blue=");
	INTERN_ALPHA = rb_intern("alpha");
	INTERN_SALPHA = rb_intern("alpha=");

	INTERN_PUTS = rb_intern("puts");
	INTERN_INSPECT = rb_intern("inspect");
	INTERN_BACKTRACE = rb_intern("backtrace");
	INTERN_CALL = rb_intern("call");
	INTERN_PACK = rb_intern("pack");
	INTERN_UNPACK = rb_intern("unpack");

	INTERN_XAXIS = rb_intern("xaxis");
	INTERN_YAXIS = rb_intern("yaxis");
	INTERN_ZAXIS = rb_intern("zaxis");

	INTERN_SDRAWING_COLOR = rb_intern("drawing_color=");
	INTERN_SLINE_WIDTH = rb_intern("line_width=");
	INTERN_SLINE_STIPPLE = rb_intern("line_stipple=");
	INTERN_DRAW = rb_intern("draw");
	INTERN_DRAW2D = rb_intern("draw2d");
	INTERN_SCREEN_COORDS = rb_intern("screen_coords");
	INTERN_CORNER = rb_intern("corner");
	INTERN_CAMERA = rb_intern("camera");
	INTERN_FOCAL_LENGTH = rb_intern("focal_length");
	INTERN_EYE = rb_intern("eye");
	INTERN_ADD = rb_intern("add");
	INTERN_CENTER = rb_intern("center");
	INTERN_VPWIDTH = rb_intern("vpwidth");
	INTERN_VPHEIGHT = rb_intern("vpheight");

	INTERN_ACTIVE_MODEL = rb_intern("active_model");
	INTERN_ACTIVE_VIEW = rb_intern("active_view");
}
