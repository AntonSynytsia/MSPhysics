/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "msp_joint_servo.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Servo::DEFAULT_MIN(-180.0f * M_DEG_TO_RAD);
const dFloat MSP::Servo::DEFAULT_MAX(180.0f * M_DEG_TO_RAD);
const bool MSP::Servo::DEFAULT_LIMITS_ENABLED(false);
const dFloat MSP::Servo::DEFAULT_RATE(360.0f * M_DEG_TO_RAD);
const dFloat MSP::Servo::DEFAULT_POWER(0.0f);
const dFloat MSP::Servo::DEFAULT_REDUCTION_RATIO(0.1f);
const dFloat MSP::Servo::DEFAULT_CONTROLLER(0.0f);
const bool MSP::Servo::DEFAULT_CONTROLLER_ENABLED(false);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Servo::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);

    dFloat inv_timestep = 1.0f / timestep;

    // Calculate position of pivot points and Jacobian direction vectors in global space.
    dMatrix matrix0, matrix1;
    MSP::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

    // Calculate angle, omega, and acceleration.
    dFloat last_angle = cj_data->m_ai->get_angle();
    dFloat last_omega = cj_data->m_cur_omega;
    dFloat sin_angle, cos_angle;
    Joint::c_calculate_angle(matrix1.m_front, matrix0.m_front, matrix0.m_right, sin_angle, cos_angle);
    cj_data->m_ai->update(cos_angle, sin_angle);
    cj_data->m_cur_omega = (cj_data->m_ai->get_angle() - last_angle) * inv_timestep;
    cj_data->m_cur_alpha = (cj_data->m_cur_omega - last_omega) * inv_timestep;
    dFloat cur_angle = cj_data->m_ai->get_angle();

    const dVector& p0 = matrix0.m_posit;
    const dVector& p1 = matrix1.m_posit;

    // Restrict movement on axes perpendicular to the pin direction.
    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_right[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Restriction rotation along the two axis perpendicular to pin direction.
    /*dVector q0(p0 + matrix0.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector q1(p1 + matrix1.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix1.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix1.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);*/
    NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_front), &matrix1.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    bool bcontinue = false;
    // Add limits
    if (cj_data->m_limits_enabled) {
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
            NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cur_angle > cj_data->m_max_ang) {
            NewtonUserJointAddAngularRow(joint, cj_data->m_max_ang - cur_angle - Joint::ANGULAR_LIMIT_EPSILON, &matrix1.m_right[0]);
            NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
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
            // Control by angle
            dFloat des_angle = cj_data->m_limits_enabled ? Util::clamp_float(cj_data->m_controller, cj_data->m_min_ang, cj_data->m_max_ang) : cj_data->m_controller;
            dFloat rel_angle = des_angle - cur_angle;
            dFloat mrt = cj_data->m_rate * cj_data->m_reduction_ratio;
            dFloat des_omega = rel_angle * inv_timestep;
            if (mrt > M_EPSILON && dAbs(rel_angle) < mrt) {
                dFloat red_omega = cj_data->m_rate * rel_angle / mrt;
                if (dAbs(red_omega) < dAbs(des_omega)) des_omega = red_omega;
            }
            else if (dAbs(des_omega) > cj_data->m_rate)
                des_omega = dSign(rel_angle) * cj_data->m_rate;
            dFloat des_accel = (des_omega - cj_data->m_cur_omega) * inv_timestep;
            dFloat step = des_omega * timestep;
            // Add angular row
            NewtonUserJointAddAngularRow(joint, step, &matrix1.m_right[0]);
            // Apply acceleration
            NewtonUserJointSetRowAcceleration(joint, des_accel);
        }
        else {
            // Add angular row
            NewtonUserJointAddAngularRow(joint, 0.0f, &matrix1.m_right[0]);
        }
        if (cj_data->m_power == 0.0f) {
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

void MSP::Servo::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);

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

    if (cj_data->m_limits_enabled) {
        info->m_minAngularDof[2] = (cj_data->m_min_ang - cj_data->m_ai->get_angle()) * M_RAD_TO_DEG;
        info->m_maxAngularDof[2] = (cj_data->m_max_ang - cj_data->m_ai->get_angle()) * M_RAD_TO_DEG;
    }
    else {
        info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
        info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
    }
}

void MSP::Servo::on_destroy(MSP::Joint::JointData* joint_data) {
    delete (reinterpret_cast<ServoData*>(joint_data->m_cj_data));
}

void MSP::Servo::on_disconnect(MSP::Joint::JointData* joint_data) {
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    cj_data->m_ai->set_angle(0.0f);
    cj_data->m_cur_omega = 0.0f;
    cj_data->m_cur_alpha = 0.0f;
}

void MSP::Servo::adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix) {
    dMatrix matrix;
    dVector centre;
    NewtonBodyGetMatrix(joint_data->m_child, &matrix[0][0]);
    NewtonBodyGetCentreOfMass(joint_data->m_child, &centre[0]);
    centre = matrix.TransformVector(centre);
    centre = pin_matrix.UntransformVector(centre);
    dVector point(0.0f, 0.0f, centre.m_z);
    pin_matrix.m_posit = pin_matrix.TransformVector(point);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Servo::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::SERVO) ? Qtrue : Qfalse;
}

VALUE MSP::Servo::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::SERVO;
    joint_data->m_cj_data = new ServoData;
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;
    joint_data->m_on_destroy = on_destroy;
    joint_data->m_on_disconnect = on_disconnect;
    //~ joint_data->adjust_pin_matrix_proc = adjust_pin_matrix_proc;

    return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::Servo::rbf_get_cur_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_ai->get_angle());
}

