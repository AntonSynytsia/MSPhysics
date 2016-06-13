#ifndef MSP_SPRING_H
#define MSP_SPRING_H

#include "msp_util.h"
#include "msp_joint.h"

namespace MSNewton {
	class Spring;
}

class MSNewton::Spring {
private:
	// Variables
	static const dFloat DEFAULT_MIN;
	static const dFloat DEFAULT_MAX;
	static const dFloat DEFAULT_ACCEL;
	static const dFloat DEFAULT_DAMP;
	static const bool DEFAULT_LIMITS_ENABLED;
	static const bool DEFAULT_STRONG_MODE_ENABLED;
	static const dFloat DEFAULT_START_POSITION;
	static const dFloat DEFAULT_CONTROLLER;

public:
	// Structures
	typedef struct SpringData
	{
		dFloat min;
		dFloat max;
		dFloat accel;
		dFloat damp;
		bool limits_enabled;
		bool strong_mode_enabled;
		dFloat cur_pos;
		dFloat cur_vel;
		dFloat cur_accel;
		dFloat start_pos;
		dFloat controller;
		dFloat desired_start_pos;
		bool temp_disable_limits;
	} SpringData;

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(JointData* joint_data);
	static void on_disconnect(JointData* joint_data);
	static void adjust_pin_matrix_proc(JointData* joint_data, dMatrix& pin_matrix);

	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_joint);
	static VALUE create(VALUE self, VALUE v_joint);
	static VALUE get_min(VALUE self, VALUE v_joint);
	static VALUE set_min(VALUE self, VALUE v_joint, VALUE v_min);
	static VALUE get_max(VALUE self, VALUE v_joint);
	static VALUE set_max(VALUE self, VALUE v_joint, VALUE v_max);
	static VALUE enable_limits(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE limits_enabled(VALUE self, VALUE v_joint);
	static VALUE enable_strong_mode(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE strong_mode_enabled(VALUE self, VALUE v_joint);
	static VALUE get_accel(VALUE self, VALUE v_joint);
	static VALUE set_accel(VALUE self, VALUE v_joint, VALUE v_accel);
	static VALUE get_damp(VALUE self, VALUE v_joint);
	static VALUE set_damp(VALUE self, VALUE v_joint, VALUE v_damp);
	static VALUE get_cur_position(VALUE self, VALUE v_joint);
	static VALUE get_cur_velocity(VALUE self, VALUE v_joint);
	static VALUE get_cur_acceleration(VALUE self, VALUE v_joint);
	static VALUE get_start_position(VALUE self, VALUE v_joint);
	static VALUE set_start_position(VALUE self, VALUE v_joint, VALUE v_pos);
	static VALUE get_controller(VALUE self, VALUE v_joint);
	static VALUE set_controller(VALUE self, VALUE v_joint, VALUE v_controller);
};

void Init_msp_spring(VALUE mNewton);

#endif	/* MSP_SPRING_H */
