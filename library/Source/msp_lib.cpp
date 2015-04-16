/*
The C++ version of MSPhysics implements the following:
  - Newton Dynamics Physics SDK by Juleo Jerez and Alain Suero.
  - V-HACD 2.2 by Khaled Mamou.
  - Exact Convex Decomposition by Fredo6. Not implemented yet.

Implementation by Anton Synytsia

Do the following when updating NewtonDynamics:
1. Change DG_MAX_MIN_VOLUME to 1.0e-8f.
   Change DG_MINK_VERTEX_ERR to 1.0e-8f.
   Files: dgCollisionConvex.cpp, dgCollisionCompound.cpp
2. Change min and max timestep to 1/30 and 1/1200.
   File: NewtonClass.h
3. Set DG_MINIMUM_MASS to 1.0e-8f.
   File: dgBody.h
4. Change min and max inertia to 0.0001 and 10000.
   File: dgBody.cpp, line 500.

To Do:
- body_get_connected_joints(body)
- body_recalculate_volume(body)
- body_get_moments_of_inertia(body)
- body_set_moments_of_inertia(body, ixx, iyy, izz)
- Joints
- User Mesh
- Fractured Compounds
- Vehicles
- Ragdols

*/

#include "NewtonStdAfx.h"
#include "RubyUtils/RubyUtils.h"
#include <stdio.h>
#include "Newton.h"
#include "NewtonClass.h"
#include "dVector.h"
#include "dMatrix.h"
#include <map>
#include <vector>
#include <math.h>
#include "VHACD.h"

#ifndef DBL2NUM
	#define DBL2NUM(dbl) rb_float_new(dbl)
#endif

typedef struct BodyTouchData
{
	long body0;
	long body1;
	dVector point;
	dVector normal;
	dVector force;
	dFloat speed;
} BodyTouchData;

typedef struct BodyTouchingData
{
	long body0;
	long body1;
} BodyTouchingData;

typedef struct BodyUntouchData
{
	long body0;
	long body1;
} BodyUntouchData;

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
	dFloat time;
} WorldData;

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
	std::map<long, bool> non_collidable_bodies;
	bool record_touch_data;
	std::map<long, short> touchers;
	dFloat magnet_force;
	dFloat magnet_range;
	bool magnetic;
	VALUE destructor_proc;
	VALUE user_data;
	dVector matrix_scale;
	bool matrix_changed;
} BodyData;

typedef struct JointData
{
	int solver;
	int max_contact_joints;
	dFloat stiffness;
	bool bodies_collidable;
	int type;
	void* data;
	VALUE destructor_proc;
	VALUE user_data;
} JointData;

typedef struct HingeJointData
{
	dFloat min;
	dFloat max;
	dFloat friction;
	bool enable_limits;
	dFloat angle;
	dFloat omega;
} HingeJointData;

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

const dFloat INCH_TO_METER				= 0.0254f;
const dFloat METER_TO_INCH				= 39.3701f;
const dFloat DEFAULT_DENSITY			= 700.0f;
const dVector DEFAULT_GRAVITY			= dVector(0.0f, 0.0f, -9.8f);
const dFloat DEFAULT_ELASTICITY			= 0.40f;
const dFloat DEFAULT_SOFTNESS			= 0.10f;
const dFloat DEFAULT_STATIC_FRICTION	= 0.90f;
const dFloat DEFAULT_DYNAMIC_FRICTION	= 0.50f;
const bool DEFAULT_ENABLE_FRICTION		= true;
const long NON_COL_CONTACTS_CAPACITY	= 30;
const int DEFAULT_SOLVER_MODEL			= 0;
const int DEFAULT_FRICTION_MODEL		= 0;
const dFloat DEFAULT_MATERIAL_THICKNESS = 1.0f / 256.0f;

const dFloat DEFAULT_CONTACT_MERGE_TOLERANCE = 1.0e-4f;
const dFloat MIN_TOUCH_DISTANCE = 0.001f;

const dFloat MIN_MASS		= 1.0e-8f;
const dFloat MAX_MASS		= 1.0e14f;
const dFloat MIN_VOLUME		= 1.0e-8f;
const dFloat MIN_DENSITY	= 1.0e-8f;

std::map<long, bool> valid_worlds;
std::map<long, bool> valid_bodies;
std::map<long, bool> valid_collisions;
std::map<long, bool> valid_joints;

const int ID_HINGE				= 0;
const int ID_MOTOR				= 1;
const int ID_SERVO				= 2;
const int ID_SLIDER				= 3;
const int ID_PISTON				= 4;
const int ID_CORKSCREW			= 5;
const int ID_BALL_AND_SOCKET	= 6;
const int ID_UNIVERSAL			= 7;
const int ID_SPRING				= 8;
const int ID_UP_VECTOR			= 9;
const int ID_FIXED				= 10;

VALUE suGeom, suPoint3d, suVector3d, suTransformation;


// *********************************************************************************************************************
//
// Helper Functions
//
// *********************************************************************************************************************

bool c_is_world_valid(long world)
{
	return valid_worlds.find(world) != valid_worlds.end();
}

bool c_is_body_valid(long body)
{
	return valid_bodies.find(body) != valid_bodies.end();
}

bool c_is_collision_valid(long collision)
{
	return valid_collisions.find(collision) != valid_collisions.end();
}

bool c_is_collision_convex(long collision)
{
	return NewtonCollisionGetType((NewtonCollision*)collision) < 10;
}

bool c_is_joint_valid(long joint)
{
	return valid_joints.find(joint) != valid_joints.end();
}

const NewtonWorld* c_value_to_world(VALUE v_world)
{
	long world = NUM2LONG(v_world);
	if( valid_worlds.find(world) == valid_worlds.end() )
		rb_raise(rb_eTypeError, "World address is invalid!");
	return (NewtonWorld*)world;
}

const NewtonBody* c_value_to_body(VALUE v_body)
{
	long body = NUM2LONG(v_body);
	if( valid_bodies.find(body) == valid_bodies.end() )
		rb_raise(rb_eTypeError, "Body address is invalid!");
	return (NewtonBody*)body;
}

const NewtonCollision* c_value_to_collision(VALUE v_collision)
{
	long collision = NUM2LONG(v_collision);
	if( valid_collisions.find(collision) == valid_collisions.end() )
		rb_raise(rb_eTypeError, "Collision address is invalid!");
	return (NewtonCollision*)collision;
}

dFloat c_clamp_dfloat(dFloat value, dFloat min, dFloat max)
{
	dFloat x = value;
	if( min != NULL && x < min ) x = min;
	if( max != NULL && x > max ) x = max;
	return x;
}

double c_clamp_double(double value, double min, double max)
{
	double x = value;
	if( min != NULL && x < min ) x = min;
	if( max != NULL && x > max ) x = max;
	return x;
}

float c_clamp_float(float value, float min, float max)
{
	float x = value;
	if( min != NULL && x < min ) x = min;
	if( max != NULL && x > max ) x = max;
	return x;
}

int c_clamp_int(int value, int min, int max)
{
	int x = value;
	if( min != NULL && x < min ) x = min;
	if( max != NULL && x > max ) x = max;
	return x;
}

unsigned int c_clamp_uint(unsigned int value, unsigned int min, unsigned int max)
{
	unsigned int x = value;
	if( min != NULL && x < min ) x = min;
	if( max != NULL && x > max ) x = max;
	return x;
}

long c_clamp_long(long value, long min, long max)
{
	long x = value;
	if( min != NULL && x < min ) x = min;
	if( max != NULL && x > max ) x = max;
	return x;
}

VALUE c_point3d_to_value(dVector point)
{
	return rb_funcall( suPoint3d, rb_intern("new"), 3, DBL2NUM(point[0] * METER_TO_INCH), DBL2NUM(point[1] * METER_TO_INCH), DBL2NUM(point[2] * METER_TO_INCH) );
}

dVector c_value_to_point3d(VALUE v_point)
{
	return dVector(
		(dFloat)NUM2DBL(rb_funcall( v_point, rb_intern("x"), 0 )) * INCH_TO_METER,
		(dFloat)NUM2DBL(rb_funcall( v_point, rb_intern("y"), 0 )) * INCH_TO_METER,
		(dFloat)NUM2DBL(rb_funcall( v_point, rb_intern("z"), 0 )) * INCH_TO_METER);
}

dFloat c_vector_get_magnitude(dVector vector)
{
	return sqrt(vector.m_x * vector.m_x + vector.m_y * vector.m_y + vector.m_z * vector.m_z);
}

dVector c_vector_set_magnitude(dVector vector, dFloat magnitude)
{
	dFloat m = c_vector_get_magnitude(vector);
	if( magnitude == 0.0f || m == 0.0f )
		return dVector(vector);
	dFloat r = magnitude / m;
	return dVector(vector.m_x * r, vector.m_y * r, vector.m_z * r, vector.m_w);
}

VALUE c_vector3d_to_value(dVector vector)
{
	return rb_funcall( suVector3d, rb_intern("new"), 3, DBL2NUM(vector[0]), DBL2NUM(vector[1]), DBL2NUM(vector[2]) );
}

dVector c_value_to_vector3d(VALUE v_vector)
{
	return dVector(
		(dFloat)NUM2DBL(rb_funcall( v_vector, rb_intern("x"), 0 )),
		(dFloat)NUM2DBL(rb_funcall( v_vector, rb_intern("y"), 0 )),
		(dFloat)NUM2DBL(rb_funcall( v_vector, rb_intern("z"), 0 )));
}

bool c_matrix_is_uniform(dMatrix matrix)
{
	return ( matrix.m_front % matrix.m_up == 0.0f && matrix.m_front % matrix.m_right == 0.0f && matrix.m_up % matrix.m_right == 0.0f );
}

bool c_matrix_is_flipped(dMatrix matrix)
{
	return ( (matrix.m_front * matrix.m_up) % matrix.m_right < 0 );
}

bool c_matrix_is_flat(dMatrix matrix)
{
	return ( c_vector_get_magnitude(matrix.m_front) == 0.0f || c_vector_get_magnitude(matrix.m_up) == 0.0f || c_vector_get_magnitude(matrix.m_right) == 0.0f );
}

dVector c_matrix_get_scale(dMatrix matrix)
{
	return dVector(
		c_vector_get_magnitude(matrix.m_front),
		c_vector_get_magnitude(matrix.m_up),
		c_vector_get_magnitude(matrix.m_right));
}

dMatrix c_matrix_set_scale(dMatrix matrix, dVector scale)
{
	return dMatrix(
		c_vector_set_magnitude(matrix.m_front, scale.m_x),
		c_vector_set_magnitude(matrix.m_up, scale.m_y),
		c_vector_set_magnitude(matrix.m_right, scale.m_z),
		matrix.m_posit);
}

dMatrix c_matrix_extract_scale(dMatrix matrix)
{
	return dMatrix(
		c_vector_set_magnitude(matrix.m_front, 1.0f),
		c_vector_set_magnitude(matrix.m_up, 1.0f),
		c_vector_set_magnitude(matrix.m_right, 1.0f),
		matrix.m_posit);
}

VALUE c_matrix_to_value(dMatrix matrix)
{
	VALUE v_matrix = rb_ary_new2(16);
	for( int i = 0; i < 4; i++ )
		for( int j = 0; j < 4; j++ )
			rb_ary_store( v_matrix, i*4+j, DBL2NUM(matrix[i][j] * (i == 3 && j < 3 ? METER_TO_INCH : 1)) );
	return rb_funcall( suTransformation, rb_intern("new"), 1, v_matrix );
}

dMatrix c_value_to_matrix(VALUE v_matrix)
{
	if( rb_class_of(v_matrix) != suTransformation )
		v_matrix = rb_funcall( suTransformation, rb_intern("new"), 1, v_matrix );
	v_matrix = rb_funcall( v_matrix, rb_intern("to_a"), 0 );
	dMatrix matrix;
	for( int i = 0; i < 4; i++ )
		for( int j = 0; j < 4; j++ )
			matrix[i][j] = (dFloat)NUM2DBL( rb_ary_entry(v_matrix, i*4+j) ) * (i == 3 && j < 3 ? INCH_TO_METER : 1);
	if( !c_matrix_is_uniform(matrix) )
		rb_raise(rb_eTypeError, "The specified matrix is not uniform. Some or all matrix axis are not perpendicular to each other.");
	if( c_matrix_is_flat(matrix) )
		rb_raise(rb_eTypeError, "The specified matrix has one or more of its axis scaled to zero. Flat matrices are not acceptable!");
	return matrix;
}

void c_body_clear_non_collidable_bodies(const NewtonBody* body)
{
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	for( std::map<long,bool>::iterator it = data->non_collidable_bodies.begin(); it != data->non_collidable_bodies.end(); ++it )
	{
		long other_body = it->first;
		if( valid_bodies.find(other_body) != valid_bodies.end() )
		{
			BodyData* other_data = (BodyData*)NewtonBodyGetUserData((NewtonBody*)other_body);
			if( other_data->non_collidable_bodies.find((long)body) != other_data->non_collidable_bodies.end() )
				other_data->non_collidable_bodies.erase((long)body);
		}
	}
	data->non_collidable_bodies.clear();
}

bool c_bodies_collidable(const NewtonBody* body0, const NewtonBody* body1)
{
	BodyData* data0 = (BodyData*)NewtonBodyGetUserData(body0);
	BodyData* data1 = (BodyData*)NewtonBodyGetUserData(body1);
	return data0->collidable == true && data1->collidable == true && data0->non_collidable_bodies.find((long)body1) == data0->non_collidable_bodies.end();
}

bool c_bodies_aabb_overlap(const NewtonBody* body0, const NewtonBody* body1)
{
	dVector minA;
	dVector maxA;
	dVector minB;
	dVector maxB;
	NewtonBodyGetAABB(body0, &minA[0], &maxA[0]);
	NewtonBodyGetAABB(body1, &minB[0], &maxB[0]);
	for( int i = 0; i < 3; i++ )
	{
		if( (minA[i] >= minB[i] && minA[i] <= maxB[i]) ||
			(maxA[i] >= minB[i] && maxA[i] <= maxB[i]) ||
			(minB[i] >= minA[i] && minB[i] <= maxA[i]) ||
			(maxB[i] >= minA[i] && maxB[i] <= maxA[i]) ) continue;
		return false;
	}
	return true;
}

void c_update_magnets(const NewtonWorld* world)
{
	dMatrix matrix;
	dVector com;
	dMatrix other_matrix;
	dVector other_com;
	dVector dir;
	for( const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body) )
	{
		BodyData* data = (BodyData*)NewtonBodyGetUserData((NewtonBody*)body);
		if( data->magnet_force == 0.0f || data->magnet_range == 0.0f ) continue;
		NewtonBodyGetMatrix(body, &matrix[0][0]);
		NewtonBodyGetCentreOfMass(body, &com[0]);
		com = matrix.TransformVector(com);
		for( const NewtonBody* other_body = NewtonWorldGetFirstBody(world); other_body; other_body = NewtonWorldGetNextBody(world, other_body) )
		{
			if( other_body == body ) continue;
			BodyData* other_data = (BodyData*)NewtonBodyGetUserData((NewtonBody*)other_body);
			if( other_data->magnetic == false ) continue;
			NewtonBodyGetMatrix(other_body, &other_matrix[0][0]);
			NewtonBodyGetCentreOfMass(other_body, &other_com[0]);
			other_com = other_matrix.TransformVector(other_com);
			dir = other_com - com;
			dFloat dist = sqrt(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
			if( dist == 0.0f || dist >= data->magnet_range ) continue;
			dir = dir.Scale( (data->magnet_range - dist) * data->magnet_force / (data->magnet_range * dist) );
			other_data->add_force -= dir;
			other_data->add_force_state = true;
			// For every action there is an equal and opposite reaction!
			data->add_force += dir;
			data->add_force_state = true;
		}
	}
}

