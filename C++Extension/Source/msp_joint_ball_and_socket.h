#ifndef MSP_BALL_AND_SOCKET_H
#define MSP_BALL_AND_SOCKET_H

#include "msp_util.h"
#include "msp_joint.h"

namespace MSNewton {
	class BallAndSocket;
}

class MSNewton::BallAndSocket {
private:
	// Variables
	static const dFloat DEFAULT_MAX_CONE_ANGLE;
	static const bool DEFAULT_CONE_LIMITS_ENABLED;
	static const dFloat DEFAULT_MIN_TWIST_ANGLE;
	static const dFloat DEFAULT_MAX_TWIST_ANGLE;
	static const bool DEFAULT_TWIST_LIMITS_ENABLED;
	static const dFloat DEFAULT_FRICTION;
	static const dFloat DEFAULT_CONTROLLER;

public:
	// Structures
	typedef struct BallAndSocketData
	{
		dFloat max_cone_angle;
		dFloat min_twist_angle;
		dFloat max_twist_angle;
		bool cone_limits_enabled;
		bool twist_limits_enabled;
		dFloat cur_cone_angle;
		dFloat cone_angle_cos;
		dFloat cone_angle_sin;
		dFloat cone_angle_half_cos;
		dFloat cone_angle_half_sin;
		AngularIntegration* twist_ai;
		dFloat friction;
		dFloat controller;
	} BallAndSocketData;

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(JointData* data);
	static void on_connect(JointData* data);
	static void on_disconnect(JointData* data);

	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_joint);
	static VALUE create(VALUE self, VALUE v_joint);
	static VALUE get_max_cone_angle(VALUE self, VALUE v_joint);
	static VALUE set_max_cone_angle(VALUE self, VALUE v_joint, VALUE v_angle);
	static VALUE enable_cone_limits(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE cone_limits_enabled(VALUE self, VALUE v_joint);
	static VALUE get_min_twist_angle(VALUE self, VALUE v_joint);
	static VALUE set_min_twist_angle(VALUE self, VALUE v_joint, VALUE v_angle);
	static VALUE get_max_twist_angle(VALUE self, VALUE v_joint);
	static VALUE set_max_twist_angle(VALUE self, VALUE v_joint, VALUE v_angle);
	static VALUE enable_twist_limits(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE twist_limits_enabled(VALUE self, VALUE v_joint);
	static VALUE get_cur_cone_angle(VALUE self, VALUE v_joint);
	static VALUE get_cur_twist_angle(VALUE self, VALUE v_joint);
	static VALUE get_friction(VALUE self, VALUE v_joint);
	static VALUE set_friction(VALUE self, VALUE v_joint, VALUE v_friction);
	static VALUE get_controller(VALUE self, VALUE v_joint);
	static VALUE set_controller(VALUE self, VALUE v_joint, VALUE v_controller);
};

void Init_msp_ball_and_socket(VALUE mNewton);

#endif	/* MSP_BALL_AND_SOCKET_H */
