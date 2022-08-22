/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_joint.h"
#include "msp_world.h"
#include "msp_body.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Joint::DEFAULT_STIFFNESS(1.0);
const int MSP::Joint::DEFAULT_SOLVER_MODEL(2);
const bool MSP::Joint::DEFAULT_BODIES_COLLIDABLE(false);
const dFloat MSP::Joint::DEFAULT_BREAKING_FORCE(0.0);
const dFloat MSP::Joint::LINEAR_STIFF(1.5e8f);
const dFloat MSP::Joint::LINEAR_DAMP(0.1e8f);
const dFloat MSP::Joint::LINEAR_DAMP2(1.0e8f);
const dFloat MSP::Joint::ANGULAR_STIFF(1.5e8f);
const dFloat MSP::Joint::ANGULAR_DAMP(0.5e8f);
const dFloat MSP::Joint::ANGULAR_DAMP2(0.1e8f);
const dFloat MSP::Joint::STIFFNESS_RATIO(0.4f);
const dFloat MSP::Joint::STIFFNESS_RATIO2(0.01f);
const dFloat MSP::Joint::LINEAR_LIMIT_EPSILON(0.00015f);
const dFloat MSP::Joint::LINEAR_LIMIT_EPSILON2(0.0003f);
const dFloat MSP::Joint::ANGULAR_LIMIT_EPSILON(0.01f * M_DEG_TO_RAD);
const dFloat MSP::Joint::ANGULAR_LIMIT_EPSILON2(0.02f * M_DEG_TO_RAD);
const char* MSP::Joint::TYPE_NAME("MSPhysicsJoint");
const dFloat MSP::Joint::CUSTOM_LARGE_VALUE(1.0e20f);
const dFloat MSP::Joint::DEFAULT_STIFFNESS_RANGE(500.0);
const dFloat MSP::Joint::MIN_PIN_LENGTH(50.0);
const char* MSP::Joint::JOINT_NAMES[16] = { "None", "Hinge", "Motor", "Servo", "Slider", "Piston", "UpVector", "Spring", "Corkscrew", "BallAndSocket", "Universal", "Fixed", "CurvySlider", "CurvyPiston", "Plane", "PointToPoint" };


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

std::map<MSP::Joint::JointData*, bool> MSP::Joint::s_valid_joints;
std::map<VALUE, std::map<MSP::Joint::JointData*, bool>> MSP::Joint::s_map_group_to_joints;


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Joint::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    JointData* joint_data = reinterpret_cast<JointData*>(NewtonJointGetUserData(joint));
    // Update
    if (timestep > M_EPSILON) {
        if (joint_data->m_submit_constraints != nullptr)
            joint_data->m_submit_constraints(joint, timestep, thread_index);
    }
    // Record force
    for (int i = 0; i < NewtonUserJoinRowsCount(joint); ++i) {
        if (i < 3)
            joint_data->m_tension1[i] = NewtonUserJointGetRowForce(joint, i);
        else if (i > 3)
            joint_data->m_tension2[i - 3] = NewtonUserJointGetRowForce(joint, i);
    }
    // Destroy constraint if force exceeds particular limit.
    if (joint_data->m_breaking_force > M_EPSILON) {
        for (int i = 0; i < NewtonUserJoinRowsCount(joint); ++i) {
            if (dAbs(NewtonUserJointGetRowForce(joint, i)) >= joint_data->m_breaking_force) {
                MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(joint_data->m_world));
                world_data->m_joints_to_disconnect.push_back(joint);
                return;
            }
        }
    }
}

void MSP::Joint::constraint_destructor(const NewtonJoint* joint) {
    JointData* joint_data = reinterpret_cast<JointData*>(NewtonJointGetUserData(joint));
    on_disconnect(joint_data);
    joint_data->m_constraint = nullptr;
    joint_data->m_connected = false;
    joint_data->m_child = nullptr;
    joint_data->m_tension1.m_x = 0.0;
    joint_data->m_tension1.m_y = 0.0;
    joint_data->m_tension1.m_z = 0.0;
    joint_data->m_tension2.m_x = 0.0;
    joint_data->m_tension2.m_y = 0.0;
    joint_data->m_tension2.m_z = 0.0;
}

