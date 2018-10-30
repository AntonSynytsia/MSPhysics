#ifndef MSP_PISTON_H
#define MSP_PISTON_H

#include "msp.h"
#include "msp_joint.h"

class MSP::Piston {
private:
	// Variables
	static const dFloat DEFAULT_MIN;
	static const dFloat DEFAULT_MAX;
	static const bool DEFAULT_LIMITS_ENABLED;
	static const dFloat DEFAULT_RATE;
	static const dFloat DEFAULT_POWER;
	static const dFloat DEFAULT_REDUCTION_RATIO;
	static const dFloat DEFAULT_CONTROLLER;
	static const bool DEFAULT_CONTROLLER_ENABLED;
	static const int DEFAULT_CONTROLLER_MODE;

	// Structures
	struct PistonData {
		dFloat m_cur_pos;
		dFloat m_cur_vel;
		dFloat m_cur_accel;
		dFloat m_des_pos;
		dFloat m_min_pos;
		dFloat m_max_pos;
		bool m_limits_enabled;
		dFloat m_rate;
		dFloat m_power;
		dFloat m_reduction_ratio;
		dFloat m_controller;
		int m_controller_mode;
		bool m_controller_enabled;
		PistonData() :
			m_cur_pos(0.0f),
			m_cur_vel(0.0f),
			m_cur_accel(0.0f),
			m_des_pos(0.0f),
			m_min_pos(DEFAULT_MIN),
			m_max_pos(DEFAULT_MAX),
			m_limits_enabled(DEFAULT_LIMITS_ENABLED),
			m_rate(DEFAULT_RATE),
			m_power(DEFAULT_POWER),
			m_reduction_ratio(DEFAULT_REDUCTION_RATIO),
			m_controller(DEFAULT_CONTROLLER),
			m_controller_mode(DEFAULT_CONTROLLER_MODE),
			m_controller_enabled(DEFAULT_CONTROLLER_ENABLED)
		{
		}
	};

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(MSP::Joint::JointData* joint_data);
	static void on_disconnect(MSP::Joint::JointData* joint_data);
	static void adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix);

public:
	// Ruby Functions
	static VALUE rbf_is_valid(VALUE self, VALUE v_joint);
	static VALUE rbf_create(VALUE self, VALUE v_joint);
	static VALUE rbf_get_cur_position(VALUE self, VALUE v_joint);
	static VALUE rbf_get_cur_velocity(VALUE self, VALUE v_joint);
	static VALUE rbf_get_cur_acceleration(VALUE self, VALUE v_joint);
	static VALUE rbf_get_min(VALUE self, VALUE v_joint);
	static VALUE rbf_set_min(VALUE self, VALUE v_joint, VALUE v_min);
	static VALUE rbf_get_max(VALUE self, VALUE v_joint);
	static VALUE rbf_set_max(VALUE self, VALUE v_joint, VALUE v_max);
	static VALUE rbf_enable_limits(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE rbf_limits_enabled(VALUE self, VALUE v_joint);
	static VALUE rbf_get_rate(VALUE self, VALUE v_joint);
	static VALUE rbf_set_rate(VALUE self, VALUE v_joint, VALUE v_rate);
	static VALUE rbf_get_power(VALUE self, VALUE v_joint);
	static VALUE rbf_set_power(VALUE self, VALUE v_joint, VALUE v_power);
	static VALUE rbf_get_reduction_ratio(VALUE self, VALUE v_joint);
	static VALUE rbf_set_reduction_ratio(VALUE self, VALUE v_joint, VALUE v_reduction_ratio);
	static VALUE rbf_get_controller(VALUE self, VALUE v_joint);
	static VALUE rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller);
	static VALUE rbf_get_controller_mode(VALUE self, VALUE v_joint);
	static VALUE rbf_set_controller_mode(VALUE self, VALUE v_joint, VALUE v_mode);

	// Main
	static void init_ruby(VALUE mNewton);
};

#endif	/* MSP_PISTON_H */
