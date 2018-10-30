#ifndef MSP_BALL_AND_SOCKET_H
#define MSP_BALL_AND_SOCKET_H

#include "msp.h"
#include "msp_joint.h"

class MSP::BallAndSocket {
private:
	// Constants
	static const dFloat DEFAULT_MAX_CONE_ANGLE;
	static const bool DEFAULT_CONE_LIMITS_ENABLED;
	static const dFloat DEFAULT_MIN_TWIST_ANGLE;
	static const dFloat DEFAULT_MAX_TWIST_ANGLE;
	static const bool DEFAULT_TWIST_LIMITS_ENABLED;
	static const dFloat DEFAULT_FRICTION;
	static const dFloat DEFAULT_CONTROLLER;

	// Structures
	struct BallAndSocketData {
		dFloat m_max_cone_angle;
		dFloat m_min_twist_angle;
		dFloat m_max_twist_angle;
		bool m_cone_limits_enabled;
		bool m_twist_limits_enabled;
		dFloat m_cur_cone_angle;
		AngularIntegration* m_twist_ai;
		dFloat m_cur_twist_omega;
		dFloat m_cur_twist_alpha;
		dFloat m_friction;
		dFloat m_controller;
		BallAndSocketData() :
			m_max_cone_angle(DEFAULT_MAX_CONE_ANGLE),
			m_min_twist_angle(DEFAULT_MIN_TWIST_ANGLE),
			m_max_twist_angle(DEFAULT_MAX_TWIST_ANGLE),
			m_cone_limits_enabled(DEFAULT_CONE_LIMITS_ENABLED),
			m_twist_limits_enabled(DEFAULT_TWIST_LIMITS_ENABLED),
			m_cur_cone_angle(0.0f),
			m_cur_twist_omega(0.0f),
			m_cur_twist_alpha(0.0f),
			m_friction(DEFAULT_FRICTION),
			m_controller(DEFAULT_CONTROLLER)
		{
			m_twist_ai = new AngularIntegration();
		}
		~BallAndSocketData()
		{
			if (m_twist_ai) {
				delete m_twist_ai;
				m_twist_ai = nullptr;
			}
		}
	};

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(MSP::Joint::JointData* joint_data);
	static void on_disconnect(MSP::Joint::JointData* joint_data);

public:
	// Ruby Functions
	static VALUE rbf_is_valid(VALUE self, VALUE v_joint);
	static VALUE rbf_create(VALUE self, VALUE v_joint);
	static VALUE rbf_get_max_cone_angle(VALUE self, VALUE v_joint);
	static VALUE rbf_set_max_cone_angle(VALUE self, VALUE v_joint, VALUE v_angle);
	static VALUE rbf_enable_cone_limits(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE rbf_cone_limits_enabled(VALUE self, VALUE v_joint);
	static VALUE rbf_get_min_twist_angle(VALUE self, VALUE v_joint);
	static VALUE rbf_set_min_twist_angle(VALUE self, VALUE v_joint, VALUE v_angle);
	static VALUE rbf_get_max_twist_angle(VALUE self, VALUE v_joint);
	static VALUE rbf_set_max_twist_angle(VALUE self, VALUE v_joint, VALUE v_angle);
	static VALUE rbf_enable_twist_limits(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE rbf_twist_limits_enabled(VALUE self, VALUE v_joint);
	static VALUE rbf_get_cur_cone_angle(VALUE self, VALUE v_joint);
	static VALUE rbf_get_cur_twist_angle(VALUE self, VALUE v_joint);
	static VALUE rbf_get_cur_twist_omega(VALUE self, VALUE v_joint);
	static VALUE rbf_get_cur_twist_alpha (VALUE self, VALUE v_joint);
	static VALUE rbf_get_friction(VALUE self, VALUE v_joint);
	static VALUE rbf_set_friction(VALUE self, VALUE v_joint, VALUE v_friction);
	static VALUE rbf_get_controller(VALUE self, VALUE v_joint);
	static VALUE rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller);

	// Main
	static void init_ruby(VALUE mNewton);
};

#endif	/* MSP_BALL_AND_SOCKET_H */
