/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_UP_VECTOR_H
#define MSP_UP_VECTOR_H

#include "msp.h"
#include "msp_joint.h"

class MSP::UpVector {
private:
    // Variables
    static const dVector DEFAULT_PIN_DIR;
    static const dFloat DEFAULT_ACCEL;
    static const dFloat DEFAULT_DAMP;
    static const dFloat DEFAULT_STRENGTH;

    // Structures
    struct UpVectorData {
        dVector m_pin_dir;
        dMatrix m_pin_matrix;
        dFloat m_accel;
        dFloat m_damp;
        dFloat m_strength;
        dFloat m_cone_angle_x;
        dFloat m_cone_angle_y;
        UpVectorData() :
            m_pin_dir(DEFAULT_PIN_DIR),
            m_pin_matrix(),
            m_accel(DEFAULT_ACCEL),
            m_damp(DEFAULT_DAMP),
            m_strength(DEFAULT_STRENGTH),
            m_cone_angle_x(0.0),
            m_cone_angle_y(0.0)
        {
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
    static VALUE rbf_get_pin_dir(VALUE self, VALUE v_joint);
    static VALUE rbf_set_pin_dir(VALUE self, VALUE v_joint, VALUE v_pin_dir);
    static VALUE rbf_get_accel(VALUE self, VALUE v_joint);
    static VALUE rbf_set_accel(VALUE self, VALUE v_joint, VALUE v_accel);
    static VALUE rbf_get_damp(VALUE self, VALUE v_joint);
    static VALUE rbf_set_damp(VALUE self, VALUE v_joint, VALUE v_damp);
    static VALUE rbf_get_strength(VALUE self, VALUE v_joint);
    static VALUE rbf_set_strength(VALUE self, VALUE v_joint, VALUE v_strength);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_UP_VECTOR_H */
