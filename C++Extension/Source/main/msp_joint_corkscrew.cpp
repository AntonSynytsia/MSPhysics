/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_joint_corkscrew.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Corkscrew::DEFAULT_MIN_POS(-10.0);
const dFloat MSP::Corkscrew::DEFAULT_MAX_POS(10.0);
const bool MSP::Corkscrew::DEFAULT_LIN_LIMITS_ENABLED(false);
const dFloat MSP::Corkscrew::DEFAULT_LIN_FRICTION(0.0);
const dFloat MSP::Corkscrew::DEFAULT_MIN_ANG(-180.0 * M_DEG_TO_RAD);
const dFloat MSP::Corkscrew::DEFAULT_MAX_ANG(180.0 * M_DEG_TO_RAD);
const bool MSP::Corkscrew::DEFAULT_ANG_LIMITS_ENABLED(false);
const dFloat MSP::Corkscrew::DEFAULT_ANG_FRICTION(0.0);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Corkscrew::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);

    dFloat inv_timestep = 1.0 / timestep;

    // Calculate position of pivot points and Jacobian direction vectors in global space.
    dMatrix matrix0, matrix1;
    MSP::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

    // Calculate position, velocity, and linear acceleration
    dFloat last_pos = cj_data->m_cur_pos;
    dFloat last_vel = cj_data->m_cur_vel;
    cj_data->m_cur_pos = (matrix0.m_posit - matrix1.m_posit).DotProduct3(matrix1.m_right);
    cj_data->m_cur_vel = (cj_data->m_cur_pos - last_pos) * inv_timestep;
    cj_data->m_cur_accel = (cj_data->m_cur_vel - last_vel) * inv_timestep;

    // Calculate angle, omega, and angular acceleration
    dFloat last_angle = cj_data->m_ai->get_angle();
    dFloat last_omega = cj_data->m_cur_omega;
    dFloat sin_angle, cos_angle;
    Joint::c_calculate_angle(matrix1.m_front, matrix0.m_front, matrix0.m_right, sin_angle, cos_angle);
    cj_data->m_ai->update(cos_angle, sin_angle);
    cj_data->m_cur_omega = (cj_data->m_ai->get_angle() - last_angle) * inv_timestep;
    cj_data->m_cur_alpha = (cj_data->m_cur_omega - last_omega) * inv_timestep;
    dFloat cur_angle = cj_data->m_ai->get_angle();

    const dVector& p0 = matrix0.m_posit;
    dVector p1(matrix1.m_posit + matrix1.m_right.Scale(cj_data->m_cur_pos));

    // Restrict movement on axes perpendicular to the pin direction.
    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Restriction rotation along the two axis perpendicular to pin direction.
    dVector q0(p0 + matrix0.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector q1(p1 + matrix1.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Add linear limits and friction
    bool bcontinue = false;
    if (cj_data->m_lin_limits_enabled) {
        if (cj_data->m_min_pos > cj_data->m_max_pos) {
            dVector s1(p0 + matrix1.m_right.Scale((cj_data->m_min_pos + cj_data->m_max_pos) * 0.5f - cj_data->m_cur_pos));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix1.m_right[0]);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_max_pos - cj_data->m_min_pos < Joint::LINEAR_LIMIT_EPSILON2) {
            dVector s1(p0 + matrix1.m_right.Scale(-cj_data->m_cur_pos));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix1.m_right[0]);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_cur_pos < cj_data->m_min_pos) {
            dVector s1(p0 + matrix1.m_right.Scale(cj_data->m_min_pos - cj_data->m_cur_pos + Joint::LINEAR_LIMIT_EPSILON));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix1.m_right[0]);
            NewtonUserJointSetRowMinimumFriction(joint, 0.0);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_cur_pos > cj_data->m_max_pos) {
            dVector s1(p0 + matrix1.m_right.Scale(cj_data->m_max_pos - cj_data->m_cur_pos - Joint::LINEAR_LIMIT_EPSILON));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix1.m_right[0]);
            NewtonUserJointSetRowMaximumFriction(joint, 0.0);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else
            bcontinue = true;
    }
    else
        bcontinue = true;
    if (bcontinue) {
        NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_right[0]);
        NewtonUserJointSetRowAcceleration(joint, -cj_data->m_cur_vel * inv_timestep);
        NewtonUserJointSetRowMinimumFriction(joint, -cj_data->m_lin_friction);
        NewtonUserJointSetRowMaximumFriction(joint, cj_data->m_lin_friction);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }

    // Add angular limits and friction.
    bcontinue = false;
    if (cj_data->m_ang_limits_enabled) {
        if (cj_data->m_min_ang > cj_data->m_max_ang) {
            NewtonUserJointAddAngularRow(joint, (cj_data->m_min_ang + cj_data->m_max_ang) * 0.5f - cur_angle, &matrix1.m_right[0]);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_max_ang - cj_data->m_min_ang < Joint::ANGULAR_LIMIT_EPSILON2) {
            NewtonUserJointAddAngularRow(joint, -cur_angle, &matrix1.m_right[0]);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cur_angle < cj_data->m_min_ang) {
            NewtonUserJointAddAngularRow(joint, cj_data->m_min_ang - cur_angle + Joint::ANGULAR_LIMIT_EPSILON, &matrix1.m_right[0]);
            NewtonUserJointSetRowMinimumFriction(joint, 0.0);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cur_angle > cj_data->m_max_ang) {
            NewtonUserJointAddAngularRow(joint, cj_data->m_max_ang - cur_angle - Joint::ANGULAR_LIMIT_EPSILON, &matrix1.m_right[0]);
            NewtonUserJointSetRowMaximumFriction(joint, 0.0);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else
            bcontinue = true;
    }
    else
        bcontinue = true;
    if (bcontinue) {
        NewtonUserJointAddAngularRow(joint, 0.0, &matrix1.m_right[0]);
        NewtonUserJointSetRowAcceleration(joint, -cj_data->m_cur_omega * inv_timestep);
        NewtonUserJointSetRowMinimumFriction(joint, -cj_data->m_ang_friction);
        NewtonUserJointSetRowMaximumFriction(joint, cj_data->m_ang_friction);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }
}