void c_process_touch_events(const NewtonWorld* world)
{
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);

	// Generate onTouch events for non-collidable bodies.
	for( const NewtonBody* body0 = NewtonWorldGetFirstBody(world); body0; body0 = NewtonWorldGetNextBody(world, body0) )
	{
		BodyData* body0_data = (BodyData*)NewtonBodyGetUserData(body0);
		if( body0_data->record_touch_data == false ) continue;
		NewtonCollision* colA = NewtonBodyGetCollision(body0);
		NewtonCollision* colB;
		dMatrix matA;
		dMatrix matB;
		NewtonBodyGetMatrix(body0, &matA[0][0]);
		dFloat points[3];
		dFloat normals[3];
		dFloat penetrations[3];
		long long attrA[1];
		long long attrB[1];
		for( const NewtonBody* body1 = NewtonWorldGetFirstBody(world); body1; body1 = NewtonWorldGetNextBody(world, body1) )
		{
			if( body0 == body1 || c_bodies_collidable(body0, body1) == true || c_bodies_aabb_overlap(body0, body1) == false ) continue;
			colB = NewtonBodyGetCollision(body1);
			NewtonBodyGetMatrix(body1, &matB[0][0]);
			int count = NewtonCollisionCollide(world, 1, colA, &matA[0][0], colB, &matB[0][0], points, normals, penetrations, attrA, attrB, 0);
			if( count == 0 ) continue;
			BodyTouchData touch_data;
			touch_data.body0 = (long)body0;
			touch_data.body1 = (long)body1;
			touch_data.point = dVector(points);
			touch_data.normal = dVector(normals);
			touch_data.force = dVector(0.0f, 0.0f, 0.0f);
			touch_data.speed = 0.0f;
			world_data->touch_data.push_back(touch_data);
			body0_data->touchers[(long)body1] = 0;
		}
	}

	// Generate onTouching and onUntouch events for all bodies with touchers.
	for( const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body) )
	{
		BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
		if( body_data->record_touch_data == false || body_data->touchers.empty() == true ) continue;
		NewtonCollision* colA = NewtonBodyGetCollision(body);
		dMatrix matrixA;
		NewtonBodyGetMatrix(body, &matrixA[0][0]);
		std::vector<long> to_erase;
		for( std::map<long,short>::iterator it = body_data->touchers.begin(); it != body_data->touchers.end(); ++it )
		{
			if( it->second == 0 )
			{
				body_data->touchers[it->first] = 1;
				continue;
			}
			bool touching = false;
			if( it->second == 1 )
			{
				NewtonCollision* colB = NewtonBodyGetCollision((NewtonBody*)(it->first));
				dMatrix matrixB;
				NewtonBodyGetMatrix((NewtonBody*)(it->first), &matrixB[0][0]);
				touching = NewtonCollisionIntersectionTest(world, colA, &matrixA[0][0], colB, &matrixB[0][0], 0) == 1;
				if( touching == false && NewtonCollisionGetType(colA) < 11 && NewtonCollisionGetType(colB) < 11 )
				{
					dVector pointA;
					dVector pointB;
					dVector normalAB;
					NewtonCollisionClosestPoint(world, colA, &matrixA[0][0], colB, &matrixB[0][0], &pointA[0], &pointB[0], &normalAB[0], 0);
					dVector diff = pointB - pointA;
					dFloat dist = sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]) - world_data->material_thinkness;
					if( dist < MIN_TOUCH_DISTANCE ) touching = true;
				}
			}
			else if( it->second == 2 )
			{
				touching = true;
				body_data->touchers[it->first] = 1;
			}
			if( touching )
			{
				BodyTouchingData touching_data;
				touching_data.body0 = (long)body;
				touching_data.body1 = it->first;
				world_data->touching_data.push_back(touching_data);
			}
			else
			{
				BodyUntouchData untouch_data;
				untouch_data.body0 = (long)body;
				untouch_data.body1 = it->first;
				world_data->untouch_data.push_back(untouch_data);
				to_erase.push_back(it->first);
			}
		}
		for( unsigned int i = 0; i < to_erase.size(); i++ )
			body_data->touchers.erase(to_erase[i]);
	}
}

void c_clear_touch_events(const NewtonWorld* world)
{
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	data->touch_data.clear();
	data->touching_data.clear();
	data->untouch_data.clear();
}

void c_clear_matrix_changed(const NewtonWorld* world)
{
	for( const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body) )
	{
		BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
		data->matrix_changed = false;
	}
}

static VALUE call_proc( VALUE v_proc )
{
	if( rb_class_of(v_proc) != rb_cProc )
		rb_raise(rb_eTypeError, "Expected Proc object!");
	return rb_funcall( v_proc, rb_intern("call"), 0 );
}

static VALUE rescue_proc( VALUE v_args, VALUE v_exception )
{
	VALUE v_message = rb_funcall(v_exception, rb_intern("inspect"), 0);
	rb_funcall(rb_stdout, rb_intern("puts"), 1, v_message);
	VALUE v_backtrace = rb_funcall(v_exception, rb_intern("backtrace"), 0);
	rb_funcall(rb_stdout, rb_intern("puts"), 1, rb_ary_entry(v_backtrace, 0));
	return Qnil;
}


// *********************************************************************************************************************
//
// Callbacks
//
// *********************************************************************************************************************

void world_destructor_callback(const NewtonWorld* const world)
{
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	valid_worlds.erase((long)world);
	data->touch_data.clear();
	data->touching_data.clear();
	data->untouch_data.clear();
	if( RARRAY_LEN(data->destructor_proc) == 1 )
		rb_rescue2(RUBY_METHOD_FUNC(call_proc), rb_ary_entry(data->destructor_proc, 0), RUBY_METHOD_FUNC(rescue_proc), Qnil, rb_eException, (VALUE)0);
	rb_ary_clear(data->destructor_proc);
	rb_ary_clear(data->user_data);
	rb_gc_unregister_address(&data->destructor_proc);
	rb_gc_unregister_address(&data->user_data);
	delete data;
}

void body_destructor_callback(const NewtonBody* const body)
{
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	c_body_clear_non_collidable_bodies(body);
	valid_bodies.erase((long)body);
	data->non_collidable_bodies.clear();
	data->touchers.clear();
	if( RARRAY_LEN(data->destructor_proc) == 1 )
		rb_rescue2(RUBY_METHOD_FUNC(call_proc), rb_ary_entry(data->destructor_proc, 0), RUBY_METHOD_FUNC(rescue_proc), Qnil, rb_eException, (VALUE)0);
	rb_ary_clear(data->destructor_proc);
	rb_ary_clear(data->user_data);
	rb_gc_unregister_address(&data->destructor_proc);
	rb_gc_unregister_address(&data->user_data);
	delete data;
}

void body_transform_callback(const NewtonBody* const body, const dFloat* const matrix, int thread_index)
{
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->matrix_changed = true;
}

void collision_copy_constructor_callback(const NewtonWorld* const world, NewtonCollision* const collision, const NewtonCollision* const source_collision)
{
	valid_collisions[(long)collision] = true;
}

void collision_destructor_callback(const NewtonWorld* const world, const NewtonCollision* const collision)
{
	valid_collisions.erase((long)collision);
}

void joint_destructor_callback(const NewtonJoint* const joint)
{
	JointData* data = (JointData*)NewtonJointGetUserData(joint);
	valid_joints.erase((long)joint);
	if( RARRAY_LEN(data->destructor_proc) == 1 )
		rb_rescue2(RUBY_METHOD_FUNC(call_proc), rb_ary_entry(data->destructor_proc, 0), RUBY_METHOD_FUNC(rescue_proc), Qnil, rb_eException, (VALUE)0);
	rb_ary_clear(data->destructor_proc);
	rb_ary_clear(data->user_data);
	rb_gc_unregister_address(&data->destructor_proc);
	rb_gc_unregister_address(&data->user_data);
	delete data;
}

void force_and_torque_callback(const NewtonBody* const body, dFloat timestep, int thread_index)
{
	NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	dFloat mass, ixx, iyy, izz;
	NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);

	dVector force = world_data->gravity.Scale(mass);
	NewtonBodySetForce(body, &force[0]);

	if( body_data->add_force_state == true )
	{
		NewtonBodyAddForce(body, &body_data->add_force[0]);
		body_data->add_force[0] = 0.0f;
		body_data->add_force[1] = 0.0f;
		body_data->add_force[2] = 0.0f;
		body_data->add_force_state = false;
	}
	if( body_data->add_torque_state == true )
	{
		NewtonBodyAddTorque(body, &body_data->add_torque[0]);
		body_data->add_torque[0] = 0.0f;
		body_data->add_torque[1] = 0.0f;
		body_data->add_torque[2] = 0.0f;
		body_data->add_torque_state = false;
	}
	if( body_data->set_force_state == true )
	{
		NewtonBodySetForce(body, &body_data->set_force[0]);
		body_data->set_force[0] = 0.0f;
		body_data->set_force[1] = 0.0f;
		body_data->set_force[2] = 0.0f;
		body_data->set_force_state = false;
	}
	if( body_data->set_torque_state == true )
	{
		NewtonBodySetTorque(body, &body_data->set_torque[0]);
		body_data->set_torque[0] = 0.0f;
		body_data->set_torque[1] = 0.0f;
		body_data->set_torque[2] = 0.0f;
		body_data->set_torque_state = false;
	}
}

int aabb_overlap_callback(const NewtonMaterial* const material, const NewtonBody* const body0, const NewtonBody* const body1, int thread_index)
{
	BodyData* data0 = (BodyData*)NewtonBodyGetUserData(body0);
	BodyData* data1 = (BodyData*)NewtonBodyGetUserData(body1);
	if( data0->collidable == false || data1->collidable == false )
		return 0;
	if( data0->non_collidable_bodies.find((long)body1) != data0->non_collidable_bodies.end() )
		return 0;
	if( data1->non_collidable_bodies.find((long)body0) != data1->non_collidable_bodies.end() )
		return 0;
	if( NewtonBodyGetFreezeState(body0) == 1 && NewtonBodyGetFreezeState(body1) == 1 )
		return 0;
	return 1;
}

void contact_callback(const NewtonJoint* const contact_joint, dFloat timestep, int thread_index)
{
	NewtonBody* body0 = NewtonJointGetBody0(contact_joint);
	NewtonBody* body1 = NewtonJointGetBody1(contact_joint);
	BodyData* data0 = (BodyData*)NewtonBodyGetUserData(body0);
	BodyData* data1 = (BodyData*)NewtonBodyGetUserData(body1);
	if( NewtonBodyGetFreezeState(body0) == 1 )
		NewtonBodySetFreezeState(body0, 0);
	if( NewtonBodyGetFreezeState(body1) == 1 )
		NewtonBodySetFreezeState(body1, 0);
	for( void* contact = NewtonContactJointGetFirstContact(contact_joint); contact; contact = NewtonContactJointGetNextContact(contact_joint, contact) )
	{
		NewtonMaterial* material = NewtonContactGetMaterial(contact);
		if( data0->friction_enabled && data1->friction_enabled )
		{
			dFloat sfc = (data0->static_friction + data1->static_friction)/2.0f;
			dFloat kfc = (data0->dynamic_friction + data1->dynamic_friction)/2.0f;
			NewtonMaterialSetContactFrictionCoef(material, sfc, kfc, 0);
			NewtonMaterialSetContactFrictionCoef(material, sfc, kfc, 1);
		}
		else
		{
			NewtonMaterialSetContactFrictionState(material, 0, 0);
			NewtonMaterialSetContactFrictionState(material, 0, 1);
		}
		dFloat cor = (data0->elasticity + data1->elasticity)/2.0f;
		dFloat sft = (data0->softness + data1->softness)/2.0f;
		NewtonMaterialSetContactElasticity(material, cor);
		NewtonMaterialSetContactSoftness(material, sft);
	}
	NewtonWorld* world = NewtonBodyGetWorld(body0);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	void* contact = NewtonContactJointGetFirstContact(contact_joint);
	NewtonMaterial* material = NewtonContactGetMaterial(contact);
	if( data0->record_touch_data )
	{
		if( data0->touchers.find((long)body1) == data0->touchers.end() )
		{
			dVector point;
			dVector normal;
			dVector force;
			NewtonMaterialGetContactPositionAndNormal(material, body0, &point[0], &normal[0]);
			NewtonMaterialGetContactForce(material, body0, &force[0]);
			BodyTouchData touch_data;
			touch_data.body0 = (long)body0;
			touch_data.body1 = (long)body1;
			touch_data.point = point;
			touch_data.normal = normal;
			touch_data.force = force;
			touch_data.speed = NewtonMaterialGetContactNormalSpeed(material);
			world_data->touch_data.push_back(touch_data);
			data0->touchers[(long)body1] = 0;
		}
		else
			data0->touchers[long(body1)] = 2;
	}
	if( data1->record_touch_data )
	{
		if( data1->touchers.find((long)body0) == data1->touchers.end() )
		{
			dVector point;
			dVector normal;
			dVector force;
			NewtonMaterialGetContactPositionAndNormal(material, body1, &point[0], &normal[0]);
			NewtonMaterialGetContactForce(material, body1, &force[0]);
			BodyTouchData touch_data;
			touch_data.body0 = (long)body1;
			touch_data.body1 = (long)body0;
			touch_data.point = point;
			touch_data.normal = normal;
			touch_data.force = force;
			touch_data.speed = NewtonMaterialGetContactNormalSpeed(material);
			world_data->touch_data.push_back(touch_data);
			data1->touchers[(long)body0] = 0;
		}
		else
			data1->touchers[long(body0)] = 2;
	}
}

dFloat ray_filter_callback(const NewtonBody* const body, const NewtonCollision* const shape_hit, const dFloat* const hit_contact, const dFloat* const hit_normal, dLong collision_id, void* const user_data, dFloat intersect_param)
{
	Hit* hit = (Hit*)user_data;
	hit->body = body;
	hit->point = dVector(hit_contact);
	hit->normal = dVector(hit_normal);
	return intersect_param;
}

dFloat continuous_ray_filter_callback(const NewtonBody* const body, const NewtonCollision* const shape_hit, const dFloat* const hit_contact, const dFloat* const hit_normal, dLong collision_id, void* const user_data, dFloat intersect_param)
{
	RayData* ray_data = (RayData*)user_data;
	Hit hit;
	hit.body = body;
	hit.point = dVector(hit_contact);
	hit.normal = dVector(hit_normal);
	ray_data->hits.push_back(hit);
	return 1.0f;
}

void collision_iterator(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id)
{
	VALUE v_faces = (VALUE)user_data;
	VALUE v_face = rb_ary_new2(vertex_count);
	for( int i = 0; i < vertex_count; i ++ )
	{
		dVector vertex(face_array[i*3+0], face_array[i*3+1], face_array[i*3+2]);
		rb_ary_store(v_face, i, c_point3d_to_value(vertex));
	}
	rb_ary_push(v_faces, v_face);
}

int body_iterator(const NewtonBody* const body, void* const user_data)
{
	rb_ary_push((VALUE)user_data, LONG2NUM((long)body));
	return 1;
}

void hinge_joint_bilateral_callback(const NewtonJoint* const user_joint, dFloat timestep, int thread_index)
{
}

void hinge_joint_bilateral_get_info_callback(const NewtonJoint* const user_joint, NewtonJointRecord* const info)
{
}


// *********************************************************************************************************************
//
// Newton Interface
//
// *********************************************************************************************************************

VALUE get_version(VALUE self)
{
	return INT2NUM(NewtonWorldGetVersion());
}

VALUE get_float_size(VALUE self)
{
	return INT2NUM(NewtonWorldFloatSize());
}

VALUE get_memory_used(VALUE self)
{
	return INT2FIX(NewtonGetMemoryUsed());
}

VALUE get_all_worlds(VALUE self)
{
	VALUE v_worlds = rb_ary_new2((long)valid_worlds.size());
	int count = 0;
	for( std::map<long,bool>::iterator it=valid_worlds.begin(); it!=valid_worlds.end(); ++it )
	{
		rb_ary_store(v_worlds, count, LONG2NUM(it->first));
		count++;
	}
	return v_worlds;
}

VALUE get_all_bodies(VALUE self)
{
	VALUE v_bodies = rb_ary_new2((long)valid_bodies.size());
	int count = 0;
	for( std::map<long,bool>::iterator it=valid_bodies.begin(); it!=valid_bodies.end(); ++it )
	{
		rb_ary_store(v_bodies, count, LONG2NUM(it->first));
		count++;
	}
	return v_bodies;
}


// *********************************************************************************************************************
//
// World Interface
//
// *********************************************************************************************************************

VALUE world_is_valid(VALUE self, VALUE v_world)
{
	return c_is_world_valid(NUM2LONG(v_world)) ? Qtrue : Qfalse;
}

