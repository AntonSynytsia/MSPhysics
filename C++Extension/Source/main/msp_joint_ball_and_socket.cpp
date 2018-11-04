/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "msp_joint_ball_and_socket.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::BallAndSocket::DEFAULT_MAX_CONE_ANGLE(30.0f * M_DEG_TO_RAD);
const bool MSP::BallAndSocket::DEFAULT_CONE_LIMITS_ENABLED( false);
const dFloat MSP::BallAndSocket::DEFAULT_MIN_TWIST_ANGLE(-180.0f * M_DEG_TO_RAD);
const dFloat MSP::BallAndSocket::DEFAULT_MAX_TWIST_ANGLE(180.0f * M_DEG_TO_RAD);
const bool MSP::BallAndSocket::DEFAULT_TWIST_LIMITS_ENABLED(false);
const dFloat MSP::BallAndSocket::DEFAULT_FRICTION(0.0f);
const dFloat MSP::BallAndSocket::DEFAULT_CONTROLLER(1.0f);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::BallAndSocket::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);

    dFloat inv_timestep = 1.0f / timestep;

    // Calculate the position of the pivot point and the Jacobian direction vectors, in global space.
    dMatrix matrix0, matrix1;
    MSP::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

    dFloat last_cone_angle = cj_data->m_cur_cone_angle;

    // Calculate current cone angle
    dFloat cur_cone_angle_cos = matrix0.m_right.DotProduct3(matrix1.m_right);
    cj_data->m_cur_cone_angle = dAcos(Util::clamp_float(cur_cone_angle_cos, -1.0f, 1.0f));

    // Calculate current twist angle, omega, and acceleration.
    if (cur_cone_angle_cos < -0.999999f) {
        cj_data->m_cur_twist_omega = 0.0f;
        cj_data->m_cur_twist_alpha = 0.0f;
    }
    else {
        dFloat last_twist_angle = cj_data->m_twist_ai->get_angle();
        dFloat last_twist_omega = cj_data->m_cur_twist_omega;
        dMatrix rot_matrix0;
        Util::rotate_matrix_to_dir(matrix0, matrix1.m_right, rot_matrix0);
        dFloat sin_angle;
        dFloat cos_angle;
        MSP::Joint::c_calculate_angle(matrix1.m_front, rot_matrix0.m_front, matrix1.m_right, sin_angle, cos_angle);
        cj_data->m_twist_ai->update(cos_angle, sin_angle);
        cj_data->m_cur_twist_omega = (cj_data->m_twist_ai->get_angle() - last_twist_angle) * inv_timestep;
        cj_data->m_cur_twist_alpha = (cj_data->m_cur_twist_omega - last_twist_omega) * inv_timestep;
    }

    // Get the current lateral and tangent dir
    dVector lateral_dir;
    dVector front_dir;
    if (dAbs(cur_cone_angle_cos) > 0.999999f) {
        lateral_dir = matrix1.m_front;
        front_dir = matrix1.m_up;
    }
    else {
        lateral_dir = matrix1.m_right.CrossProduct(matrix0.m_right);
        front_dir = matrix1.m_right.CrossProduct(lateral_dir);
    }

    // Restrict the movement on the pivot point along all tree orthonormal directions.
    NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix1.m_front[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix1.m_up[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix1.m_right[0]);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Calculate friction
    dFloat power = cj_data->m_friction * dAbs(cj_data->m_controller);

    // Handle cone angle
    if (cj_data->m_cone_limits_enabled && cj_data->m_max_cone_angle < Joint::ANGULAR_LIMIT_EPSILON2) {
        // Handle in case joint being a hinge; max cone angle is near zero.
        NewtonUserJointAddAngularRow(joint, MSP::Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_front), &matrix1.m_front[0]);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

        NewtonUserJointAddAngularRow(joint, MSP::Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_up[0]);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }
    else if (cj_data->m_cone_limits_enabled && cj_data->m_cur_cone_angle > cj_data->m_max_cone_angle) {
        // Handle in case current cone angle is greater than max cone angle
        dFloat dangle = cj_data->m_cur_cone_angle - cj_data->m_max_cone_angle;
        NewtonUserJointAddAngularRow(joint, dangle, &lateral_dir[0]);
        NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

        NewtonUserJointAddAngularRow(joint, 0.0f, &front_dir[0]);
        NewtonUserJointSetRowMinimumFriction(joint, -power);
        NewtonUserJointSetRowMaximumFriction(joint, power);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }
    else {
        // Handle in case limits are not necessary
        dFloat cur_cone_omega = (cj_data->m_cur_cone_angle - last_cone_angle) * inv_timestep;
        dFloat des_cone_accel = -cur_cone_omega * inv_timestep;

        NewtonUserJointAddAngularRow(joint, 0.0f, &lateral_dir[0]);
        NewtonUserJointSetRowAcceleration(joint, des_cone_accel);
        NewtonUserJointSetRowMinimumFriction(joint, -power);
        NewtonUserJointSetRowMaximumFriction(joint, power);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

        NewtonUserJointAddAngularRow(joint, 0.0f, &front_dir[0]);
        NewtonUserJointSetRowMinimumFriction(joint, -power);
        NewtonUserJointSetRowMaximumFriction(joint, power);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }

    // Handle twist angle
    bool bcontinue = false;
    if (cj_data->m_twist_limits_enabled) {
        if (cj_data->m_min_twist_angle > cj_data->m_max_twist_angle) {
            // Handle in case min angle is greater than max
            NewtonUserJointAddAngularRow(joint, (cj_data->m_min_twist_angle + cj_data->m_max_twist_angle) * 0.5f - cj_data->m_twist_ai->get_angle(), &matrix0.m_right[0]);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_max_twist_angle - cj_data->m_min_twist_angle < Joint::ANGULAR_LIMIT_EPSILON2) {
            // Handle in case min angle is almost equal to max
            NewtonUserJointAddAngularRow(joint, cj_data->m_max_twist_angle - cj_data->m_twist_ai->get_angle(), &matrix0.m_right[0]);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_twist_ai->get_angle() < cj_data->m_min_twist_angle) {
            // Handle in case current twist angle is less than min
            NewtonUserJointAddAngularRow(joint, cj_data->m_min_twist_angle - cj_data->m_twist_ai->get_angle() + Joint::ANGULAR_LIMIT_EPSILON, &matrix0.m_right[0]);
            NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_twist_ai->get_angle() > cj_data->m_max_twist_angle) {
            // Handle in case current twist angle is greater than max
            NewtonUserJointAddAngularRow(joint, cj_data->m_max_twist_angle - cj_data->m_twist_ai->get_angle() - Joint::ANGULAR_LIMIT_EPSILON, &matrix0.m_right[0]);
            NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else
            bcontinue = true;
    }
    else
        bcontinue = true;
    if (bcontinue) {
        // Handle in case limits are not necessary
        NewtonUserJointAddAngularRow(joint, 0.0f, &matrix0.m_right[0]);
        NewtonUserJointSetRowAcceleration(joint, -cj_data->m_cur_twist_omega * inv_timestep);
        NewtonUserJointSetRowMinimumFriction(joint, -power);
        NewtonUserJointSetRowMaximumFriction(joint, power);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }
}

