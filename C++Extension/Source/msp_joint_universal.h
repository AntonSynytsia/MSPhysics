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

public:
	// Structures
	typedef struct UniversalData
	{
		AngularIntegration* ai0;
		dFloat cur_omega0;
		dFloat cur_accel0;
		dFloat min0;
		dFloat max0;
		bool limits_enabled0;
		dFloat angular_accel0;
		dFloat angular_damp0;
		bool motor_enabled0;
		AngularIntegration* ai1;
		dFloat cur_omega1;
		dFloat cur_accel1;
		dFloat min1;
		dFloat max1;
		bool limits_enabled1;
		dFloat angular_accel1;
		dFloat angular_damp1;
		bool motor_enabled1;
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
	static VALUE get_cur_angle0(VALUE self, VALUE v_joint);
	static VALUE get_cur_omega0(VALUE self, VALUE v_joint);
	static VALUE get_cur_acceleration0(VALUE self, VALUE v_joint);
	static VALUE get_min0(VALUE self, VALUE v_joint);
	static VALUE set_min0(VALUE self, VALUE v_joint, VALUE v_min);
	static VALUE get_max0(VALUE self, VALUE v_joint);
	static VALUE set_max0(VALUE self, VALUE v_joint, VALUE v_max);
	static VALUE enable_limits0(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE limits_enabled0(VALUE self, VALUE v_joint);
	static VALUE get_friction0(VALUE self, VALUE v_joint);
	static VALUE set_friction0(VALUE self, VALUE v_joint, VALUE v_friction);
	static VALUE get_accel0(VALUE self, VALUE v_joint);
	static VALUE set_accel0(VALUE self, VALUE v_joint, VALUE v_accel);
	static VALUE get_damp0(VALUE self, VALUE v_joint);
	static VALUE set_damp0(VALUE self, VALUE v_joint, VALUE v_damp);
	static VALUE enable_motor0(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE motor_enabled0(VALUE self, VALUE v_joint);
	static VALUE get_controller0(VALUE self, VALUE v_joint);
	static VALUE set_controller0(VALUE self, VALUE v_joint, VALUE v_controller);
	static VALUE get_cur_angle1(VALUE self, VALUE v_joint);
	static VALUE get_cur_omega1(VALUE self, VALUE v_joint);
	static VALUE get_cur_acceleration1(VALUE self, VALUE v_joint);
	static VALUE get_min1(VALUE self, VALUE v_joint);
	static VALUE set_min1(VALUE self, VALUE v_joint, VALUE v_min);
	static VALUE get_max1(VALUE self, VALUE v_joint);
	static VALUE set_max1(VALUE self, VALUE v_joint, VALUE v_max);
	static VALUE enable_limits1(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE limits_enabled1(VALUE self, VALUE v_joint);
	static VALUE get_friction1(VALUE self, VALUE v_joint);
	static VALUE set_friction1(VALUE self, VALUE v_joint, VALUE v_friction);
	static VALUE get_accel1(VALUE self, VALUE v_joint);
	static VALUE set_accel1(VALUE self, VALUE v_joint, VALUE v_accel);
	static VALUE get_damp1(VALUE self, VALUE v_joint);
	static VALUE set_damp1(VALUE self, VALUE v_joint, VALUE v_damp);
	static VALUE enable_motor1(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE motor_enabled1(VALUE self, VALUE v_joint);
	static VALUE get_controller1(VALUE self, VALUE v_joint);
	static VALUE set_controller1(VALUE self, VALUE v_joint, VALUE v_controller);
};

void Init_msp_universal(VALUE mNewton);

#endif	/* MSP_UNIVERSAL_H */
