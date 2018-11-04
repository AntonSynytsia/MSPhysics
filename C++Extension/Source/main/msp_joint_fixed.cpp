/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "msp_joint_fixed.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Fixed::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));

    // Calculate position of pivot points and Jacobian direction vectors in global space.
    dMatrix matrix0, matrix1;
    MSP::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

    const dVector& p0 = matrix0.m_posit;
    const dVector& p1 = matrix1.m_posit;

    // Restrict movement on axes perpendicular to the pin direction.
    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_front[0]);
    //NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP2);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_up[0]);
    //NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP2);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_right[0]);
    //NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP2);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Restriction rotation along the all, three axis.
    /*dVector q0(p0 + matrix0.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector q1(p1 + matrix1.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector r0(p0 + matrix0.m_front.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector r1(p1 + matrix1.m_front.Scale(MSP::Joint::MIN_PIN_LENGTH));

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_front[0]);
    //NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP2);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_up[0]);
    //NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP2);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &r0[0], &r1[0], &matrix0.m_up[0]);
    //NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP2);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);*/

    NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_front), &matrix1.m_front[0]);
    //NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_up[0]);
    //NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_front, matrix1.m_front, matrix1.m_right), &matrix1.m_right[0]);
    //NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
}

void MSP::Fixed::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    info->m_minLinearDof[0] = -0.0f;
    info->m_maxLinearDof[0] = 0.0f;
    info->m_minLinearDof[1] = -0.0f;
    info->m_maxLinearDof[1] = 0.0f;
    info->m_minLinearDof[2] = -0.0f;
    info->m_maxLinearDof[2] = 0.0f;

    info->m_minAngularDof[0] = -0.0f;
    info->m_maxAngularDof[0] = 0.0f;
    info->m_minAngularDof[1] = -0.0f;
    info->m_maxAngularDof[1] = 0.0f;
    info->m_minAngularDof[2] = -0.0f;
    info->m_maxAngularDof[2] = 0.0f;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Fixed::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::FIXED) ? Qtrue : Qfalse;
}

VALUE MSP::Fixed::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::FIXED;
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;

    return MSP::Joint::c_joint_to_value(joint_data);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Fixed::init_ruby(VALUE mNewton) {
    VALUE mFixed = rb_define_module_under(mNewton, "Fixed");

    rb_define_module_function(mFixed, "is_valid?", VALUEFUNC(MSP::Fixed::rbf_is_valid), 1);
    rb_define_module_function(mFixed, "create", VALUEFUNC(MSP::Fixed::rbf_create), 1);
}
