/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_joint_spring.h"
#include "msp_world.h"
#include "msp_body.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Spring::DEFAULT_MIN(-10.0);
const dFloat MSP::Spring::DEFAULT_MAX(10.0);
const bool MSP::Spring::DEFAULT_LIMITS_ENABLED(false);
const bool MSP::Spring::DEFAULT_ROTATION_ENABLED(false);
const int MSP::Spring::DEFAULT_MODE(0);
const dFloat MSP::Spring::DEFAULT_ACCEL(40.0);
const dFloat MSP::Spring::DEFAULT_DAMP(0.1f);
const dFloat MSP::Spring::DEFAULT_STRENGTH(0.8f);
const dFloat MSP::Spring::DEFAULT_SPRING_CONSTANT(40.0);
const dFloat MSP::Spring::DEFAULT_SPRING_DRAG(1.0);
const dFloat MSP::Spring::DEFAULT_START_POSITION(0.0);
const dFloat MSP::Spring::DEFAULT_CONTROLLER(1.0);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Spring::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);

    dFloat inv_timestep = 1.0 / timestep;

    // Calculate position of pivot points and Jacobian direction vectors in global space.
    dMatrix matrix0, matrix1;
    MSP::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

    // Calculate position, velocity, and acceleration
    dFloat last_pos = cj_data->m_cur_pos;
    dFloat last_vel = cj_data->m_cur_vel;
    cj_data->m_cur_pos = (matrix0.m_posit - matrix1.m_posit).DotProduct3(matrix1.m_right);
    cj_data->m_cur_vel = (cj_data->m_cur_pos - last_pos) * inv_timestep;
    cj_data->m_cur_accel = (cj_data->m_cur_vel - last_vel) * inv_timestep;
    dFloat cur_pos = cj_data->m_cur_pos - cj_data->m_start_pos * cj_data->m_controller;

    const dVector& p0 = matrix0.m_posit;
    dVector p1(matrix1.m_posit + matrix1.m_right.Scale(cj_data->m_cur_pos));

    // Restrict movement on axes perpendicular to the pin direction.
    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Restriction rotation along the all, three axis.
    /*dVector q0(p0 + matrix0.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector q1(p1 + matrix1.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector r0(p0 + matrix0.m_front.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector r1(p1 + matrix1.m_front.Scale(MSP::Joint::MIN_PIN_LENGTH));

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &r0[0], &r1[0], &matrix0.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);*/
    NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_front), &matrix1.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    if (cj_data->m_rotation_enabled) {
        NewtonUserJointAddAngularRow(joint, 0.0, &matrix1.m_right[0]);
        NewtonUserJointSetRowMinimumFriction(joint, 0.0);
        NewtonUserJointSetRowMaximumFriction(joint, 0.0);
    }
    else {
        NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_front, matrix1.m_front, matrix1.m_right), &matrix1.m_right[0]);
    }
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Check if need to re-enable limits.
    if (cj_data->m_temp_disable_limits && cur_pos >= cj_data->m_min_pos && cur_pos <= cj_data->m_max_pos)
        cj_data->m_temp_disable_limits = false;

    bool bcontinue = false;
    // Add limits
    if (cj_data->m_limits_enabled && !cj_data->m_temp_disable_limits) {
        if (cj_data->m_min_pos > cj_data->m_max_pos) {
            dVector s1(p0 + matrix1.m_right.Scale((cj_data->m_min_pos + cj_data->m_max_pos) * 0.5f - cur_pos));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix0.m_right[0]);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_max_pos - cj_data->m_min_pos < Joint::LINEAR_LIMIT_EPSILON2) {
            dVector s1(p0 + matrix1.m_right.Scale(-cur_pos));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix0.m_right[0]);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cur_pos < cj_data->m_min_pos) {
            dVector s1(p0 + matrix1.m_right.Scale(cj_data->m_min_pos - cur_pos + Joint::LINEAR_LIMIT_EPSILON));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix0.m_right[0]);
            NewtonUserJointSetRowMinimumFriction(joint, 0.0);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cur_pos > cj_data->m_max_pos) {
            dVector s1(p0 + matrix1.m_right.Scale(cj_data->m_max_pos - cur_pos - Joint::LINEAR_LIMIT_EPSILON));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix0.m_right[0]);
            NewtonUserJointSetRowMaximumFriction(joint, 0.0);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else
            bcontinue = true;
    }
    else
        bcontinue = true;
    // Add friction
    if (bcontinue) {
        if (cj_data->m_mode == 1) {
            NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_right[0]);
            dFloat force = cj_data->m_spring_constant * -cur_pos - cj_data->m_cur_vel * cj_data->m_spring_drag;
            MSP::Body::BodyData* cbody_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(joint_data->m_child));
            dVector force_dir(matrix1.m_right.Scale(force));
            MSP::Body::c_body_add_force(cbody_data, &force_dir[0]);
            if (joint_data->m_parent != nullptr) {
                MSP::Body::BodyData* pbody_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(joint_data->m_parent));
                force_dir = matrix0.m_right.Scale(-force);
                MSP::Body::c_body_add_force(pbody_data, &force_dir[0]);
            }
            //NewtonUserJointSetRowAcceleration(joint, 0.0);
            NewtonUserJointSetRowMinimumFriction(joint, 0.0);
            NewtonUserJointSetRowMaximumFriction(joint, 0.0);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else {
            NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_right[0]);
            dFloat accel = cj_data->m_accel * -cur_pos - cj_data->m_cur_vel * inv_timestep * cj_data->m_damp;
            NewtonUserJointSetRowAcceleration(joint, accel);
            dFloat stiffness = 0.999f - (1.0 - joint_data->m_stiffness * cj_data->m_strength) * Joint::DEFAULT_STIFFNESS_RANGE;
            NewtonUserJointSetRowStiffness(joint, stiffness);
        }
    }
}