VALUE world_create(VALUE self)
{
	NewtonWorld* world = NewtonCreate();
	NewtonInvalidateCache(world);

	WorldData *data = new WorldData;
	valid_worlds[(long)world] = true;
	data->solver_model = DEFAULT_SOLVER_MODEL;
	data->friction_model = DEFAULT_FRICTION_MODEL;
	data->material_thinkness = DEFAULT_MATERIAL_THICKNESS;
	data->gravity = dVector(DEFAULT_GRAVITY);
	data->destructor_proc = rb_ary_new();
	data->user_data = rb_ary_new();
	data->touch_data;
	data->touching_data;
	data->untouch_data;
	data->time = 0.0f;
	NewtonWorldSetUserData(world, data);
	rb_gc_register_address(&data->destructor_proc);
	rb_gc_register_address(&data->user_data);

	NewtonSetContactMergeTolerance(world, DEFAULT_CONTACT_MERGE_TOLERANCE);

	int id = NewtonMaterialGetDefaultGroupID(world);

	NewtonMaterialSetSurfaceThickness(world, id, id, DEFAULT_MATERIAL_THICKNESS);
	NewtonMaterialSetDefaultFriction(world, id, id, DEFAULT_STATIC_FRICTION, DEFAULT_DYNAMIC_FRICTION);
	NewtonMaterialSetDefaultElasticity(world, id, id, DEFAULT_ELASTICITY);
	NewtonMaterialSetDefaultSoftness(world, id, id, DEFAULT_SOFTNESS);

	NewtonSetSolverModel(world, DEFAULT_SOLVER_MODEL);
	NewtonSetFrictionModel(world, DEFAULT_FRICTION_MODEL);

	NewtonMaterialSetCollisionCallback(world, id, id, NULL, aabb_overlap_callback, contact_callback);
	NewtonWorldSetDestructorCallback(world, world_destructor_callback);
	NewtonWorldSetCollisionConstructorDestructorCallback(world, collision_copy_constructor_callback, collision_destructor_callback);
	return LONG2NUM((long)world);
}

VALUE world_destroy(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	NewtonDestroy(world);
	return Qnil;
}

VALUE world_get_max_threads_count(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	return INT2FIX( NewtonGetMaxThreadsCount(world) );
}

VALUE world_get_threads_count(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	return INT2FIX( NewtonGetThreadsCount(world) );
}

VALUE world_set_threads_count(VALUE self, VALUE v_world, VALUE v_count)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	NewtonSetThreadsCount(world, c_clamp_int(NUM2INT(v_count), 1, NULL));
	return INT2FIX( NewtonGetThreadsCount(world) );
}

VALUE world_destroy_all_bodies(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	int count = NewtonWorldGetBodyCount(world);
	NewtonDestroyAllBodies(world);
	return INT2FIX(count);
}

VALUE world_get_body_count(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	return LONG2NUM( NewtonWorldGetBodyCount(world) );
}

VALUE world_get_constraint_count(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	return LONG2NUM( NewtonWorldGetConstraintCount(world) );
}

VALUE world_update(VALUE self, VALUE v_world, VALUE v_timestep)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat timestep = c_clamp_float((dFloat)NUM2DBL(v_timestep), DG_MIN_TIMESTEP, DG_MAX_TIMESTEP);
	c_clear_matrix_changed(world);
	c_clear_touch_events(world);
	c_update_magnets(world);
	NewtonUpdate(world, timestep);
	c_process_touch_events(world);
	data->time += timestep;
	return DBL2NUM( timestep );
}

VALUE world_update_async(VALUE self, VALUE v_world, VALUE v_timestep)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat timestep = c_clamp_float((dFloat)NUM2DBL(v_timestep), DG_MIN_TIMESTEP, DG_MAX_TIMESTEP);
	c_clear_matrix_changed(world);
	c_clear_touch_events(world);
	c_update_magnets(world);
	NewtonUpdateAsync(world, timestep);
	c_process_touch_events(world);
	data->time += timestep;
	return DBL2NUM( timestep );
}

VALUE world_get_gravity(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return c_vector3d_to_value( data->gravity );
}

VALUE world_set_gravity(VALUE self, VALUE v_world, VALUE v_gravity)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	data->gravity = c_value_to_vector3d(v_gravity);
	return c_vector3d_to_value( data->gravity );
}

VALUE world_get_bodies(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	VALUE v_bodies = rb_ary_new2(NewtonWorldGetBodyCount(world));
	int i = 0;
	for( const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body) )
	{
		rb_ary_store(v_bodies, i, LONG2NUM((long)body));
		i++;
	}
	return v_bodies;
}

VALUE world_get_bodies_in_aabb(VALUE self, VALUE v_world, VALUE v_min_pt, VALUE v_max_pt)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	VALUE v_bodies = rb_ary_new();
	NewtonWorldForEachBodyInAABBDo(world, &c_value_to_point3d(v_min_pt)[0], &c_value_to_point3d(v_max_pt)[1], body_iterator, (void* const)v_bodies);
	return v_bodies;
}

VALUE world_get_first_body(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	NewtonBody* body = NewtonWorldGetFirstBody(world);
	return body ? LONG2NUM((long)body) : Qnil;
}

VALUE world_get_next_body(VALUE self, VALUE v_world, VALUE v_body)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonBody* next_body = NewtonWorldGetNextBody(world, body);
	return next_body ? LONG2NUM((long)next_body) : Qnil;
}

VALUE world_get_solver_model(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return LONG2NUM( data->solver_model );
}

VALUE world_set_solver_model(VALUE self, VALUE v_world, VALUE v_solver_model)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	data->solver_model = c_clamp_int(NUM2INT(v_solver_model), 0, 256);
	NewtonSetSolverModel(world, data->solver_model);
	return LONG2NUM( data->solver_model );
}

VALUE world_get_friction_model(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return LONG2NUM( data->friction_model );
}

VALUE world_set_friction_model(VALUE self, VALUE v_world, VALUE v_friction_model)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	data->friction_model = c_clamp_int(NUM2INT(v_friction_model), 0, 1);
	NewtonSetFrictionModel(world, data->friction_model);
	return LONG2NUM( data->friction_model );
}

VALUE world_get_material_thickness(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return DBL2NUM( data->material_thinkness );
}

VALUE world_set_material_thickness(VALUE self, VALUE v_world, VALUE v_material_thinkness)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	data->material_thinkness = c_clamp_dfloat((dFloat)NUM2DBL(v_material_thinkness), 0.0f, 1.0f/32.0f);
	NewtonMaterialSetSurfaceThickness(world, 0, 0, data->material_thinkness);
	return DBL2NUM( data->material_thinkness );
}

VALUE world_ray_cast(VALUE self, VALUE v_world, VALUE v_point1, VALUE v_point2)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	dVector point1 = c_value_to_point3d(v_point1);
	dVector point2 = c_value_to_point3d(v_point2);
	Hit hit;
	NewtonWorldRayCast(world, &point1[0], &point2[0], ray_filter_callback, &hit, NULL, 0);
	return hit.body ? rb_ary_new3(3, LONG2NUM((long)hit.body), c_point3d_to_value(hit.point), c_vector3d_to_value(hit.normal)) : Qnil;
}

VALUE world_continuous_ray_cast(VALUE self, VALUE v_world, VALUE v_point1, VALUE v_point2)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	dVector point1 = c_value_to_point3d(v_point1);
	dVector point2 = c_value_to_point3d(v_point2);
	RayData ray_data;
	NewtonWorldRayCast(world, &point1[0], &point2[0], continuous_ray_filter_callback, &ray_data, NULL, 0);
	VALUE hits = rb_ary_new2( (long)ray_data.hits.size() );
	for( unsigned int i = 0; i < ray_data.hits.size(); i++ )
	{
		VALUE hit = rb_ary_new3(3, LONG2NUM((long)ray_data.hits[i].body), c_point3d_to_value(ray_data.hits[i].point), c_vector3d_to_value(ray_data.hits[i].normal));
		rb_ary_store(hits, i, hit);
	}
	return hits;
}

VALUE world_convex_ray_cast(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_target)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	const NewtonCollision* collision = c_value_to_collision(v_collision);
	dMatrix matrix = c_value_to_matrix(v_matrix);
	dVector target = c_value_to_point3d(v_target);
	Hit hit;
	NewtonWorldConvexRayCast(world, collision, &matrix[0][0], &target[0], ray_filter_callback, &hit, NULL, 0);
	return hit.body ? rb_ary_new3(3, LONG2NUM((long)hit.body), c_point3d_to_value(hit.point), c_vector3d_to_value(hit.normal)) : Qnil;
}

VALUE world_continuous_convex_ray_cast(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_target)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	const NewtonCollision* collision = c_value_to_collision(v_collision);
	dMatrix matrix = c_value_to_matrix(v_matrix);
	dVector target = c_value_to_point3d(v_target);
	RayData ray_data;
	NewtonWorldConvexRayCast(world, collision, &matrix[0][0], &target[0], continuous_ray_filter_callback, &ray_data, NULL, 0);
	VALUE hits = rb_ary_new2( (long)ray_data.hits.size() );
	for( unsigned int i = 0; i < ray_data.hits.size(); i++ )
	{
		VALUE hit = rb_ary_new3(3, LONG2NUM((long)ray_data.hits[i].body), c_point3d_to_value(ray_data.hits[i].point), c_vector3d_to_value(ray_data.hits[i].normal));
		rb_ary_store(hits, i, hit);
	}
	return hits;
}

VALUE world_add_explosion(VALUE self, VALUE v_world, VALUE v_center, VALUE v_blast_radius, VALUE v_blast_force)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	dMatrix matrix;
	dVector point1 = c_value_to_point3d(v_center);
	dVector point2;
	dFloat blast_radius = c_clamp_dfloat((dFloat)NUM2DBL(v_blast_radius), 0.0f, NULL);
	dFloat blast_force = c_clamp_dfloat((dFloat)NUM2DBL(v_blast_force), 0.0f, NULL);
	if( blast_radius == 0.0f || blast_force == 0.0f )
		return Qfalse;
	for( const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body) )
	{
		dFloat mass, ixx, iyy, izz;
		NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);
		if( mass == 0.0f ) continue;
		NewtonBodyGetMatrix(body, &matrix[0][0]);
		NewtonBodyGetCentreOfMass(body, &point2[0]);
		point2 = matrix.TransformVector(point2);
		Hit hit;
		NewtonWorldRayCast(world, &point1[0], &point2[0], ray_filter_callback, &hit, NULL, 0);
		if( hit.body != body ) continue;
		dVector force = hit.point - point1;
		dFloat mag = sqrt(force[0]*force[0] + force[1]*force[1] + force[2]*force[2]);
		if( mag == 0.0f || mag > blast_radius ) continue;
		force = force.Scale( (blast_radius - mag) * blast_force / (blast_radius * mag) );
		//dVector r = hit.point - point2;
		//dVector torque = r * force;
		BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
		data->add_force += force;
		data->add_force_state = true;
		//data->add_torque += torque;
		//data->add_torque_state = true;
	}
	return Qtrue;
}

VALUE world_get_aabb(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	if( NewtonWorldGetBodyCount(world) == 0 ) return Qnil;
	dVector world_min;
	dVector world_max;
	bool first_time = true;
	for( const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body) )
	{
		if( first_time )
		{
			NewtonBodyGetAABB(body, &world_min[0], &world_max[0]);
			first_time = false;
			continue;
		}
		dVector min;
		dVector max;
		NewtonBodyGetAABB(body, &min[0], &max[0]);
		for( int i = 0; i < 3; i++ )
		{
			if( min[i] < world_min[i] ) world_min[i] = min[i];
			if( max[i] > world_max[i] ) world_max[i] = max[i];
		}
	}
	return rb_ary_new3(2, c_point3d_to_value(world_min), c_point3d_to_value(world_max));
}

VALUE world_get_destructor_proc(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	if( RARRAY_LEN(data->destructor_proc) == 0 ) return Qnil;
	return rb_ary_entry(data->destructor_proc, 0);
}

VALUE world_set_destructor_proc(VALUE self, VALUE v_world, VALUE v_proc)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	if( v_proc == Qnil )
		rb_ary_clear(data->destructor_proc);
	else if( rb_class_of(v_proc) == rb_cProc )
		rb_ary_store(data->destructor_proc, 0, v_proc);
	else
		rb_raise(rb_eTypeError, "Expected nil or a Proc object!");
	return Qtrue;
}

VALUE world_get_user_data(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	if( RARRAY_LEN(data->user_data) == 0 ) return Qnil;
	return rb_ary_entry(data->user_data, 0);
}

VALUE world_set_user_data(VALUE self, VALUE v_world, VALUE v_user_data)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	if( v_user_data == Qnil )
		rb_ary_clear(data->user_data);
	else
		rb_ary_store(data->user_data, 0, v_user_data);
	return data->user_data;
}

VALUE world_get_touch_data_at(VALUE self, VALUE v_world, VALUE v_index)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	unsigned int index = NUM2UINT(v_index);
	if( index < 0 || index >= data->touch_data.size() ) return Qnil;
	BodyTouchData touch_data = data->touch_data[index];
	VALUE v_touch_data = rb_ary_new2(6);
	rb_ary_store(v_touch_data, 0, LONG2NUM(touch_data.body0));
	rb_ary_store(v_touch_data, 1, LONG2NUM(touch_data.body1));
	rb_ary_store(v_touch_data, 2, c_point3d_to_value(touch_data.point));
	rb_ary_store(v_touch_data, 3, c_vector3d_to_value(touch_data.normal));
	rb_ary_store(v_touch_data, 4, c_vector3d_to_value(touch_data.force));
	rb_ary_store(v_touch_data, 5, DBL2NUM(touch_data.speed));
	return v_touch_data;
}

VALUE world_get_touch_data_count(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return LONG2NUM( (long)data->touch_data.size() );
}

VALUE world_get_touching_data_at(VALUE self, VALUE v_world, VALUE v_index)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	unsigned int index = NUM2LONG(v_index);
	if( index < 0 || index >= data->touching_data.size() ) return Qnil;
	BodyTouchingData touching_data = data->touching_data[index];
	VALUE v_touching_data = rb_ary_new2(2);
	rb_ary_store(v_touching_data, 0, LONG2NUM(touching_data.body0));
	rb_ary_store(v_touching_data, 1, LONG2NUM(touching_data.body1));
	return v_touching_data;
}

VALUE world_get_touching_data_count(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return LONG2NUM( (long)data->touching_data.size() );
}

VALUE world_get_untouch_data_at(VALUE self, VALUE v_world, VALUE v_index)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	unsigned int index = NUM2LONG(v_index);
	if( index < 0 || index >= data->untouch_data.size() ) return Qnil;
	BodyUntouchData untouch_data = data->untouch_data[index];
	VALUE v_untouch_data = rb_ary_new2(2);
	rb_ary_store(v_untouch_data, 0, LONG2NUM(untouch_data.body0));
	rb_ary_store(v_untouch_data, 1, LONG2NUM(untouch_data.body1));
	return v_untouch_data;
}

VALUE world_get_untouch_data_count(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return LONG2NUM( (long)data->untouch_data.size() );
}

VALUE world_get_time(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return DBL2NUM( data->time );
}

VALUE world_serialize_to_file(VALUE self, VALUE v_world, VALUE v_full_path)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	NewtonSerializeToFile(world, StringValuePtr(v_full_path));
	return Qnil;
}


// *********************************************************************************************************************
//
// Collision Interface
//
// *********************************************************************************************************************

VALUE collision_create_null(VALUE self, VALUE v_world)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	long col = (long)NewtonCreateNull(world);
	valid_collisions[col] = true;
	return LONG2NUM(col);
}

VALUE collision_create_box(VALUE self, VALUE v_world, VALUE v_width, VALUE v_height, VALUE v_depth, VALUE v_id, VALUE v_offset_matrix)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	long col = (long)NewtonCreateBox(
		world,
		(dFloat)NUM2DBL(v_width)*INCH_TO_METER,
		(dFloat)NUM2DBL(v_height)*INCH_TO_METER,
		(dFloat)NUM2DBL(v_depth)*INCH_TO_METER,
		NUM2LONG(v_id),
		v_offset_matrix == Qnil ? NULL : &c_value_to_matrix(v_offset_matrix)[0][0]);
	valid_collisions[col] = true;
	return LONG2NUM(col);
}

VALUE collision_create_sphere(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_id, VALUE v_offset_matrix)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	long col = (long)NewtonCreateSphere(
		world,
		(dFloat)NUM2DBL(v_radius) * INCH_TO_METER,
		NUM2LONG(v_id),
		v_offset_matrix == Qnil ? NULL : &c_value_to_matrix(v_offset_matrix)[0][0]);
	valid_collisions[col] = true;
	return LONG2NUM(col);
}

