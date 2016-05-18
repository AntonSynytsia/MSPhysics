#ifndef MSP_UTIL_H
#define MSP_UTIL_H

#include "ruby_util.h"
#include <cmath>
#include "NewtonClass.h"
#include "Newton.h"
#include "dVector.h"
#include "dMatrix.h"
#include "dQuaternion.h"
#include <iostream>
#include <map>
#include <vector>
#include <float.h>
#include "VHACD.h"

// Constants
const dFloat PI									= 3.14159265f;
const dFloat PI2								= 6.28318530f;
const dFloat EPSILON							= 1.0e-6f;
const dFloat EPSILON2							= 1.0e-3f;
const dFloat INCH_TO_METER						= 0.02540f;
const dFloat METER_TO_INCH						= 39.3701f;
const dFloat DEG_TO_RAD							= 0.01745329f;
const dFloat RAD_TO_DEG							= 57.2957795f;

const dVector X_AXIS(1.0f, 0.0f, 0.0f);
const dVector Y_AXIS(0.0f, 1.0f, 0.0f);
const dVector Z_AXIS(0.0f, 0.0f, 1.0f);
const dVector ORIGIN(0.0f, 0.0f, 0.0f);

const dVector DEFAULT_GRAVITY(0.0f, 0.0f, -9.8f);
const dFloat DEFAULT_SCALE						= 1.0f;
const dFloat DEFAULT_DENSITY					= 700.0f;
const dFloat DEFAULT_ELASTICITY					= 0.40f;
const dFloat DEFAULT_SOFTNESS					= 0.10f;
const dFloat DEFAULT_STATIC_FRICTION			= 0.90f;
const dFloat DEFAULT_DYNAMIC_FRICTION			= 0.50f;
const bool DEFAULT_ENABLE_FRICTION				= true;
const long NON_COL_CONTACTS_CAPACITY			= 32;
const int DEFAULT_SOLVER_MODEL					= 4;
const int DEFAULT_FRICTION_MODEL				= 0;
const int DEFAULT_CONVERGENCE_QUALITY			= 1;
const dFloat DEFAULT_MATERIAL_THICKNESS			= 1.0f / 256.0f;
const dFloat DEFAULT_CONTACT_MERGE_TOLERANCE	= 1.0e-3f;
const dFloat MIN_TOUCH_DISTANCE					= 0.005f;

const dFloat MIN_MASS							= 1.0e-6f;
const dFloat MAX_MASS							= 1.0e14f;
const dFloat MIN_VOLUME							= 1.0e-6f;
const dFloat MIN_DENSITY						= 1.0e-6f;

// Sketchup API Access Constants
const VALUE suSketchup			= rb_define_module("Sketchup");
const VALUE suColor				= rb_define_class_under(suSketchup, "Color", rb_cObject);
const VALUE suGeom				= rb_define_module("Geom");
const VALUE suPoint3d			= rb_define_class_under(suGeom, "Point3d", rb_cObject);
const VALUE suVector3d			= rb_define_class_under(suGeom, "Vector3d", rb_cObject);
const VALUE suTransformation	= rb_define_class_under(suGeom, "Transformation", rb_cObject);

// Optimization interns
const ID INTERN_NEW				= rb_intern("new");
const ID INTERN_PACK			= rb_intern("pack");
const ID INTERN_UNPACK			= rb_intern("unpack");
const ID INTERN_X				= rb_intern("x");
const ID INTERN_Y				= rb_intern("y");
const ID INTERN_Z				= rb_intern("z");
const ID INTERN_TO_A			= rb_intern("to_a");
const ID INTERN_RED				= rb_intern("red");
const ID INTERN_GREEN			= rb_intern("green");
const ID INTERN_BLUE			= rb_intern("blue");
const ID INTERN_ALPHA			= rb_intern("alpha");
const ID INTERN_SRED			= rb_intern("red=");
const ID INTERN_SGREEN			= rb_intern("green=");
const ID INTERN_SBLUE			= rb_intern("blue=");
const ID INTERN_SALPHA			= rb_intern("alpha=");
const ID INTERN_ROTATION		= rb_intern("rotation");
const ID INTERN_INVERSE			= rb_intern("inverse");
const ID INTERN_TRANSFORM		= rb_intern("transform");
const ID INTERN_CALL			= rb_intern("call");
const ID INTERN_PUTS			= rb_intern("puts");
const ID INTERN_INSPECT			= rb_intern("inspect");
const ID INTERN_BACKTRACE		= rb_intern("backtrace");
const ID INTERN_XAXIS			= rb_intern("xaxis");
const ID INTERN_YAXIS			= rb_intern("yaxis");
const ID INTERN_ZAXIS			= rb_intern("zaxis");
const ID INTERN_ACTIVE_MODEL	= rb_intern("active_model");
const ID INTERN_ACTIVE_VIEW		= rb_intern("active_view");
const ID INTERN_CAMERA			= rb_intern("camera");
const ID INTERN_EYE				= rb_intern("eye");
const ID INTERN_DRAW			= rb_intern("draw");
const ID INTERN_SDRAWING_COLOR	= rb_intern("drawing_color=");
const ID INTERN_ADD				= rb_intern("add");