void MSP::Spring::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);

    info->m_minLinearDof[0] = -0.0;
    info->m_maxLinearDof[0] = 0.0;
    info->m_minLinearDof[1] = -0.0;
    info->m_maxLinearDof[1] = 0.0;

    if (cj_data->m_limits_enabled) {
        info->m_minLinearDof[2] = (cj_data->m_min_pos - cj_data->m_cur_pos);
        info->m_minLinearDof[2] = (cj_data->m_max_pos - cj_data->m_cur_pos);
    }
    else {
        info->m_minLinearDof[2] = -Joint::CUSTOM_LARGE_VALUE;
        info->m_minLinearDof[2] = Joint::CUSTOM_LARGE_VALUE;
    }

    info->m_minAngularDof[0] = -0.0;
    info->m_maxAngularDof[0] = 0.0;
    info->m_minAngularDof[1] = -0.0;
    info->m_maxAngularDof[1] = 0.0;
    info->m_minAngularDof[2] = -0.0;
    info->m_maxAngularDof[2] = 0.0;
}

void MSP::Spring::on_destroy(MSP::Joint::JointData* joint_data) {
    delete (reinterpret_cast<SpringData*>(joint_data->m_cj_data));
}

void MSP::Spring::on_disconnect(MSP::Joint::JointData* joint_data) {
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_cur_pos = 0.0;
    cj_data->m_cur_vel = 0.0;
    cj_data->m_cur_accel = 0.0;
}

void MSP::Spring::adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix) {
    dMatrix matrix;
    dVector centre;
    NewtonBodyGetMatrix(joint_data->m_child, &matrix[0][0]);
    NewtonBodyGetCentreOfMass(joint_data->m_child, &centre[0]);
    pin_matrix.m_posit = matrix.TransformVector(centre);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Spring::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::SPRING) ? Qtrue : Qfalse;
}

VALUE MSP::Spring::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);
    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::SPRING;
    joint_data->m_cj_data = new SpringData();
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;
    joint_data->m_on_destroy = on_destroy;
    joint_data->m_on_disconnect = on_disconnect;
    //~ joint_data->m_adjust_pin_matrix_proc = adjust_pin_matrix_proc;

    return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::Spring::rbf_get_min(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_min_pos * M_INCH_TO_METER);
}

VALUE MSP::Spring::rbf_set_min(VALUE self, VALUE v_joint, VALUE v_min) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_min_pos = Util::value_to_dFloat(v_min) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Spring::rbf_get_max(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_max_pos * M_INCH_TO_METER);
}

