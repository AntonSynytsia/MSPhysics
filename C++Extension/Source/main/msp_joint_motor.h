/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_MOTOR_H
#define MSP_MOTOR_H

#include "msp.h"
#include "msp_joint.h"

class MSP::Motor {
private:
    // Variables
    static const dFloat DEFAULT_ACCEL;
    static const dFloat DEFAULT_DAMP;
    static const bool DEFAULT_FREE_ROTATE_ENABLED;
    static const dFloat DEFAULT_CONTROLLER;

    // Structures
    struct MotorData
    {
        AngularIntegration* m_ai;
        dFloat m_cur_omega;
        dFloat m_cur_alpha;
        dFloat m_accel;
        dFloat m_damp;
        bool m_free_rotate_enabled;
        dFloat m_controller;
        MotorData() :
            m_cur_omega(0.0f),
            m_cur_alpha (0.0f),
            m_accel(DEFAULT_ACCEL),
            m_damp(DEFAULT_DAMP),
            m_free_rotate_enabled(DEFAULT_FREE_ROTATE_ENABLED),
            m_controller(DEFAULT_CONTROLLER)
        {
            m_ai = new AngularIntegration();
        }
        ~MotorData()
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
    static VALUE rbf_get_cur_angle(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_omega(VALUE self, VALUE v_joint);
    static VALUE rbf_get_cur_alpha(VALUE self, VALUE v_joint);
    static VALUE rbf_get_accel(VALUE self, VALUE v_joint);
    static VALUE rbf_set_accel(VALUE self, VALUE v_joint, VALUE v_accel);
    static VALUE rbf_get_damp(VALUE self, VALUE v_joint);
    static VALUE rbf_set_damp(VALUE self, VALUE v_joint, VALUE v_damp);
    static VALUE rbf_enable_free_rotate(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_is_free_rotate_enabled(VALUE self, VALUE v_joint);
    static VALUE rbf_get_controller(VALUE self, VALUE v_joint);
    static VALUE rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_MOTOR_H */
