/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_joint_up_vector.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dVector MSP::UpVector::DEFAULT_PIN_DIR(0.0, 0.0, 1.0);
const dFloat MSP::UpVector::DEFAULT_ACCEL(40.0);
const dFloat MSP::UpVector::DEFAULT_DAMP(10.0);
const dFloat MSP::UpVector::DEFAULT_STRENGTH(0.9f);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::UpVector::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    UpVectorData* cj_data = reinterpret_cast<UpVectorData*>(joint_data->m_cj_data);

    dFloat inv_timestep = 1.0 / timestep;

    // Calculate position of pivot points and Jacobian direction vectors in global space.
    dMatrix matrix0, matrix1;
    MSP::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

    dMatrix pin_matrix(cj_data->m_pin_matrix * matrix1);

    dFloat last_cone_angle_x = cj_data->m_cone_angle_x;
    dFloat last_cone_angle_y = cj_data->m_cone_angle_y;

    // Calculate current cone angles
    cj_data->m_cone_angle_x = Joint::c_calculate_angle2(pin_matrix.m_right, matrix0.m_right, pin_matrix.m_front);
    cj_data->m_cone_angle_y = Joint::c_calculate_angle2(pin_matrix.m_right, matrix0.m_right, pin_matrix.m_up);

    // Calculate current cone omegas
    dFloat cur_omega_x = (cj_data->m_cone_angle_x - last_cone_angle_x) * inv_timestep;
    dFloat cur_omega_y = (cj_data->m_cone_angle_y - last_cone_angle_y) * inv_timestep;

    dFloat stiffness = 0.999f - (1.0 - joint_data->m_stiffness_ratio * cj_data->m_strength) * Joint::DEFAULT_STIFFNESS_RANGE;

    // Add cone omegas
    NewtonUserJointAddAngularRow(joint, cj_data->m_cone_angle_x, &pin_matrix.m_front[0]);
    dFloat accel = NewtonCalculateSpringDamperAcceleration(timestep, cj_data->m_accel, cj_data->m_cone_angle_x, cj_data->m_damp, cur_omega_x);
    NewtonUserJointSetRowAcceleration(joint, accel);
    NewtonUserJointSetRowStiffness(joint, stiffness);

    NewtonUserJointAddAngularRow(joint, cj_data->m_cone_angle_y, &pin_matrix.m_up[0]);
    accel = NewtonCalculateSpringDamperAcceleration(timestep, cj_data->m_accel, cj_data->m_cone_angle_y, cj_data->m_damp, cur_omega_y);
    NewtonUserJointSetRowAcceleration(joint, accel);
    NewtonUserJointSetRowStiffness(joint, stiffness);
}

void MSP::UpVector::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    info->m_minLinearDof[0] = -Joint::CUSTOM_LARGE_VALUE;
    info->m_maxLinearDof[0] = Joint::CUSTOM_LARGE_VALUE;
    info->m_minLinearDof[1] = -Joint::CUSTOM_LARGE_VALUE;
    info->m_maxLinearDof[1] = Joint::CUSTOM_LARGE_VALUE;
    info->m_minLinearDof[2] = -Joint::CUSTOM_LARGE_VALUE;
    info->m_maxLinearDof[2] = Joint::CUSTOM_LARGE_VALUE;

    info->m_minAngularDof[0] = -0.0;
    info->m_maxAngularDof[0] = 0.0;
    info->m_minAngularDof[1] = -0.0;
    info->m_maxAngularDof[1] = 0.0;
    info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
    info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
}

void MSP::UpVector::on_destroy(MSP::Joint::JointData* joint_data) {
    delete (reinterpret_cast<UpVectorData*>(joint_data->m_cj_data));
}

void MSP::UpVector::on_disconnect(MSP::Joint::JointData* joint_data) {
    UpVectorData* cj_data = reinterpret_cast<UpVectorData*>(joint_data->m_cj_data);
    cj_data->m_cone_angle_x = 0.0;
    cj_data->m_cone_angle_y = 0.0;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::UpVector::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::UP_VECTOR) ? Qtrue : Qfalse;
}

VALUE MSP::UpVector::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);
    UpVectorData* cj_data = new UpVectorData();
    Util::matrix_from_pin_dir(Util::ORIGIN, cj_data->m_pin_dir, cj_data->m_pin_matrix);

    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::UP_VECTOR;
    joint_data->m_cj_data = cj_data;
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;
    joint_data->m_on_destroy = on_destroy;
    joint_data->m_on_disconnect = on_disconnect;

    return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::UpVector::rbf_get_pin_dir(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UP_VECTOR);
    UpVectorData* cj_data = reinterpret_cast<UpVectorData*>(joint_data->m_cj_data);
    return Util::vector_to_value(cj_data->m_pin_dir);
}