VALUE MSP::Spring::rbf_set_max(VALUE self, VALUE v_joint, VALUE v_max) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_max_pos = Util::value_to_dFloat(v_max) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Spring::rbf_enable_limits(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_limits_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Spring::rbf_limits_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_limits_enabled);
}

VALUE MSP::Spring::rbf_enable_rotation(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_rotation_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Spring::rbf_rotation_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_rotation_enabled);
}

VALUE MSP::Spring::rbf_get_mode(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_mode);
}

VALUE MSP::Spring::rbf_set_mode(VALUE self, VALUE v_joint, VALUE v_mode) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_mode = Util::value_to_int(v_mode) == 1 ? 1 : 0;
    return Qnil;
}

VALUE MSP::Spring::rbf_get_accel(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_accel);
}

VALUE MSP::Spring::rbf_set_accel(VALUE self, VALUE v_joint, VALUE v_accel) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_accel = Util::max_dFloat(Util::value_to_dFloat(v_accel), 0.0);
    return Qnil;
}

VALUE MSP::Spring::rbf_get_damp(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_damp);
}

VALUE MSP::Spring::rbf_set_damp(VALUE self, VALUE v_joint, VALUE v_damp) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_damp = Util::clamp_dFloat(Util::value_to_dFloat(v_damp), 0.0, 1.0);
    return Qnil;
}

VALUE MSP::Spring::rbf_get_strength(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_strength);
}

VALUE MSP::Spring::rbf_set_strength(VALUE self, VALUE v_joint, VALUE v_strength) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_strength = Util::clamp_dFloat(Util::value_to_dFloat(v_strength), 0.0, 1.0);
    return Qnil;
}

VALUE MSP::Spring::rbf_get_spring_constant(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_spring_constant * M_INCH_TO_METER);
}

VALUE MSP::Spring::rbf_set_spring_constant(VALUE self, VALUE v_joint, VALUE v_spring_constant) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_spring_constant = Util::max_dFloat(Util::value_to_dFloat(v_spring_constant) * M_METER_TO_INCH, 0.0);
    return Qnil;
}

VALUE MSP::Spring::rbf_get_spring_drag(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_spring_drag * M_INCH_TO_METER);
}

VALUE MSP::Spring::rbf_set_spring_drag(VALUE self, VALUE v_joint, VALUE v_spring_drag) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_spring_drag = Util::max_dFloat(Util::value_to_dFloat(v_spring_drag) * M_METER_TO_INCH, 0.0);
    return Qnil;
}

VALUE MSP::Spring::rbf_get_start_position(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_start_pos * M_INCH_TO_METER);
}

VALUE MSP::Spring::rbf_set_start_position(VALUE self, VALUE v_joint, VALUE v_pos) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_start_pos = Util::value_to_dFloat(v_pos) * M_METER_TO_INCH;
    dFloat desired_start_pos = cj_data->m_start_pos * cj_data->m_controller;
    if (cj_data->m_desired_start_pos != desired_start_pos) {
        cj_data->m_temp_disable_limits = true;
        cj_data->m_desired_start_pos = desired_start_pos;
        if (joint_data->m_connected)
            NewtonBodySetSleepState(joint_data->m_child, 0);
    }
    return Qnil;
}

VALUE MSP::Spring::rbf_get_controller(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_controller);
}

VALUE MSP::Spring::rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    cj_data->m_controller = Util::value_to_dFloat(v_controller);
    dFloat desired_start_pos = cj_data->m_start_pos * cj_data->m_controller;
    if (cj_data->m_desired_start_pos != desired_start_pos) {
        cj_data->m_temp_disable_limits = true;
        cj_data->m_desired_start_pos = desired_start_pos;
        if (joint_data->m_connected)
            NewtonBodySetSleepState(joint_data->m_child, 0);
    }
    return Qnil;
}

VALUE MSP::Spring::rbf_get_cur_position(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value((cj_data->m_cur_pos - cj_data->m_start_pos * cj_data->m_controller) * M_INCH_TO_METER);
}

VALUE MSP::Spring::rbf_get_cur_velocity(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_vel * M_INCH_TO_METER);
}

