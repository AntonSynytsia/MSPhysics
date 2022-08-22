/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_joint_hinge.h"
#include "msp_world.h"
#include "msp_body.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Hinge::DEFAULT_MIN(-180.0 * M_DEG_TO_RAD);
const dFloat MSP::Hinge::DEFAULT_MAX(180.0 * M_DEG_TO_RAD);
const bool MSP::Hinge::DEFAULT_LIMITS_ENABLED(false);
const int MSP::Hinge::DEFAULT_MODE(0);
const dFloat MSP::Hinge::DEFAULT_FRICTION(0.0);
const dFloat MSP::Hinge::DEFAULT_ACCEL(40.0);
const dFloat MSP::Hinge::DEFAULT_DAMP(0.1f);
const dFloat MSP::Hinge::DEFAULT_STRENGTH(0.8f);
const dFloat MSP::Hinge::DEFAULT_SPRING_CONSTANT(40.0);
const dFloat MSP::Hinge::DEFAULT_SPRING_DRAG(1.0);
const dFloat MSP::Hinge::DEFAULT_START_ANGLE(0.0);
const dFloat MSP::Hinge::DEFAULT_CONTROLLER(1.0);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Hinge::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);

    dFloat inv_timestep = 1.0 / timestep;

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
    dFloat cur_angle = cj_data->m_ai->get_angle() - cj_data->m_start_angle * cj_data->m_controller;

    const dVector& p0 = matrix0.m_posit;
    const dVector& p1 = matrix1.m_posit;

    // Restrict movement on axes perpendicular to the pin direction.
    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_right[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Restriction rotation along the two axis perpendicular to pin direction.
    /*dVector q0(p0 + matrix0.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector q1(p1 + matrix1.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);*/
    dFloat angle0 = Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_front);
    NewtonUserJointAddAngularRow(joint, angle0, &matrix1.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    if (dAbs(angle0) > joint_data->m_max_angle_error) {
		const dFloat alpha = NewtonUserJointCalculateRowZeroAcceleration(joint) + dFloat(0.25f) * angle0 / (timestep * timestep);
		NewtonUserJointSetRowAcceleration(joint, alpha);
	}

    dFloat angle1 = Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_up);
    NewtonUserJointAddAngularRow(joint, angle1, &matrix1.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    if (dAbs(angle1) > joint_data->m_max_angle_error) {
		const dFloat alpha = NewtonUserJointCalculateRowZeroAcceleration(joint) + dFloat(0.25f) * angle1 / (timestep * timestep);
		NewtonUserJointSetRowAcceleration(joint, alpha);
	}

    // Check if need to re-enable limits.
    if (cj_data->m_temp_disable_limits && cur_angle >= cj_data->m_min_ang && cur_angle <= cj_data->m_max_ang)
        cj_data->m_temp_disable_limits = false;

    bool bcontinue = false;
    // Add limits
    if (cj_data->m_limits_enabled && !cj_data->m_temp_disable_limits) {
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
    // Add friction
    if (bcontinue) {
        if (cj_data->m_mode == 2) {
            NewtonUserJointAddAngularRow(joint, cur_angle, &matrix1.m_right[0]);
            dFloat force = -cj_data->m_spring_constant * cur_angle - cj_data->m_cur_omega * cj_data->m_spring_drag;
            MSP::Body::BodyData* cbody_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(joint_data->m_child));
            dVector force_dir(matrix1.m_right.Scale(force));
            MSP::Body::c_body_add_torque(cbody_data, &force_dir[0]);
            if (joint_data->m_parent != nullptr) {
                MSP::Body::BodyData* pbody_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(joint_data->m_parent));
                force_dir = matrix0.m_right.Scale(-force);
                MSP::Body::c_body_add_torque(pbody_data, &force_dir[0]);
            }
            //NewtonUserJointSetRowAcceleration(joint, 0.0);
            NewtonUserJointSetRowMinimumFriction(joint, 0.0);
            NewtonUserJointSetRowMaximumFriction(joint, 0.0);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_mode == 1) {
            NewtonUserJointAddAngularRow(joint, cur_angle, &matrix1.m_right[0]);
            dFloat accel = cj_data->m_accel * -cur_angle - cj_data->m_cur_omega * inv_timestep * cj_data->m_damp;
            NewtonUserJointSetRowAcceleration(joint, accel);
            dFloat stiffness = 0.999f - (1.0 - joint_data->m_stiffness * cj_data->m_strength) * Joint::DEFAULT_STIFFNESS_RANGE;
            NewtonUserJointSetRowStiffness(joint, stiffness);
        }
        else {
            NewtonUserJointAddAngularRow(joint, 0.0, &matrix1.m_right[0]);
            NewtonUserJointSetRowAcceleration(joint, -cj_data->m_cur_omega * inv_timestep);
            dFloat power = cj_data->m_friction * dAbs(cj_data->m_controller);
            NewtonUserJointSetRowMinimumFriction(joint, -power);
            NewtonUserJointSetRowMaximumFriction(joint, power);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
    }
}

