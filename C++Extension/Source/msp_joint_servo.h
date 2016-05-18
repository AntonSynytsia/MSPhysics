#ifndef MSP_SERVO_H
#define MSP_SERVO_H

#include "msp_util.h"
#include "msp_joint.h"

namespace MSNewton {
	class Servo;
}

class MSNewton::Servo {
private:
	// Variables
	static const dFloat DEFAULT_MIN;
	static const dFloat DEFAULT_MAX;
	static const bool DEFAULT_LIMITS_ENABLED;
	static const dFloat DEFAULT_ACCEL;
	static const dFloat DEFAULT_DAMP;
	static const dFloat DEFAULT_STRENGTH;
	static const dFloat DEFAULT_REDUCTION_RATIO;
	static const dFloat DEFAULT_CONTROLLER;
	static const bool DEFAULT_CONTROLLER_ENABLED;
	static const bool DEFAULT_SP_MODE_ENABLED;

public:
	// Structures
	typedef struct ServoData
	{
		AngularIntegration* ai;
		dFloat cur_omega;
		dFloat cur_accel;
		dFloat min;
		dFloat max;
		bool limits_enabled;
		dFloat accel;
		dFloat damp;
		dFloat reduction_ratio;
		dFloat strength;
		dFloat controller;
		bool controller_enabled;
		bool sp_mode_enabled;
	} ServoData;

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(JointData* joint_data);
	static void on_connect(JointData* joint_data);
	static void on_disconnect(JointData* joint_data);

	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_joint);
	static VALUE create(VALUE self, VALUE v_joint);
	static VALUE get_cur_angle(VALUE self, VALUE v_joint);
	static VALUE get_cur_omega(VALUE self, VALUE v_joint);
	static VALUE get_cur_acceleration(VALUE self, VALUE v_joint);
	static VALUE get_min(VALUE self, VALUE v_joint);
	static VALUE set_min(VALUE self, VALUE v_joint, VALUE v_min);
	static VALUE get_max(VALUE self, VALUE v_joint);
	static VALUE set_max(VALUE self, VALUE v_joint, VALUE v_max);
	static VALUE enable_limits(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE limits_enabled(VALUE self, VALUE v_joint);
	static VALUE get_accel(VALUE self, VALUE v_joint);
	static VALUE set_accel(VALUE self, VALUE v_joint, VALUE v_accel);
	static VALUE get_damp(VALUE self, VALUE v_joint);
	static VALUE set_damp(VALUE self, VALUE v_joint, VALUE v_damp);
	static VALUE get_strength(VALUE self, VALUE v_joint);
	static VALUE set_strength(VALUE self, VALUE v_joint, VALUE v_damp);
	static VALUE get_reduction_ratio(VALUE self, VALUE v_joint);
	static VALUE set_reduction_ratio(VALUE self, VALUE v_joint, VALUE v_reduction_ratio);
	static VALUE get_controller(VALUE self, VALUE v_joint);
	static VALUE set_controller(VALUE self, VALUE v_joint, VALUE v_controller);
	static VALUE enable_sp_mode(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE sp_mode_enabled(VALUE self, VALUE v_joint);
};

void Init_msp_servo(VALUE mNewton);

#endif	/* MSP_SERVO_H */