VALUE collision_create_cone(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	long col = (long)NewtonCreateCone(
		world,
		(dFloat)NUM2DBL(v_radius) * INCH_TO_METER,
		(dFloat)NUM2DBL(v_height) * INCH_TO_METER,
		NUM2LONG(v_id),
		v_offset_matrix == Qnil ? NULL : &c_value_to_matrix(v_offset_matrix)[0][0]);
	valid_collisions[col] = true;
	return LONG2NUM(col);
}

VALUE collision_create_cylinder(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	long col = (long)NewtonCreateCylinder(
		world,
		(dFloat)NUM2DBL(v_radius) * INCH_TO_METER,
		(dFloat)NUM2DBL(v_height) * INCH_TO_METER,
		NUM2LONG(v_id),
		v_offset_matrix == Qnil ? NULL : &c_value_to_matrix(v_offset_matrix)[0][0]);
	valid_collisions[col] = true;
	return LONG2NUM(col);
}

VALUE collision_create_capsule(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	long col = (long)NewtonCreateCapsule(
		world,
		(dFloat)NUM2DBL(v_radius) * INCH_TO_METER,
		(dFloat)NUM2DBL(v_height) * INCH_TO_METER,
		NUM2LONG(v_id),
		v_offset_matrix == Qnil ? NULL : &c_value_to_matrix(v_offset_matrix)[0][0]);
	valid_collisions[col] = true;
	return LONG2NUM(col);
}

VALUE collision_create_tapered_capsule(VALUE self, VALUE v_world, VALUE v_radius0, VALUE v_radius1, VALUE v_height, VALUE v_id, VALUE v_offset_matrix)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	long col = (long)NewtonCreateTaperedCapsule(
		world,
		(dFloat)NUM2DBL(v_radius0) * INCH_TO_METER,
		(dFloat)NUM2DBL(v_radius1) * INCH_TO_METER,
		(dFloat)NUM2DBL(v_height) * INCH_TO_METER,
		NUM2LONG(v_id),
		v_offset_matrix == Qnil ? NULL : &c_value_to_matrix(v_offset_matrix)[0][0]);
	valid_collisions[col] = true;
	return LONG2NUM(col);
}

VALUE collision_create_tapered_cylinder(VALUE self, VALUE v_world, VALUE v_radius0, VALUE v_radius1, VALUE v_height, VALUE v_id, VALUE v_offset_matrix)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	long col = (long)NewtonCreateTaperedCylinder(
		world,
		(dFloat)NUM2DBL(v_radius0) * INCH_TO_METER,
		(dFloat)NUM2DBL(v_radius1) * INCH_TO_METER,
		(dFloat)NUM2DBL(v_height) * INCH_TO_METER,
		NUM2LONG(v_id),
		v_offset_matrix == Qnil ? NULL : &c_value_to_matrix(v_offset_matrix)[0][0]);
	valid_collisions[col] = true;
	return LONG2NUM(col);
}

VALUE collision_create_chamfer_cylinder(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	long col = (long)NewtonCreateChamferCylinder(
		world,
		(dFloat)NUM2DBL(v_radius) * INCH_TO_METER,
		(dFloat)NUM2DBL(v_height) * INCH_TO_METER,
		NUM2LONG(v_id),
		v_offset_matrix == Qnil ? NULL : &c_value_to_matrix(v_offset_matrix)[0][0]);
	valid_collisions[col] = true;
	return LONG2NUM(col);
}

VALUE collision_create_convex_hull(VALUE self, VALUE v_world, VALUE v_vertices, VALUE v_tolerance, VALUE v_id, VALUE v_offset_matrix)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	Check_Type(v_vertices, T_ARRAY);
	std::vector<dFloat> vertex_cloud;
	for( int i = 0; i < RARRAY_LEN(v_vertices); i++ )
	{
		dVector point = c_value_to_point3d( rb_ary_entry(v_vertices, i) );
		vertex_cloud.push_back(point.m_x);
		vertex_cloud.push_back(point.m_y);
		vertex_cloud.push_back(point.m_z);
	}
	long col = (long)NewtonCreateConvexHull(
		world,
		RARRAY_LEN(v_vertices),
		&vertex_cloud[0],
		sizeof(dFloat)*3,
		(dFloat)NUM2DBL(v_tolerance),
		NUM2LONG(v_id),
		v_offset_matrix == Qnil ? NULL : &c_value_to_matrix(v_offset_matrix)[0][0]);
	valid_collisions[col] = true;
	return LONG2NUM(col);
}

VALUE collision_create_compound(VALUE self, VALUE v_world, VALUE v_convex_collisions)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	Check_Type(v_convex_collisions, T_ARRAY);
	NewtonCollision* compound = NewtonCreateCompoundCollision(world, 0);
	NewtonCompoundCollisionBeginAddRemove(compound);
	for( int i = 0; i < RARRAY_LEN(v_convex_collisions); i++ )
	{
		long col = NUM2LONG(rb_ary_entry(v_convex_collisions, i));
		if( c_is_collision_valid(col) == true && c_is_collision_convex(col) == true )
			NewtonCompoundCollisionAddSubCollision(compound, (NewtonCollision*)col);
	}
	NewtonCompoundCollisionEndAddRemove(compound);
	valid_collisions[(long)compound] = true;
	return LONG2NUM((long)compound);
}

VALUE collision_create_compound_from_cd1(
	VALUE self,
	VALUE v_world,
	VALUE v_polygons,
	VALUE v_max_concavity,
	VALUE v_back_face_distance_factor,
	VALUE v_max_hull_count,
	VALUE v_max_vertices_per_hull)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	Check_Type(v_polygons, T_ARRAY);
	NewtonMesh* mesh = NewtonMeshCreate(world);
	int total_vertex_count = 0;
	NewtonMeshBeginFace(mesh);
	for( int i = 0; i < RARRAY_LEN(v_polygons); i++ )
	{
		VALUE v_polygon = rb_ary_entry(v_polygons, i);
		std::vector<dFloat> vertex_cloud;
		int vertex_count = 0;
		for( int j = 0; j < RARRAY_LEN(v_polygon); j++ )
		{
			dVector point = c_value_to_point3d( rb_ary_entry(v_polygon, j) );
			vertex_cloud.push_back(point[0]);
			vertex_cloud.push_back(point[1]);
			vertex_cloud.push_back(point[2]);
			vertex_count++;
		}
		total_vertex_count += vertex_count;
		NewtonMeshAddFace(mesh, vertex_count, &vertex_cloud[0], sizeof(dFloat)*3, 0);
	}
	NewtonMeshEndFace(mesh);
	NewtonRemoveUnusedVertices(mesh, NULL);
	NewtonMeshFixTJoints(mesh);
	dFloat max_concavity = (dFloat)NUM2DBL(v_max_concavity);
	dFloat back_face_dist_factor = (dFloat)NUM2DBL(v_back_face_distance_factor);
	int max_hull_count = NUM2LONG(v_max_hull_count);
	int max_vertices_per_hull = NUM2LONG(v_max_vertices_per_hull);
	NewtonMesh* convex_approximation = NewtonMeshApproximateConvexDecomposition(mesh, max_concavity, back_face_dist_factor, max_hull_count, max_vertices_per_hull, NULL, NULL);
	NewtonMeshDestroy(mesh);
	NewtonCollision* collision = NewtonCreateCompoundCollisionFromMesh(world, convex_approximation, max_concavity, 0, 0);
	NewtonMeshDestroy(convex_approximation);
	valid_collisions[(long)collision] = true;
	return LONG2NUM((long)collision);
}

VALUE collision_create_compound_from_cd2(
	VALUE self,
	VALUE v_world,
	VALUE v_points,
	VALUE v_indices,
	VALUE v_params)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	Check_Type(v_points, T_ARRAY);
	Check_Type(v_indices, T_ARRAY);
	Check_Type(v_params, T_HASH);

	// Convert polygons to an array of triangles and points.
	unsigned int nTriangles = RARRAY_LEN(v_indices);
	unsigned int nPoints = RARRAY_LEN(v_points);

	if( nTriangles == 0 || nPoints == 0 ) return Qnil;

	std::vector<int> triangles(nTriangles*3, 0); // array of indices
	std::vector<double> points(nPoints*3, 0.0f); // array of coordinates
	for( unsigned int i = 0; i < nTriangles; i++ )
	{
		VALUE v_triangle = rb_ary_entry(v_indices, i);
		for( int j = 0; j < 3; j++ )
			triangles[i*3+j] = NUM2INT(rb_ary_entry(v_triangle, j));
	}

	for( unsigned int i = 0; i < nPoints; i++ )
	{
		dVector point = c_value_to_point3d( rb_ary_entry(v_points, i) );
		for( int j = 0; j < 3; j++ )
			points[i*3+j] = point[j];
	}

	if( nPoints == 0 ) return Qnil;

	// V-HACD parameters
	VHACD::IVHACD::Parameters params;
	VALUE val;
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("resolution")) );
	if( val != Qnil ) params.m_resolution = c_clamp_uint(NUM2UINT(val), 10000, 64000000);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("depth")) );
	if( val != Qnil ) params.m_depth = c_clamp_int(NUM2INT(val), 1, 32);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("concavity")) );
	if( val != Qnil ) params.m_concavity = c_clamp_double(NUM2DBL(val), 0.0f, 1.0f);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("plane_downsampling")) );
	if( val != Qnil ) params.m_planeDownsampling = c_clamp_int(NUM2INT(val), 1, 16);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("convex_hull_downsampling")) );
	if( val != Qnil ) params.m_convexhullDownsampling = c_clamp_int(NUM2INT(val), 1, 16);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("alpha")) );
	if( val != Qnil ) params.m_alpha = c_clamp_double(NUM2DBL(val), 0.0f, 1.0f);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("beta")) );
	if( val != Qnil ) params.m_beta = c_clamp_double(NUM2DBL(val), 0.0f, 1.0f);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("gamma")) );
	if( val != Qnil ) params.m_gamma = c_clamp_double(NUM2DBL(val), 0.0f, 1.0f);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("delta")) );
	if( val != Qnil ) params.m_delta = c_clamp_double(NUM2DBL(val), 0.0f, 1.0f);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("pca")) );
	if( val != Qnil ) params.m_pca = c_clamp_int(NUM2INT(val), 0, 1);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("mode")) );
	if( val != Qnil ) params.m_mode = c_clamp_int(NUM2INT(val), 0, 1);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("max_num_vertices_per_ch")) );
	if( val != Qnil ) params.m_maxNumVerticesPerCH = c_clamp_uint(NUM2UINT(val), 4, 1024);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("min_volume_per_ch")) );
	if( val != Qnil ) params.m_minVolumePerCH = c_clamp_double(NUM2DBL(val), 0.0f, 0.01f);

	// Create interface
	VHACD::IVHACD * interfaceVHACD = VHACD::CreateVHACD();

	// Compute approximate convex decomposition
	bool res = interfaceVHACD->Compute(&points[0], 3, nPoints, &triangles[0], 3, nTriangles, params);

	if( !res )
	{
		interfaceVHACD->Clean();
		interfaceVHACD->Release();
		return Qnil;
	}

	// Get number of convex hulls
	unsigned int nConvexHulls = interfaceVHACD->GetNConvexHulls();
	VHACD::IVHACD::ConvexHull ch;
	// Create compound collision
	NewtonCollision* compound = NewtonCreateCompoundCollision(world, 0);
	NewtonCompoundCollisionBeginAddRemove(compound);
	// Process all convex hulls
	for( unsigned int i = 0; i < nConvexHulls; i++ )
	{
		// Get the i-th convex-hull information
		interfaceVHACD->GetConvexHull(i, ch);
		// Create compound sub collision
		std::vector<dFloat> vertex_cloud;
		for( unsigned int j = 0; j < ch.m_nPoints; j++ )
		{
			vertex_cloud.push_back( (dFloat)ch.m_points[j*3+0] );
			vertex_cloud.push_back( (dFloat)ch.m_points[j*3+1] );
			vertex_cloud.push_back( (dFloat)ch.m_points[j*3+2] );
		}
		NewtonCollision* col = NewtonCreateConvexHull(world, ch.m_nPoints, &vertex_cloud[0], sizeof(dFloat)*3, 0.0f, 0, NULL);
		NewtonCompoundCollisionAddSubCollision(compound, col);
		NewtonDestroyCollision(col);
	}
	NewtonCompoundCollisionEndAddRemove(compound);

	interfaceVHACD->Clean();
	interfaceVHACD->Release();

	valid_collisions[(long)compound] = true;
	return LONG2NUM((long)compound);
}

VALUE collision_create_compound_from_cd3(VALUE self, VALUE v_world, VALUE v_points, VALUE v_indices, VALUE v_max_concavity_angle)
{
	return Qnil;
}

VALUE collision_create_static_mesh(VALUE self, VALUE v_world, VALUE v_polygons, VALUE v_simplify, VALUE v_optimize)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	Check_Type(v_polygons, T_ARRAY);
	NewtonMesh* mesh = NewtonMeshCreate(world);
	int total_vertex_count = 0;
	NewtonMeshBeginFace(mesh);
	for( int i = 0; i < RARRAY_LEN(v_polygons); i++ )
	{
		VALUE v_polygon = rb_ary_entry(v_polygons, i);
		if( TYPE(v_polygon) != T_ARRAY ) continue;
		std::vector<dFloat> vertex_cloud;
		int vertex_count = 0;
		for( int j = 0; j < RARRAY_LEN(v_polygon); j++ )
		{
			dVector point = c_value_to_point3d( rb_ary_entry(v_polygon, j) );
			vertex_cloud.push_back(point[0]);
			vertex_cloud.push_back(point[1]);
			vertex_cloud.push_back(point[2]);
			vertex_count++;
		}
		total_vertex_count += vertex_count;
		NewtonMeshAddFace(mesh, vertex_count, &vertex_cloud[0], sizeof(dFloat)*3, 0);
	}
	NewtonMeshEndFace(mesh);
	if( RTEST(v_simplify) )
		NewtonRemoveUnusedVertices(mesh, NULL);
	NewtonMeshFixTJoints(mesh);
	int optimize = NUM2LONG(v_optimize);
	if( optimize == 1 )
		NewtonMeshTriangulate(mesh);
	else if( optimize == 2 )
		NewtonMeshPolygonize(mesh);
	NewtonCollision* collision = NewtonCreateTreeCollisionFromMesh(world, mesh, 0);
	NewtonMeshDestroy(mesh);
	valid_collisions[(long)collision] = true;
	return LONG2NUM((long)collision);
}

VALUE collision_get_type(VALUE self, VALUE v_collision)
{
	const NewtonCollision* collision = c_value_to_collision(v_collision);
	return INT2FIX( NewtonCollisionGetType(collision) );
}

VALUE collision_is_valid(VALUE self, VALUE v_collision)
{
	return c_is_collision_valid(NUM2LONG(v_collision)) ? Qtrue : Qfalse;
}

VALUE collision_destroy(VALUE self, VALUE v_collision)
{
	const NewtonCollision* collision = c_value_to_collision(v_collision);
	NewtonDestroyCollision(collision);
	return Qnil;
}


// *********************************************************************************************************************
//
// Bodies Interface
//
// *********************************************************************************************************************

VALUE bodies_aabb_overlap(VALUE self, VALUE v_body1, VALUE v_body2)
{
	const NewtonBody* body1 = c_value_to_body(v_body1);
	const NewtonBody* body2 = c_value_to_body(v_body2);
	if( body1 == body2 )
		rb_raise(rb_eTypeError, "Expected two unique bodies!");
	return c_bodies_aabb_overlap(body1, body2) ? Qtrue : Qfalse;
}

VALUE bodies_collidable(VALUE self, VALUE v_body1, VALUE v_body2)
{
	const NewtonBody* body1 = c_value_to_body(v_body1);
	const NewtonBody* body2 = c_value_to_body(v_body2);
	if( body1 == body2 )
		rb_raise(rb_eTypeError, "Expected two unique bodies!");
	return c_bodies_collidable(body1, body2) ? Qtrue : Qfalse;
}

VALUE bodies_touching(VALUE self, VALUE v_body1, VALUE v_body2)
{
	const NewtonBody* body1 = c_value_to_body(v_body1);
	const NewtonBody* body2 = c_value_to_body(v_body2);
	if( body1 == body2 )
		rb_raise(rb_eTypeError, "Expected two unique bodies!");
	NewtonWorld* world = NewtonBodyGetWorld(body1);
	NewtonCollision* colA = NewtonBodyGetCollision(body1);
	NewtonCollision* colB = NewtonBodyGetCollision(body2);
	dMatrix matrixA;
	dMatrix matrixB;
	NewtonBodyGetMatrix(body1, &matrixA[0][0]);
	NewtonBodyGetMatrix(body2, &matrixB[0][0]);
	return NewtonCollisionIntersectionTest(world, colA, &matrixA[0][0], colB, &matrixB[0][0], 0) == 1 ? Qtrue : Qfalse;
}

