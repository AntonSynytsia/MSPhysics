#ifndef MSP_SPRING_H
#define MSP_SPRING_H

#include "msp.h"
#include "msp_joint.h"

class MSP::Spring {
private:
	// Constants
	static const dFloat DEFAULT_MIN;
	static const dFloat DEFAULT_MAX;
	static const bool DEFAULT_LIMITS_ENABLED;
	static const bool DEFAULT_ROTATION_ENABLED;
	static const int DEFAULT_MODE;
	static const dFloat DEFAULT_ACCEL;
	static const dFloat DEFAULT_DAMP;
	static const dFloat DEFAULT_STRENGTH;
	static const dFloat DEFAULT_SPRING_CONSTANT;
	static const dFloat DEFAULT_SPRING_DRAG;
	static const dFloat DEFAULT_START_POSITION;
	static const dFloat DEFAULT_CONTROLLER;

	// Structures
	struct SpringData
	{
		dFloat m_min_pos;
		dFloat m_max_pos;
		bool m_limits_enabled;
		bool m_rotation_enabled;
		int m_mode;
		dFloat m_accel;
		dFloat m_damp;
		dFloat m_strength;
		dFloat m_spring_constant;
		dFloat m_spring_drag;
		dFloat m_start_pos;
		dFloat m_controller;
		dFloat m_cur_pos;
		dFloat m_cur_vel;
		dFloat m_cur_accel;
		dFloat m_desired_start_pos;
		bool m_temp_disable_limits;
		SpringData() :
			m_min_pos(DEFAULT_MIN),
			m_max_pos(DEFAULT_MAX),
			m_limits_enabled(DEFAULT_LIMITS_ENABLED),
			m_rotation_enabled(DEFAULT_ROTATION_ENABLED),
			m_mode(DEFAULT_MODE),
			m_accel(DEFAULT_ACCEL),
			m_damp(DEFAULT_DAMP),
			m_strength(DEFAULT_STRENGTH),
			m_spring_constant(DEFAULT_SPRING_CONSTANT),
			m_spring_drag(DEFAULT_SPRING_DRAG),
			m_start_pos(DEFAULT_START_POSITION),
			m_controller(DEFAULT_CONTROLLER),
			m_cur_pos(0.0f),
			m_cur_vel(0.0f),
			m_cur_accel(0.0f),
			m_desired_start_pos(DEFAULT_START_POSITION * DEFAULT_CONTROLLER),
			m_temp_disable_limits(true)
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
	static VALUE rbf_get_min(VALUE self, VALUE v_joint);
	static VALUE rbf_set_min(VALUE self, VALUE v_joint, VALUE v_min);
	static VALUE rbf_get_max(VALUE self, VALUE v_joint);
	static VALUE rbf_set_max(VALUE self, VALUE v_joint, VALUE v_max);
	static VALUE rbf_enable_limits(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE rbf_limits_enabled(VALUE self, VALUE v_joint);
	static VALUE rbf_enable_rotation(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE rbf_rotation_enabled(VALUE self, VALUE v_joint);
	static VALUE rbf_get_mode(VALUE self, VALUE v_joint);
	static VALUE rbf_set_mode(VALUE self, VALUE v_joint, VALUE v_mode);
	static VALUE rbf_get_accel(VALUE self, VALUE v_joint);
	static VALUE rbf_set_accel(VALUE self, VALUE v_joint, VALUE v_accel);
	static VALUE rbf_get_damp(VALUE self, VALUE v_joint);
	static VALUE rbf_set_damp(VALUE self, VALUE v_joint, VALUE v_damp);
	static VALUE rbf_get_strength(VALUE self, VALUE v_joint);
	static VALUE rbf_set_strength(VALUE self, VALUE v_joint, VALUE v_strength);
	static VALUE rbf_get_spring_constant(VALUE self, VALUE v_joint);
	static VALUE rbf_set_spring_constant(VALUE self, VALUE v_joint, VALUE v_spring_constant);
	static VALUE rbf_get_spring_drag(VALUE self, VALUE v_joint);
	static VALUE rbf_set_spring_drag(VALUE self, VALUE v_joint, VALUE v_spring_drag);
	static VALUE rbf_get_start_position(VALUE self, VALUE v_joint);
	static VALUE rbf_set_start_position(VALUE self, VALUE v_joint, VALUE v_pos);
	static VALUE rbf_get_controller(VALUE self, VALUE v_joint);
	static VALUE rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller);
	static VALUE rbf_get_cur_position(VALUE self, VALUE v_joint);
	static VALUE rbf_get_cur_velocity(VALUE self, VALUE v_joint);
	static VALUE rbf_get_cur_acceleration(VALUE self, VALUE v_joint);

	// Main
	static void init_ruby(VALUE mNewton);
};

#endif	/* MSP_SPRING_H */
