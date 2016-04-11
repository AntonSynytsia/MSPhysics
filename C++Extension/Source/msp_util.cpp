#include "msp_util.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

std::map<const NewtonWorld*, bool> valid_worlds;
std::map<const NewtonBody*, bool> valid_bodies;
std::map<const NewtonCollision*, bool> valid_collisions;
std::map<JointData*, bool> valid_joints;
std::map<StandardJointData*, bool> valid_standard_joints;
bool validate_objects;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Utilty Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

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
	long length = (long)wcslen(value);
	VALUE v_data = rb_ary_new2(length);
	for (long i = 0; i < length; ++i)
		rb_ary_store(v_data, i, INT2FIX(value[i]));
	return rb_funcall(v_data, rb_intern("pack"), 1, rb_str_new2("U*"));
}

VALUE Util::to_value(const wchar_t* value, long length) {
	VALUE v_data = rb_ary_new2(length);
	for (long i = 0; i < length; ++i)
		rb_ary_store(v_data, i, INT2FIX(value[i]));
	return rb_funcall(v_data, rb_intern("pack"), 1, rb_str_new2("U*"));
}

VALUE Util::vector_to_value(const dVector& value, dFloat scale) {
	return rb_funcall(
		suVector3d,
		rb_intern("new"),
		3,
		rb_float_new(value.m_x * scale),
		rb_float_new(value.m_y * scale),
		rb_float_new(value.m_z * scale)
	);
}

VALUE Util::point_to_value(const dVector& value, dFloat scale) {
	dFloat s = METER_TO_INCH * scale;
	return rb_funcall(
		suPoint3d,
		rb_intern("new"),
		3,
		rb_float_new(value.m_x * s),
		rb_float_new(value.m_y * s),
		rb_float_new(value.m_z * s)
	);
}

VALUE Util::matrix_to_value(const dMatrix& value, dFloat scale) {
	VALUE v_matrix = rb_ary_new2(16);
	dFloat s = METER_TO_INCH * scale;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			rb_ary_store( v_matrix, i*4+j, rb_float_new(value[i][j] * (i == 3 && j < 3 ? s : 1)) );
	return rb_funcall(suTransformation, rb_intern("new"), 1, v_matrix);
}

dFloat Util::value_to_dFloat2(VALUE value, dFloat scale, dFloat min, dFloat max) {
	dFloat num = static_cast<dFloat>(rb_num2dbl(value)) * scale;
	if (num < min || num > max) {
		dFloat iscale = 1.0f / scale;
		rb_raise(rb_eRangeError, "Value, %.3f, is out of range! It should be between %.3f and %.3f.", num * iscale, min * iscale, max * iscale);
	}
	return num;
}

char* Util::value_to_c_str(VALUE value) {
	return RSTRING_PTR(StringValue(value));
}

wchar_t* Util::value_to_c_str2(VALUE value) {
	VALUE v_str = StringValue(value);
	VALUE v_data = rb_funcall(v_str, rb_intern("unpack"), 1, rb_str_new2("U*"));
	long len = RARRAY_LEN(v_data);
	wchar_t* text = new wchar_t[len+1];
	for (long i = 0; i < len; ++i)
		text[i] = (wchar_t)Util::value_to_int(rb_ary_entry(v_data, i));
	text[len] = '\0';
	return text;
}

dVector Util::value_to_vector(VALUE value, dFloat scale) {
	return dVector(
		dFloat(rb_num2dbl(rb_funcall( value, rb_intern("x"), 0 )) * scale),
		dFloat(rb_num2dbl(rb_funcall( value, rb_intern("y"), 0 )) * scale),
		dFloat(rb_num2dbl(rb_funcall( value, rb_intern("z"), 0 )) * scale)
	);
}

dVector Util::value_to_point(VALUE value, dFloat scale) {
	dFloat s = INCH_TO_METER * scale;
	return dVector(
		dFloat(rb_num2dbl(rb_funcall( value, rb_intern("x"), 0 )) * s),
		dFloat(rb_num2dbl(rb_funcall( value, rb_intern("y"), 0 )) * s),
		dFloat(rb_num2dbl(rb_funcall( value, rb_intern("z"), 0 )) * s)
	);
}

