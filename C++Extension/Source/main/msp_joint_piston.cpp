/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_joint_piston.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Piston::DEFAULT_MIN(-10.0);
const dFloat MSP::Piston::DEFAULT_MAX(10.0);
const bool MSP::Piston::DEFAULT_LIMITS_ENABLED(false);
const dFloat MSP::Piston::DEFAULT_RATE(40.0);
const dFloat MSP::Piston::DEFAULT_POWER(0.0);
const dFloat MSP::Piston::DEFAULT_REDUCTION_RATIO(0.1f);
const dFloat MSP::Piston::DEFAULT_CONTROLLER(0.0);
const int MSP::Piston::DEFAULT_CONTROLLER_MODE(0);
const bool MSP::Piston::DEFAULT_CONTROLLER_ENABLED(false);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Piston::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);

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

    const dVector& p0 = matrix0.m_posit;
    dVector p1(matrix1.m_posit + matrix1.m_right.Scale(cj_data->m_cur_pos));

    // Restrict movement on axes perpendicular to the pin direction.
    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Restriction rotation along the all, three axis.
    dVector q0(p0 + matrix0.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector q1(p1 + matrix1.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector r0(p0 + matrix0.m_front.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector r1(p1 + matrix1.m_front.Scale(MSP::Joint::MIN_PIN_LENGTH));

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &r0[0], &r1[0], &matrix0.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Add limits and friction.
    bool bcontinue = false;
    if (cj_data->m_limits_enabled) {
        if (cj_data->m_min_pos > cj_data->m_max_pos) {
            dVector s1(p0 + matrix1.m_right.Scale((cj_data->m_min_pos + cj_data->m_max_pos) * 0.5f - cj_data->m_cur_pos));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix0.m_right[0]);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_max_pos - cj_data->m_min_pos < Joint::LINEAR_LIMIT_EPSILON2) {
            dVector s1(p0 + matrix1.m_right.Scale(-cj_data->m_cur_pos));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix0.m_right[0]);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_cur_pos < cj_data->m_min_pos) {
            dVector s1(p0 + matrix1.m_right.Scale(cj_data->m_min_pos - cj_data->m_cur_pos + Joint::LINEAR_LIMIT_EPSILON));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix0.m_right[0]);
            NewtonUserJointSetRowMinimumFriction(joint, 0.0);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_cur_pos > cj_data->m_max_pos) {
            dVector s1(p0 + matrix1.m_right.Scale(cj_data->m_max_pos - cj_data->m_cur_pos - Joint::LINEAR_LIMIT_EPSILON));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix0.m_right[0]);
            NewtonUserJointSetRowMaximumFriction(joint, 0.0);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else
            bcontinue = true;
    }
    else
        bcontinue = true;
    // Add power
    if (bcontinue) {
        if (cj_data->m_controller_enabled) {
            dFloat des_vel = 0.0;
            if (cj_data->m_controller_mode == 2) {
                // Control by speed - adaptive
                des_vel = cj_data->m_rate * cj_data->m_controller;
                if (cj_data->m_limits_enabled) {
                    dFloat next_pos = cj_data->m_cur_pos + des_vel * timestep;
                    if (next_pos < cj_data->m_min_pos)
                        des_vel = (cj_data->m_min_pos - cj_data->m_cur_pos) * inv_timestep;
                    else if (next_pos > cj_data->m_max_pos)
                        des_vel = (cj_data->m_max_pos - cj_data->m_cur_pos) * inv_timestep;
                }
                cj_data->m_des_pos = cj_data->m_cur_pos;
            }
            else if (cj_data->m_controller_mode == 1) {
                // Control by speed - strict
                dFloat des_step = cj_data->m_rate * cj_data->m_controller * timestep;
                if (cj_data->m_limits_enabled)
                    cj_data->m_des_pos = Util::clamp_dFloat(cj_data->m_des_pos + des_step, cj_data->m_min_pos, cj_data->m_max_pos);
                else
                    cj_data->m_des_pos += des_step;
                dFloat rel_pos = cj_data->m_des_pos - cj_data->m_cur_pos;
                des_vel = rel_pos * inv_timestep;
            }
            else {
                // Control by position
                if (cj_data->m_limits_enabled)
                    cj_data->m_des_pos = Util::clamp_dFloat(cj_data->m_controller, cj_data->m_min_pos, cj_data->m_max_pos);
                else
                    cj_data->m_des_pos = cj_data->m_controller;
                dFloat rel_pos = cj_data->m_des_pos - cj_data->m_cur_pos;
                dFloat mrt = cj_data->m_rate * cj_data->m_reduction_ratio;
                des_vel = rel_pos * inv_timestep;
                if (mrt > M_EPSILON && dAbs(rel_pos) < mrt) {
                    dFloat red_vel = cj_data->m_rate * rel_pos / mrt;
                    if (dAbs(red_vel) < dAbs(des_vel)) des_vel = red_vel;
                }
                else if (dAbs(des_vel) > cj_data->m_rate)
                    des_vel = dSign(rel_pos) * cj_data->m_rate;
            }
            dFloat des_accel = (des_vel - cj_data->m_cur_vel) * inv_timestep;
            // Add linear row
            dVector point(matrix1.UntransformVector(matrix0.m_posit));
            point.m_z = des_vel * timestep;
            point = matrix1.TransformVector(point);
            NewtonUserJointAddLinearRow(joint, &point[0], &p1[0], &matrix1.m_right[0]);
            // Apply acceleration
            NewtonUserJointSetRowAcceleration(joint, des_accel);
        }
        else {
            NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_right[0]);
            NewtonUserJointSetRowAcceleration(joint, -cj_data->m_cur_vel * inv_timestep);
        }
        if (cj_data->m_power == 0.0) {
            NewtonUserJointSetRowMinimumFriction(joint, -Joint::CUSTOM_LARGE_VALUE);
            NewtonUserJointSetRowMaximumFriction(joint, Joint::CUSTOM_LARGE_VALUE);
        }
        else {
            NewtonUserJointSetRowMinimumFriction(joint, -cj_data->m_power);
            NewtonUserJointSetRowMaximumFriction(joint, cj_data->m_power);
        }
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }
}