void MSP::Corkscrew::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);

    info->m_minLinearDof[0] = -0.0;
    info->m_maxLinearDof[0] = 0.0;
    info->m_minLinearDof[1] = -0.0;
    info->m_maxLinearDof[1] = 0.0;

    if (cj_data->m_lin_limits_enabled) {
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

    if (cj_data->m_ang_limits_enabled) {
        info->m_minAngularDof[2] = (cj_data->m_min_ang - cj_data->m_ai->get_angle()) * M_RAD_TO_DEG;
        info->m_maxAngularDof[2] = (cj_data->m_max_ang - cj_data->m_ai->get_angle()) * M_RAD_TO_DEG;
    }
    else {
        info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
        info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
    }
}

void MSP::Corkscrew::on_destroy(MSP::Joint::JointData* joint_data) {
    delete (reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data));
}

void MSP::Corkscrew::on_disconnect(MSP::Joint::JointData* joint_data) {
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    cj_data->m_ai->set_angle(0.0);
    cj_data->m_cur_omega = 0.0;
    cj_data->m_cur_alpha = 0.0;
    cj_data->m_cur_pos = 0.0;
    cj_data->m_cur_vel = 0.0;
    cj_data->m_cur_accel = 0.0;
}

void MSP::Corkscrew::adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix) {
    dMatrix matrix;
    dVector centre;
    NewtonBodyGetMatrix(joint_data->m_child, &matrix[0][0]);
    NewtonBodyGetCentreOfMass(joint_data->m_child, &centre[0]);
    centre = matrix.TransformVector(centre);
    centre = pin_matrix.UntransformVector(centre);
    dVector point(0.0, 0.0, centre.m_z);
    pin_matrix.m_posit = pin_matrix.TransformVector(point);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Corkscrew::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::CORKSCREW) ? Qtrue : Qfalse;
}

VALUE MSP::Corkscrew::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::CORKSCREW;
    joint_data->m_cj_data = new CorkscrewData();
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;
    joint_data->m_on_destroy = on_destroy;
    joint_data->m_on_disconnect = on_disconnect;
    //~ joint_data->adjust_pin_matrix_proc = adjust_pin_matrix_proc;

    return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::Corkscrew::rbf_get_cur_position(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_pos * M_INCH_TO_METER);
}

VALUE MSP::Corkscrew::rbf_get_cur_velocity(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_vel * M_INCH_TO_METER);
}

VALUE MSP::Corkscrew::rbf_get_cur_acceleration(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_accel * M_INCH_TO_METER);
}

VALUE MSP::Corkscrew::rbf_get_min_position(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_min_pos * M_INCH_TO_METER);
}

VALUE MSP::Corkscrew::rbf_set_min_position(VALUE self, VALUE v_joint, VALUE v_min) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    cj_data->m_min_pos = Util::value_to_dFloat(v_min) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Corkscrew::rbf_get_max_position(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_max_pos * M_INCH_TO_METER);
}

VALUE MSP::Corkscrew::rbf_set_max_position(VALUE self, VALUE v_joint, VALUE v_max) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    cj_data->m_max_pos = Util::value_to_dFloat(v_max) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Corkscrew::rbf_enable_linear_limits(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    cj_data->m_lin_limits_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Corkscrew::rbf_linear_limits_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_lin_limits_enabled);
}

VALUE MSP::Corkscrew::rbf_get_linear_friction(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_lin_friction * M_INCH_TO_METER);
}

VALUE MSP::Corkscrew::rbf_set_linear_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    cj_data->m_lin_friction = Util::max_dFloat(Util::value_to_dFloat(v_friction) * M_METER_TO_INCH, 0.0);
    return Qnil;
}

VALUE MSP::Corkscrew::rbf_get_cur_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_ai->get_angle());
}

VALUE MSP::Corkscrew::rbf_get_cur_omega(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_omega);
}

VALUE MSP::Corkscrew::rbf_get_cur_alpha(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_alpha);
}

VALUE MSP::Corkscrew::rbf_get_min_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_min_ang);
}

