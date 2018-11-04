/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_PLANE_H
#define MSP_PLANE_H

#include "msp.h"
#include "msp_joint.h"

class MSP::Plane {
private:
    // Constants
    static const dFloat DEFAULT_LINEAR_FRICTION;
    static const dFloat DEFAULT_ANGULAR_FRICTION;
    static const bool DEFAULT_ROTATION_ENABLED;

    // Structures
    struct PlaneData {
        dFloat m_lin_friction;
        dFloat m_ang_friction;
        bool m_rotation_enabled;
        PlaneData() :
            m_lin_friction(DEFAULT_LINEAR_FRICTION),
            m_ang_friction(DEFAULT_ANGULAR_FRICTION),
            m_rotation_enabled(DEFAULT_ROTATION_ENABLED)
        {
        }
    };

    // Callback Functions
    static void submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index);
    static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
    static void on_destroy(MSP::Joint::JointData* joint_data);
    static void adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix);

public:
    // Ruby Functions
    static VALUE rbf_is_valid(VALUE self, VALUE v_joint);
    static VALUE rbf_create(VALUE self, VALUE v_joint);
    static VALUE rbf_get_linear_friction(VALUE self, VALUE v_joint);
    static VALUE rbf_set_linear_friction(VALUE self, VALUE v_joint, VALUE v_friction);
    static VALUE rbf_get_angular_friction(VALUE self, VALUE v_joint);
    static VALUE rbf_set_angular_friction(VALUE self, VALUE v_joint, VALUE v_friction);
    static VALUE rbf_enable_rotation(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_rotation_enabled(VALUE self, VALUE v_joint);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_PLANE_H */