void MSP::Piston::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);

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

void MSP::Piston::on_destroy(MSP::Joint::JointData* joint_data) {
    delete (reinterpret_cast<PistonData*>(joint_data->m_cj_data));
}

void MSP::Piston::on_disconnect(MSP::Joint::JointData* joint_data) {
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    cj_data->m_cur_pos = 0.0;
    cj_data->m_cur_vel = 0.0;
    cj_data->m_cur_accel = 0.0;
    cj_data->m_des_pos = 0.0;
}

void MSP::Piston::adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix) {
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

VALUE MSP::Piston::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::PISTON) ? Qtrue : Qfalse;
}

VALUE MSP::Piston::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::PISTON;
    joint_data->m_cj_data = new PistonData();
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;
    joint_data->m_on_destroy = on_destroy;
    joint_data->m_on_disconnect = on_disconnect;
    //~ joint_data->m_adjust_pin_matrix_proc = adjust_pin_matrix_proc;

    return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::Piston::rbf_get_cur_position(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_pos * M_INCH_TO_METER);
}

VALUE MSP::Piston::rbf_get_cur_velocity(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_vel * M_INCH_TO_METER);
}

VALUE MSP::Piston::rbf_get_cur_acceleration(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_accel * M_INCH_TO_METER);
}

VALUE MSP::Piston::rbf_get_min(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_min_pos * M_INCH_TO_METER);
}

VALUE MSP::Piston::rbf_set_min(VALUE self, VALUE v_joint, VALUE v_min) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    cj_data->m_min_pos = Util::value_to_dFloat(v_min) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Piston::rbf_get_max(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_max_pos * M_INCH_TO_METER);
}

VALUE MSP::Piston::rbf_set_max(VALUE self, VALUE v_joint, VALUE v_max) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    cj_data->m_max_pos = Util::value_to_dFloat(v_max) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Piston::rbf_enable_limits(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    cj_data->m_limits_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Piston::rbf_limits_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_limits_enabled);
}

VALUE MSP::Piston::rbf_get_rate(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_rate * M_INCH_TO_METER);
}

VALUE MSP::Piston::rbf_set_rate(VALUE self, VALUE v_joint, VALUE v_rate) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    cj_data->m_rate = Util::max_dFloat(Util::value_to_dFloat(v_rate) * M_METER_TO_INCH, 0.0);
    return Qnil;
}

VALUE MSP::Piston::rbf_get_power(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_power * M_INCH_TO_METER);
}

VALUE MSP::Piston::rbf_set_power(VALUE self, VALUE v_joint, VALUE v_power) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    cj_data->m_power = Util::max_dFloat(Util::value_to_dFloat(v_power) * M_METER_TO_INCH, 0.0);
    return Qnil;
}

VALUE MSP::Piston::rbf_get_reduction_ratio(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_reduction_ratio);
}

VALUE MSP::Piston::rbf_set_reduction_ratio(VALUE self, VALUE v_joint, VALUE v_reduction_ratio) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    cj_data->m_reduction_ratio = Util::clamp_dFloat(Util::value_to_dFloat(v_reduction_ratio), 0.0, 1.0);
    return Qnil;
}

