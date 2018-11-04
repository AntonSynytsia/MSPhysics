/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_POINT_TO_POINT_H
#define MSP_POINT_TO_POINT_H

#include "msp.h"
#include "msp_joint.h"

class MSP::PointToPoint {
private:
    // Variables
    static const dFloat DEFAULT_ACCEL;
    static const dFloat DEFAULT_DAMP;
    static const dFloat DEFAULT_STRENGTH;
    static const int DEFAULT_MODE;
    static const dFloat DEFAULT_START_DISTANCE;
    static const dFloat DEFAULT_CONTROLLER;

    // Structures
    struct PointToPointData {
        dFloat m_accel;
        dFloat m_damp;
        dFloat m_strength;
        int m_mode;
        dFloat m_start_distance;
        dFloat m_controller;
        dFloat m_cur_distance;
        PointToPointData() :
            m_accel(DEFAULT_ACCEL),
            m_damp(DEFAULT_DAMP),
            m_strength(DEFAULT_STRENGTH),
            m_mode(DEFAULT_MODE),
            m_start_distance(DEFAULT_START_DISTANCE),
            m_controller(DEFAULT_CONTROLLER),
            m_cur_distance(0.0f)
        {
        }
    };

    // Callback Functions
    static void submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index);
    static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
    static void on_destroy(MSP::Joint::JointData* joint_data);
    static void on_connect(MSP::Joint::JointData* joint_data);
    static void on_disconnect(MSP::Joint::JointData* joint_data);

public:
    // Ruby Functions
    static VALUE rbf_is_valid(VALUE self, VALUE v_joint);
    static VALUE rbf_create(VALUE self, VALUE v_joint);
    static VALUE rbf_get_accel(VALUE self, VALUE v_joint);
    static VALUE rbf_set_accel(VALUE self, VALUE v_joint, VALUE v_accel);
    static VALUE rbf_get_damp(VALUE self, VALUE v_joint);
    static VALUE rbf_set_damp(VALUE self, VALUE v_joint, VALUE v_damp);
    static VALUE rbf_get_strength(VALUE self, VALUE v_joint);
    static VALUE rbf_set_strength(VALUE self, VALUE v_joint, VALUE v_strength);
    static VALUE rbf_get_mode(VALUE self, VALUE v_joint);
    static VALUE rbf_set_mode(VALUE self, VALUE v_joint, VALUE v_mode);
    static VALUE rbf_get_start_distance(VALUE self, VALUE v_joint);
    static VALUE rbf_set_start_distance(VALUE self, VALUE v_joint, VALUE v_distance);
    static VALUE rbf_get_controller(VALUE self, VALUE v_joint);
    static VALUE rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller);
    static VALUE rbf_get_cur_distance(VALUE self, VALUE v_joint);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_POINT_TO_POINT_H */