// Enumerators
enum JointType {
	JT_NONE,
	JT_HINGE,
	JT_MOTOR,
	JT_SERVO,
	JT_SLIDER,
	JT_PISTON,
	JT_UP_VECTOR,
	JT_SPRING,
	JT_CORKSCREW,
	JT_BALL_AND_SOCKET,
	JT_UNIVERSAL,
	JT_FIXED
};

enum ConstraintType {
	CT_STANDARD,
	CT_FLEXIBLE,
	CT_ROBUST
};

// Structures
typedef struct BodyTouchData
{
	const NewtonBody* body0;
	const NewtonBody* body1;
	dVector point;
	dVector normal;
	dVector force;
	dFloat speed;
} BodyTouchData;

typedef struct BodyTouchingData
{
	const NewtonBody* body0;
	const NewtonBody* body1;
} BodyTouchingData;

typedef struct BodyUntouchData
{
	const NewtonBody* body0;
	const NewtonBody* body1;
} BodyUntouchData;

typedef struct CollisionIteratorData
{
	VALUE faces;
	dFloat scale;
} CollisionIteratorData;

typedef struct CollisionIteratorData2
{
	const NewtonBody* body;
	dVector centre;
	dFloat density;
	dVector force;
	dVector torque;
} CollisionIteratorData2;

typedef struct Hit
{
	const NewtonBody* body;
	dVector point;
	dVector normal;
} Hit;

typedef struct RayData
{
	std::vector<Hit> hits;
} RayData;

typedef struct PickAndDragData
{
	dVector loc_pick_pt;
	dVector dest_pt;
	dFloat stiffness;
	dFloat damper;
} PickAndDragData;

typedef struct BuoyancyData
{
	dVector plane;
	dVector current;
	dFloat density;
	dFloat linear_ratio;
	dFloat angular_ratio;
} BuoyancyData;

typedef struct BodyData
{
	dVector add_force;
	dVector add_torque;
	dVector set_force;
	dVector set_torque;
	bool add_force_state;
	bool add_torque_state;
	bool set_force_state;
	bool set_torque_state;
	dVector add_force2;
	dVector add_torque2;
	dVector set_force2;
	dVector set_torque2;
	bool add_force2_state;
	bool add_torque2_state;
	bool set_force2_state;
	bool set_torque2_state;
	bool dynamic;
	bool bstatic;
	dFloat density;
	dFloat volume;
	dFloat mass;
	dFloat elasticity;
	dFloat softness;
	dFloat static_friction;
	dFloat dynamic_friction;
	bool friction_enabled;
	bool collidable;
	std::map<const NewtonBody*, bool> non_collidable_bodies;
	bool record_touch_data;
	std::map<const NewtonBody*, short> touchers;
	dFloat magnet_force;
	dFloat magnet_range;
	bool magnetic;
	VALUE destructor_proc;
	VALUE user_data;
	dVector matrix_scale;
	dVector default_collision_scale;
	dVector default_collision_offset;
	bool matrix_changed;
	bool gravity_enabled;
	int material_id;
	std::vector<PickAndDragData> pick_and_drag;
	std::vector<BuoyancyData> buoyancy;
} BodyData;

typedef struct JointData
{
	const NewtonWorld* world;
	unsigned int dof;
	JointType jtype;
	ConstraintType ctype;
	dFloat stiffness;
	bool bodies_collidable;
	dFloat breaking_force;
	NewtonJoint* constraint;
	bool connected;
	const NewtonBody* parent;
	const NewtonBody* child;
	dMatrix pin_matrix;
	dMatrix local_matrix0;
	dMatrix local_matrix1;
	VALUE user_data;
	void* cj_data;
	NewtonUserBilateralCallback submit_constraints;
	NewtonUserBilateralGetInfoCallback get_info;
	//void (*submit_constraints)(const NewtonJoint *user_joint, dgFloat32 timestep, int thread_index);
	void (*on_destroy)(JointData* data);
	void (*on_connect)(JointData* data);
	void (*on_disconnect)(JointData* data);
	void (*on_collidable_changed)(JointData* data);
	void (*on_stiffness_changed)(JointData* data);
	void (*on_pin_matrix_changed)(JointData* data);
} JointData;

typedef struct StandardJointData
{
	const NewtonWorld* world;
	JointType jtype;
	dFloat stiffness;
	bool bodies_collidable;
	dFloat breaking_force;
	NewtonJoint* constraint;
	bool connected;
	const NewtonBody* parent;
	const NewtonBody* child;
	dMatrix pin_matrix;
	dMatrix local_matrix0;
	dMatrix local_matrix1;
	VALUE user_data;
	void* cj_data;
	NewtonUserBilateralCallback submit_constraints;
	void (*on_destroy)(StandardJointData* data);
	void (*on_connect)(StandardJointData* data);
	void (*on_disconnect)(StandardJointData* data);
} StandardJointData;