VALUE MSP::Piston::rbf_get_controller(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    if (cj_data->m_controller_enabled) {
        if (cj_data->m_controller_mode == 0)
            return Util::to_value(cj_data->m_controller * M_INCH_TO_METER);
        else
            return Util::to_value(cj_data->m_controller);
    }
    else
        return Qnil;
}

VALUE MSP::Piston::rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    if (v_controller == Qnil) {
        if (cj_data->m_controller_enabled) {
            cj_data->m_controller_enabled = false;
            if (joint_data->m_connected)
                NewtonBodySetSleepState(joint_data->m_child, 0);
        }
    }
    else {
        dFloat controller = Util::value_to_dFloat(v_controller);
        if (cj_data->m_controller_mode == 0)
             controller *= M_METER_TO_INCH;
        if (!cj_data->m_controller_enabled || controller != cj_data->m_controller) {
            cj_data->m_controller = controller;
            cj_data->m_controller_enabled = true;
            if (joint_data->m_connected)
                NewtonBodySetSleepState(joint_data->m_child, 0);
        }
    }
    return Qnil;
}

VALUE MSP::Piston::rbf_get_controller_mode(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_controller_mode);
}

VALUE MSP::Piston::rbf_set_controller_mode(VALUE self, VALUE v_joint, VALUE v_mode) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::PISTON);
    PistonData* cj_data = reinterpret_cast<PistonData*>(joint_data->m_cj_data);
    cj_data->m_controller_mode = Util::clamp_int(Util::value_to_int(v_mode), 0, 2);
    return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Piston::init_ruby(VALUE mNewton) {
    VALUE mPiston = rb_define_module_under(mNewton, "Piston");

    rb_define_module_function(mPiston, "is_valid?", VALUEFUNC(MSP::Piston::rbf_is_valid), 1);
    rb_define_module_function(mPiston, "create", VALUEFUNC(MSP::Piston::rbf_create), 1);
    rb_define_module_function(mPiston, "get_cur_position", VALUEFUNC(MSP::Piston::rbf_get_cur_position), 1);
    rb_define_module_function(mPiston, "get_cur_velocity", VALUEFUNC(MSP::Piston::rbf_get_cur_velocity), 1);
    rb_define_module_function(mPiston, "get_cur_acceleration", VALUEFUNC(MSP::Piston::rbf_get_cur_acceleration), 1);
    rb_define_module_function(mPiston, "get_min", VALUEFUNC(MSP::Piston::rbf_get_min), 1);
    rb_define_module_function(mPiston, "set_min", VALUEFUNC(MSP::Piston::rbf_set_min), 2);
    rb_define_module_function(mPiston, "get_max", VALUEFUNC(MSP::Piston::rbf_get_max), 1);
    rb_define_module_function(mPiston, "set_max", VALUEFUNC(MSP::Piston::rbf_set_max), 2);
    rb_define_module_function(mPiston, "enable_limits", VALUEFUNC(MSP::Piston::rbf_enable_limits), 2);
    rb_define_module_function(mPiston, "limits_enabled?", VALUEFUNC(MSP::Piston::rbf_limits_enabled), 1);
    rb_define_module_function(mPiston, "get_rate", VALUEFUNC(MSP::Piston::rbf_get_rate), 1);
    rb_define_module_function(mPiston, "set_rate", VALUEFUNC(MSP::Piston::rbf_set_rate), 2);
    rb_define_module_function(mPiston, "get_power", VALUEFUNC(MSP::Piston::rbf_get_power), 1);
    rb_define_module_function(mPiston, "set_power", VALUEFUNC(MSP::Piston::rbf_set_power), 2);
    rb_define_module_function(mPiston, "get_reduction_ratio", VALUEFUNC(MSP::Piston::rbf_get_reduction_ratio), 1);
    rb_define_module_function(mPiston, "set_reduction_ratio", VALUEFUNC(MSP::Piston::rbf_set_reduction_ratio), 2);
    rb_define_module_function(mPiston, "get_controller", VALUEFUNC(MSP::Piston::rbf_get_controller), 1);
    rb_define_module_function(mPiston, "set_controller", VALUEFUNC(MSP::Piston::rbf_set_controller), 2);
    rb_define_module_function(mPiston, "get_controller_mode", VALUEFUNC(MSP::Piston::rbf_get_controller_mode), 1);
    rb_define_module_function(mPiston, "set_controller_mode", VALUEFUNC(MSP::Piston::rbf_set_controller_mode), 2);
}
