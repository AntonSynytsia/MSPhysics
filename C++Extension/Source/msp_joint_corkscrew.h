#ifndef MSP_CORKSCREW_H
#define MSP_CORKSCREW_H

#include "msp_util.h"
#include "msp_joint.h"

namespace MSNewton {
	class Corkscrew;
}

class MSNewton::Corkscrew {
private:
	// Variables
	static const dFloat DEFAULT_MIN_ANG;
	static const dFloat DEFAULT_MAX_ANG;
	static const dFloat DEFAULT_MIN_POS;
	static const dFloat DEFAULT_MAX_POS;
	static const bool DEFAULT_LIN_LIMITS_ENABLED;
	static const bool DEFAULT_ANG_LIMITS_ENABLED;
	static const dFloat DEFAULT_LIN_FRICTION;
	static const dFloat DEFAULT_ANG_FRICTION;

public:
	// Structures
	typedef struct CorkscrewData
	{
		AngularIntegration* ai;
		dFloat cur_pos;
		dFloat cur_vel;
		dFloat cur_omega;
		dFloat cur_lin_accel;
		dFloat cur_ang_accel;
		dFloat min_pos;
		dFloat max_pos;
		dFloat min_ang;
		dFloat max_ang;
		dFloat lin_friction;
		dFloat ang_friction;
		bool lin_limits_enabled;
		bool ang_limits_enabled;
	} CorkscrewData;

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(JointData* joint_data);
	static void on_connect(JointData* joint_data);
	static void on_disconnect(JointData* joint_data);

	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_joint);
	static VALUE create(VALUE self, VALUE v_joint);
	static VALUE get_cur_position(VALUE self, VALUE v_joint);
	static VALUE get_cur_velocity(VALUE self, VALUE v_joint);
	static VALUE get_cur_linear_acceleration(VALUE self, VALUE v_joint);
	static VALUE get_cur_angle(VALUE self, VALUE v_joint);
	static VALUE get_cur_omega(VALUE self, VALUE v_joint);
	static VALUE get_cur_angular_acceleration(VALUE self, VALUE v_joint);
	static VALUE get_min_position(VALUE self, VALUE v_joint);
	static VALUE set_min_position(VALUE self, VALUE v_joint, VALUE v_min);
	static VALUE get_max_position(VALUE self, VALUE v_joint);
	static VALUE set_max_position(VALUE self, VALUE v_joint, VALUE v_max);
	static VALUE get_min_angle(VALUE self, VALUE v_joint);
	static VALUE set_min_angle(VALUE self, VALUE v_joint, VALUE v_min);
	static VALUE get_max_angle(VALUE self, VALUE v_joint);
	static VALUE set_max_angle(VALUE self, VALUE v_joint, VALUE v_max);
	static VALUE enable_linear_limits(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE enable_angular_limits(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE linear_limits_enabled(VALUE self, VALUE v_joint);
	static VALUE angular_limits_enabled(VALUE self, VALUE v_joint);
	static VALUE get_linear_friction(VALUE self, VALUE v_joint);
	static VALUE set_linear_friction(VALUE self, VALUE v_joint, VALUE v_friction);
	static VALUE get_angular_friction(VALUE self, VALUE v_joint);
	static VALUE set_angular_friction(VALUE self, VALUE v_joint, VALUE v_friction);
};

void Init_msp_corkscrew(VALUE mNewton);

#endif	/* MSP_CORKSCREW_H */