void MSP::BallAndSocket::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    info->m_minLinearDof[0] = -0.0f;
    info->m_maxLinearDof[0] = 0.0f;
    info->m_minLinearDof[1] = -0.0f;
    info->m_maxLinearDof[1] = 0.0f;
    info->m_minLinearDof[2] = -0.0f;
    info->m_maxLinearDof[2] = 0.0f;

    info->m_minAngularDof[0] = -Joint::CUSTOM_LARGE_VALUE;
    info->m_maxAngularDof[0] = Joint::CUSTOM_LARGE_VALUE;
    info->m_minAngularDof[1] = -Joint::CUSTOM_LARGE_VALUE;
    info->m_maxAngularDof[1] = Joint::CUSTOM_LARGE_VALUE;
    info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
    info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
}

void MSP::BallAndSocket::on_destroy(MSP::Joint::JointData* joint_data) {
    delete (reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data));
}

void MSP::BallAndSocket::on_disconnect(MSP::Joint::JointData* joint_data) {
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    cj_data->m_cur_cone_angle = 0.0f;
    cj_data->m_twist_ai->set_angle(0.0f);
    cj_data->m_cur_twist_omega = 0.0f;
    cj_data->m_cur_twist_alpha = 0.0f;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::BallAndSocket::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::BALL_AND_SOCKET) ? Qtrue : Qfalse;
}

VALUE MSP::BallAndSocket::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::BALL_AND_SOCKET;
    joint_data->m_cj_data = new BallAndSocketData();
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;
    joint_data->m_on_destroy = on_destroy;
    joint_data->m_on_disconnect = on_disconnect;

    return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::BallAndSocket::rbf_get_max_cone_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_max_cone_angle);
}

VALUE MSP::BallAndSocket::rbf_set_max_cone_angle(VALUE self, VALUE v_joint, VALUE v_angle) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    cj_data->m_max_cone_angle = Util::clamp_float(Util::value_to_dFloat(v_angle), 0.0f, M_SPI);
    return Qnil;
}

VALUE MSP::BallAndSocket::rbf_enable_cone_limits(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    cj_data->m_cone_limits_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::BallAndSocket::rbf_cone_limits_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cone_limits_enabled);
}

VALUE MSP::BallAndSocket::rbf_get_min_twist_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_min_twist_angle);
}

VALUE MSP::BallAndSocket::rbf_set_min_twist_angle(VALUE self, VALUE v_joint, VALUE v_angle) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    cj_data->m_min_twist_angle = Util::value_to_dFloat(v_angle);
    return Qnil;
}

VALUE MSP::BallAndSocket::rbf_get_max_twist_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_max_twist_angle);
}

VALUE MSP::BallAndSocket::rbf_set_max_twist_angle(VALUE self, VALUE v_joint, VALUE v_angle) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    cj_data->m_max_twist_angle = Util::value_to_dFloat(v_angle);
    return Qnil;
}