VALUE bodies_get_closest_points(VALUE self, VALUE v_body1, VALUE v_body2)
{
	const NewtonBody* body1 = c_value_to_body(v_body1);
	const NewtonBody* body2 = c_value_to_body(v_body2);
	if( body1 == body2 )
		rb_raise(rb_eTypeError, "Expected two unique bodies!");
	NewtonWorld* world = NewtonBodyGetWorld(body1);
	NewtonCollision* colA = NewtonBodyGetCollision(body1);
	NewtonCollision* colB = NewtonBodyGetCollision(body2);
	dMatrix matrixA;
	dMatrix matrixB;
	NewtonBodyGetMatrix(body1, &matrixA[0][0]);
	NewtonBodyGetMatrix(body2, &matrixB[0][0]);
	dVector pointA;
	dVector pointB;
	dVector normalAB;
	if( NewtonCollisionClosestPoint(world, colA, &matrixA[0][0], colB, &matrixB[0][0], &pointA[0], &pointB[0], &normalAB[0], 0) == 0 )
		return Qnil;
	return rb_ary_new3(2, c_point3d_to_value(pointA), c_point3d_to_value(pointB));
}

VALUE bodies_get_force_in_between(VALUE self, VALUE v_body1, VALUE v_body2)
{
	const NewtonBody* body1 = c_value_to_body(v_body1);
	const NewtonBody* body2 = c_value_to_body(v_body2);
	if( body1 == body2 )
		rb_raise(rb_eTypeError, "Expected two unique bodies!");
	dVector net_force(0.0f, 0.0f, 0.0f);
	for( NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body1); joint; joint = NewtonBodyGetNextContactJoint(body1, joint) )
	{
		if( NewtonJointGetBody0(joint) == body2 || NewtonJointGetBody1(joint) == body2 )
		{
			for( void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact))
			{
				NewtonMaterial* material = NewtonContactGetMaterial(contact);
				dVector force;
				NewtonMaterialGetContactForce(material, body1, &force[0]);
				net_force += force;
			}
		}
	}
	return c_vector3d_to_value(net_force);
}


// *********************************************************************************************************************
//
// Body Interface
//
// *********************************************************************************************************************

VALUE body_is_valid(VALUE self, VALUE v_body)
{
	return c_is_body_valid(NUM2LONG(v_body)) ? Qtrue : Qfalse;
}

VALUE body_create_dynamic(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix)
{
	const NewtonWorld* world = c_value_to_world(v_world);
	const NewtonCollision* collision = c_value_to_collision(v_collision);
	dMatrix matrix = c_value_to_matrix(v_matrix);
	dVector scale = c_matrix_get_scale(matrix);
	if( c_matrix_is_flipped(matrix) )
	{
		matrix.m_front.m_x *= -1;
		matrix.m_front.m_y *= -1;
		matrix.m_front.m_z *= -1;
		scale.m_x *= -1;
	}
	NewtonBody* body = NewtonCreateDynamicBody(world, collision, &c_matrix_extract_scale(matrix)[0][0]);
	valid_bodies[(long)body] = true;
	BodyData* data = new BodyData;
	data->volume = NewtonConvexCollisionCalculateVolume(collision);
	if( data->volume == 0.0f )
	{
		data->mass = 0.0f;
		data->density = 0.0f;
		data->dynamic = false;
		data->bstatic = true;
	}
	else
	{
		data->dynamic = true;
		data->bstatic = false;
		dFloat desired_mass = data->volume * DEFAULT_DENSITY;
		data->mass = c_clamp_float(desired_mass, MIN_MASS, MAX_MASS);
		if( data->mass != desired_mass )
			data->density = data->mass / data->volume;
	}
	data->elasticity = DEFAULT_ELASTICITY;
	data->softness = DEFAULT_SOFTNESS;
	data->static_friction = DEFAULT_STATIC_FRICTION;
	data->dynamic_friction = DEFAULT_DYNAMIC_FRICTION;
	data->friction_enabled = DEFAULT_ENABLE_FRICTION;
	data->add_force = dVector(0.0f, 0.0f, 0.0f);
	data->add_force_state = false;
	data->add_torque = dVector(0.0f, 0.0f, 0.0f);
	data->add_torque_state = false;
	data->set_force = dVector(0.0f, 0.0f, 0.0f);
	data->set_force_state = false;
	data->set_torque = dVector(0.0f, 0.0f, 0.0f);
	data->set_torque_state = false;
	data->collidable = true;
	data->record_touch_data = false;
	data->magnet_force = 0.0f;
	data->magnet_range = 0.0f;
	data->magnetic = false;
	data->destructor_proc = rb_ary_new();
	data->user_data = rb_ary_new();
	data->non_collidable_bodies;
	data->touchers;
	data->matrix_scale = scale;
	data->matrix_changed = false;

	NewtonBodySetUserData(body, data);
	rb_gc_register_address(&data->destructor_proc);
	rb_gc_register_address(&data->user_data);

	NewtonBodySetForceAndTorqueCallback(body, force_and_torque_callback);
	NewtonBodySetDestructorCallback(body, body_destructor_callback);
	NewtonBodySetTransformCallback(body, body_transform_callback);
	return LONG2NUM( (long)body );
}

VALUE body_destroy(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonDestroyBody(body);
	return Qnil;
}

VALUE body_get_world(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	return LONG2NUM( (long)NewtonBodyGetWorld(body) );
}

VALUE body_get_collision(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	return LONG2NUM( (long)NewtonBodyGetCollision(body) );
}

VALUE body_get_simulation_state(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	return NewtonBodyGetSimulationState(body) == 1 ? Qtrue : Qfalse;
}

VALUE body_set_simulation_state(VALUE self, VALUE v_body, VALUE v_state)
{
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonBodySetSimulationState(body, RTEST(v_state) ? 1 : 0);
	return NewtonBodyGetSimulationState(body) == 1 ? Qtrue : Qfalse;
}

VALUE body_get_continuous_collision_state(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	return NewtonBodyGetContinuousCollisionMode(body) == 1 ? Qtrue : Qfalse;
}

VALUE body_set_continuous_collision_state(VALUE self, VALUE v_body, VALUE v_state)
{
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonBodySetContinuousCollisionMode(body, RTEST(v_state) ? 1 : 0);
	return NewtonBodyGetContinuousCollisionMode(body) == 1 ? Qtrue : Qfalse;
}

VALUE body_get_matrix(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	return c_matrix_to_value( c_matrix_set_scale(matrix, data->matrix_scale) );
}

VALUE body_get_normal_matrix(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	if( data->matrix_scale.m_x < 0 )
	{
		matrix.m_front.m_x *= -1;
		matrix.m_front.m_y *= -1;
		matrix.m_front.m_z *= -1;
	}
	return c_matrix_to_value( matrix );
}

VALUE body_set_matrix(VALUE self, VALUE v_body, VALUE v_matrix)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	dMatrix matrix = c_value_to_matrix(v_matrix);
	if( c_matrix_is_flipped(matrix) )
	{
		matrix.m_front.m_x *= -1;
		matrix.m_front.m_y *= -1;
		matrix.m_front.m_z *= -1;
	}
	NewtonBodySetMatrix(body, &c_matrix_extract_scale(matrix)[0][0]);
	return c_matrix_to_value( c_matrix_set_scale(matrix, data->matrix_scale) );
}

VALUE body_get_velocity(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector velocity;
	NewtonBodyGetVelocity(body, &velocity[0]);
	return c_vector3d_to_value(velocity);
}

VALUE body_set_velocity(VALUE self, VALUE v_body, VALUE v_velocity)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector velocity = c_value_to_vector3d(v_velocity);
	NewtonBodySetVelocity(body, &velocity[0]);
	return c_vector3d_to_value(velocity);
}

VALUE body_get_omega(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector omega;
	NewtonBodyGetOmega(body, &omega[0]);
	return c_vector3d_to_value(omega);
}

VALUE body_set_omega(VALUE self, VALUE v_body, VALUE v_omega)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector omega = c_value_to_vector3d(v_omega);
	NewtonBodySetOmega(body, &omega[0]);
	return c_vector3d_to_value(omega);
}

VALUE body_get_centre_of_mass(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	com.m_x /= data->matrix_scale.m_x;
	com.m_y /= data->matrix_scale.m_y;
	com.m_z /= data->matrix_scale.m_z;
	return c_point3d_to_value(com);
}

VALUE body_set_centre_of_mass(VALUE self, VALUE v_body, VALUE v_com)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	dVector com = c_value_to_vector3d(v_com);
	com.m_x *= data->matrix_scale.m_x;
	com.m_y *= data->matrix_scale.m_y;
	com.m_z *= data->matrix_scale.m_z;
	NewtonBodySetCentreOfMass(body, &com[0]);
	com.m_x /= data->matrix_scale.m_x;
	com.m_y /= data->matrix_scale.m_y;
	com.m_z /= data->matrix_scale.m_z;
	return c_point3d_to_value(com);
}

VALUE body_get_position(VALUE self, VALUE v_body, VALUE v_mode)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	if( NUM2LONG(v_mode) == 0 )
		return c_point3d_to_value( dVector(matrix[3][0], matrix[3][1], matrix[3][2]) );
	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	return c_point3d_to_value( matrix.TransformVector(com) );
}

VALUE body_set_position(VALUE self, VALUE v_body, VALUE v_position, VALUE v_mode)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector position = c_value_to_point3d(v_position);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	if( NUM2LONG(v_mode) == 1 )
	{
		dVector com;
		NewtonBodyGetCentreOfMass(body, &com[0]);
		position = matrix.TransformVector(matrix.UntransformVector(position) - com);
	}
	matrix[3] = position;
	NewtonBodySetMatrix(body, &matrix[0][0]);
	return c_point3d_to_value(position);
}

VALUE body_get_mass(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return DBL2NUM( data->mass );
}

VALUE body_set_mass(VALUE self, VALUE v_body, VALUE v_mass)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->dynamic == false ) return Qfalse;
	dFloat mass = c_clamp_dfloat((dFloat)NUM2DBL(v_mass), MIN_MASS, MAX_MASS);
	data->mass = mass;
	data->density = mass / data->volume;
	NewtonBodySetMassProperties(body, data->bstatic ? 0.0f : data->mass, NewtonBodyGetCollision(body));
	return DBL2NUM( data->mass );
}

VALUE body_get_density(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return DBL2NUM( data->density );
}

VALUE body_set_density(VALUE self, VALUE v_body, VALUE v_density)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->dynamic == false ) return Qfalse;
	data->density = c_clamp_dfloat((dFloat)NUM2DBL(v_density), MIN_DENSITY, NULL);
	dFloat desired_mass = data->density * data->volume;
	data->mass = c_clamp_dfloat(desired_mass, MIN_MASS, MAX_MASS);
	if( data->mass != desired_mass )
		data->density = data->mass / data->volume;
	NewtonBodySetMassProperties(body, data->bstatic ? 0.0f : data->mass, NewtonBodyGetCollision(body));
	return DBL2NUM( data->density );
}

VALUE body_get_volume(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return DBL2NUM( data->volume );
}

VALUE body_set_volume(VALUE self, VALUE v_body, VALUE v_volume)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->dynamic == false ) return Qfalse;
	data->volume = c_clamp_dfloat((dFloat)NUM2DBL(v_volume), MIN_VOLUME, NULL);
	dFloat desired_mass = data->density * data->volume;
	data->mass = c_clamp_dfloat(desired_mass, MIN_MASS, MAX_MASS);
	if( data->mass != desired_mass )
		data->volume = data->mass / data->density;
	NewtonBodySetMassProperties(body, data->bstatic ? 0.0f : data->mass, NewtonBodyGetCollision(body));
	return DBL2NUM( data->volume );
}

VALUE body_is_static(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return data->bstatic ? Qtrue : Qfalse;
}

VALUE body_set_static(VALUE self, VALUE v_body, VALUE v_static)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->dynamic == false ) return Qfalse;
	data->bstatic = RTEST(v_static);
	NewtonBodySetMassProperties(body, data->bstatic ? 0.0f : data->mass, NewtonBodyGetCollision(body));
	return data->bstatic ? Qtrue : Qfalse;
}

VALUE body_is_collidable(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return data->collidable ? Qtrue : Qfalse;
}

VALUE body_set_collidable(VALUE self, VALUE v_body, VALUE v_collidable)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->collidable = RTEST(v_collidable);
	return data->collidable ? Qtrue : Qfalse;
}

VALUE body_is_frozen(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	return NewtonBodyGetFreezeState(body) == 1 ? Qtrue : Qfalse;
}

VALUE body_set_frozen(VALUE self, VALUE v_body, VALUE v_state)
{
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonBodySetFreezeState(body, RTEST(v_state) ? 1 : 0);
	return NewtonBodyGetFreezeState(body) == 1 ? Qtrue : Qfalse;
}

VALUE body_is_sleeping(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	return NewtonBodyGetSleepState(body) == 1 ? Qtrue : Qfalse;
}

VALUE body_set_sleeping(VALUE self, VALUE v_body, VALUE v_state)
{
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonBodySetSleepState(body, RTEST(v_state) ? 1 : 0);
	return NewtonBodyGetSleepState(body) == 1 ? Qtrue : Qfalse;
}

VALUE body_get_auto_sleep_state(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	return NewtonBodyGetAutoSleep(body) == 1 ? Qtrue : Qfalse;
}

VALUE body_set_auto_sleep_state(VALUE self, VALUE v_body, VALUE v_state)
{
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonBodySetAutoSleep(body, RTEST(v_state) ? 1 : 0);
	return NewtonBodyGetAutoSleep(body) == 1 ? Qtrue : Qfalse;
}

VALUE body_is_non_collidable_with(VALUE self, VALUE v_body, VALUE v_other_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	const NewtonBody* other_body = c_value_to_body(v_other_body);
	if( body == other_body )
		rb_raise(rb_eTypeError, "Expected two unique bodies!");
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return data->non_collidable_bodies.find((long)other_body) != data->non_collidable_bodies.end() ? Qtrue : Qfalse;
}

VALUE body_set_non_collidable_with(VALUE self, VALUE v_body, VALUE v_other_body, VALUE v_state)
{
	const NewtonBody* body = c_value_to_body(v_body);
	const NewtonBody* other_body = c_value_to_body(v_other_body);
	if( body == other_body )
		rb_raise(rb_eTypeError, "Expected two unique bodies!");
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	BodyData* other_data = (BodyData*)NewtonBodyGetUserData(other_body);
	if( RTEST(v_state) )
	{
		data->non_collidable_bodies[(long)other_body] = true;
		other_data->non_collidable_bodies[(long)body] = true;
	}
	else
	{
		if( data->non_collidable_bodies.find((long)other_body) != data->non_collidable_bodies.end() )
			data->non_collidable_bodies.erase((long)other_body);
		if( other_data->non_collidable_bodies.find((long)body) != other_data->non_collidable_bodies.end() )
			other_data->non_collidable_bodies.erase((long)body);
	}
	return RTEST(v_state) ? Qtrue : Qfalse;
}

VALUE body_get_non_collidable_bodies(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	VALUE non_collidable_bodies = rb_ary_new();
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	for( std::map<long,bool>::iterator it = data->non_collidable_bodies.begin(); it != data->non_collidable_bodies.end(); ++it )
		rb_ary_push(non_collidable_bodies, LONG2NUM(it->first));
	return non_collidable_bodies;
}

VALUE body_clear_non_collidable_bodies(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	int count = (int)data->non_collidable_bodies.size();
	c_body_clear_non_collidable_bodies(body);
	return INT2FIX(count);
}

VALUE body_get_elasticity(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return DBL2NUM( data->elasticity );
}

VALUE body_set_elasticity(VALUE self, VALUE v_body, VALUE v_elasticity)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->elasticity = c_clamp_dfloat((dFloat)NUM2DBL(v_elasticity), 0.01f, 2.00f);
	return DBL2NUM( data->elasticity );
}

VALUE body_get_softness(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return DBL2NUM( data->softness );
}