VALUE MSP::Spring::rbf_get_cur_acceleration(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SPRING);
    SpringData* cj_data = reinterpret_cast<SpringData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_accel * M_INCH_TO_METER);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Spring::init_ruby(VALUE mNewton) {
    VALUE mSpring = rb_define_module_under(mNewton, "Spring");

    rb_define_module_function(mSpring, "is_valid?", VALUEFUNC(MSP::Spring::rbf_is_valid), 1);
    rb_define_module_function(mSpring, "create", VALUEFUNC(MSP::Spring::rbf_create), 1);
    rb_define_module_function(mSpring, "get_min", VALUEFUNC(MSP::Spring::rbf_get_min), 1);
    rb_define_module_function(mSpring, "set_min", VALUEFUNC(MSP::Spring::rbf_set_min), 2);
    rb_define_module_function(mSpring, "get_max", VALUEFUNC(MSP::Spring::rbf_get_max), 1);
    rb_define_module_function(mSpring, "set_max", VALUEFUNC(MSP::Spring::rbf_set_max), 2);
    rb_define_module_function(mSpring, "enable_limits", VALUEFUNC(MSP::Spring::rbf_enable_limits), 2);
    rb_define_module_function(mSpring, "limits_enabled?", VALUEFUNC(MSP::Spring::rbf_limits_enabled), 1);
    rb_define_module_function(mSpring, "enable_rotation", VALUEFUNC(MSP::Spring::rbf_enable_rotation), 2);
    rb_define_module_function(mSpring, "rotation_enabled?", VALUEFUNC(MSP::Spring::rbf_rotation_enabled), 1);
    rb_define_module_function(mSpring, "get_mode", VALUEFUNC(MSP::Spring::rbf_get_mode), 1);
    rb_define_module_function(mSpring, "set_mode", VALUEFUNC(MSP::Spring::rbf_set_mode), 2);
    rb_define_module_function(mSpring, "get_accel", VALUEFUNC(MSP::Spring::rbf_get_accel), 1);
    rb_define_module_function(mSpring, "set_accel", VALUEFUNC(MSP::Spring::rbf_set_accel), 2);
    rb_define_module_function(mSpring, "get_damp", VALUEFUNC(MSP::Spring::rbf_get_damp), 1);
    rb_define_module_function(mSpring, "set_damp", VALUEFUNC(MSP::Spring::rbf_set_damp), 2);
    rb_define_module_function(mSpring, "get_strength", VALUEFUNC(MSP::Spring::rbf_get_strength), 1);
    rb_define_module_function(mSpring, "set_strength", VALUEFUNC(MSP::Spring::rbf_set_strength), 2);
    rb_define_module_function(mSpring, "get_spring_constant", VALUEFUNC(MSP::Spring::rbf_get_spring_constant), 1);
    rb_define_module_function(mSpring, "set_spring_constant", VALUEFUNC(MSP::Spring::rbf_set_spring_constant), 2);
    rb_define_module_function(mSpring, "get_spring_drag", VALUEFUNC(MSP::Spring::rbf_get_spring_drag), 1);
    rb_define_module_function(mSpring, "set_spring_drag", VALUEFUNC(MSP::Spring::rbf_set_spring_drag), 2);
    rb_define_module_function(mSpring, "get_start_position", VALUEFUNC(MSP::Spring::rbf_get_start_position), 1);
    rb_define_module_function(mSpring, "set_start_position", VALUEFUNC(MSP::Spring::rbf_set_start_position), 2);
    rb_define_module_function(mSpring, "get_controller", VALUEFUNC(MSP::Spring::rbf_get_controller), 1);
    rb_define_module_function(mSpring, "set_controller", VALUEFUNC(MSP::Spring::rbf_set_controller), 2);
    rb_define_module_function(mSpring, "get_cur_position", VALUEFUNC(MSP::Spring::rbf_get_cur_position), 1);
    rb_define_module_function(mSpring, "get_cur_velocity", VALUEFUNC(MSP::Spring::rbf_get_cur_velocity), 1);
    rb_define_module_function(mSpring, "get_cur_acceleration", VALUEFUNC(MSP::Spring::rbf_get_cur_acceleration), 1);
}
