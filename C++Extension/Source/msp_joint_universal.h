#ifndef MSP_UNIVERSAL_H
#define MSP_UNIVERSAL_H

#include "msp_util.h"
#include "msp_joint.h"

namespace MSNewton {
	class Universal;
}

class MSNewton::Universal {
private:
	// Variables
	static const dFloat DEFAULT_MIN;
	static const dFloat DEFAULT_MAX;
	static const bool DEFAULT_LIMITS_ENABLED;
	static const dFloat DEFAULT_FRICTION;
	static const dFloat DEFAULT_CONTROLLER;

public:
	// Structures
	typedef struct UniversalData
	{
		AngularIntegration* ai1;
		dFloat cur_omega1;
		dFloat cur_accel1;
		dFloat min1;
		dFloat max1;
		bool limits_enabled1;
		AngularIntegration* ai2;
		dFloat cur_omega2;
		dFloat cur_accel2;
		dFloat min2;
		dFloat max2;
		bool limits_enabled2;
		dFloat friction;
		dFloat controller;
	} UniversalData;

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(JointData* joint_data);
	static void on_connect(JointData* joint_data);
	static void on_disconnect(JointData* joint_data);

	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_joint);
	static VALUE create(VALUE self, VALUE v_joint);
	static VALUE get_cur_angle1(VALUE self, VALUE v_joint);
	static VALUE get_cur_omega1(VALUE self, VALUE v_joint);
	static VALUE get_cur_acceleration1(VALUE self, VALUE v_joint);
	static VALUE get_min1(VALUE self, VALUE v_joint);
	static VALUE set_min1(VALUE self, VALUE v_joint, VALUE v_min);
	static VALUE get_max1(VALUE self, VALUE v_joint);
	static VALUE set_max1(VALUE self, VALUE v_joint, VALUE v_max);
	static VALUE enable_limits1(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE limits_enabled1(VALUE self, VALUE v_joint);
	static VALUE get_cur_angle2(VALUE self, VALUE v_joint);
	static VALUE get_cur_omega2(VALUE self, VALUE v_joint);
	static VALUE get_cur_acceleration2(VALUE self, VALUE v_joint);
	static VALUE get_min2(VALUE self, VALUE v_joint);
	static VALUE set_min2(VALUE self, VALUE v_joint, VALUE v_min);
	static VALUE get_max2(VALUE self, VALUE v_joint);
	static VALUE set_max2(VALUE self, VALUE v_joint, VALUE v_max);
	static VALUE enable_limits2(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE limits_enabled2(VALUE self, VALUE v_joint);
	static VALUE get_friction(VALUE self, VALUE v_joint);
	static VALUE set_friction(VALUE self, VALUE v_joint, VALUE v_friction);
	static VALUE get_controller(VALUE self, VALUE v_joint);
	static VALUE set_controller(VALUE self, VALUE v_joint, VALUE v_controller);
};

void Init_msp_universal(VALUE mNewton);

#endif	/* MSP_UNIVERSAL_H */
