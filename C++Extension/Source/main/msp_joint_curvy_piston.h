/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_CURVY_PISTON_H
#define MSP_CURVY_PISTON_H

#include "msp.h"
#include "msp_joint.h"

class MSP::CurvyPiston {
private:
    // Constants
    static const dFloat DEFAULT_ANGULAR_FRICTION;
    static const dFloat DEFAULT_RATE;
    static const dFloat DEFAULT_POWER;
    static const dFloat DEFAULT_ALIGNMENT_POWER;
    static const dFloat DEFAULT_REDUCTION_RATIO;
    static const dFloat DEFAULT_CONTROLLER;
    static const bool DEFAULT_CONTROLLER_ENABLED;
    static const bool DEFAULT_LOOP_ENABLED;
    static const bool DEFAULT_ALIGNMENT_ENABLED;
    static const bool DEFAULT_ROTATION_ENABLED;
    static const int DEFAULT_CONTROLLER_MODE;

    // Structures
    struct EdgeData {
        dMatrix m_normal_matrix;
        dFloat m_length;
        dFloat m_preceding_curve_length;
        unsigned int m_start_index;
        unsigned int m_end_index;
        EdgeData(dFloat length, dFloat preceding_curve_length, unsigned int start_index, unsigned int end_index) :
            m_length(length),
            m_preceding_curve_length(preceding_curve_length),
            m_start_index(start_index),
            m_end_index(end_index)
        {
        }
    };

    struct CurvyPistonData {
        std::vector<dVector> m_points;
        std::map<unsigned int, EdgeData*> m_edges;
        dFloat m_curve_len;
        dFloat m_cur_pos;
        dFloat m_cur_vel;
        dFloat m_cur_accel;
        dFloat m_cur_dist;
        dFloat m_des_pos;
        dMatrix m_cur_normal_matrix;
        bool m_cur_normal_matrix_set;
        dFloat m_angular_friction;
        dFloat m_rate;
        dFloat m_power;
        dFloat m_alignment_power;
        dFloat m_reduction_ratio;
        dFloat m_controller;
        int m_controller_mode;
        bool m_controller_enabled;
        bool m_loop;
        bool m_align;
        bool m_rotate;
        unsigned int m_initial_edge_index;
        CurvyPistonData() :
            m_curve_len(0.0f),
            m_cur_pos(0.0f),
            m_cur_vel(0.0f),
            m_cur_accel(0.0f),
            m_cur_dist(0.0f),
            m_des_pos(0.0f),
            m_cur_normal_matrix_set(false),
            m_angular_friction(DEFAULT_ANGULAR_FRICTION),
            m_rate(DEFAULT_RATE),
            m_power(DEFAULT_POWER),
            m_alignment_power(DEFAULT_ALIGNMENT_POWER),
            m_reduction_ratio(DEFAULT_REDUCTION_RATIO),
            m_controller(DEFAULT_CONTROLLER),
            m_controller_mode(DEFAULT_CONTROLLER_MODE),
            m_controller_enabled(DEFAULT_CONTROLLER_ENABLED),
            m_loop(DEFAULT_LOOP_ENABLED),
            m_align(DEFAULT_ALIGNMENT_ENABLED),
            m_rotate(DEFAULT_ROTATION_ENABLED),
            m_initial_edge_index(0)
        {
        }
    };

    // Callback Functions
    static void submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index);
    static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
    static void on_destroy(MSP::Joint::JointData* joint_data);
    static void on_connect(MSP::Joint::JointData* joint_data);
    static void on_disconnect(MSP::Joint::JointData* joint_data);
    static void adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix);

    // Helper Functions
    static void c_clear_curve_edges(MSP::Joint::JointData* joint_data);
    static void c_update_curve_edges(MSP::Joint::JointData* joint_data);
    static bool c_calc_curve_data_at_position(const MSP::Joint::JointData* joint_data, dFloat position, dMatrix& normal_matrix, dFloat &distance, dFloat &overpass);
    static bool c_calc_curve_data_at_point(const MSP::Joint::JointData* joint_data, const dVector& location, dMatrix& normal_matrix, dFloat &distance, dFloat &overpass);
    static bool c_calc_curve_data_at_point2(const MSP::Joint::JointData* joint_data, const dVector& location, dVector& point, dVector& vector, dFloat &distance, unsigned int &edge_index);
    static bool c_calc_curve_data_at_point3(const MSP::Joint::JointData* joint_data, dFloat last_dist, const dVector& location, dMatrix& normal_matrix, dFloat &distance, dFloat &overpass);
    static void c_calc_pivot_normal(const dMatrix& normal_matrix1, const dMatrix& normal_matrix2, const dVector& pivot_point, const dVector& location, dMatrix& normal_matrix_out);

public:
    // Ruby Functions
    static VALUE rbf_is_valid(VALUE self, VALUE v_joint);
    static VALUE rbf_create(VALUE self, VALUE v_joint);
    static VALUE rbf_add_point(VALUE self, VALUE v_joint, VALUE v_position);
    static VALUE rbf_remove_point(VALUE self, VALUE v_joint, VALUE v_point_index);
    static VALUE rbf_get_points(VALUE self, VALUE v_joint);
    static VALUE rbf_get_points_size(VALUE self, VALUE v_joint);
    static VALUE rbf_clear_points(VALUE self, VALUE v_joint);
    static VALUE rbf_get_point_position(VALUE self, VALUE v_joint, VALUE v_point_index);
    static VALUE rbf_set_point_position(VALUE self, VALUE v_joint, VALUE v_point_index, VALUE v_position);
    static VALUE rbf_get_length(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_position(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_velocity(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_acceleration(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_point(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_vector(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_normal_matrix(VALUE self, VALUE v_joint);
    static VALUE rbf_get_angular_friction(VALUE self, VALUE v_joint);
    static VALUE rbf_set_angular_friction(VALUE self, VALUE v_joint, VALUE v_friction);
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
    static VALUE rbf_loop_enabled(VALUE self, VALUE v_joint);
    static VALUE rbf_enable_loop(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_alignment_enabled(VALUE self, VALUE v_joint);
    static VALUE rbf_enable_alignment(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_get_alignment_power(VALUE self, VALUE v_joint);
    static VALUE rbf_set_alignment_power(VALUE self, VALUE v_joint, VALUE v_power);
    static VALUE rbf_rotation_enabled(VALUE self, VALUE v_joint);
    static VALUE rbf_enable_rotation(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_get_normal_martix_at_position(VALUE self, VALUE v_joint, VALUE v_position);
    static VALUE rbf_get_normal_martix_at_point(VALUE self, VALUE v_joint, VALUE v_point);
    static VALUE rbf_get_normal_matrices(VALUE self, VALUE v_joint);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_CURVY_PISTON_H */