VALUE body_set_softness(VALUE self, VALUE v_body, VALUE v_softness)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->softness = c_clamp_dfloat((dFloat)NUM2DBL(v_softness), 0.01f, 1.00f);
	return DBL2NUM( data->softness );
}

VALUE body_get_static_friction(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return DBL2NUM( data->static_friction );
}

VALUE body_set_static_friction(VALUE self, VALUE v_body, VALUE v_friction)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->static_friction = c_clamp_dfloat((dFloat)NUM2DBL(v_friction), 0.01f, 2.00f);
	return DBL2NUM( data->static_friction );
}

VALUE body_get_dynamic_friction(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return DBL2NUM( data->dynamic_friction );
}

VALUE body_set_dynamic_friction(VALUE self, VALUE v_body, VALUE v_friction)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->static_friction = c_clamp_dfloat((dFloat)NUM2DBL(v_friction), 0.01f, 2.00f);
	return DBL2NUM( data->dynamic_friction );
}

VALUE body_get_friction_state(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return data->friction_enabled ? Qtrue : Qfalse;
}

VALUE body_set_friction_state(VALUE self, VALUE v_body, VALUE v_state)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->friction_enabled = RTEST(v_state);
	return data->friction_enabled ? Qtrue : Qfalse;
}

VALUE body_get_magnet_force(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return DBL2NUM( data->magnet_force );
}

VALUE body_set_magnet_force(VALUE self, VALUE v_body, VALUE v_force)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->magnet_force = (dFloat)NUM2DBL(v_force);
	return DBL2NUM( data->magnet_force );
}

VALUE body_get_magnet_range(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return DBL2NUM( data->magnet_range );
}

VALUE body_set_magnet_range(VALUE self, VALUE v_body, VALUE v_range)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->magnet_range = (dFloat)NUM2DBL(v_range);
	return DBL2NUM( data->magnet_range );
}

VALUE body_is_magnetic(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return data->magnetic ? Qtrue : Qfalse;
}

VALUE body_set_magnetic(VALUE self, VALUE v_body, VALUE v_state)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->magnetic = RTEST(v_state);
	return data->magnetic ? Qtrue : Qfalse;
}

VALUE body_get_aabb(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector min;
	dVector max;
	NewtonBodyGetAABB(body, &min[0], &max[0]);
	return rb_ary_new3(2, c_point3d_to_value(min), c_point3d_to_value(max));
}

VALUE body_get_linear_damping(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	return DBL2NUM( NewtonBodyGetLinearDamping(body) );
}

VALUE body_set_linear_damping(VALUE self, VALUE v_body, VALUE v_damp)
{
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonBodySetLinearDamping(body, (dFloat)NUM2DBL(v_damp));
	return DBL2NUM( NewtonBodyGetLinearDamping(body) );
}

VALUE body_get_angular_damping(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector damp;
	NewtonBodyGetAngularDamping(body, &damp[0]);
	return c_vector3d_to_value(damp);
}

VALUE body_set_angular_damping(VALUE self, VALUE v_body, VALUE v_damp)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector damp = c_value_to_vector3d(v_damp);
	NewtonBodySetAngularDamping(body, &damp[0]);
	NewtonBodyGetAngularDamping(body, &damp[0]);
	return c_vector3d_to_value(damp);
}

VALUE body_get_point_velocity(VALUE self, VALUE v_body, VALUE v_point)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector point = c_value_to_vector3d(v_point);
	dVector velocity;
	NewtonBodyGetPointVelocity(body, &point[0], &velocity[0]);
	return c_vector3d_to_value(velocity);
}

VALUE body_add_point_force(VALUE self, VALUE v_body, VALUE v_point, VALUE v_force)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->bstatic ) return Qfalse;
	dMatrix matrix;
	dVector centre;
	dVector point = c_value_to_point3d(v_point);
	dVector force = c_value_to_vector3d(v_force);
	NewtonBodyGetCentreOfMass(body, &centre[0]);
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	centre = matrix.TransformVector(centre);
	dVector r = point - centre;
	dVector torque = r * force;
	data->add_force += force;
	data->add_force_state = true;
	data->add_torque += torque;
	data->add_torque_state = true;
	return Qtrue;
}

VALUE body_add_impulse(VALUE self, VALUE v_body, VALUE v_center, VALUE v_delta_vel)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->bstatic ) return Qfalse;
	dVector center = c_value_to_point3d(v_center);
	dVector delta_vel = c_value_to_vector3d(v_delta_vel);
	NewtonBodyAddImpulse(body, &center[0], &delta_vel[0]);
	return Qtrue;
}

VALUE body_get_force(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector force;
	NewtonBodyGetForce(body, &force[0]);
	return c_vector3d_to_value(force);
}

VALUE body_get_force_acc(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector force;
	NewtonBodyGetForceAcc(body, &force[0]);
	return c_vector3d_to_value(force);
}

VALUE body_add_force(VALUE self, VALUE v_body, VALUE v_force)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->bstatic ) return Qfalse;
	data->add_force += c_value_to_vector3d(v_force);
	data->add_force_state = true;
	return Qtrue;
}

VALUE body_set_force(VALUE self, VALUE v_body, VALUE v_force)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->bstatic ) return Qfalse;
	data->set_force += c_value_to_vector3d(v_force);
	data->set_force_state = true;
	return Qtrue;
}

VALUE body_get_torque(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector torque;
	NewtonBodyGetTorque(body, &torque[0]);
	return c_vector3d_to_value(torque);
}

VALUE body_get_torque_acc(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector torque;
	NewtonBodyGetTorqueAcc(body, &torque[0]);
	return c_vector3d_to_value(torque);
}

VALUE body_add_torque(VALUE self, VALUE v_body, VALUE v_torque)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->bstatic ) return Qfalse;
	data->add_torque += c_value_to_vector3d(v_torque);
	data->add_torque_state = true;
	return Qtrue;
}

VALUE body_set_torque(VALUE self, VALUE v_body, VALUE v_torque)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->bstatic ) return Qfalse;
	data->set_torque += c_value_to_vector3d(v_torque);
	data->set_torque_state = true;
	return Qtrue;
}

VALUE body_get_net_contact_force(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dVector net_force(0.0f, 0.0f, 0.0f, 0.0f);
	for( NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint) )
	{
		for( void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact))
		{
			NewtonMaterial* material = NewtonContactGetMaterial(contact);
			dVector force;
			NewtonMaterialGetContactForce(material, body, &force[0]);
			net_force += force;
		}
	}
	return c_vector3d_to_value(net_force);
}

VALUE body_get_contacts(VALUE self, VALUE v_body, VALUE inc_non_collidable)
{
	const NewtonBody* body = c_value_to_body(v_body);
	VALUE v_contacts = rb_ary_new();
	for( NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint) )
	{
		NewtonBody* touching_body = NewtonJointGetBody0(joint);
		if( touching_body == body )
			touching_body = NewtonJointGetBody1(joint);
		for( void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact))
		{
			NewtonMaterial* material = NewtonContactGetMaterial(contact);
			dVector point;
			dVector normal;
			dVector force;
			NewtonMaterialGetContactPositionAndNormal(material, body, &point[0], &normal[0]);
			NewtonMaterialGetContactForce(material, body, &force[0]);
			dFloat speed = NewtonMaterialGetContactNormalSpeed(material);
			rb_ary_push(v_contacts, rb_ary_new3(5, LONG2NUM((long)touching_body), c_point3d_to_value(point), c_vector3d_to_value(normal), c_vector3d_to_value(force), DBL2NUM(speed)));
		}
	}
	if( RTEST(inc_non_collidable) )
	{
		NewtonWorld* world = NewtonBodyGetWorld(body);
		NewtonCollision* colA = NewtonBodyGetCollision(body);
		NewtonCollision* colB;
		dMatrix matA;
		dMatrix matB;
		NewtonBodyGetMatrix(body, &matA[0][0]);
		dFloat points[3*NON_COL_CONTACTS_CAPACITY];
		dFloat normals[3*NON_COL_CONTACTS_CAPACITY];
		dFloat penetrations[3*NON_COL_CONTACTS_CAPACITY];
		long long attrA[NON_COL_CONTACTS_CAPACITY];
		long long attrB[NON_COL_CONTACTS_CAPACITY];
		for( const NewtonBody* tbody = NewtonWorldGetFirstBody(world); tbody; tbody = NewtonWorldGetNextBody(world, tbody) )
		{
			if( tbody == body || c_bodies_collidable(tbody, body) == true || c_bodies_aabb_overlap(tbody, body) == false ) continue;
			colB = NewtonBodyGetCollision(tbody);
			NewtonBodyGetMatrix(tbody, &matB[0][0]);
			int count = NewtonCollisionCollide(world, NON_COL_CONTACTS_CAPACITY, colA, &matA[0][0], colB, &matB[0][0], points, normals, penetrations, attrA, attrB, 0);
			if( count == 0 ) continue;
			for( int i = 0; i < count*3; i += 3 )
			{
				VALUE point = rb_ary_new3(3, DBL2NUM(points[i+0] * METER_TO_INCH), DBL2NUM(points[i+1] * METER_TO_INCH), DBL2NUM(points[i+2] * METER_TO_INCH));
				VALUE normal = rb_ary_new3(3, DBL2NUM(normals[i+0]), DBL2NUM(normals[i+1]), DBL2NUM(normals[i+2]));
				VALUE force = rb_ary_new3(3, DBL2NUM(0.0f), DBL2NUM(0.0f), DBL2NUM(0.0f));
				VALUE speed = DBL2NUM(0.0f);
				rb_ary_push(v_contacts, rb_ary_new3(5, LONG2NUM((long)tbody), point, normal, force, speed));
			}
		}
	}
	return v_contacts;
}

VALUE body_get_touching_bodies(VALUE self, VALUE v_body, VALUE inc_non_collidable)
{
	const NewtonBody* body = c_value_to_body(v_body);
	VALUE v_touching_bodies = rb_ary_new();
	for( NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint) )
	{
		NewtonBody* other_body = NewtonJointGetBody0(joint);
		if( RTEST(inc_non_collidable) == false && c_bodies_collidable(other_body, body) == false ) continue;
		if( other_body == body )
			other_body = NewtonJointGetBody1(joint);
		rb_ary_push(v_touching_bodies, LONG2NUM((long)other_body));
	}
	/*if( RTEST(inc_non_collidable) )
	{
		NewtonWorld* world = NewtonBodyGetWorld(body);
		NewtonCollision* colA = NewtonBodyGetCollision(body);
		NewtonCollision* colB;
		dMatrix matA;
		dMatrix matB;
		NewtonBodyGetMatrix(body, &matA[0][0]);
		for( const NewtonBody* tbody = NewtonWorldGetFirstBody(world); tbody; tbody = NewtonWorldGetNextBody(world, tbody) )
		{
			if( tbody == body || c_bodies_collidable(tbody, body) == true || c_bodies_aabb_overlap(tbody, body) == false ) continue;
			colB = NewtonBodyGetCollision(tbody);
			NewtonBodyGetMatrix(tbody, &matB[0][0]);
			if( NewtonCollisionIntersectionTest(world, colA, &matA[0][0], colB, &matB[0][0], 0) == 1 )
				rb_ary_push(v_touching_bodies, LONG2NUM((long)tbody));
		}
	}*/
	return v_touching_bodies;
}

VALUE body_get_contact_points(VALUE self, VALUE v_body, VALUE inc_non_collidable)
{
	const NewtonBody* body = c_value_to_body(v_body);
	VALUE v_contact_points = rb_ary_new();
	for( NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint) )
	{
		for( void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact))
		{
			NewtonMaterial* material = NewtonContactGetMaterial(contact);
			dVector point;
			dVector normal;
			NewtonMaterialGetContactPositionAndNormal(material, body, &point[0], &normal[0]);
			rb_ary_push(v_contact_points, c_point3d_to_value(point));
		}
	}
	if( RTEST(inc_non_collidable) )
	{
		NewtonWorld* world = NewtonBodyGetWorld(body);
		NewtonCollision* colA = NewtonBodyGetCollision(body);
		NewtonCollision* colB;
		dMatrix matA;
		dMatrix matB;
		dFloat points[3*NON_COL_CONTACTS_CAPACITY];
		dFloat normals[3*NON_COL_CONTACTS_CAPACITY];
		dFloat penetrations[3*NON_COL_CONTACTS_CAPACITY];
		long long attrA[NON_COL_CONTACTS_CAPACITY];
		long long attrB[NON_COL_CONTACTS_CAPACITY];
		NewtonBodyGetMatrix(body, &matA[0][0]);
		for( const NewtonBody* tbody = NewtonWorldGetFirstBody(world); tbody; tbody = NewtonWorldGetNextBody(world, tbody) )
		{
			if( tbody == body || c_bodies_collidable(tbody, body) == true || c_bodies_aabb_overlap(tbody, body) == false ) continue;
			colB = NewtonBodyGetCollision(tbody);
			NewtonBodyGetMatrix(tbody, &matB[0][0]);
			int count = NewtonCollisionCollide(world, NON_COL_CONTACTS_CAPACITY, colA, &matA[0][0], colB, &matB[0][0], points, normals, penetrations, attrA, attrB, 0);
			if( count == 0 ) continue;
			for( int i = 0; i < count*3; i += 3 )
			{
				VALUE point = rb_ary_new3(3, DBL2NUM(points[i+0] * METER_TO_INCH), DBL2NUM(points[i+1] * METER_TO_INCH), DBL2NUM(points[i+2] * METER_TO_INCH));
				rb_ary_push(v_contact_points, point);
			}
		}
	}
	return v_contact_points;
}

VALUE body_get_rotation(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dFloat rotation[4];
	NewtonBodyGetRotation(body, rotation);
	VALUE v_rotation = rb_ary_new2(4);
	for( int i = 0; i < 4; i++ )
		rb_ary_store(v_rotation, i, DBL2NUM(rotation[i]));
	return v_rotation;
}

VALUE body_get_euler_angles(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	dVector angles0;
	dVector angles1;
	NewtonGetEulerAngle(&matrix[0][0], &angles0[0], &angles1[0]);
	return c_vector3d_to_value(angles0);
}

VALUE body_set_euler_angles(VALUE self, VALUE v_body, VALUE v_roll, VALUE v_yaw, VALUE v_pitch)
{
	const NewtonBody* body = c_value_to_body(v_body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	dVector angles((dFloat)NUM2DBL(v_roll), (dFloat)NUM2DBL(v_yaw), (dFloat)NUM2DBL(v_pitch));
	NewtonSetEulerAngle(&angles[0], &matrix[0][0]);
	NewtonBodySetMatrix(body, &matrix[0][0]);
	dVector angles0;
	dVector angles1;
	NewtonGetEulerAngle(&matrix[0][0], &angles0[0], &angles1[0]);
	return c_vector3d_to_value(angles0);
}

VALUE body_apply_pick_and_drag(VALUE self, VALUE v_body, VALUE v_pick_pt, VALUE v_dest_pt, VALUE v_stiffness, VALUE v_damp)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->bstatic ) return Qfalse;
	dVector pick_pt = c_value_to_point3d(v_pick_pt);
	dVector dest_pt = c_value_to_point3d(v_dest_pt);
	dFloat stiffness = (dFloat)NUM2DBL(v_stiffness);
	dFloat damp = (dFloat)NUM2DBL(v_damp);
	// Get data
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	dVector loc_pick_pt = matrix.UntransformVector(pick_pt);
	dVector velocity;
	NewtonBodyGetVelocity(body, &velocity[0]);
	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	// Calculate force
	dVector force = (dest_pt - pick_pt).Scale(data->mass * stiffness);
	force -= velocity.Scale(data->mass * damp);
	// Calculate torque
	dVector point = matrix.RotateVector(loc_pick_pt - com);
	dVector torque = point * force;
	// Make sure body is not frozen
	NewtonBodySetFreezeState(body, 0);
	// Add force and torque
	data->add_force += force;
	data->add_force_state = true;
	data->add_torque += torque;
	data->add_torque_state = true;
	return Qtrue;
}

