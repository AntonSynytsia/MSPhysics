/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "msp_joint_plane.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Plane::DEFAULT_LINEAR_FRICTION(0.0f);
const dFloat MSP::Plane::DEFAULT_ANGULAR_FRICTION(0.0f);
const bool MSP::Plane::DEFAULT_ROTATION_ENABLED(false);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Plane::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    PlaneData* cj_data = reinterpret_cast<PlaneData*>(joint_data->m_cj_data);

    dFloat inv_timestep = 1.0f / timestep;

    // Calculate position of pivot points and Jacobian direction vectors in global space.
    dMatrix matrix0, matrix1;
    MSP::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

    dVector veloc0(0.0f);
    dVector veloc1(0.0f);
    NewtonBodyGetVelocity(joint_data->m_child, &veloc0[0]);
    if (joint_data->m_parent != nullptr)
        NewtonBodyGetVelocity(joint_data->m_parent, &veloc1[0]);

    dVector loc_veloc(matrix1.UnrotateVector(veloc0 - veloc1));
    dVector loc_desired_lin_accel(loc_veloc.Scale(-1.0f * inv_timestep));

    const dVector& p0 = matrix0.m_posit;
    dVector p1(matrix1.UntransformVector(matrix0.m_posit));
    p1.m_z = 0.0f;
    p1 = matrix1.TransformVector(p1);

    // Add friction on axes perpendicular to the pin direction.
    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_front[0]);
    NewtonUserJointSetRowAcceleration(joint, loc_desired_lin_accel.m_x);
    NewtonUserJointSetRowMinimumFriction(joint, -cj_data->m_lin_friction);
    NewtonUserJointSetRowMaximumFriction(joint, cj_data->m_lin_friction);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_up[0]);
    NewtonUserJointSetRowAcceleration(joint, loc_desired_lin_accel.m_y);
    NewtonUserJointSetRowMinimumFriction(joint, -cj_data->m_lin_friction);
    NewtonUserJointSetRowMaximumFriction(joint, cj_data->m_lin_friction);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Restrict movement along the pin direction.
    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_right[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Restriction rotation along the two axis perpendicular to pin.
    NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_front), &matrix1.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    if (cj_data->m_rotation_enabled) {
        dVector omega0(0.0f);
        dVector omega1(0.0f);
        NewtonBodyGetOmega(joint_data->m_child, &omega0[0]);
        if (joint_data->m_parent != nullptr)
            NewtonBodyGetOmega(joint_data->m_parent, &omega1[0]);

        dVector loc_omega(matrix1.UnrotateVector(omega0 - omega1));
        dFloat loc_desired_ang_accel = -loc_omega.m_z * inv_timestep;

        NewtonUserJointAddAngularRow(joint, 0.0f, &matrix1.m_right[0]);
        NewtonUserJointSetRowAcceleration(joint, loc_desired_ang_accel);
        NewtonUserJointSetRowMinimumFriction(joint, -cj_data->m_ang_friction);
        NewtonUserJointSetRowMaximumFriction(joint, cj_data->m_ang_friction);
    }
    else
        NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_front, matrix1.m_front, matrix1.m_right), &matrix1.m_right[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
}

void MSP::Plane::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    info->m_minLinearDof[0] = -Joint::CUSTOM_LARGE_VALUE;
    info->m_maxLinearDof[0] = Joint::CUSTOM_LARGE_VALUE;
    info->m_minLinearDof[1] = -Joint::CUSTOM_LARGE_VALUE;
    info->m_maxLinearDof[1] = Joint::CUSTOM_LARGE_VALUE;
    info->m_minLinearDof[2] = -0.0f;
    info->m_maxLinearDof[2] = 0.0f;

    info->m_minAngularDof[0] = -0.0f;
    info->m_maxAngularDof[0] = 0.0f;
    info->m_minAngularDof[1] = -0.0f;
    info->m_maxAngularDof[1] = 0.0f;
    info->m_minAngularDof[2] = -0.0f;
    info->m_maxAngularDof[2] = 0.0f;
}

void MSP::Plane::on_destroy(MSP::Joint::JointData* joint_data) {
    delete (reinterpret_cast<PlaneData*>(joint_data->m_cj_data));
}

void MSP::Plane::adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix) {
    dMatrix matrix;
    dVector ccentre;
    NewtonBodyGetMatrix(joint_data->m_child, &matrix[0][0]);
    NewtonBodyGetCentreOfMass(joint_data->m_child, &ccentre[0]);
    ccentre = matrix.TransformVector(ccentre);
    pin_matrix.m_posit = ccentre;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Plane::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::PLANE) ? Qtrue : Qfalse;
}

VALUE MSP::Plane::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::PLANE;
    joint_data->m_cj_data = new PlaneData;
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;
    joint_data->m_on_destroy = on_destroy;
    joint_data->m_adjust_pin_matrix_proc = adjust_pin_matrix_proc;

    return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::Plane::rbf_get_linear_friction(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PLANE);
    PlaneData* cj_data = reinterpret_cast<PlaneData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_lin_friction * M_INCH_TO_METER);
}

VALUE MSP::Plane::rbf_set_linear_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PLANE);
    PlaneData* cj_data = reinterpret_cast<PlaneData*>(joint_data->m_cj_data);
    cj_data->m_lin_friction = Util::max_float(Util::value_to_dFloat(v_friction) * M_METER_TO_INCH, 0.0f);
    return Qnil;
}

VALUE MSP::Plane::rbf_get_angular_friction(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PLANE);
    PlaneData* cj_data = reinterpret_cast<PlaneData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_ang_friction * M_INCH2_TO_METER2);
}

VALUE MSP::Plane::rbf_set_angular_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PLANE);
    PlaneData* cj_data = reinterpret_cast<PlaneData*>(joint_data->m_cj_data);
    cj_data->m_ang_friction = Util::max_float(Util::value_to_dFloat(v_friction) * M_METER2_TO_INCH2, 0.0f);
    return Qnil;
}

VALUE MSP::Plane::rbf_enable_rotation(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PLANE);
    PlaneData* cj_data = reinterpret_cast<PlaneData*>(joint_data->m_cj_data);
    cj_data->m_rotation_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Plane::rbf_rotation_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PLANE);
    PlaneData* cj_data = reinterpret_cast<PlaneData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_rotation_enabled);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Plane::init_ruby(VALUE mNewton) {
    VALUE mPlane = rb_define_module_under(mNewton, "Plane");

    rb_define_module_function(mPlane, "is_valid?", VALUEFUNC(MSP::Plane::rbf_is_valid), 1);
    rb_define_module_function(mPlane, "create", VALUEFUNC(MSP::Plane::rbf_create), 1);
    rb_define_module_function(mPlane, "get_linear_friction", VALUEFUNC(MSP::Plane::rbf_get_linear_friction), 1);
    rb_define_module_function(mPlane, "set_linear_friction", VALUEFUNC(MSP::Plane::rbf_set_linear_friction), 2);
    rb_define_module_function(mPlane, "get_angular_friction", VALUEFUNC(MSP::Plane::rbf_get_angular_friction), 1);
    rb_define_module_function(mPlane, "set_angular_friction", VALUEFUNC(MSP::Plane::rbf_set_angular_friction), 2);
    rb_define_module_function(mPlane, "enable_rotation", VALUEFUNC(MSP::Plane::rbf_enable_rotation), 2);
    rb_define_module_function(mPlane, "rotation_enabled?", VALUEFUNC(MSP::Plane::rbf_rotation_enabled), 1);
}