typedef struct WorldData
{
	int solver_model;
	int friction_model;
	dFloat material_thinkness;
	dVector gravity;
	VALUE destructor_proc;
	VALUE user_data;
	std::vector<BodyTouchData> touch_data;
	std::vector<BodyTouchingData> touching_data;
	std::vector<BodyUntouchData> untouch_data;
	std::vector<JointData*> joints_to_disconnect;
	dFloat time;
	dFloat scale;
	dFloat scale3;
	dFloat scale4;
	dFloat inverse_scale;
	dFloat inverse_scale3;
	dFloat inverse_scale4;
	bool gravity_enabled;
	int material_id;
	bool process_info;
	std::vector<const NewtonBody*> temp_cccd_bodies;
} WorldData;

// Variables
extern std::map<const NewtonWorld*, bool> valid_worlds;
extern std::map<const NewtonBody*, bool> valid_bodies;
extern std::map<const NewtonCollision*, dVector> valid_collisions;
extern std::map<JointData*, bool> valid_joints;
extern bool validate_objects;

// Utility Functions
namespace Util {

template<typename T>
T clamp(T value, T min, T max) {
	if (value < min)
		return min;
	else if (value > max)
		return max;
	else
		return value;
}

template<typename T>
T clamp_min(T value, T min) {
	return (value < min) ? min : value;
}

template<typename T>
T clamp_max(T value, T max) {
	return (value > max) ? max : value;
}

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

inline VALUE to_value(const NewtonBody* value) {
	return rb_ll2inum((long long)value);
}

inline VALUE to_value(const NewtonWorld* value) {
	return rb_ll2inum((long long)value);
}

inline VALUE to_value(const NewtonCollision* value) {
	return rb_ll2inum((long long)value);
}

inline VALUE to_value(JointData* value) {
	return rb_ll2inum((long long)value);
}

VALUE to_value(const char* value);
VALUE to_value(const char* value, unsigned int length);

VALUE to_value(const wchar_t* value);
VALUE to_value(const wchar_t* value, long length);

VALUE vector_to_value(const dVector& value, dFloat scale = DEFAULT_SCALE);
VALUE point_to_value(const dVector& value, dFloat scale = DEFAULT_SCALE);
VALUE point_to_value2(const dVector& value);
VALUE matrix_to_value(const dMatrix& value, dFloat scale = DEFAULT_SCALE);
VALUE color_to_value(const dVector& value, dFloat alpha = 1.0f);


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
	return static_cast<short>NUM2INT(value);
}

inline unsigned short value_to_ushort(VALUE value) {
	return static_cast<unsigned short>NUM2UINT(value);
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

dFloat value_to_dFloat2(VALUE value, dFloat scale, dFloat min, dFloat max);

char* value_to_c_str(VALUE value);
wchar_t* value_to_c_str2(VALUE value);

inline unsigned int get_string_length(VALUE value) {
	return RSTRING_LEN(StringValue(value));
}

dVector value_to_vector(VALUE value, dFloat scale = DEFAULT_SCALE);
dVector value_to_point(VALUE value, dFloat scale = DEFAULT_SCALE);
dVector value_to_point2(VALUE value);
dMatrix value_to_matrix(VALUE value, dFloat scale = DEFAULT_SCALE);
dVector value_to_color(VALUE value);

const NewtonWorld* value_to_world(VALUE value);
const NewtonBody* value_to_body(VALUE value);
const NewtonCollision* value_to_collision(VALUE value);
JointData* value_to_joint(VALUE value);
JointData* value_to_joint2(VALUE value, JointType joint_type);

dFloat get_vector_magnitude(const dVector& vector);
void set_vector_magnitude(dVector& vector, dFloat magnitude);
void normalize_vector(dVector& vector);
bool vectors_identical(const dVector& a, const dVector& b);
bool is_vector_valid(const dVector& vector);

bool is_matrix_uniform(const dMatrix& matrix);
bool is_matrix_flat(const dMatrix& matrix);
bool is_matrix_flipped(const dMatrix& matrix);
dVector get_matrix_scale(const dMatrix& matrix);
void set_matrix_scale(dMatrix& matrix, const dVector& scale);
void extract_matrix_scale(dMatrix& matrix);
dMatrix matrix_from_pin_dir(const dVector& pos, const dVector& dir);
dMatrix rotate_matrix_to_dir(const dMatrix& matrix, const dVector& dir);
dVector rotate_vector(const dVector& vector, const dVector& normal, const dFloat& angle);

bool is_world_valid(const NewtonWorld* address);
bool is_body_valid(const NewtonBody* address);
bool is_collision_valid(const NewtonCollision* address);
bool is_joint_valid(JointData* address);
bool is_collision_convex(const NewtonCollision* address);

bool bodies_collidable(const NewtonBody* body0, const NewtonBody* body1);
bool bodies_aabb_overlap(const NewtonBody* body0, const NewtonBody* body1);

void validate_two_bodies(const NewtonBody* body1, const NewtonBody* body2);

VALUE call_proc(VALUE v_proc);
VALUE rescue_proc(VALUE v_args, VALUE v_exception);

template<typename T>
inline bool is_number(T number) {
	return number == number ? true : false;
}

} /* namespace Util */

// Main
void Init_msp_util(VALUE mMSPhysics);

#endif	/* MSP_UTIL_H */
