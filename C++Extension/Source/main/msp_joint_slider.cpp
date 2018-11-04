/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "msp_joint_slider.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Slider::DEFAULT_MIN(-10.0f);
const dFloat MSP::Slider::DEFAULT_MAX(10.0f);
const bool MSP::Slider::DEFAULT_LIMITS_ENABLED(false);
const dFloat MSP::Slider::DEFAULT_FRICTION(0.0f);
const dFloat MSP::Slider::DEFAULT_CONTROLLER(1.0f);

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Slider::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);

    dFloat inv_timestep = 1.0f / timestep;

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
            NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else if (cj_data->m_cur_pos > cj_data->m_max_pos) {
            dVector s1(p0 + matrix1.m_right.Scale(cj_data->m_max_pos - cj_data->m_cur_pos - Joint::LINEAR_LIMIT_EPSILON));
            NewtonUserJointAddLinearRow(joint, &p0[0], &s1[0], &matrix0.m_right[0]);
            NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else
            bcontinue = true;
    }
    else
        bcontinue = true;
    if (bcontinue) {
        NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_right[0]);
        NewtonUserJointSetRowAcceleration(joint, -cj_data->m_cur_vel * inv_timestep);
        dFloat power = cj_data->m_friction * dAbs(cj_data->m_controller);
        NewtonUserJointSetRowMinimumFriction(joint, -power);
        NewtonUserJointSetRowMaximumFriction(joint, power);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }
}

void MSP::Slider::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);

    info->m_minLinearDof[0] = -0.0f;
    info->m_maxLinearDof[0] = 0.0f;
    info->m_minLinearDof[1] = -0.0f;
    info->m_maxLinearDof[1] = 0.0f;

    if (cj_data->m_limits_enabled) {
        info->m_minLinearDof[2] = (cj_data->m_min_pos - cj_data->m_cur_pos);
        info->m_minLinearDof[2] = (cj_data->m_max_pos - cj_data->m_cur_pos);
    }
    else {
        info->m_minLinearDof[2] = -Joint::CUSTOM_LARGE_VALUE;
        info->m_minLinearDof[2] = Joint::CUSTOM_LARGE_VALUE;
    }

    info->m_minAngularDof[0] = -0.0f;
    info->m_maxAngularDof[0] = 0.0f;
    info->m_minAngularDof[1] = -0.0f;
    info->m_maxAngularDof[1] = 0.0f;
    info->m_minAngularDof[2] = -0.0f;
    info->m_maxAngularDof[2] = 0.0f;
}

void MSP::Slider::on_destroy(MSP::Joint::JointData* joint_data) {
    delete (reinterpret_cast<SliderData*>(joint_data->m_cj_data));
}

void MSP::Slider::on_disconnect(MSP::Joint::JointData* joint_data) {
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    cj_data->m_cur_pos = 0.0f;
    cj_data->m_cur_vel = 0.0f;
    cj_data->m_cur_accel = 0.0f;
}

void MSP::Slider::adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix) {
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

VALUE MSP::Slider::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::SLIDER) ? Qtrue : Qfalse;
}

VALUE MSP::Slider::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::SLIDER;
    joint_data->m_cj_data = new SliderData;
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;
    joint_data->m_on_destroy = on_destroy;
    joint_data->m_on_disconnect = on_disconnect;
    //~ joint_data->m_adjust_pin_matrix_proc = adjust_pin_matrix_proc;

    return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::Slider::rbf_get_cur_position(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_pos * M_INCH_TO_METER);
}

VALUE MSP::Slider::rbf_get_cur_velocity(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_vel * M_INCH_TO_METER);
}

VALUE MSP::Slider::rbf_get_cur_acceleration(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_accel * M_INCH_TO_METER);
}

VALUE MSP::Slider::rbf_get_min(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_min_pos * M_INCH_TO_METER);
}

VALUE MSP::Slider::rbf_set_min(VALUE self, VALUE v_joint, VALUE v_min) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    cj_data->m_min_pos = Util::value_to_dFloat(v_min) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Slider::rbf_get_max(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_max_pos * M_INCH_TO_METER);
}

VALUE MSP::Slider::rbf_set_max(VALUE self, VALUE v_joint, VALUE v_max) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    cj_data->m_max_pos = Util::value_to_dFloat(v_max) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Slider::rbf_enable_limits(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    cj_data->m_limits_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Slider::rbf_limits_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_limits_enabled);
}

VALUE MSP::Slider::rbf_get_friction(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_friction * M_INCH_TO_METER);
}

VALUE MSP::Slider::rbf_set_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    cj_data->m_friction = Util::max_float(Util::value_to_dFloat(v_friction) * M_METER_TO_INCH, 0.0f);
    return Qnil;
}

VALUE MSP::Slider::rbf_get_controller(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_controller);
}

VALUE MSP::Slider::rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::SLIDER);
    SliderData* cj_data = reinterpret_cast<SliderData*>(joint_data->m_cj_data);
    dFloat controller = Util::value_to_dFloat(v_controller);
    if (controller != cj_data->m_controller) {
        cj_data->m_controller = controller;
        if (joint_data->m_connected) NewtonBodySetSleepState(joint_data->m_child, 0);
    }
    return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Slider::init_ruby(VALUE mNewton) {
    VALUE mSlider = rb_define_module_under(mNewton, "Slider");

    rb_define_module_function(mSlider, "is_valid?", VALUEFUNC(MSP::Slider::rbf_is_valid), 1);
    rb_define_module_function(mSlider, "create", VALUEFUNC(MSP::Slider::rbf_create), 1);
    rb_define_module_function(mSlider, "get_cur_position", VALUEFUNC(MSP::Slider::rbf_get_cur_position), 1);
    rb_define_module_function(mSlider, "get_cur_velocity", VALUEFUNC(MSP::Slider::rbf_get_cur_velocity), 1);
    rb_define_module_function(mSlider, "get_cur_acceleration", VALUEFUNC(MSP::Slider::rbf_get_cur_acceleration), 1);
    rb_define_module_function(mSlider, "get_min", VALUEFUNC(MSP::Slider::rbf_get_min), 1);
    rb_define_module_function(mSlider, "set_min", VALUEFUNC(MSP::Slider::rbf_set_min), 2);
    rb_define_module_function(mSlider, "get_max", VALUEFUNC(MSP::Slider::rbf_get_max), 1);
    rb_define_module_function(mSlider, "set_max", VALUEFUNC(MSP::Slider::rbf_set_max), 2);
    rb_define_module_function(mSlider, "enable_limits", VALUEFUNC(MSP::Slider::rbf_enable_limits), 2);
    rb_define_module_function(mSlider, "limits_enabled?", VALUEFUNC(MSP::Slider::rbf_limits_enabled), 1);
    rb_define_module_function(mSlider, "get_friction", VALUEFUNC(MSP::Slider::rbf_get_friction), 1);
    rb_define_module_function(mSlider, "set_friction", VALUEFUNC(MSP::Slider::rbf_set_friction), 2);
    rb_define_module_function(mSlider, "get_controller", VALUEFUNC(MSP::Slider::rbf_get_controller), 1);
    rb_define_module_function(mSlider, "set_controller", VALUEFUNC(MSP::Slider::rbf_set_controller), 2);
}
