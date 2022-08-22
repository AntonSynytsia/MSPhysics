/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_CORKSCREW_H
#define MSP_CORKSCREW_H

#include "msp.h"
#include "msp_joint.h"

class MSP::Corkscrew {
private:
    // Constants
    static const dFloat DEFAULT_MIN_ANG;
    static const dFloat DEFAULT_MAX_ANG;
    static const dFloat DEFAULT_MIN_POS;
    static const dFloat DEFAULT_MAX_POS;
    static const bool DEFAULT_ANG_LIMITS_ENABLED;
    static const bool DEFAULT_LIN_LIMITS_ENABLED;
    static const dFloat DEFAULT_ANG_FRICTION;
    static const dFloat DEFAULT_LIN_FRICTION;

    // Structures
    struct CorkscrewData {
        AngularIntegration* m_ai;
        dFloat m_cur_omega;
        dFloat m_cur_alpha;
        dFloat m_cur_pos;
        dFloat m_cur_vel;
        dFloat m_cur_accel;
        dFloat m_min_ang;
        dFloat m_max_ang;
        dFloat m_min_pos;
        dFloat m_max_pos;
        dFloat m_ang_friction;
        dFloat m_lin_friction;
        bool m_ang_limits_enabled;
        bool m_lin_limits_enabled;
        CorkscrewData() :
            m_cur_omega(0.0),
            m_cur_alpha(0.0),
            m_cur_pos(0.0),
            m_cur_vel(0.0),
            m_cur_accel(0.0),
            m_min_ang(DEFAULT_MIN_ANG),
            m_max_ang(DEFAULT_MAX_ANG),
            m_min_pos(DEFAULT_MIN_POS),
            m_max_pos(DEFAULT_MAX_POS),
            m_ang_friction(DEFAULT_ANG_FRICTION),
            m_lin_friction(DEFAULT_LIN_FRICTION),
            m_ang_limits_enabled(DEFAULT_ANG_LIMITS_ENABLED),
            m_lin_limits_enabled(DEFAULT_LIN_LIMITS_ENABLED)
        {
            m_ai = new AngularIntegration();
        }
        ~CorkscrewData()
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
    static void on_disconnect(MSP::Joint::JointData* joint_data);
    static void adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix);

public:
    // Ruby Functions
    static VALUE rbf_is_valid(VALUE self, VALUE v_joint);
    static VALUE rbf_create(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_position(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_velocity(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_acceleration(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_angle(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_omega(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_alpha(VALUE self, VALUE v_joint);
    static VALUE rbf_get_min_position(VALUE self, VALUE v_joint);
    static VALUE rbf_set_min_position(VALUE self, VALUE v_joint, VALUE v_min);
    static VALUE rbf_get_max_position(VALUE self, VALUE v_joint);
    static VALUE rbf_set_max_position(VALUE self, VALUE v_joint, VALUE v_max);
    static VALUE rbf_get_min_angle(VALUE self, VALUE v_joint);
    static VALUE rbf_set_min_angle(VALUE self, VALUE v_joint, VALUE v_min);
    static VALUE rbf_get_max_angle(VALUE self, VALUE v_joint);
    static VALUE rbf_set_max_angle(VALUE self, VALUE v_joint, VALUE v_max);
    static VALUE rbf_enable_linear_limits(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_enable_angular_limits(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_linear_limits_enabled(VALUE self, VALUE v_joint);
    static VALUE rbf_angular_limits_enabled(VALUE self, VALUE v_joint);
    static VALUE rbf_get_linear_friction(VALUE self, VALUE v_joint);
    static VALUE rbf_set_linear_friction(VALUE self, VALUE v_joint, VALUE v_friction);
    static VALUE rbf_get_angular_friction(VALUE self, VALUE v_joint);
    static VALUE rbf_set_angular_friction(VALUE self, VALUE v_joint, VALUE v_friction);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_CORKSCREW_H */