dMatrix Util::value_to_matrix(VALUE value, dFloat scale) {
	if (rb_class_of(value) != suTransformation)
		value = rb_funcall(suTransformation, rb_intern("new"), 1, value);
	VALUE v_matrix_ary = rb_funcall(value, rb_intern("to_a"), 0);
	dFloat s = INCH_TO_METER * scale;
	/*dMatrix matrix;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			matrix[i][j] = (dFloat)rb_num2dbl(rb_ary_entry(v_matrix_ary, i*4+j)) * (i == 3 && j < 3 ? s : 1);*/
	dFloat ma[16];
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			ma[i*4+j] = (dFloat)rb_num2dbl(rb_ary_entry(v_matrix_ary, i*4+j)) * (i == 3 && j < 3 ? s : 1);
	dMatrix matrix(ma);
	if (!is_matrix_uniform(matrix))
		rb_raise(rb_eTypeError, "Given matrix is not uniform. Some or all matrix axis are not perpendicular to each other.");
	if (is_matrix_flat(matrix))
		rb_raise(rb_eTypeError, "Given matrix has one or more of its axis scaled to zero. Flat matrices are not acceptable!");
	return matrix;
}

const NewtonWorld* Util::value_to_world(VALUE value) {
	const NewtonWorld* const address = (const NewtonWorld*)NUM2LL(value);
	if (validate_objects == true && valid_worlds.find(address) == valid_worlds.end())
		rb_raise(rb_eTypeError, "Given address doesn't reference a valid world!");
	return address;
}

const NewtonBody* Util::value_to_body(VALUE value) {
	const NewtonBody* address = (const NewtonBody*)NUM2LL(value);
	if (validate_objects == true && valid_bodies.find(address) == valid_bodies.end())
		rb_raise(rb_eTypeError, "Given address doesn't reference a valid body!");
	return address;
}

const NewtonCollision* Util::value_to_collision(VALUE value) {
	const NewtonCollision* address = (const NewtonCollision*)NUM2LL(value);
	if (validate_objects == true && valid_collisions.find(address) == valid_collisions.end())
		rb_raise(rb_eTypeError, "Given address doesn't reference a valid collision!");
	return address;
}

JointData* Util::value_to_joint(VALUE value) {
	JointData* address = (JointData*)NUM2LL(value);
	if (validate_objects == true && valid_joints.find(address) == valid_joints.end())
		rb_raise(rb_eTypeError, "Given address doesn't reference a valid joint!");
	return address;
}

JointData* Util::value_to_joint2(VALUE value, JointType joint_type) {
	JointData* address = (JointData*)NUM2LL(value);
	if (validate_objects == true && valid_joints.find(address) == valid_joints.end())
		rb_raise(rb_eTypeError, "Given address doesn't reference a valid joint!");
	if (validate_objects == true && address->jtype != joint_type)
		rb_raise(rb_eTypeError, "Given address doesn't reference a joint of a particular type!");
	return address;
}

dFloat Util::get_vector_magnitude(const dVector& vector) {
	return dSqrt(vector.m_x * vector.m_x + vector.m_y * vector.m_y + vector.m_z * vector.m_z);
}

void Util::set_vector_magnitude(dVector& vector, dFloat magnitude) {
	dFloat m = get_vector_magnitude(vector);
	if (dAbs(m - magnitude) < EPSILON) return;
	if (m < EPSILON)
		rb_raise(rb_eTypeError, "Can't change normal of a zero lengthed vector!");
	dFloat r = magnitude / m;
	vector.m_x *= r;
	vector.m_y *= r;
	vector.m_z *= r;
}

void Util::normalize_vector(dVector& vector) {
	set_vector_magnitude(vector, 1.0f);
}

bool Util::vectors_identical(dVector& a, dVector& b) {
	return ((dAbs(a.m_x - b.m_x) < EPSILON && dAbs(a.m_y - b.m_y) < EPSILON && dAbs(a.m_z - b.m_z) < EPSILON)) ? true : false;
}

bool Util::is_vector_valid(dVector& vector) {
	return (vector.m_x == vector.m_x && vector.m_y == vector.m_y && vector.m_z == vector.m_z) ? true : false;
}

bool Util::is_matrix_uniform(const dMatrix& matrix) {
	return (dAbs(matrix.m_front % matrix.m_up) < EPSILON2 && dAbs(matrix.m_front % matrix.m_right) < EPSILON2 && dAbs(matrix.m_up % matrix.m_right) < EPSILON2) ? true : false;
}

bool Util::is_matrix_flat(const dMatrix& matrix) {
	return (get_vector_magnitude(matrix.m_front) < EPSILON || get_vector_magnitude(matrix.m_up) < EPSILON || get_vector_magnitude(matrix.m_right) < EPSILON) ? true : false;
}