void MSP::Joint::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
    JointData* joint_data = reinterpret_cast<JointData*>(NewtonJointGetUserData(joint));

    strcpy(info->m_descriptionType, TYPE_NAME);

    info->m_attachBody_0 = joint_data->m_child;
    info->m_attachBody_1 = joint_data->m_parent;

    memcpy(info->m_attachmenMatrix_0, &joint_data->m_local_matrix0, sizeof(dMatrix));
    memcpy(info->m_attachmenMatrix_1, &joint_data->m_local_matrix1, sizeof(dMatrix));

    info->m_bodiesCollisionOn = joint_data->m_bodies_collidable;

    if (joint_data->m_get_info != nullptr)
        joint_data->m_get_info(joint, info);
}

void MSP::Joint::on_destroy(JointData* joint_data) {
    if (joint_data->m_on_destroy != nullptr)
        joint_data->m_on_destroy(joint_data);
}

void MSP::Joint::on_connect(JointData* joint_data) {
    if (joint_data->m_on_connect != nullptr)
        joint_data->m_on_connect(joint_data);
}

void MSP::Joint::on_disconnect(JointData* joint_data) {
    if (joint_data->m_on_disconnect != nullptr)
        joint_data->m_on_disconnect(joint_data);
}

void MSP::Joint::on_collidable_changed(JointData* joint_data) {
    if (joint_data->m_on_collidable_changed != nullptr)
        joint_data->m_on_collidable_changed(joint_data);
}

void MSP::Joint::on_stiffness_changed(JointData* joint_data) {
    if (joint_data->m_on_stiffness_changed != nullptr)
        joint_data->m_on_stiffness_changed(joint_data);
}

void MSP::Joint::on_pin_matrix_changed(JointData* joint_data) {
    if (joint_data->m_on_pin_matrix_changed != nullptr)
        joint_data->m_on_pin_matrix_changed(joint_data);
}

