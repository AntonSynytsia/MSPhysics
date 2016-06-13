#ifndef MSP_CURVY_PISTON_H
#define MSP_CURVY_PISTON_H

#include "msp_util.h"
#include "msp_joint.h"

namespace MSNewton {
	class CurvyPiston;
}

class MSNewton::CurvyPiston {
private:
	// Constants
	static const dFloat DEFAULT_ANGULAR_FRICTION;
	static const dFloat DEFAULT_RATE;
	static const dFloat DEFAULT_POWER;
	static const dFloat DEFAULT_REDUCTION_RATIO;
	static const dFloat DEFAULT_CONTROLLER;
	static const bool DEFAULT_CONTROLLER_ENABLED;
	static const bool DEFAULT_LOOP_ENABLED;
	static const bool DEFAULT_ALIGNMENT_ENABLED;
	static const bool DEFAULT_ROTATION_ENABLED;

	// Structures
	typedef struct CurvyPistonData {
		std::vector<dVector> points;
		dFloat curve_len;
		dFloat cur_pos;
		dFloat cur_vel;
		dFloat cur_accel;
		dFloat last_dist;
		dVector cur_point;
		dVector cur_vector;
		dVector cur_tangent;
		dVector last_dir;
		bool cur_data_set;
		dFloat angular_friction;
		dFloat rate;
		dFloat power;
		dFloat reduction_ratio;
		dFloat controller;
		bool controller_enabled;
		bool loop;
		bool align;
		bool rotate;
	} CurvyPistonData;

public:
	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(JointData* joint_data);
	static void on_disconnect(JointData* joint_data);
	static void adjust_pin_matrix_proc(JointData* joint_data, dMatrix& pin_matrix);

	// Helper Functions
	static dFloat c_calc_curve_length(std::vector<dVector>& curve_points, bool loop);
	static bool c_calc_curve_data_at_location(const CurvyPistonData* cj_data, const dVector& location, dVector& point, dVector& vector, dFloat& distance, dVector& min_pt, dVector& max_pt, dFloat& min_len, dFloat& max_len);
	static bool c_calc_curve_line_by_pos(const CurvyPistonData* cj_data, dFloat distance, dVector& point, dVector& vector);

	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_joint);
	static VALUE create(VALUE self, VALUE v_joint);
	static VALUE add_point(VALUE self, VALUE v_joint, VALUE v_position);
	static VALUE remove_point(VALUE self, VALUE v_joint, VALUE v_point_index);
	static VALUE get_points(VALUE self, VALUE v_joint);
	static VALUE get_points_size(VALUE self, VALUE v_joint);
	static VALUE clear_points(VALUE self, VALUE v_joint);
	static VALUE get_point_position(VALUE self, VALUE v_joint, VALUE v_point_index);
	static VALUE set_point_position(VALUE self, VALUE v_joint, VALUE v_point_index, VALUE v_position);
	static VALUE get_length(VALUE self, VALUE v_joint);
	static VALUE get_cur_position(VALUE self, VALUE v_joint);
	static VALUE get_cur_velocity(VALUE self, VALUE v_joint);
	static VALUE get_cur_point(VALUE self, VALUE v_joint);
	static VALUE get_cur_vector(VALUE self, VALUE v_joint);
	static VALUE get_cur_tangent(VALUE self, VALUE v_joint);
	static VALUE get_cur_acceleration(VALUE self, VALUE v_joint);
	static VALUE get_angular_friction(VALUE self, VALUE v_joint);
	static VALUE set_angular_friction(VALUE self, VALUE v_joint, VALUE v_friction);
	static VALUE get_rate(VALUE self, VALUE v_joint);
	static VALUE set_rate(VALUE self, VALUE v_joint, VALUE v_rate);
	static VALUE get_power(VALUE self, VALUE v_joint);
	static VALUE set_power(VALUE self, VALUE v_joint, VALUE v_power);
	static VALUE get_reduction_ratio(VALUE self, VALUE v_joint);
	static VALUE set_reduction_ratio(VALUE self, VALUE v_joint, VALUE v_reduction_ratio);
	static VALUE get_controller(VALUE self, VALUE v_joint);
	static VALUE set_controller(VALUE self, VALUE v_joint, VALUE v_controller);
	static VALUE loop_enabled(VALUE self, VALUE v_joint);
	static VALUE enable_loop(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE alignment_enabled(VALUE self, VALUE v_joint);
	static VALUE enable_alignment(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE rotation_enabled(VALUE self, VALUE v_joint);
	static VALUE enable_rotation(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE get_info_by_pos(VALUE self, VALUE v_joint, VALUE v_pos);
};

void Init_msp_curvy_piston(VALUE mNewton);

#endif	/* MSP_CURVY_PISTON_H */