VALUE body_apply_pick_and_drag2(VALUE self, VALUE v_body, VALUE v_pick_pt, VALUE v_dest_pt, VALUE v_stiffness, VALUE v_angular_damp, VALUE v_time_step)
{
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonWorld* world = NewtonBodyGetWorld(body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( data->bstatic ) return Qfalse;
	dVector pick_pt = c_value_to_point3d(v_pick_pt);
	dVector dest_pt = c_value_to_point3d(v_dest_pt);
	dFloat stiffness = (dFloat)NUM2DBL(v_stiffness);
	dFloat angular_damp = (dFloat)NUM2DBL(v_angular_damp);
	dFloat time_step = (dFloat)NUM2DBL(v_time_step);
	dFloat inv_time_step = 1.0f / time_step;
	dVector com;
	dMatrix matrix;
	dVector omega0;
	dVector veloc0;
	dVector omega1;
	dVector veloc1;
	dVector point_veloc;

	NewtonWorldCriticalSectionLock(world, 0);

	// Calculate the desired impulse
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	NewtonBodyGetOmega(body, &omega0[0]);
	NewtonBodyGetVelocity(body, &veloc0[0]);
	NewtonBodyGetPointVelocity(body, &pick_pt[0], &point_veloc[0]);

	dVector delta_veloc(dest_pt - pick_pt);
	delta_veloc = delta_veloc.Scale(stiffness * inv_time_step) - point_veloc;
	for( int i = 0; i < 3; i++ )
	{
		dVector veloc(0.0f, 0.0f, 0.0f, 0.0f);
		veloc[i] = delta_veloc[i];
		NewtonBodyAddImpulse(body, &veloc[0], &pick_pt[0]);
	}

	// Damp angular velocity
	NewtonBodyGetOmega(body, &omega1[0]);
	NewtonBodyGetVelocity(body, &veloc1[0]);
	omega1 = omega1.Scale(angular_damp);

	// Restore body linear and angular velocity
	NewtonBodySetOmega(body, &omega0[0]);
	NewtonBodySetVelocity(body, &veloc0[0]);

	// Convert the delta velocity change to an external force and torque
	dFloat Ixx, Iyy, Izz, mass;
	NewtonBodyGetMassMatrix(body, &mass, &Ixx, &Iyy, &Izz);

	dVector angular_momentum(Ixx, Iyy, Izz, 0.0f);
	angular_momentum = matrix.RotateVector( angular_momentum.CompProduct(matrix.UnrotateVector(omega1 - omega0)) );

	dVector force( (veloc1 - veloc0).Scale(mass * inv_time_step) );
	dVector torque( angular_momentum.Scale(inv_time_step) );

	// Add force and torque
	data->add_force += force;
	data->add_force_state = true;
	data->add_torque += torque;
	data->add_torque_state = true;

	// Make sure body is not frozen
	NewtonBodySetFreezeState(body, 0);

	NewtonWorldCriticalSectionUnlock(world);
	return Qtrue;
}

VALUE body_get_collision_faces(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonCollision* collision = NewtonBodyGetCollision(body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	VALUE v_faces = rb_ary_new();
	NewtonCollisionForEachPolygonDo(collision, &matrix[0][0], collision_iterator, (void* const)v_faces);
	return v_faces;
}

VALUE body_add_buoyancy(VALUE self, VALUE v_body, VALUE v_normal, VALUE v_height, VALUE v_density, VALUE v_viscosity)
{
	const NewtonBody* body = c_value_to_body(v_body);

	dFloat mass, ixx, iyy, izz;
	NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);
	if( mass == 0.0f ) return Qfalse;

	dVector plane = c_value_to_vector3d(v_normal);
	plane.m_w = (dFloat)NUM2DBL(v_height) * -INCH_TO_METER;

	dFloat density = c_clamp_dfloat((dFloat)NUM2DBL(v_density), 0.001f, NULL);
	dFloat viscosity = 1.0f - c_clamp_dfloat((dFloat)NUM2DBL(v_viscosity), 0.0f, 1.0f);

	NewtonCollision* collision = NewtonBodyGetCollision(body);
	NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);

	if( body_data->volume == 0.0f ) return Qfalse;

	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);

	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	com = matrix.TransformVector(com);

	dVector force;
	dVector torque;
	NewtonConvexCollisionCalculateBuoyancyAcceleration(collision, &matrix[0][0], &com[0], &(world_data->gravity)[0], &plane[0], density, viscosity, &force[0], &torque[0]);

	if( c_vector_get_magnitude(force) > 0 )
	{
		dVector omega;
		NewtonBodyGetOmega(body, &omega[0]);
		omega = omega.Scale(viscosity);
		NewtonBodySetOmega(body, &omega[0]);
		dVector velocity;
		NewtonBodyGetVelocity(body, &velocity[0]);
		velocity = velocity.Scale(viscosity);
		NewtonBodySetVelocity(body, &velocity[0]);

		body_data->add_force += force;
		body_data->add_torque += torque;
		body_data->add_force_state = true;
		body_data->add_torque_state = true;
	}

	return Qtrue;
}

VALUE body_copy(VALUE self, VALUE v_body, VALUE v_matrix, VALUE v_reapply_forces)
{
	const NewtonBody* body = c_value_to_body(v_body);
	NewtonWorld* world = NewtonBodyGetWorld(body);
	NewtonCollision* new_col = NewtonCollisionCreateInstance(NewtonBodyGetCollision(body));

	dMatrix matrix;
	if( v_matrix == Qnil )
		NewtonBodyGetMatrix(body, &matrix[0][0]);
	else
		matrix = c_value_to_matrix(v_matrix);
	if( c_matrix_is_flipped(matrix) )
	{
		matrix.m_front.m_x *= -1;
		matrix.m_front.m_y *= -1;
		matrix.m_front.m_z *= -1;
	}

	NewtonBody* new_body = NewtonCreateDynamicBody(world, new_col, &c_matrix_extract_scale(matrix)[0][0]);
	valid_bodies[(long)new_body] = true;

	NewtonDestroyCollision(new_col);

	dFloat mass, ixx, iyy, izz;
	NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);
	NewtonBodySetMassMatrix(new_body, mass, ixx, iyy, izz);

	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	NewtonBodySetCentreOfMass(new_body, &com[0]);

	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	BodyData* new_data = new BodyData;

	new_data->add_force = dVector(0.0f, 0.0f, 0.0f);
	new_data->set_force = dVector(0.0f, 0.0f, 0.0f);
	new_data->add_torque = dVector(0.0f, 0.0f, 0.0f);
	new_data->set_torque = dVector(0.0f, 0.0f, 0.0f);
	new_data->add_force_state = false;
	new_data->set_force_state = false;
	new_data->add_torque_state = false;
	new_data->set_torque_state = false;
	new_data->dynamic = data->dynamic;
	new_data->bstatic = data->bstatic;
	new_data->density = data->density;
	new_data->volume = data->volume;
	new_data->mass = data->mass;
	new_data->elasticity = data->elasticity;
	new_data->softness = data->softness;
	new_data->static_friction = data->static_friction;
	new_data->dynamic_friction = data->dynamic_friction;
	new_data->friction_enabled = data->friction_enabled;
	new_data->collidable = data->collidable;
	new_data->non_collidable_bodies = std::map<long, bool>(data->non_collidable_bodies);
	new_data->record_touch_data = false;
	new_data->magnet_force = data->magnet_force;
	new_data->magnet_range = data->magnet_range;
	new_data->magnetic = data->magnetic;
	new_data->touchers;
	new_data->destructor_proc = rb_ary_new();
	new_data->user_data = rb_ary_new();
	new_data->matrix_scale = dVector(data->matrix_scale);
	new_data->matrix_changed = false;

	NewtonBodySetUserData(new_body, new_data);
	rb_gc_register_address(&new_data->destructor_proc);
	rb_gc_register_address(&new_data->user_data);

	NewtonBodySetForceAndTorqueCallback(new_body, force_and_torque_callback);
	NewtonBodySetDestructorCallback(new_body, body_destructor_callback);
	NewtonBodySetTransformCallback(new_body, body_transform_callback);

	NewtonBodySetLinearDamping(new_body, NewtonBodyGetLinearDamping(body));
	dVector angular_damp;
	NewtonBodyGetAngularDamping(body, &angular_damp[0]);
	NewtonBodySetAngularDamping(new_body, &angular_damp[0]);

	NewtonBodySetSimulationState(new_body, NewtonBodyGetSimulationState(body));
	NewtonBodySetContinuousCollisionMode(new_body, NewtonBodyGetContinuousCollisionMode(body));

	NewtonBodySetFreezeState(new_body, NewtonBodyGetFreezeState(body));
	NewtonBodySetAutoSleep(new_body, NewtonBodyGetAutoSleep(body));

	if( RTEST(v_reapply_forces) )
	{
		dVector omega;
		dVector velocity;
		NewtonBodyGetOmega(body, &omega[0]);
		NewtonBodyGetVelocity(body, &velocity[0]);
		NewtonBodySetOmega(new_body, &omega[0]);
		NewtonBodySetVelocity(new_body, &velocity[0]);
	}

	return LONG2NUM( (long)new_body );
}

VALUE body_get_destructor_proc(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( RARRAY_LEN(data->destructor_proc) == 0 ) return Qnil;
	return rb_ary_entry(data->destructor_proc, 0);
}

VALUE body_set_destructor_proc(VALUE self, VALUE v_body, VALUE v_proc)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( v_proc == Qnil )
		rb_ary_clear(data->destructor_proc);
	else if( rb_class_of(v_proc) == rb_cProc )
		rb_ary_store(data->destructor_proc, 0, v_proc);
	else
		rb_raise(rb_eTypeError, "Expected nil or a Proc object!");
	return Qtrue;
}

VALUE body_get_user_data(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( RARRAY_LEN(data->user_data) == 0 ) return Qnil;
	return rb_ary_entry(data->user_data, 0);
}

VALUE body_set_user_data(VALUE self, VALUE v_body, VALUE v_user_data)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if( v_user_data == Qnil )
		rb_ary_clear(data->user_data);
	else
		rb_ary_store(data->user_data, 0, v_user_data);
	return Qtrue;
}

VALUE body_get_record_touch_data_state(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return data->record_touch_data ? Qtrue : Qfalse;
}

VALUE body_set_record_touch_data_state(VALUE self, VALUE v_body, VALUE v_state)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->record_touch_data = RTEST(v_state);
	if( !data->record_touch_data ) data->touchers.clear();
	return data->record_touch_data ? Qtrue : Qfalse;
}

VALUE body_get_matrix_scale(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return c_vector3d_to_value( data->matrix_scale );
}

VALUE body_set_matrix_scale(VALUE self, VALUE v_body, VALUE v_scale)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->matrix_scale = c_value_to_vector3d(v_scale);
	return c_vector3d_to_value( data->matrix_scale );
}

VALUE body_matrix_changed(VALUE self, VALUE v_body)
{
	const NewtonBody* body = c_value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return data->matrix_changed ? Qtrue : Qfalse;
}


// *********************************************************************************************************************
//
// Joint Interface
//
// *********************************************************************************************************************

VALUE joint_is_valid(VALUE self, VALUE v_joint)
{
	return c_is_joint_valid(NUM2LONG(v_joint)) ? Qtrue : Qfalse;
}

/*
VALUE joint_create(VALUE self, VALUE v_world)
{
}

VALUE joint_destroy(VALUE self, VALUE v_joint)
{
}

VALUE joint_get_parent(VALUE self, VALUE v_joint)
{
}

VALUE joint_get_child(VALUE self, VALUE v_joint)
{
}

VALUE joint_connect(VALUE self, VALUE v_joint, VALUE v_child_body)
{
}

VALUE joint_disconnect(VALUE self, VALUE v_joint)
{
}

VALUE joint_is_connected(VALUE self, VALUE v_joint)
{
}

VALUE joint_get_position(VALUE self, VALUE v_joint)
{
}

VALUE joint_set_position(VALUE self, VALUE v_joint, VALUE v_position)
{
}

VALUE joint_get_direction(VALUE self, VALUE v_joint)
{
}

VALUE joint_set_direction(VALUE self, VALUE v_joint, VALUE v_direction)
{
}

VALUE joint_is_collidable(VALUE self, VALUE v_joint)
{
}

VALUE joint_set_collidable(VALUE self, VALUE v_joint)
{
}

VALUE joint_get_solver(VALUE self, VALUE v_joint)
{
}

VALUE joint_set_solver(VALUE self, VALUE v_joint, VALUE v_model)
{
}

VALUE joint_get_max_contact_joints(VALUE self, VALUE v_joint)
{
}

VALUE joint_set_max_contact_joints(VALUE self, VALUE v_joint, VALUE v_max)
{
}

VALUE joint_get_stiffness(VALUE self, VALUE v_stiff)
{
}

VALUE joint_set_stiffness(VALUE self, VALUE v_joint, VALUE v_stiffness)
{
}


// *********************************************************************************************************************
//
// Hinge Joint Interface
//
// *********************************************************************************************************************

VALUE hinge_create(VALUE self, VALUE v_joint)
{
}

VALUE hinge_get_min(VALUE self, VALUE v_joint)
{
}

VALUE hinge_set_min(VALUE self, VALUE v_joint, VALUE v_min)
{
}

VALUE hinge_get_max(VALUE self, VALUE v_joint)
{
}

VALUE hinge_set_max(VALUE self, VALUE v_joint, VALUE v_max)
{
}

VALUE hinge_get_limits_state(VALUE self, VALUE v_joint)
{

}

VALUE hinge_set_limits_state(VALUE self, VALUE v_joint, VALUE v_state)
{
}

VALUE hinge_get_friction(VALUE self, VALUE v_joint)
{
}

VALUE hinge_set_friction(VALUE self, VALUE v_joint, VALUE v_friction)
{
}

VALUE hinge_get_angle(VALUE self, VALUE v_joint)
{
}

VALUE hinge_get_omega(VALUE self, VALUE v_joint)
{
}
*/

// *********************************************************************************************************************
//
// Main
//
// *********************************************************************************************************************