void MSP::Joint::adjust_pin_matrix_proc(JointData* joint_data, dMatrix& pin_matrix) {
    if (joint_data->m_adjust_pin_matrix_proc != nullptr)
        joint_data->m_adjust_pin_matrix_proc(joint_data, pin_matrix);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

bool MSP::Joint::c_is_joint_valid(JointData* address) {
    return s_valid_joints.find(address) != s_valid_joints.end();
}

VALUE MSP::Joint::c_joint_to_value(JointData* joint_data) {
    return rb_ull2inum(reinterpret_cast<unsigned long long>(joint_data));
}

MSP::Joint::JointData* MSP::Joint::c_value_to_joint(VALUE v_joint) {
    JointData* address = reinterpret_cast<JointData*>(rb_num2ull(v_joint));
    if (Util::s_validate_objects && s_valid_joints.find(address) == s_valid_joints.end())
        rb_raise(rb_eTypeError, "Given address doesn't reference a valid joint!");
    return address;
}

MSP::Joint::JointData* MSP::Joint::c_value_to_joint2(VALUE v_joint, JointType joint_type) {
    JointData* address = reinterpret_cast<JointData*>(rb_num2ull(v_joint));
    if (Util::s_validate_objects && s_valid_joints.find(address) == s_valid_joints.end())
        rb_raise(rb_eTypeError, "Given address doesn't reference a valid joint!");
    if (Util::s_validate_objects && address->m_jtype != joint_type)
        rb_raise(rb_eTypeError, "Given address doesn't reference a joint of a particular type!");
    return address;
}

void MSP::Joint::c_calculate_local_matrix(JointData* joint_data) {
    dMatrix pin_matrix, matrix0, matrix1;
    NewtonBodyGetMatrix(joint_data->m_child, &matrix0[0][0]);
    if (joint_data->m_parent != nullptr) {
        NewtonBodyGetMatrix(joint_data->m_parent, &matrix1[0][0]);
        pin_matrix = joint_data->m_pin_matrix * matrix1;
    }
    else
        pin_matrix = joint_data->m_pin_matrix;
    // Joint pin matrix with respect to parent body.
    joint_data->m_local_matrix2 = joint_data->m_parent != nullptr ? pin_matrix * matrix1.Inverse() : pin_matrix;
    // Adjust joint pin matrix.
    adjust_pin_matrix_proc(joint_data, pin_matrix);
    // Adjusted joint pin matrix with respect to child body.
    joint_data->m_local_matrix0 = pin_matrix * matrix0.Inverse();
    // Adjusted joint pin matrix with respect to parent body.
    joint_data->m_local_matrix1 = joint_data->m_parent != nullptr ? pin_matrix * matrix1.Inverse() : pin_matrix;
}

void MSP::Joint::c_calculate_global_matrix(JointData* joint_data, dMatrix& matrix0, dMatrix& matrix1) {
    dMatrix matrix;
    NewtonBodyGetMatrix(joint_data->m_child, &matrix[0][0]);
    matrix0 = joint_data->m_local_matrix0 * matrix;
    if (joint_data->m_parent != nullptr) {
        NewtonBodyGetMatrix(joint_data->m_parent, &matrix[0][0]);
        matrix1 = joint_data->m_local_matrix1 * matrix;
    }
    else
        matrix1 = joint_data->m_local_matrix1;
}

void MSP::Joint::c_calculate_global_matrix2(JointData* joint_data, dMatrix& matrix0, dMatrix& matrix1, dMatrix& matrix2) {
    dMatrix matrix;
    NewtonBodyGetMatrix(joint_data->m_child, &matrix[0][0]);
    matrix0 = joint_data->m_local_matrix0 * matrix;
    if (joint_data->m_parent != nullptr) {
        NewtonBodyGetMatrix(joint_data->m_parent, &matrix[0][0]);
        matrix1 = joint_data->m_local_matrix1 * matrix;
        matrix2 = joint_data->m_local_matrix2 * matrix;
    }
    else {
        matrix1 = joint_data->m_local_matrix1;
        matrix2 = joint_data->m_local_matrix2;
    }
}

void MSP::Joint::c_calculate_global_parent_matrix(JointData* joint_data, dMatrix& parent_matrix) {
    if (joint_data->m_parent != nullptr) {
        dMatrix matrix;
        NewtonBodyGetMatrix(joint_data->m_parent, &matrix[0][0]);
        parent_matrix = joint_data->m_local_matrix1 * matrix;
    }
    else
        parent_matrix = joint_data->m_local_matrix1;
}

void MSP::Joint::c_calculate_angle(const dVector& dir, const dVector& cosDir, const dVector& sinDir, dFloat& sinAngle, dFloat& cosAngle) {
    cosAngle = dir.DotProduct3(cosDir);
    sinAngle = (dir.CrossProduct(cosDir)).DotProduct3(sinDir);
}

dFloat MSP::Joint::c_calculate_angle2(const dVector& dir, const dVector& cosDir, const dVector& sinDir, dFloat& sinAngle, dFloat& cosAngle) {
    cosAngle = dir.DotProduct3(cosDir);
    sinAngle = (dir.CrossProduct(cosDir)).DotProduct3(sinDir);
    return dAtan2(sinAngle, cosAngle);
}

dFloat MSP::Joint::c_calculate_angle2(const dVector& dir, const dVector& cosDir, const dVector& sinDir) {
    dFloat sinAngle;
    dFloat cosAngle;
    return c_calculate_angle2(dir, cosDir, sinDir, sinAngle, cosAngle);
}

void MSP::Joint::c_get_pin_matrix(JointData* joint_data, dMatrix& matrix_out) {
    if (joint_data->m_parent != nullptr && MSP::Body::c_is_body_valid(joint_data->m_parent)) {
        dMatrix parent_matrix;
        NewtonBodyGetMatrix(joint_data->m_parent, &parent_matrix[0][0]);
        matrix_out = joint_data->m_pin_matrix * parent_matrix;
    }
    else
        matrix_out = joint_data->m_pin_matrix;
}

MSP::Joint::JointData* MSP::Joint::c_create(const NewtonWorld* world, const NewtonBody* parent, dMatrix pin_matrix, VALUE v_group) {
    if (parent != nullptr) {
        dMatrix parent_matrix;
        NewtonBodyGetMatrix(parent, &parent_matrix[0][0]);
        pin_matrix = pin_matrix * parent_matrix.Inverse();
    }
    JointData* joint_data = new JointData(world, 6, parent, pin_matrix, v_group);
    if (v_group != Qnil) s_map_group_to_joints[v_group][joint_data] = true;
    s_valid_joints[joint_data] = true;
    return joint_data;
}

void MSP::Joint::c_destroy(JointData* joint_data) {
    if (s_valid_joints.find(joint_data) != s_valid_joints.end())
        s_valid_joints.erase(joint_data);
    VALUE v_group = rb_ary_entry(joint_data->m_user_data, 1);
    if (v_group != Qnil) {
        std::map<VALUE, std::map<JointData*, bool>>::iterator it(s_map_group_to_joints.find(v_group));
        if (it != s_map_group_to_joints.end()) {
            if (it->second.find(joint_data) != it->second.end())
                it->second.erase(joint_data);
            if (it->second.empty())
                s_map_group_to_joints.erase(it);
        }
    }
    if (joint_data->m_connected)
        NewtonDestroyJoint(joint_data->m_world, joint_data->m_constraint);
    on_destroy(joint_data);
    delete joint_data;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Joint::rbf_is_valid(VALUE self, VALUE v_joint) {
    return c_is_joint_valid(reinterpret_cast<JointData*>(Util::value_to_ull(v_joint))) ? Qtrue : Qfalse;
}

VALUE MSP::Joint::rbf_create(VALUE self, VALUE v_world, VALUE v_parent, VALUE v_pin_matrix, VALUE v_group) {
    const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
    const NewtonBody* parent = (v_parent == Qnil) ? nullptr : MSP::Body::c_value_to_body(v_parent);
    dMatrix pin_matrix(Util::value_to_matrix(v_pin_matrix));
    Util::extract_matrix_scale(pin_matrix);
    if (parent != nullptr) {
        const NewtonWorld* parent_world = NewtonBodyGetWorld(parent);
        if (parent_world != world)
            rb_raise(rb_eTypeError, "Given parent body is not from the preset world!");
    }
    JointData* joint_data = c_create(world, parent, pin_matrix, v_group);
    return c_joint_to_value(joint_data);
}

VALUE MSP::Joint::rbf_destroy(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    c_destroy(joint_data);
    return Qnil;
}

VALUE MSP::Joint::rbf_connect(VALUE self, VALUE v_joint, VALUE v_child) {
    JointData* joint_data = c_value_to_joint(v_joint);
    const NewtonBody* child = MSP::Body::c_value_to_body(v_child);
    if (joint_data->m_connected) return Qfalse;
    const NewtonWorld* child_world = NewtonBodyGetWorld(child);
    if (child_world != joint_data->m_world)
        rb_raise(rb_eTypeError, "Given child body is not from the preset world!");
    if (joint_data->m_parent != nullptr && !MSP::Body::c_is_body_valid(joint_data->m_parent))
        joint_data->m_parent = nullptr;
    if (child == joint_data->m_parent)
        rb_raise(rb_eTypeError, "Using same body as parent and child is not allowed!");
    joint_data->m_child = child;
    c_calculate_local_matrix(joint_data);
    joint_data->m_constraint = NewtonConstraintCreateUserJoint(joint_data->m_world, joint_data->m_dof, submit_constraints, joint_data->m_child, joint_data->m_parent);
    NewtonJointSetCollisionState(joint_data->m_constraint, joint_data->m_bodies_collidable ? 1 : 0);
    NewtonUserJointSetSolverModel(joint_data->m_constraint, joint_data->m_solver_model);
    NewtonJointSetUserData(joint_data->m_constraint, joint_data);
    NewtonJointSetDestructor(joint_data->m_constraint, constraint_destructor);
    joint_data->m_connected = true;
    on_connect(joint_data);
    return Qtrue;
}

VALUE MSP::Joint::rbf_disconnect(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    if (!joint_data->m_connected) return Qfalse;
    NewtonDestroyJoint(joint_data->m_world, joint_data->m_constraint);
    return Qtrue;
}

VALUE MSP::Joint::rbf_is_connected(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return Util::to_value(joint_data->m_connected);
}

VALUE MSP::Joint::rbf_get_world(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return MSP::World::c_world_to_value(joint_data->m_world);
}

VALUE MSP::Joint::rbf_get_group(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return joint_data->m_group;
}

VALUE MSP::Joint::rbf_get_dof(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return Util::to_value(joint_data->m_dof);
}

VALUE MSP::Joint::rbf_get_type(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return Util::to_value(joint_data->m_jtype);
}

VALUE MSP::Joint::rbf_get_parent(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    if (joint_data->m_parent != nullptr && !MSP::Body::c_is_body_valid(joint_data->m_parent))
        joint_data->m_parent = nullptr;
    return joint_data->m_parent != nullptr ? MSP::Body::c_body_to_value(joint_data->m_parent) : Qnil;
}

VALUE MSP::Joint::rbf_get_child(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return joint_data->m_child != nullptr ? MSP::Body::c_body_to_value(joint_data->m_child) : Qnil;
}

VALUE MSP::Joint::rbf_get_pin_matrix(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    if (joint_data->m_parent != nullptr && !MSP::Body::c_is_body_valid(joint_data->m_parent))
        joint_data->m_parent = nullptr;
    dMatrix pin_matrix;
    if (joint_data->m_parent != nullptr) {
        dMatrix parent_matrix;
        NewtonBodyGetMatrix(joint_data->m_parent, &parent_matrix[0][0]);
        pin_matrix = joint_data->m_pin_matrix * parent_matrix;
    }
    else
        pin_matrix = joint_data->m_pin_matrix;
    return Util::matrix_to_value(pin_matrix);
}

VALUE MSP::Joint::rbf_set_pin_matrix(VALUE self, VALUE v_joint, VALUE v_pin_matrix) {
    JointData* joint_data = c_value_to_joint(v_joint);
    dMatrix pin_matrix(Util::value_to_matrix(v_pin_matrix));
    Util::extract_matrix_scale(pin_matrix);
    if (joint_data->m_parent != nullptr && !MSP::Body::c_is_body_valid(joint_data->m_parent))
        joint_data->m_parent = nullptr;
    if (joint_data->m_parent != nullptr) {
        dMatrix parent_matrix;
        NewtonBodyGetMatrix(joint_data->m_parent, &parent_matrix[0][0]);
        joint_data->m_pin_matrix = pin_matrix * parent_matrix.Inverse();
    }
    else
        joint_data->m_pin_matrix = pin_matrix;
    if (joint_data->m_connected) {
        c_calculate_local_matrix(joint_data);
        on_pin_matrix_changed(joint_data);
    }
    return Qnil;
}

VALUE MSP::Joint::rbf_get_pin_matrix2(VALUE self, VALUE v_joint, VALUE v_mode) {
    JointData* joint_data = c_value_to_joint(v_joint);
    int mode = Util::value_to_int(v_mode);
    if (!joint_data->m_connected)
        return Qnil;
    if (mode == 0) {
        dMatrix matrix;
        NewtonBodyGetMatrix(joint_data->m_child, &matrix[0][0]);
        return Util::matrix_to_value(joint_data->m_local_matrix0 * matrix);
    }
    else if (mode == 1) {
        if (joint_data->m_parent != nullptr) {
            dMatrix matrix;
            NewtonBodyGetMatrix(joint_data->m_parent, &matrix[0][0]);
            return Util::matrix_to_value(joint_data->m_local_matrix1 * matrix);
        }
        else
            return Util::matrix_to_value(joint_data->m_local_matrix1);
    }
    else {
        if (joint_data->m_parent != nullptr) {
            dMatrix matrix;
            NewtonBodyGetMatrix(joint_data->m_parent, &matrix[0][0]);
            return Util::matrix_to_value(joint_data->m_local_matrix2 * matrix);
        }
        else
            return Util::matrix_to_value(joint_data->m_local_matrix2);
    }
}

VALUE MSP::Joint::rbf_bodies_collidable(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return Util::to_value(joint_data->m_bodies_collidable);
}

VALUE MSP::Joint::rbf_set_bodies_collidable(VALUE self, VALUE v_joint, VALUE v_state) {
    JointData* joint_data = c_value_to_joint(v_joint);
    joint_data->m_bodies_collidable = Util::value_to_bool(v_state);
    if (joint_data->m_constraint != nullptr)
        NewtonJointSetCollisionState(joint_data->m_constraint, joint_data->m_bodies_collidable ? 1 : 0);
    on_collidable_changed(joint_data);
    return Qnil;
}

VALUE MSP::Joint::rbf_get_stiffness(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return Util::to_value(joint_data->m_stiffness);
}

VALUE MSP::Joint::rbf_set_stiffness(VALUE self, VALUE v_joint, VALUE v_stiffness) {
    JointData* joint_data = c_value_to_joint(v_joint);
    joint_data->m_stiffness = Util::clamp_dFloat(Util::value_to_dFloat(v_stiffness), 0.0, 1.0);
    on_stiffness_changed(joint_data);
    return Qnil;
}

VALUE MSP::Joint::rbf_get_user_data(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return joint_data->m_user_data;
}

VALUE MSP::Joint::rbf_set_user_data(VALUE self, VALUE v_joint, VALUE v_user_data) {
    JointData* joint_data = c_value_to_joint(v_joint);
    joint_data->m_user_data = v_user_data;
    return Qnil;
}

VALUE MSP::Joint::rbf_get_breaking_force(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return Util::to_value(joint_data->m_breaking_force * M_INCH_TO_METER);
}

VALUE MSP::Joint::rbf_set_breaking_force(VALUE self, VALUE v_joint, VALUE v_force) {
    JointData* joint_data = c_value_to_joint(v_joint);
    joint_data->m_breaking_force = Util::max_dFloat(Util::value_to_dFloat(v_force) * M_METER_TO_INCH, 0.0);
    return Qnil;
}

VALUE MSP::Joint::rbf_get_solver_model(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    return Util::to_value(joint_data->m_solver_model);
}

VALUE MSP::Joint::rbf_set_solver_model(VALUE self, VALUE v_joint, VALUE v_solver_model) {
    JointData* joint_data = c_value_to_joint(v_joint);
    joint_data->m_solver_model = Util::clamp_int(Util::value_to_int(v_solver_model), 0, 2);
    if (joint_data->m_constraint != nullptr)
        NewtonUserJointSetSolverModel(joint_data->m_constraint, joint_data->m_solver_model);
    return Qnil;
}

VALUE MSP::Joint::rbf_get_tension1(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    dMatrix parent_matrix;
    MSP::Joint::c_calculate_global_parent_matrix(joint_data, parent_matrix);
    dVector tension(parent_matrix.RotateVector(joint_data->m_tension1));
    return Util::vector_to_value(tension.Scale(M_INCH_TO_METER));
}

VALUE MSP::Joint::rbf_get_tension2(VALUE self, VALUE v_joint) {
    JointData* joint_data = c_value_to_joint(v_joint);
    dMatrix parent_matrix;
    MSP::Joint::c_calculate_global_parent_matrix(joint_data, parent_matrix);
    dVector tension(parent_matrix.RotateVector(joint_data->m_tension2));
    return Util::vector_to_value(tension.Scale(M_INCH_TO_METER));
}

VALUE MSP::Joint::rbf_get_joint_by_group(VALUE self, VALUE v_group) {
    std::map<VALUE, std::map<JointData*, bool>>::iterator it(s_map_group_to_joints.find(v_group));
    if (it == s_map_group_to_joints.end() || it->second.empty())
        return Qnil;
    else
        return c_joint_to_value(it->second.begin()->first);
}

VALUE MSP::Joint::rbf_get_joints_by_group(VALUE self, VALUE v_group) {
    std::map<VALUE, std::map<JointData*, bool>>::iterator it(s_map_group_to_joints.find(v_group));
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_joints = rb_ary_new();
    if (it == s_map_group_to_joints.end() || it->second.empty())
        return v_joints;
    else {
        for (std::map<JointData*, bool>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            VALUE v_address = c_joint_to_value(it2->first);
            if (proc_given) {
                VALUE v_result = rb_yield_values(2, v_address, it2->first->m_user_data);
                if (v_result != Qnil) rb_ary_push(v_joints, v_result);
            }
            else
                rb_ary_push(v_joints, v_address);
        }
        return v_joints;
    }
}

VALUE MSP::Joint::rbf_get_joint_data_by_group(VALUE self, VALUE v_group) {
    std::map<VALUE, std::map<JointData*, bool>>::iterator it(s_map_group_to_joints.find(v_group));
    if (it == s_map_group_to_joints.end() || it->second.empty())
        return Qnil;
    else {
        for (std::map<JointData*, bool>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            VALUE v_user_data = rb_ary_entry(it2->first->m_user_data, 0);
            if (v_user_data != Qnil) return v_user_data;
        }
    }
    return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Joint::init_ruby(VALUE mNewton) {
    VALUE mJoint = rb_define_module_under(mNewton, "Joint");

    rb_define_module_function(mJoint, "is_valid?", VALUEFUNC(MSP::Joint::rbf_is_valid), 1);
    rb_define_module_function(mJoint, "create", VALUEFUNC(MSP::Joint::rbf_create), 4);
    rb_define_module_function(mJoint, "destroy", VALUEFUNC(MSP::Joint::rbf_destroy), 1);
    rb_define_module_function(mJoint, "connect", VALUEFUNC(MSP::Joint::rbf_connect), 2);
    rb_define_module_function(mJoint, "disconnect", VALUEFUNC(MSP::Joint::rbf_disconnect), 1);
    rb_define_module_function(mJoint, "is_connected?", VALUEFUNC(MSP::Joint::rbf_is_connected), 1);
    rb_define_module_function(mJoint, "get_world", VALUEFUNC(MSP::Joint::rbf_get_world), 1);
    rb_define_module_function(mJoint, "get_group", VALUEFUNC(MSP::Joint::rbf_get_group), 1);
    rb_define_module_function(mJoint, "get_dof", VALUEFUNC(MSP::Joint::rbf_get_dof), 1);
    rb_define_module_function(mJoint, "get_type", VALUEFUNC(MSP::Joint::rbf_get_type), 1);
    rb_define_module_function(mJoint, "get_parent", VALUEFUNC(MSP::Joint::rbf_get_parent), 1);
    rb_define_module_function(mJoint, "get_child", VALUEFUNC(MSP::Joint::rbf_get_child), 1);
    rb_define_module_function(mJoint, "get_pin_matrix", VALUEFUNC(MSP::Joint::rbf_get_pin_matrix), 1);
    rb_define_module_function(mJoint, "set_pin_matrix", VALUEFUNC(MSP::Joint::rbf_set_pin_matrix), 2);
    rb_define_module_function(mJoint, "get_pin_matrix2", VALUEFUNC(MSP::Joint::rbf_get_pin_matrix2), 2);
    rb_define_module_function(mJoint, "bodies_collidable?", VALUEFUNC(MSP::Joint::rbf_bodies_collidable), 1);
    rb_define_module_function(mJoint, "set_bodies_collidable", VALUEFUNC(MSP::Joint::rbf_set_bodies_collidable), 2);
    rb_define_module_function(mJoint, "get_stiffness", VALUEFUNC(MSP::Joint::rbf_get_stiffness), 1);
    rb_define_module_function(mJoint, "set_stiffness", VALUEFUNC(MSP::Joint::rbf_set_stiffness), 2);
    rb_define_module_function(mJoint, "get_user_data", VALUEFUNC(MSP::Joint::rbf_get_user_data), 1);
    rb_define_module_function(mJoint, "set_user_data", VALUEFUNC(MSP::Joint::rbf_set_user_data), 2);
    rb_define_module_function(mJoint, "get_breaking_force", VALUEFUNC(MSP::Joint::rbf_get_breaking_force), 1);
    rb_define_module_function(mJoint, "set_breaking_force", VALUEFUNC(MSP::Joint::rbf_set_breaking_force), 2);
    rb_define_module_function(mJoint, "get_solver_model", VALUEFUNC(MSP::Joint::rbf_get_solver_model), 1);
    rb_define_module_function(mJoint, "set_solver_model", VALUEFUNC(MSP::Joint::rbf_set_solver_model), 2);
    rb_define_module_function(mJoint, "get_tension1", VALUEFUNC(MSP::Joint::rbf_get_tension1), 1);
    rb_define_module_function(mJoint, "get_tension2", VALUEFUNC(MSP::Joint::rbf_get_tension2), 1);
    rb_define_module_function(mJoint, "get_joint_by_group", VALUEFUNC(MSP::Joint::rbf_get_joint_by_group), 1);
    rb_define_module_function(mJoint, "get_joints_by_group", VALUEFUNC(MSP::Joint::rbf_get_joints_by_group), 1);
    rb_define_module_function(mJoint, "get_joint_data_by_group", VALUEFUNC(MSP::Joint::rbf_get_joint_data_by_group), 1);
}