VALUE MSP::UpVector::rbf_set_pin_dir(VALUE self, VALUE v_joint, VALUE v_pin_dir) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UP_VECTOR);
    UpVectorData* cj_data = reinterpret_cast<UpVectorData*>(joint_data->m_cj_data);
    dVector pin_dir(Util::value_to_vector(v_pin_dir));
    if (Util::get_vector_magnitude(pin_dir) < M_EPSILON)
        rb_raise(rb_eTypeError, "Zero lengthed vectors are not allowed!");
    Util::normalize_vector(pin_dir);
    if (!Util::vectors_identical(pin_dir, cj_data->m_pin_dir)) {
        cj_data->m_pin_dir = pin_dir;
        Util::matrix_from_pin_dir(Util::ORIGIN, cj_data->m_pin_dir, cj_data->m_pin_matrix);
        if (joint_data->m_connected)
            NewtonBodySetSleepState(joint_data->m_child, 0);
    }
    return Qnil;
}

VALUE MSP::UpVector::rbf_get_accel(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UP_VECTOR);
    UpVectorData* cj_data = reinterpret_cast<UpVectorData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_accel);
}

VALUE MSP::UpVector::rbf_set_accel(VALUE self, VALUE v_joint, VALUE v_accel) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UP_VECTOR);
    UpVectorData* cj_data = reinterpret_cast<UpVectorData*>(joint_data->m_cj_data);
    cj_data->m_accel = Util::max_dFloat(Util::value_to_dFloat(v_accel), 0.0);
    return Qnil;
}

VALUE MSP::UpVector::rbf_get_damp(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UP_VECTOR);
    UpVectorData* cj_data = reinterpret_cast<UpVectorData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_damp);
}

VALUE MSP::UpVector::rbf_set_damp(VALUE self, VALUE v_joint, VALUE v_damp) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UP_VECTOR);
    UpVectorData* cj_data = reinterpret_cast<UpVectorData*>(joint_data->m_cj_data);
    cj_data->m_damp = Util::max_dFloat(Util::value_to_dFloat(v_damp), 0.0);
    return Qnil;
}

VALUE MSP::UpVector::rbf_get_strength(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UP_VECTOR);
    UpVectorData* cj_data = reinterpret_cast<UpVectorData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_strength);
}

VALUE MSP::UpVector::rbf_set_strength(VALUE self, VALUE v_joint, VALUE v_strength) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UP_VECTOR);
    UpVectorData* cj_data = reinterpret_cast<UpVectorData*>(joint_data->m_cj_data);
    cj_data->m_strength = Util::clamp_dFloat(Util::value_to_dFloat(v_strength), 0.0, 1.0);
    return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::UpVector::init_ruby(VALUE mNewton) {
    VALUE mUpVector = rb_define_module_under(mNewton, "UpVector");

    rb_define_module_function(mUpVector, "is_valid?", VALUEFUNC(MSP::UpVector::rbf_is_valid), 1);
    rb_define_module_function(mUpVector, "create", VALUEFUNC(MSP::UpVector::rbf_create), 1);
    rb_define_module_function(mUpVector, "get_pin_dir", VALUEFUNC(MSP::UpVector::rbf_get_pin_dir), 1);
    rb_define_module_function(mUpVector, "set_pin_dir", VALUEFUNC(MSP::UpVector::rbf_set_pin_dir), 2);
    rb_define_module_function(mUpVector, "get_accel", VALUEFUNC(MSP::UpVector::rbf_get_accel), 1);
    rb_define_module_function(mUpVector, "set_accel", VALUEFUNC(MSP::UpVector::rbf_set_accel), 2);
    rb_define_module_function(mUpVector, "get_damp", VALUEFUNC(MSP::UpVector::rbf_get_damp), 1);
    rb_define_module_function(mUpVector, "set_damp", VALUEFUNC(MSP::UpVector::rbf_set_damp), 2);
    rb_define_module_function(mUpVector, "get_strength", VALUEFUNC(MSP::UpVector::rbf_get_strength), 1);
    rb_define_module_function(mUpVector, "set_strength", VALUEFUNC(MSP::UpVector::rbf_set_strength), 2);
}