bool Util::is_matrix_flipped(const dMatrix& matrix) {
	return ((matrix.m_front * matrix.m_up) % matrix.m_right < 0) ? true : false;
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

dMatrix Util::matrix_from_pin_dir(const dVector& pos, const dVector& dir) {
	/*dVector xaxis(Z_AXIS * dir);
	normalize_vector(xaxis);
	dVector yaxis = dir * xaxis;
	return dMatrix(xaxis, yaxis, dir, pos);*/
	VALUE v_tra = rb_funcall(suTransformation, rb_intern("new"), 2, Util::point_to_value(pos), Util::vector_to_value(dir));
	return Util::value_to_matrix(v_tra);
}

dMatrix Util::rotate_matrix_to_dir(const dMatrix& matrix, const dVector& dir) {
	//dMatrix ta(matrix.m_front, matrix.m_up, matrix.m_right, ORIGIN);
	//dMatrix tb(matrix.m_front, matrix.m_up, dir, ORIGIN);
	dMatrix ta = matrix_from_pin_dir(ORIGIN, matrix.m_right);
	dMatrix tb = matrix_from_pin_dir(ORIGIN, dir);
	return (matrix * ta.Inverse()) * tb;
}

dVector Util::rotate_vector(const dVector& vector, const dVector& normal, const dFloat& angle) {
	VALUE v_tra = rb_funcall(
		suTransformation,
		rb_intern("rotation"),
		3,
		Util::point_to_value(ORIGIN),
		Util::vector_to_value(normal),
		Util::to_value(angle));
	VALUE v_itra = rb_funcall(v_tra, rb_intern("inverse"), 0);
	VALUE v_rvector = rb_funcall(Util::vector_to_value(vector), rb_intern("transform"), 1, v_itra);
	return Util::value_to_vector(v_rvector);
}

bool Util::is_world_valid(const NewtonWorld* address) {
	return valid_worlds.find(address) != valid_worlds.end();
}

bool Util::is_body_valid(const NewtonBody* address) {
	return valid_bodies.find(address) != valid_bodies.end();
}

bool Util::is_collision_valid(const NewtonCollision* address) {
	return valid_collisions.find(address) != valid_collisions.end();
}

bool Util::is_joint_valid(JointData* address) {
	return valid_joints.find(address) != valid_joints.end();
}

bool Util::is_collision_convex(const NewtonCollision* address) {
	return NewtonCollisionGetType(address) < 10;
}

bool Util::bodies_collidable(const NewtonBody* body0, const NewtonBody* body1) {
	BodyData* data0 = (BodyData*)NewtonBodyGetUserData(body0);
	BodyData* data1 = (BodyData*)NewtonBodyGetUserData(body1);
	return data0->collidable == true && data1->collidable == true && data0->non_collidable_bodies.find(body1) == data0->non_collidable_bodies.end();
}

bool Util::bodies_aabb_overlap(const NewtonBody* body0, const NewtonBody* body1) {
	dVector minA;
	dVector maxA;
	dVector minB;
	dVector maxB;
	NewtonBodyGetAABB(body0, &minA[0], &maxA[0]);
	NewtonBodyGetAABB(body1, &minB[0], &maxB[0]);
	for (int i = 0; i < 3; ++i) {
		if ((minA[i] >= minB[i] && minA[i] <= maxB[i]) ||
			(maxA[i] >= minB[i] && maxA[i] <= maxB[i]) ||
			(minB[i] >= minA[i] && minB[i] <= maxA[i]) ||
			(maxB[i] >= minA[i] && maxB[i] <= maxA[i])) continue;
		return false;
	}
	return true;
}

void Util::validate_two_bodies(const NewtonBody* body1, const NewtonBody* body2) {
	const NewtonWorld* world1 = NewtonBodyGetWorld(body1);
	const NewtonWorld* world2 = NewtonBodyGetWorld(body2);
	if (body1 == body2)
		rb_raise(rb_eTypeError, "Expected two unique bodies!");
	if (world1 != world2)
		rb_raise(rb_eTypeError, "Expected two bodies from a same world!");
}

VALUE Util::call_proc(VALUE v_proc) {
	if (rb_class_of(v_proc) != rb_cProc)
		rb_raise(rb_eTypeError, "Expected a Proc object!");
	return rb_funcall( v_proc, rb_intern("call"), 0 );
}

VALUE Util::rescue_proc(VALUE v_args, VALUE v_exception) {
	VALUE v_message = rb_funcall(v_exception, rb_intern("inspect"), 0);
	rb_funcall(rb_stdout, rb_intern("puts"), 1, v_message);
	VALUE v_backtrace = rb_funcall(v_exception, rb_intern("backtrace"), 0);
	rb_funcall(rb_stdout, rb_intern("puts"), 1, rb_ary_entry(v_backtrace, 0));
	return Qnil;
}

void Init_msp_util(VALUE mMSPhysics) {
	validate_objects = true;
}
