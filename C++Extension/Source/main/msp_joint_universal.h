/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_UNIVERSAL_H
#define MSP_UNIVERSAL_H

#include "msp.h"
#include "msp_joint.h"

class MSP::Universal {
private:
    // Variables
    static const dFloat DEFAULT_MIN;
    static const dFloat DEFAULT_MAX;
    static const bool DEFAULT_LIMITS_ENABLED;
    static const dFloat DEFAULT_FRICTION;
    static const dFloat DEFAULT_CONTROLLER;

    // Structures
    struct UniversalData
    {
        AngularIntegration* m_ai1;
        dFloat m_cur_omega1;
        dFloat m_cur_alpha1;
        dFloat m_min1;
        dFloat m_max1;
        bool m_limits1_enabled;
        AngularIntegration* m_ai2;
        dFloat m_cur_omega2;
        dFloat m_cur_alpha2;
        dFloat m_min2;
        dFloat m_max2;
        bool m_limits2_enabled;
        dFloat m_friction;
        dFloat m_controller;
        UniversalData() :
            m_cur_omega1(0.0f),
            m_cur_alpha1(0.0f),
            m_min1(DEFAULT_MIN),
            m_max1(DEFAULT_MAX),
            m_limits1_enabled(DEFAULT_LIMITS_ENABLED),
            m_cur_omega2(0.0f),
            m_cur_alpha2(0.0f),
            m_min2(DEFAULT_MIN),
            m_max2(DEFAULT_MAX),
            m_limits2_enabled(DEFAULT_LIMITS_ENABLED),
            m_friction(DEFAULT_FRICTION),
            m_controller(DEFAULT_CONTROLLER)
        {
            m_ai1 = new AngularIntegration();
            m_ai2 = new AngularIntegration();
        }
        ~UniversalData() {
            if (m_ai1) {
                delete m_ai1;
                m_ai1 = nullptr;
            }
            if (m_ai2) {
                delete m_ai2;
                m_ai2 = nullptr;
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
    static VALUE rbf_get_cur_angle1(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_omega1(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_alpha1(VALUE self, VALUE v_joint);
    static VALUE rbf_get_min1(VALUE self, VALUE v_joint);
    static VALUE rbf_set_min1(VALUE self, VALUE v_joint, VALUE v_min);
    static VALUE rbf_get_max1(VALUE self, VALUE v_joint);
    static VALUE rbf_set_max1(VALUE self, VALUE v_joint, VALUE v_max);
    static VALUE rbf_enable_limits1(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_limits1_enabled(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_angle2(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_omega2(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_alpha2(VALUE self, VALUE v_joint);
    static VALUE rbf_get_min2(VALUE self, VALUE v_joint);
    static VALUE rbf_set_min2(VALUE self, VALUE v_joint, VALUE v_min);
    static VALUE rbf_get_max2(VALUE self, VALUE v_joint);
    static VALUE rbf_set_max2(VALUE self, VALUE v_joint, VALUE v_max);
    static VALUE rbf_enable_limits2(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_limits2_enabled(VALUE self, VALUE v_joint);
    static VALUE rbf_get_friction(VALUE self, VALUE v_joint);
    static VALUE rbf_set_friction(VALUE self, VALUE v_joint, VALUE v_friction);
    static VALUE rbf_get_controller(VALUE self, VALUE v_joint);
    static VALUE rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_UNIVERSAL_H */