void Init_msp_lib( void )
{
	suGeom = rb_define_module( "Geom" );
	suPoint3d = rb_define_class_under( suGeom, "Point3d", rb_cObject );
	suVector3d = rb_define_class_under( suGeom, "Vector3d", rb_cObject );
	suTransformation = rb_define_class_under( suGeom, "Transformation", rb_cObject );

	VALUE mMSPhysics = rb_define_module( "MSPhysics" );

	VALUE mNewton = rb_define_module_under( mMSPhysics, "Newton" );
	VALUE mWorld = rb_define_module_under( mNewton, "World" );
	VALUE mCollision = rb_define_module_under( mNewton, "Collision" );
	VALUE mBodies = rb_define_module_under( mNewton, "Bodies" );
	VALUE mBody = rb_define_module_under( mNewton, "Body" );
	VALUE mJoint = rb_define_module_under( mNewton, "Joint" );
	//VALUE mHinge = rb_define_module_under( mNewton, "Hinge" );

	// Newton Interface
	rb_define_module_function( mNewton, "get_version", VALUEFUNC(get_version), 0 );
	rb_define_module_function( mNewton, "get_float_size", VALUEFUNC(get_float_size), 0 );
	rb_define_module_function( mNewton, "get_memory_used", VALUEFUNC(get_memory_used), 0 );
	rb_define_module_function( mNewton, "get_all_worlds", VALUEFUNC(get_all_worlds), 0 );
	rb_define_module_function( mNewton, "get_all_bodies", VALUEFUNC(get_all_bodies), 0 );

	// Newton World Interface
	rb_define_module_function( mWorld, "is_valid?", VALUEFUNC(world_is_valid), 1 );
	rb_define_module_function( mWorld, "create", VALUEFUNC(world_create), 0 );
	rb_define_module_function( mWorld, "destroy", VALUEFUNC(world_destroy), 1 );
	rb_define_module_function( mWorld, "get_max_threads_count", VALUEFUNC(world_get_max_threads_count), 1 );
	rb_define_module_function( mWorld, "get_threads_count", VALUEFUNC(world_get_threads_count), 1 );
	rb_define_module_function( mWorld, "set_threads_count", VALUEFUNC(world_set_threads_count), 2 );
	rb_define_module_function( mWorld, "destroy_all_bodies", VALUEFUNC(world_destroy_all_bodies), 1 );
	rb_define_module_function( mWorld, "get_body_count", VALUEFUNC(world_get_body_count), 1 );
	rb_define_module_function( mWorld, "get_constraint_count", VALUEFUNC(world_get_constraint_count), 1 );
	rb_define_module_function( mWorld, "update", VALUEFUNC(world_update), 2 );
	rb_define_module_function( mWorld, "update_async", VALUEFUNC(world_update_async), 2 );
	rb_define_module_function( mWorld, "get_gravity", VALUEFUNC(world_get_gravity), 1 );
	rb_define_module_function( mWorld, "set_gravity", VALUEFUNC(world_set_gravity), 2 );
	rb_define_module_function( mWorld, "get_bodies", VALUEFUNC(world_get_bodies), 1 );
	rb_define_module_function( mWorld, "get_bodies_in_aabb", VALUEFUNC(world_get_bodies_in_aabb), 3 );
	rb_define_module_function( mWorld, "get_first_body", VALUEFUNC(world_get_first_body), 1 );
	rb_define_module_function( mWorld, "get_next_body", VALUEFUNC(world_get_next_body), 2 );
	rb_define_module_function( mWorld, "get_solver_model", VALUEFUNC(world_get_solver_model), 1 );
	rb_define_module_function( mWorld, "set_solver_model", VALUEFUNC(world_set_solver_model), 2 );
	rb_define_module_function( mWorld, "get_friction_model", VALUEFUNC(world_get_friction_model), 1 );
	rb_define_module_function( mWorld, "set_friction_model", VALUEFUNC(world_set_friction_model), 2 );
	rb_define_module_function( mWorld, "get_material_thickness", VALUEFUNC(world_get_material_thickness), 1 );
	rb_define_module_function( mWorld, "set_material_thickness", VALUEFUNC(world_set_material_thickness), 2 );
	rb_define_module_function( mWorld, "ray_cast", VALUEFUNC(world_ray_cast), 3 );
	rb_define_module_function( mWorld, "continuous_ray_cast", VALUEFUNC(world_continuous_ray_cast), 3 );
	rb_define_module_function( mWorld, "convex_ray_cast", VALUEFUNC(world_convex_ray_cast), 4 );
	rb_define_module_function( mWorld, "continuous_convex_ray_cast", VALUEFUNC(world_continuous_convex_ray_cast), 4 );
	rb_define_module_function( mWorld, "add_explosion", VALUEFUNC(world_add_explosion), 4 );
	rb_define_module_function( mWorld, "get_aabb", VALUEFUNC(world_get_aabb), 1 );
	rb_define_module_function( mWorld, "get_destructor_proc", VALUEFUNC(world_get_destructor_proc), 1 );
	rb_define_module_function( mWorld, "set_destructor_proc", VALUEFUNC(world_set_destructor_proc), 2 );
	rb_define_module_function( mWorld, "get_user_data", VALUEFUNC(world_get_user_data), 1 );
	rb_define_module_function( mWorld, "set_user_data", VALUEFUNC(world_set_user_data), 2 );
	rb_define_module_function( mWorld, "get_touch_data_at", VALUEFUNC(world_get_touch_data_at), 2 );
	rb_define_module_function( mWorld, "get_touch_data_count", VALUEFUNC(world_get_touch_data_count), 1 );
	rb_define_module_function( mWorld, "get_touching_data_at", VALUEFUNC(world_get_touching_data_at), 2 );
	rb_define_module_function( mWorld, "get_touching_data_count", VALUEFUNC(world_get_touching_data_count), 1 );
	rb_define_module_function( mWorld, "get_untouch_data_at", VALUEFUNC(world_get_untouch_data_at), 2 );
	rb_define_module_function( mWorld, "get_untouch_data_count", VALUEFUNC(world_get_untouch_data_count), 1 );
	rb_define_module_function( mWorld, "get_time", VALUEFUNC(world_get_time), 1 );
	rb_define_module_function( mWorld, "serialize_to_file", VALUEFUNC(world_serialize_to_file), 2 );

	// Newton Collision Interface
	rb_define_module_function( mCollision, "create_null", VALUEFUNC(collision_create_null), 1 );
	rb_define_module_function( mCollision, "create_box", VALUEFUNC(collision_create_box), 6 );
	rb_define_module_function( mCollision, "create_sphere", VALUEFUNC(collision_create_sphere), 4 );
	rb_define_module_function( mCollision, "create_cone", VALUEFUNC(collision_create_cone), 5 );
	rb_define_module_function( mCollision, "create_cylinder", VALUEFUNC(collision_create_cylinder), 5 );
	rb_define_module_function( mCollision, "create_capsule", VALUEFUNC(collision_create_capsule), 5 );
	rb_define_module_function( mCollision, "create_tapered_capsule", VALUEFUNC(collision_create_tapered_capsule), 6 );
	rb_define_module_function( mCollision, "create_tapered_cylinder", VALUEFUNC(collision_create_tapered_cylinder), 6 );
	rb_define_module_function( mCollision, "create_chamfer_cylinder", VALUEFUNC(collision_create_chamfer_cylinder), 5 );
	rb_define_module_function( mCollision, "create_convex_hull", VALUEFUNC(collision_create_convex_hull), 5 );
	rb_define_module_function( mCollision, "create_compound", VALUEFUNC(collision_create_compound), 2 );
	rb_define_module_function( mCollision, "create_compound_from_cd1", VALUEFUNC(collision_create_compound_from_cd1), 6 );
	rb_define_module_function( mCollision, "create_compound_from_cd2", VALUEFUNC(collision_create_compound_from_cd2), 4 );
	rb_define_module_function( mCollision, "create_compound_from_cd3", VALUEFUNC(collision_create_compound_from_cd3), 4 );
	rb_define_module_function( mCollision, "create_static_mesh", VALUEFUNC(collision_create_static_mesh), 4 );
	rb_define_module_function( mCollision, "get_type", VALUEFUNC(collision_get_type), 1 );
	rb_define_module_function( mCollision, "is_valid?", VALUEFUNC(collision_is_valid), 1 );
	rb_define_module_function( mCollision, "destroy", VALUEFUNC(collision_destroy), 1 );

	// Newton Bodies Interface
	rb_define_module_function( mBodies, "aabb_overlap?", VALUEFUNC(bodies_aabb_overlap), 2 );
	rb_define_module_function( mBodies, "collidable?", VALUEFUNC(bodies_collidable), 2 );
	rb_define_module_function( mBodies, "touching?", VALUEFUNC(bodies_touching), 2 );
	rb_define_module_function( mBodies, "get_closest_points", VALUEFUNC(bodies_get_closest_points), 2 );
	rb_define_module_function( mBodies, "get_force_in_between", VALUEFUNC(bodies_get_force_in_between), 2 );

	// Newton Body Interface
	rb_define_module_function( mBody, "is_valid?", VALUEFUNC(body_is_valid), 1 );
	rb_define_module_function( mBody, "create_dynamic", VALUEFUNC(body_create_dynamic), 3 );
	rb_define_module_function( mBody, "destroy", VALUEFUNC(body_destroy), 1 );
	rb_define_module_function( mBody, "get_world", VALUEFUNC(body_get_world), 1 );
	rb_define_module_function( mBody, "get_collision", VALUEFUNC(body_get_collision), 1 );
	rb_define_module_function( mBody, "get_simulation_state", VALUEFUNC(body_get_simulation_state), 1 );
	rb_define_module_function( mBody, "set_simulation_state", VALUEFUNC(body_set_simulation_state), 2 );
	rb_define_module_function( mBody, "get_continuous_collision_state", VALUEFUNC(body_get_continuous_collision_state), 1 );
	rb_define_module_function( mBody, "set_continuous_collision_state", VALUEFUNC(body_set_continuous_collision_state), 2 );
	rb_define_module_function( mBody, "get_matrix", VALUEFUNC(body_get_matrix), 1 );
	rb_define_module_function( mBody, "get_normal_matrix", VALUEFUNC(body_get_normal_matrix), 1 );
	rb_define_module_function( mBody, "set_matrix", VALUEFUNC(body_set_matrix), 2 );
	rb_define_module_function( mBody, "get_velocity", VALUEFUNC(body_get_velocity), 1 );
	rb_define_module_function( mBody, "set_velocity", VALUEFUNC(body_set_velocity), 2 );
	rb_define_module_function( mBody, "get_omega", VALUEFUNC(body_get_omega), 1 );
	rb_define_module_function( mBody, "set_omega", VALUEFUNC(body_set_omega), 2 );
	rb_define_module_function( mBody, "get_centre_of_mass", VALUEFUNC(body_get_centre_of_mass), 1 );
	rb_define_module_function( mBody, "set_centre_of_mass", VALUEFUNC(body_set_centre_of_mass), 2 );
	rb_define_module_function( mBody, "get_position", VALUEFUNC(body_get_position), 2 );
	rb_define_module_function( mBody, "set_position", VALUEFUNC(body_set_position), 3 );
	rb_define_module_function( mBody, "get_mass", VALUEFUNC(body_get_mass), 1 );
	rb_define_module_function( mBody, "set_mass", VALUEFUNC(body_set_mass), 2 );
	rb_define_module_function( mBody, "get_density", VALUEFUNC(body_get_density), 1 );
	rb_define_module_function( mBody, "set_density", VALUEFUNC(body_set_density), 2 );
	rb_define_module_function( mBody, "get_volume", VALUEFUNC(body_get_volume), 1 );
	rb_define_module_function( mBody, "set_volume", VALUEFUNC(body_set_volume), 2 );
	rb_define_module_function( mBody, "is_static?", VALUEFUNC(body_is_static), 1 );
	rb_define_module_function( mBody, "set_static", VALUEFUNC(body_set_static), 2 );
	rb_define_module_function( mBody, "is_collidable?", VALUEFUNC(body_is_collidable), 1 );
	rb_define_module_function( mBody, "set_collidable", VALUEFUNC(body_set_collidable), 2 );
	rb_define_module_function( mBody, "is_frozen?", VALUEFUNC(body_is_frozen), 1 );
	rb_define_module_function( mBody, "set_frozen", VALUEFUNC(body_set_frozen), 2 );
	rb_define_module_function( mBody, "is_sleeping?", VALUEFUNC(body_is_sleeping), 1 );
	rb_define_module_function( mBody, "set_sleeping", VALUEFUNC(body_set_sleeping), 2 );
	rb_define_module_function( mBody, "get_auto_sleep_state", VALUEFUNC(body_get_auto_sleep_state), 1 );
	rb_define_module_function( mBody, "set_auto_sleep_state", VALUEFUNC(body_set_auto_sleep_state), 2 );
	rb_define_module_function( mBody, "is_non_collidable_with?", VALUEFUNC(body_is_non_collidable_with), 2 );
	rb_define_module_function( mBody, "set_non_collidable_with", VALUEFUNC(body_set_non_collidable_with), 3 );
	rb_define_module_function( mBody, "get_non_collidable_bodies", VALUEFUNC(body_get_non_collidable_bodies), 1 );
	rb_define_module_function( mBody, "clear_non_collidable_bodies", VALUEFUNC(body_clear_non_collidable_bodies), 1 );
	rb_define_module_function( mBody, "get_elasticity", VALUEFUNC(body_get_elasticity), 1 );
	rb_define_module_function( mBody, "set_elasticity", VALUEFUNC(body_set_elasticity), 2 );
	rb_define_module_function( mBody, "get_softness", VALUEFUNC(body_get_softness), 1 );
	rb_define_module_function( mBody, "set_softness", VALUEFUNC(body_set_softness), 2 );
	rb_define_module_function( mBody, "get_static_friction", VALUEFUNC(body_get_static_friction), 1 );
	rb_define_module_function( mBody, "set_static_friction", VALUEFUNC(body_set_static_friction), 2 );
	rb_define_module_function( mBody, "get_dynamic_friction", VALUEFUNC(body_get_dynamic_friction), 1 );
	rb_define_module_function( mBody, "set_dynamic_friction", VALUEFUNC(body_set_dynamic_friction), 2 );
	rb_define_module_function( mBody, "get_friction_state", VALUEFUNC(body_get_friction_state), 1 );
	rb_define_module_function( mBody, "set_friction_state", VALUEFUNC(body_set_friction_state), 2 );
	rb_define_module_function( mBody, "get_magnet_force", VALUEFUNC(body_get_magnet_force), 1 );
	rb_define_module_function( mBody, "set_magnet_force", VALUEFUNC(body_set_magnet_force), 2 );
	rb_define_module_function( mBody, "get_magnet_range", VALUEFUNC(body_get_magnet_range), 1 );
	rb_define_module_function( mBody, "set_magnet_range", VALUEFUNC(body_set_magnet_range), 2 );
	rb_define_module_function( mBody, "is_magnetic?", VALUEFUNC(body_is_magnetic), 1 );
	rb_define_module_function( mBody, "set_magnetic", VALUEFUNC(body_set_magnetic), 2 );
	rb_define_module_function( mBody, "get_aabb", VALUEFUNC(body_get_aabb), 1 );
	rb_define_module_function( mBody, "get_linear_damping", VALUEFUNC(body_get_linear_damping), 1 );
	rb_define_module_function( mBody, "set_linear_damping", VALUEFUNC(body_set_linear_damping), 2 );
	rb_define_module_function( mBody, "get_angular_damping", VALUEFUNC(body_get_angular_damping), 1 );
	rb_define_module_function( mBody, "set_angular_damping", VALUEFUNC(body_set_angular_damping), 2 );
	rb_define_module_function( mBody, "get_point_velocity", VALUEFUNC(body_get_point_velocity), 2 );
	rb_define_module_function( mBody, "add_point_force", VALUEFUNC(body_add_point_force), 3 );
	rb_define_module_function( mBody, "add_impulse", VALUEFUNC(body_add_impulse), 3 );
	rb_define_module_function( mBody, "get_force", VALUEFUNC(body_get_force), 1 );
	rb_define_module_function( mBody, "get_force_acc", VALUEFUNC(body_get_force_acc), 1 );
	rb_define_module_function( mBody, "add_force", VALUEFUNC(body_add_force), 2 );
	rb_define_module_function( mBody, "set_force", VALUEFUNC(body_set_force), 2 );
	rb_define_module_function( mBody, "get_torque", VALUEFUNC(body_get_torque), 1 );
	rb_define_module_function( mBody, "get_torque_acc", VALUEFUNC(body_get_torque_acc), 1 );
	rb_define_module_function( mBody, "add_torque", VALUEFUNC(body_add_torque), 2 );
	rb_define_module_function( mBody, "set_torque", VALUEFUNC(body_set_torque), 2 );
	rb_define_module_function( mBody, "get_net_contact_force", VALUEFUNC(body_get_net_contact_force), 1 );
	rb_define_module_function( mBody, "get_contacts", VALUEFUNC(body_get_contacts), 2 );
	rb_define_module_function( mBody, "get_touching_bodies", VALUEFUNC(body_get_touching_bodies), 2 );
	rb_define_module_function( mBody, "get_contact_points", VALUEFUNC(body_get_contact_points), 2 );
	rb_define_module_function( mBody, "get_rotation", VALUEFUNC(body_get_rotation), 1 );
	rb_define_module_function( mBody, "get_euler_angles", VALUEFUNC(body_get_euler_angles), 1 );
	rb_define_module_function( mBody, "set_euler_angles", VALUEFUNC(body_set_euler_angles), 4 );
	rb_define_module_function( mBody, "apply_pick_and_drag", VALUEFUNC(body_apply_pick_and_drag), 5 );
	rb_define_module_function( mBody, "apply_pick_and_drag2", VALUEFUNC(body_apply_pick_and_drag2), 6 );
	rb_define_module_function( mBody, "get_collision_faces", VALUEFUNC(body_get_collision_faces), 1 );
	rb_define_module_function( mBody, "add_buoyancy", VALUEFUNC(body_add_buoyancy), 5 );
	rb_define_module_function( mBody, "copy", VALUEFUNC(body_copy), 3 );
	rb_define_module_function( mBody, "get_destructor_proc", VALUEFUNC(body_get_destructor_proc), 1 );
	rb_define_module_function( mBody, "set_destructor_proc", VALUEFUNC(body_set_destructor_proc), 2 );
	rb_define_module_function( mBody, "get_user_data", VALUEFUNC(body_get_user_data), 1 );
	rb_define_module_function( mBody, "set_user_data", VALUEFUNC(body_set_user_data), 2 );
	rb_define_module_function( mBody, "get_record_touch_data_state", VALUEFUNC(body_get_record_touch_data_state), 1 );
	rb_define_module_function( mBody, "set_record_touch_data_state", VALUEFUNC(body_set_record_touch_data_state), 2 );
	rb_define_module_function( mBody, "get_matrix_scale", VALUEFUNC(body_get_matrix_scale), 1 );
	rb_define_module_function( mBody, "set_matrix_scale", VALUEFUNC(body_set_matrix_scale), 2 );
	rb_define_module_function( mBody, "matrix_changed?", VALUEFUNC(body_matrix_changed), 1 );

	// Joint Interface
	rb_define_module_function( mJoint, "is_valid?", VALUEFUNC(joint_is_valid), 1 );


	// Hinge Joint Interface

}