void MSP::Hinge::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);

    info->m_minLinearDof[0] = -0.0;
    info->m_maxLinearDof[0] = 0.0;
    info->m_minLinearDof[1] = -0.0;
    info->m_maxLinearDof[1] = 0.0;
    info->m_minLinearDof[2] = -0.0;
    info->m_maxLinearDof[2] = 0.0;

    info->m_minAngularDof[0] = -0.0;
    info->m_maxAngularDof[0] = 0.0;
    info->m_minAngularDof[1] = -0.0;
    info->m_maxAngularDof[1] = 0.0;

    if (cj_data->m_limits_enabled) {
        info->m_minAngularDof[2] = (cj_data->m_min_ang - cj_data->m_ai->get_angle()) * M_RAD_TO_DEG;
        info->m_maxAngularDof[2] = (cj_data->m_max_ang - cj_data->m_ai->get_angle()) * M_RAD_TO_DEG;
    }
    else {
        info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
        info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
    }
}

void MSP::Hinge::on_destroy(MSP::Joint::JointData* joint_data) {
    delete (reinterpret_cast<HingeData*>(joint_data->m_cj_data));
}

void MSP::Hinge::on_connect(MSP::Joint::JointData* joint_data) {
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_desired_start_angle = cj_data->m_start_angle * cj_data->m_controller;
    cj_data->m_temp_disable_limits = true;
}

void MSP::Hinge::on_disconnect(MSP::Joint::JointData* joint_data) {
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_ai->set_angle(0.0);
    cj_data->m_cur_omega = 0.0;
    cj_data->m_cur_alpha = 0.0;
}

void MSP::Hinge::adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix) {
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

VALUE MSP::Hinge::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::HINGE) ? Qtrue : Qfalse;
}

VALUE MSP::Hinge::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::HINGE;
    joint_data->m_cj_data = new HingeData();
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;
    joint_data->m_on_destroy = on_destroy;
    joint_data->m_on_connect = on_connect;
    joint_data->m_on_disconnect = on_disconnect;
    //~ joint_data->m_adjust_pin_matrix_proc = adjust_pin_matrix_proc;

    return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::Hinge::rbf_get_min(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_min_ang);
}

VALUE MSP::Hinge::rbf_set_min(VALUE self, VALUE v_joint, VALUE v_min) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_min_ang = Util::value_to_dFloat(v_min);
    return Qnil;
}

VALUE MSP::Hinge::rbf_get_max(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_max_ang);
}

VALUE MSP::Hinge::rbf_set_max(VALUE self, VALUE v_joint, VALUE v_max) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_max_ang = Util::value_to_dFloat(v_max);
    return Qnil;
}

VALUE MSP::Hinge::rbf_enable_limits(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_limits_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Hinge::rbf_limits_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_limits_enabled);
}

VALUE MSP::Hinge::rbf_get_mode(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_mode);
}

VALUE MSP::Hinge::rbf_set_mode(VALUE self, VALUE v_joint, VALUE v_mode) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_mode = Util::clamp_int(Util::value_to_int(v_mode), 0, 2);
    return Qnil;
}

