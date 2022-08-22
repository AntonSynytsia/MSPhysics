/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_HINGE_H
#define MSP_HINGE_H

#include "msp.h"
#include "msp_joint.h"

class MSP::Hinge {
public:
    // Constants
    static const dFloat DEFAULT_MIN;
    static const dFloat DEFAULT_MAX;
    static const bool DEFAULT_LIMITS_ENABLED;
    static const int DEFAULT_MODE;
    static const dFloat DEFAULT_FRICTION;
    static const dFloat DEFAULT_ACCEL;
    static const dFloat DEFAULT_DAMP;
    static const dFloat DEFAULT_STRENGTH;
    static const dFloat DEFAULT_SPRING_CONSTANT;
    static const dFloat DEFAULT_SPRING_DRAG;
    static const dFloat DEFAULT_START_ANGLE;
    static const dFloat DEFAULT_CONTROLLER;

    // Structures
    struct HingeData {
        dFloat m_min_ang;
        dFloat m_max_ang;
        bool m_limits_enabled;
        int m_mode;
        dFloat m_friction;
        dFloat m_accel;
        dFloat m_damp;
        dFloat m_strength;
        dFloat m_spring_constant;
        dFloat m_spring_drag;
        dFloat m_start_angle;
        dFloat m_controller;
        AngularIntegration* m_ai;
        dFloat m_cur_omega;
        dFloat m_cur_alpha;
        dFloat m_desired_start_angle;
        bool m_temp_disable_limits;
        HingeData() :
            m_min_ang(DEFAULT_MIN),
            m_max_ang(DEFAULT_MAX),
            m_limits_enabled(DEFAULT_LIMITS_ENABLED),
            m_mode(DEFAULT_MODE),
            m_friction(DEFAULT_FRICTION),
            m_accel(DEFAULT_ACCEL),
            m_damp(DEFAULT_DAMP),
            m_strength(DEFAULT_STRENGTH),
            m_spring_constant(DEFAULT_SPRING_CONSTANT),
            m_spring_drag(DEFAULT_SPRING_DRAG),
            m_start_angle(DEFAULT_START_ANGLE),
            m_controller(DEFAULT_CONTROLLER),
            m_cur_omega(0.0),
            m_cur_alpha(0.0),
            m_desired_start_angle(DEFAULT_START_ANGLE * DEFAULT_CONTROLLER),
            m_temp_disable_limits(true)
        {
            m_ai = new AngularIntegration();
        }
        ~HingeData()
        {
            if (m_ai) {
                delete m_ai;
                m_ai = nullptr;
            }
        }
    };

    // Callback Functions
    static void submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index);
    static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
    static void on_destroy(MSP::Joint::JointData* joint_data);
    static void on_connect(MSP::Joint::JointData* joint_data);
    static void on_disconnect(MSP::Joint::JointData* joint_data);
    static void adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix);

    // Ruby Functions
    static VALUE rbf_is_valid(VALUE self, VALUE v_joint);
    static VALUE rbf_create(VALUE self, VALUE v_joint);
    static VALUE rbf_get_min(VALUE self, VALUE v_joint);
    static VALUE rbf_set_min(VALUE self, VALUE v_joint, VALUE v_min);
    static VALUE rbf_get_max(VALUE self, VALUE v_joint);
    static VALUE rbf_set_max(VALUE self, VALUE v_joint, VALUE v_max);
    static VALUE rbf_enable_limits(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_limits_enabled(VALUE self, VALUE v_joint);
    static VALUE rbf_get_mode(VALUE self, VALUE v_joint);
    static VALUE rbf_set_mode(VALUE self, VALUE v_joint, VALUE v_mode);
    static VALUE rbf_get_friction(VALUE self, VALUE v_joint);
    static VALUE rbf_set_friction(VALUE self, VALUE v_joint, VALUE v_friction);
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
    static VALUE rbf_get_start_angle(VALUE self, VALUE v_joint);
    static VALUE rbf_set_start_angle(VALUE self, VALUE v_joint, VALUE v_angle);
    static VALUE rbf_get_controller(VALUE self, VALUE v_joint);
    static VALUE rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller);
    static VALUE rbf_get_cur_angle(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_omega(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_alpha(VALUE self, VALUE v_joint);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_HINGE_H */
