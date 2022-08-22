/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_SLIDER_H
#define MSP_SLIDER_H

#include "msp.h"
#include "msp_joint.h"

class MSP::Slider {
private:
    // Variables
    static const dFloat DEFAULT_MIN;
    static const dFloat DEFAULT_MAX;
    static const bool DEFAULT_LIMITS_ENABLED;
    static const dFloat DEFAULT_FRICTION;
    static const dFloat DEFAULT_CONTROLLER;

    // Structures
    struct SliderData {
        dFloat m_min_pos;
        dFloat m_max_pos;
        dFloat m_cur_pos;
        dFloat m_cur_vel;
        dFloat m_cur_accel;
        bool m_limits_enabled;
        dFloat m_friction;
        dFloat m_controller;
        SliderData() :
            m_min_pos(DEFAULT_MIN),
            m_max_pos(DEFAULT_MAX),
            m_cur_pos(0.0),
            m_cur_vel(0.0),
            m_cur_accel(0.0),
            m_limits_enabled(DEFAULT_LIMITS_ENABLED),
            m_friction(DEFAULT_FRICTION),
            m_controller(DEFAULT_CONTROLLER)
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
    static VALUE rbf_get_friction(VALUE self, VALUE v_joint);
    static VALUE rbf_set_friction(VALUE self, VALUE v_joint, VALUE v_friction);
    static VALUE rbf_get_controller(VALUE self, VALUE v_joint);
    static VALUE rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_SLIDER_H */