VALUE MSP::Hinge::rbf_get_friction(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_friction * M_INCH2_TO_METER2);
}

VALUE MSP::Hinge::rbf_set_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_friction = Util::max_dFloat(Util::value_to_dFloat(v_friction) * M_METER2_TO_INCH2, 0.0);
    return Qnil;
}

VALUE MSP::Hinge::rbf_get_accel(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_accel);
}

VALUE MSP::Hinge::rbf_set_accel(VALUE self, VALUE v_joint, VALUE v_accel) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_accel = Util::max_dFloat(Util::value_to_dFloat(v_accel), 0.0);
    return Qnil;
}

VALUE MSP::Hinge::rbf_get_damp(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_damp);
}

VALUE MSP::Hinge::rbf_set_damp(VALUE self, VALUE v_joint, VALUE v_damp) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_damp = Util::clamp_dFloat(Util::value_to_dFloat(v_damp), 0.0, 1.0);
    return Qnil;
}

VALUE MSP::Hinge::rbf_get_strength(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_strength);
}

VALUE MSP::Hinge::rbf_set_strength(VALUE self, VALUE v_joint, VALUE v_strength) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_strength = Util::clamp_dFloat(Util::value_to_dFloat(v_strength), 0.0, 1.0);
    return Qnil;
}

VALUE MSP::Hinge::rbf_get_spring_constant(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_spring_constant * M_INCH2_TO_METER2);
}

VALUE MSP::Hinge::rbf_set_spring_constant(VALUE self, VALUE v_joint, VALUE v_spring_constant) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_spring_constant = Util::max_dFloat(Util::value_to_dFloat(v_spring_constant) * M_METER2_TO_INCH2, 0.0);
    return Qnil;
}

VALUE MSP::Hinge::rbf_get_spring_drag(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_spring_drag * M_INCH2_TO_METER2);
}

VALUE MSP::Hinge::rbf_set_spring_drag(VALUE self, VALUE v_joint, VALUE v_spring_drag) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_spring_drag = Util::max_dFloat(Util::value_to_dFloat(v_spring_drag) * M_METER2_TO_INCH2, 0.0);
    return Qnil;
}

VALUE MSP::Hinge::rbf_get_start_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_start_angle);
}

VALUE MSP::Hinge::rbf_set_start_angle(VALUE self, VALUE v_joint, VALUE v_angle) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_start_angle = Util::value_to_dFloat(v_angle);
    if (cj_data->m_mode != 0) {
        dFloat desired_start_angle = cj_data->m_start_angle * cj_data->m_controller;
        if (cj_data->m_desired_start_angle != desired_start_angle) {
            cj_data->m_temp_disable_limits = true;
            cj_data->m_desired_start_angle = desired_start_angle;
            if (joint_data->m_connected)
                NewtonBodySetSleepState(joint_data->m_child, 0);
        }
    }
    return Qnil;
}

VALUE MSP::Hinge::rbf_get_controller(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_controller);
}

VALUE MSP::Hinge::rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    cj_data->m_controller = Util::value_to_dFloat(v_controller);
    if (cj_data->m_mode != 0) {
        dFloat desired_start_angle = cj_data->m_start_angle * cj_data->m_controller;
        if (cj_data->m_desired_start_angle != desired_start_angle) {
            cj_data->m_temp_disable_limits = true;
            cj_data->m_desired_start_angle = desired_start_angle;
            if (joint_data->m_connected)
                NewtonBodySetSleepState(joint_data->m_child, 0);
        }
    }
    return Qnil;
}

VALUE MSP::Hinge::rbf_get_cur_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_ai->get_angle() - cj_data->m_start_angle * cj_data->m_controller);
}

VALUE MSP::Hinge::rbf_get_cur_omega(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_omega);
}

