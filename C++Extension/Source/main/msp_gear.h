/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_GEAR_H
#define MSP_GEAR_H

#include "msp.h"
#include "msp_joint.h"

class MSP::Gear {
public:
    // Constants
    static const dFloat DEFAULT_RATIO;

    // Structures
    struct GearData {
        const NewtonWorld* m_world;
        MSP::Joint::JointData* m_joint_data1;
        MSP::Joint::JointData* m_joint_data2;
        dFloat m_ratio;
        VALUE m_user_data;
        dFloat m_initial_position1;
        dFloat m_initial_position2;
        GearData(const NewtonWorld* world, Joint::JointData* joint_data1, Joint::JointData* joint_data2, dFloat initial_position1, dFloat initial_position2) :
            m_world(world),
            m_joint_data1(joint_data1),
            m_joint_data2(joint_data2),
            m_ratio(DEFAULT_RATIO),
            m_user_data(Qnil),
            m_initial_position1(initial_position1),
            m_initial_position2(initial_position2)
        {
        }
        ~GearData()
        {
        }
    };

    // Variables
    static std::set<GearData*> s_valid_gears;

    // Helper Functions
    static bool c_is_gear_valid(GearData* address);
    static VALUE c_gear_to_value(GearData* gear_data);
    static GearData* c_value_to_gear(VALUE v_gear);
    static GearData* c_create(const NewtonWorld* world, MSP::Joint::JointData* joint_data1, MSP::Joint::JointData* joint_data2);
    static void c_destroy(GearData* gear_data);
    static bool c_are_joints_gearable(MSP::Joint::JointData* joint_data1, MSP::Joint::JointData* joint_data2);

    // Ruby Functions
    static VALUE rbf_is_valid(VALUE self, VALUE v_gear);
    static VALUE rbf_create(VALUE self, VALUE v_world, VALUE v_joint1, VALUE v_joint2);
    static VALUE rbf_destroy(VALUE self, VALUE v_gear);
    static VALUE rbf_get_user_data(VALUE self, VALUE v_gear);
    static VALUE rbf_set_user_data(VALUE self, VALUE v_gear, VALUE v_user_data);
    static VALUE rbf_get_ratio(VALUE self, VALUE v_gear);
    static VALUE rbf_set_ratio(VALUE self, VALUE v_gear, VALUE v_ratio);
    static VALUE rbf_get_joint1(VALUE self, VALUE v_gear);
    static VALUE rbf_get_joint2(VALUE self, VALUE v_gear);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_GEAR_H */