VALUE MSP::Servo::rbf_get_cur_omega(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_omega);
}

VALUE MSP::Servo::rbf_get_cur_alpha(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_alpha);
}

VALUE MSP::Servo::rbf_get_min(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_min_ang);
}

VALUE MSP::Servo::rbf_set_min(VALUE self, VALUE v_joint, VALUE v_min) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    cj_data->m_min_ang = Util::value_to_dFloat(v_min);
    return Qnil;
}

VALUE MSP::Servo::rbf_get_max(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_max_ang);
}

VALUE MSP::Servo::rbf_set_max(VALUE self, VALUE v_joint, VALUE v_max) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    cj_data->m_max_ang = Util::value_to_dFloat(v_max);
    return Qnil;
}

VALUE MSP::Servo::rbf_enable_limits(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    cj_data->m_limits_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Servo::rbf_limits_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_limits_enabled);
}

VALUE MSP::Servo::rbf_get_rate(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_rate);
}

VALUE MSP::Servo::rbf_set_rate(VALUE self, VALUE v_joint, VALUE v_rate) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    cj_data->m_rate = Util::max_float(Util::value_to_dFloat(v_rate), 0.0f);
    return Qnil;
}

VALUE MSP::Servo::rbf_get_power(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_power * M_INCH2_TO_METER2);
}

VALUE MSP::Servo::rbf_set_power(VALUE self, VALUE v_joint, VALUE v_power) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    cj_data->m_power = Util::max_float(Util::value_to_dFloat(v_power) * M_METER2_TO_INCH2, 0.0f);
    return Qnil;
}

VALUE MSP::Servo::rbf_get_reduction_ratio(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_reduction_ratio);
}

VALUE MSP::Servo::rbf_set_reduction_ratio(VALUE self, VALUE v_joint, VALUE v_reduction_ratio) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    cj_data->m_reduction_ratio = Util::clamp_float(Util::value_to_dFloat(v_reduction_ratio), 0.0f, 1.0f);
    return Qnil;
}

VALUE MSP::Servo::rbf_get_controller(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    return (cj_data->m_controller_enabled ? Util::to_value(cj_data->m_controller) : Qnil);
}

VALUE MSP::Servo::rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SERVO);
    ServoData* cj_data = reinterpret_cast<ServoData*>(joint_data->m_cj_data);
    if (v_controller == Qnil) {
        if (cj_data->m_controller_enabled) {
            cj_data->m_controller_enabled = false;
            if (joint_data->m_connected)
                NewtonBodySetSleepState(joint_data->m_child, 0);
        }
    }
    else {
        dFloat controller = Util::value_to_dFloat(v_controller);
        if (!cj_data->m_controller_enabled || controller != cj_data->m_controller) {
            cj_data->m_controller_enabled = true;
            cj_data->m_controller = controller;
            if (joint_data->m_connected)
                NewtonBodySetSleepState(joint_data->m_child, 0);
        }
    }
    return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Servo::init_ruby(VALUE mNewton) {
    VALUE mServo = rb_define_module_under(mNewton, "Servo");

    rb_define_module_function(mServo, "is_valid?", VALUEFUNC(MSP::Servo::rbf_is_valid), 1);
    rb_define_module_function(mServo, "create", VALUEFUNC(MSP::Servo::rbf_create), 1);
    rb_define_module_function(mServo, "get_cur_angle", VALUEFUNC(MSP::Servo::rbf_get_cur_angle), 1);
    rb_define_module_function(mServo, "get_cur_omega", VALUEFUNC(MSP::Servo::rbf_get_cur_omega), 1);
    rb_define_module_function(mServo, "get_cur_alpha", VALUEFUNC(MSP::Servo::rbf_get_cur_alpha), 1);
    rb_define_module_function(mServo, "get_min", VALUEFUNC(MSP::Servo::rbf_get_min), 1);
    rb_define_module_function(mServo, "set_min", VALUEFUNC(MSP::Servo::rbf_set_min), 2);
    rb_define_module_function(mServo, "get_max", VALUEFUNC(MSP::Servo::rbf_get_max), 1);
    rb_define_module_function(mServo, "set_max", VALUEFUNC(MSP::Servo::rbf_set_max), 2);
    rb_define_module_function(mServo, "enable_limits", VALUEFUNC(MSP::Servo::rbf_enable_limits), 2);
    rb_define_module_function(mServo, "limits_enabled?", VALUEFUNC(MSP::Servo::rbf_limits_enabled), 1);
    rb_define_module_function(mServo, "get_rate", VALUEFUNC(MSP::Servo::rbf_get_rate), 1);
    rb_define_module_function(mServo, "set_rate", VALUEFUNC(MSP::Servo::rbf_set_rate), 2);
    rb_define_module_function(mServo, "get_power", VALUEFUNC(MSP::Servo::rbf_get_power), 1);
    rb_define_module_function(mServo, "set_power", VALUEFUNC(MSP::Servo::rbf_set_power), 2);
    rb_define_module_function(mServo, "get_reduction_ratio", VALUEFUNC(MSP::Servo::rbf_get_reduction_ratio), 1);
    rb_define_module_function(mServo, "set_reduction_ratio", VALUEFUNC(MSP::Servo::rbf_set_reduction_ratio), 2);
    rb_define_module_function(mServo, "get_controller", VALUEFUNC(MSP::Servo::rbf_get_controller), 1);
    rb_define_module_function(mServo, "set_controller", VALUEFUNC(MSP::Servo::rbf_set_controller), 2);
}