VALUE MSP::Hinge::rbf_get_cur_alpha(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::HINGE);
    HingeData* cj_data = reinterpret_cast<HingeData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_alpha);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Hinge::init_ruby(VALUE mNewton) {
    VALUE mHinge = rb_define_module_under(mNewton, "Hinge");

    rb_define_module_function(mHinge, "is_valid?", VALUEFUNC(MSP::Hinge::rbf_is_valid), 1);
    rb_define_module_function(mHinge, "create", VALUEFUNC(MSP::Hinge::rbf_create), 1);
    rb_define_module_function(mHinge, "get_min", VALUEFUNC(MSP::Hinge::rbf_get_min), 1);
    rb_define_module_function(mHinge, "set_min", VALUEFUNC(MSP::Hinge::rbf_set_min), 2);
    rb_define_module_function(mHinge, "get_max", VALUEFUNC(MSP::Hinge::rbf_get_max), 1);
    rb_define_module_function(mHinge, "set_max", VALUEFUNC(MSP::Hinge::rbf_set_max), 2);
    rb_define_module_function(mHinge, "enable_limits", VALUEFUNC(MSP::Hinge::rbf_enable_limits), 2);
    rb_define_module_function(mHinge, "limits_enabled?", VALUEFUNC(MSP::Hinge::rbf_limits_enabled), 1);
    rb_define_module_function(mHinge, "get_mode", VALUEFUNC(MSP::Hinge::rbf_get_mode), 1);
    rb_define_module_function(mHinge, "set_mode", VALUEFUNC(MSP::Hinge::rbf_set_mode), 2);
    rb_define_module_function(mHinge, "get_friction", VALUEFUNC(MSP::Hinge::rbf_get_friction), 1);
    rb_define_module_function(mHinge, "set_friction", VALUEFUNC(MSP::Hinge::rbf_set_friction), 2);
    rb_define_module_function(mHinge, "get_accel", VALUEFUNC(MSP::Hinge::rbf_get_accel), 1);
    rb_define_module_function(mHinge, "set_accel", VALUEFUNC(MSP::Hinge::rbf_set_accel), 2);
    rb_define_module_function(mHinge, "get_damp", VALUEFUNC(MSP::Hinge::rbf_get_damp), 1);
    rb_define_module_function(mHinge, "set_damp", VALUEFUNC(MSP::Hinge::rbf_set_damp), 2);
    rb_define_module_function(mHinge, "get_strength", VALUEFUNC(MSP::Hinge::rbf_get_strength), 1);
    rb_define_module_function(mHinge, "set_strength", VALUEFUNC(MSP::Hinge::rbf_set_strength), 2);
    rb_define_module_function(mHinge, "get_spring_constant", VALUEFUNC(MSP::Hinge::rbf_get_spring_constant), 1);
    rb_define_module_function(mHinge, "set_spring_constant", VALUEFUNC(MSP::Hinge::rbf_set_spring_constant), 2);
    rb_define_module_function(mHinge, "get_spring_drag", VALUEFUNC(MSP::Hinge::rbf_get_spring_drag), 1);
    rb_define_module_function(mHinge, "set_spring_drag", VALUEFUNC(MSP::Hinge::rbf_set_spring_drag), 2);
    rb_define_module_function(mHinge, "get_start_angle", VALUEFUNC(MSP::Hinge::rbf_get_start_angle), 1);
    rb_define_module_function(mHinge, "set_start_angle", VALUEFUNC(MSP::Hinge::rbf_set_start_angle), 2);
    rb_define_module_function(mHinge, "get_controller", VALUEFUNC(MSP::Hinge::rbf_get_controller), 1);
    rb_define_module_function(mHinge, "set_controller", VALUEFUNC(MSP::Hinge::rbf_set_controller), 2);
    rb_define_module_function(mHinge, "get_cur_angle", VALUEFUNC(MSP::Hinge::rbf_get_cur_angle), 1);
    rb_define_module_function(mHinge, "get_cur_omega", VALUEFUNC(MSP::Hinge::rbf_get_cur_omega), 1);
    rb_define_module_function(mHinge, "get_cur_alpha", VALUEFUNC(MSP::Hinge::rbf_get_cur_alpha), 1);
}
