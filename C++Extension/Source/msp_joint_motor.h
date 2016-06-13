#ifndef MSP_MOTOR_H
#define MSP_MOTOR_H

#include "msp_util.h"
#include "msp_joint.h"

namespace MSNewton {
	class Motor;
}

class MSNewton::Motor {
private:
	// Variables
	static const dFloat DEFAULT_ACCEL;
	static const dFloat DEFAULT_DAMP;
	static const bool DEFAULT_FREE_ROTATE_ENABLED;
	static const dFloat DEFAULT_CONTROLLER;

public:
	// Structures
	typedef struct MotorData
	{
		AngularIntegration* ai;
		dFloat cur_omega;
		dFloat cur_accel;
		dFloat accel;
		dFloat damp;
		bool free_rotate_enabled;
		dFloat controller;
	} MotorData;

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(JointData* joint_data);
	static void on_disconnect(JointData* joint_data);
	static void adjust_pin_matrix_proc(JointData* joint_data, dMatrix& pin_matrix);

	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_joint);
	static VALUE create(VALUE self, VALUE v_joint);
	static VALUE get_cur_angle(VALUE self, VALUE v_joint);
	static VALUE get_cur_omega(VALUE self, VALUE v_joint);
	static VALUE get_cur_acceleration(VALUE self, VALUE v_joint);
	static VALUE get_accel(VALUE self, VALUE v_joint);
	static VALUE set_accel(VALUE self, VALUE v_joint, VALUE v_accel);
	static VALUE get_damp(VALUE self, VALUE v_joint);
	static VALUE set_damp(VALUE self, VALUE v_joint, VALUE v_damp);
	static VALUE enable_free_rotate(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE is_free_rotate_enabled(VALUE self, VALUE v_joint);
	static VALUE get_controller(VALUE self, VALUE v_joint);
	static VALUE set_controller(VALUE self, VALUE v_joint, VALUE v_controller);
};

void Init_msp_motor(VALUE mNewton);

#endif	/* MSP_MOTOR_H */