VALUE MSP::BallAndSocket::rbf_enable_twist_limits(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    cj_data->m_twist_limits_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::BallAndSocket::rbf_twist_limits_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_twist_limits_enabled);
}

VALUE MSP::BallAndSocket::rbf_get_cur_cone_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_cone_angle);
}

VALUE MSP::BallAndSocket::rbf_get_cur_twist_angle(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_twist_ai->get_angle());
}

VALUE MSP::BallAndSocket::rbf_get_cur_twist_omega(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_twist_omega);
}

VALUE MSP::BallAndSocket::rbf_get_cur_twist_alpha(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_twist_alpha);
}

VALUE MSP::BallAndSocket::rbf_get_friction(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_friction * M_INCH2_TO_METER2);
}

VALUE MSP::BallAndSocket::rbf_set_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    cj_data->m_friction = Util::max_float(Util::value_to_dFloat(v_friction) * M_METER2_TO_INCH2, 0.0f);
    return Qnil;
}

VALUE MSP::BallAndSocket::rbf_get_controller(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_controller);
}

VALUE MSP::BallAndSocket::rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::BALL_AND_SOCKET);
    BallAndSocketData* cj_data = reinterpret_cast<BallAndSocketData*>(joint_data->m_cj_data);
    dFloat desired_controller = Util::value_to_dFloat(v_controller);
    if (cj_data->m_controller != desired_controller) {
        cj_data->m_controller = desired_controller;
        if (joint_data->m_connected)
            NewtonBodySetSleepState(joint_data->m_child, 0);
    }
    return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::BallAndSocket::init_ruby(VALUE mNewton) {
    VALUE mBallAndSocket = rb_define_module_under(mNewton, "BallAndSocket");

    rb_define_module_function(mBallAndSocket, "is_valid?", VALUEFUNC(MSP::BallAndSocket::rbf_is_valid), 1);
    rb_define_module_function(mBallAndSocket, "create", VALUEFUNC(MSP::BallAndSocket::rbf_create), 1);
    rb_define_module_function(mBallAndSocket, "get_max_cone_angle", VALUEFUNC(MSP::BallAndSocket::rbf_get_max_cone_angle), 1);
    rb_define_module_function(mBallAndSocket, "set_max_cone_angle", VALUEFUNC(MSP::BallAndSocket::rbf_set_max_cone_angle), 2);
    rb_define_module_function(mBallAndSocket, "enable_cone_limits", VALUEFUNC(MSP::BallAndSocket::rbf_enable_cone_limits), 2);
    rb_define_module_function(mBallAndSocket, "cone_limits_enabled?", VALUEFUNC(MSP::BallAndSocket::rbf_cone_limits_enabled), 1);
    rb_define_module_function(mBallAndSocket, "get_min_twist_angle", VALUEFUNC(MSP::BallAndSocket::rbf_get_min_twist_angle), 1);
    rb_define_module_function(mBallAndSocket, "set_min_twist_angle", VALUEFUNC(MSP::BallAndSocket::rbf_set_min_twist_angle), 2);
    rb_define_module_function(mBallAndSocket, "get_max_twist_angle", VALUEFUNC(MSP::BallAndSocket::rbf_get_max_twist_angle), 1);
    rb_define_module_function(mBallAndSocket, "set_max_twist_angle", VALUEFUNC(MSP::BallAndSocket::rbf_set_max_twist_angle), 2);
    rb_define_module_function(mBallAndSocket, "enable_twist_limits", VALUEFUNC(MSP::BallAndSocket::rbf_enable_twist_limits), 2);
    rb_define_module_function(mBallAndSocket, "twist_limits_enabled?", VALUEFUNC(MSP::BallAndSocket::rbf_twist_limits_enabled), 1);
    rb_define_module_function(mBallAndSocket, "get_cur_cone_angle", VALUEFUNC(MSP::BallAndSocket::rbf_get_cur_cone_angle), 1);
    rb_define_module_function(mBallAndSocket, "get_cur_twist_angle", VALUEFUNC(MSP::BallAndSocket::rbf_get_cur_twist_angle), 1);
    rb_define_module_function(mBallAndSocket, "get_cur_twist_omega", VALUEFUNC(MSP::BallAndSocket::rbf_get_cur_twist_omega), 1);
    rb_define_module_function(mBallAndSocket, "get_cur_twist_alpha", VALUEFUNC(MSP::BallAndSocket::rbf_get_cur_twist_alpha), 1);
    rb_define_module_function(mBallAndSocket, "get_friction", VALUEFUNC(MSP::BallAndSocket::rbf_get_friction), 1);
    rb_define_module_function(mBallAndSocket, "set_friction", VALUEFUNC(MSP::BallAndSocket::rbf_set_friction), 2);
    rb_define_module_function(mBallAndSocket, "get_controller", VALUEFUNC(MSP::BallAndSocket::rbf_get_controller), 1);
    rb_define_module_function(mBallAndSocket, "set_controller", VALUEFUNC(MSP::BallAndSocket::rbf_set_controller), 2);
}