VALUE MSP::Corkscrew::rbf_set_min_angle(VALUE self, VALUE v_joint, VALUE v_min) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    cj_data->m_min_ang = Util::value_to_dFloat(v_min);
    return Qnil;
}

VALUE MSP::Corkscrew::rbf_get_max_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_max_ang);
}

VALUE MSP::Corkscrew::rbf_set_max_angle(VALUE self, VALUE v_joint, VALUE v_max) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    cj_data->m_max_ang = Util::value_to_dFloat(v_max);
    return Qnil;
}

VALUE MSP::Corkscrew::rbf_enable_angular_limits(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    cj_data->m_ang_limits_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Corkscrew::rbf_angular_limits_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_ang_limits_enabled);
}

VALUE MSP::Corkscrew::rbf_get_angular_friction(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_ang_friction * M_INCH2_TO_METER2);
}

VALUE MSP::Corkscrew::rbf_set_angular_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CORKSCREW);
    CorkscrewData* cj_data = reinterpret_cast<CorkscrewData*>(joint_data->m_cj_data);
    cj_data->m_ang_friction = Util::max_dFloat(Util::value_to_dFloat(v_friction) * M_METER2_TO_INCH2, 0.0);
    return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Corkscrew::init_ruby(VALUE mNewton) {
    VALUE mCorkscrew = rb_define_module_under(mNewton, "Corkscrew");

    rb_define_module_function(mCorkscrew, "is_valid?", VALUEFUNC(MSP::Corkscrew::rbf_is_valid), 1);
    rb_define_module_function(mCorkscrew, "create", VALUEFUNC(MSP::Corkscrew::rbf_create), 1);
    rb_define_module_function(mCorkscrew, "get_cur_position", VALUEFUNC(MSP::Corkscrew::rbf_get_cur_position), 1);
    rb_define_module_function(mCorkscrew, "get_cur_velocity", VALUEFUNC(MSP::Corkscrew::rbf_get_cur_velocity), 1);
    rb_define_module_function(mCorkscrew, "get_cur_acceleration", VALUEFUNC(MSP::Corkscrew::rbf_get_cur_acceleration), 1);
    rb_define_module_function(mCorkscrew, "get_min_position", VALUEFUNC(MSP::Corkscrew::rbf_get_min_position), 1);
    rb_define_module_function(mCorkscrew, "set_min_position", VALUEFUNC(MSP::Corkscrew::rbf_set_min_position), 2);
    rb_define_module_function(mCorkscrew, "get_max_position", VALUEFUNC(MSP::Corkscrew::rbf_get_max_position), 1);
    rb_define_module_function(mCorkscrew, "set_max_position", VALUEFUNC(MSP::Corkscrew::rbf_set_max_position), 2);
    rb_define_module_function(mCorkscrew, "enable_linear_limits", VALUEFUNC(MSP::Corkscrew::rbf_enable_linear_limits), 2);
    rb_define_module_function(mCorkscrew, "linear_limits_enabled?", VALUEFUNC(MSP::Corkscrew::rbf_linear_limits_enabled), 1);
    rb_define_module_function(mCorkscrew, "get_linear_friction", VALUEFUNC(MSP::Corkscrew::rbf_get_linear_friction), 1);
    rb_define_module_function(mCorkscrew, "set_linear_friction", VALUEFUNC(MSP::Corkscrew::rbf_set_linear_friction), 2);
    rb_define_module_function(mCorkscrew, "get_cur_angle", VALUEFUNC(MSP::Corkscrew::rbf_get_cur_angle), 1);
    rb_define_module_function(mCorkscrew, "get_cur_omega", VALUEFUNC(MSP::Corkscrew::rbf_get_cur_omega), 1);
    rb_define_module_function(mCorkscrew, "get_cur_alpha", VALUEFUNC(MSP::Corkscrew::rbf_get_cur_alpha), 1);
    rb_define_module_function(mCorkscrew, "get_min_angle", VALUEFUNC(MSP::Corkscrew::rbf_get_min_angle), 1);
    rb_define_module_function(mCorkscrew, "set_min_angle", VALUEFUNC(MSP::Corkscrew::rbf_set_min_angle), 2);
    rb_define_module_function(mCorkscrew, "get_max_angle", VALUEFUNC(MSP::Corkscrew::rbf_get_max_angle), 1);
    rb_define_module_function(mCorkscrew, "set_max_angle", VALUEFUNC(MSP::Corkscrew::rbf_set_max_angle), 2);
    rb_define_module_function(mCorkscrew, "enable_angular_limits", VALUEFUNC(MSP::Corkscrew::rbf_enable_angular_limits), 2);
    rb_define_module_function(mCorkscrew, "angular_limits_enabled?", VALUEFUNC(MSP::Corkscrew::rbf_angular_limits_enabled), 1);
    rb_define_module_function(mCorkscrew, "get_angular_friction", VALUEFUNC(MSP::Corkscrew::rbf_get_angular_friction), 1);
    rb_define_module_function(mCorkscrew, "set_angular_friction", VALUEFUNC(MSP::Corkscrew::rbf_set_angular_friction), 2);
}
