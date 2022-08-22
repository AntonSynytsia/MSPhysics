/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_JOINT_H
#define MSP_JOINT_H

#include "msp.h"
#include "angular_integration.h"

class MSP::Joint {
public:
    // Constants
    static const dFloat DEFAULT_STIFFNESS;
    static const int DEFAULT_SOLVER_MODEL;
    static const bool DEFAULT_BODIES_COLLIDABLE;
    static const dFloat DEFAULT_BREAKING_FORCE;
    static const dFloat LINEAR_STIFF;
    static const dFloat LINEAR_DAMP;
    static const dFloat LINEAR_DAMP2;
    static const dFloat ANGULAR_STIFF;
    static const dFloat ANGULAR_DAMP;
    static const dFloat ANGULAR_DAMP2;
    static const dFloat STIFFNESS_RATIO;
    static const dFloat STIFFNESS_RATIO2;
    static const dFloat LINEAR_LIMIT_EPSILON;
    static const dFloat LINEAR_LIMIT_EPSILON2;
    static const dFloat ANGULAR_LIMIT_EPSILON;
    static const dFloat ANGULAR_LIMIT_EPSILON2;
    static const char* TYPE_NAME;
    static const dFloat CUSTOM_LARGE_VALUE;
    static const dFloat DEFAULT_STIFFNESS_RANGE;
    static const dFloat MIN_PIN_LENGTH;
    static const char* JOINT_NAMES[16];

    // Enumerators
    enum JointType {
        NONE,
        HINGE,
        MOTOR,
        SERVO,
        SLIDER,
        PISTON,
        UP_VECTOR,
        SPRING,
        CORKSCREW,
        BALL_AND_SOCKET,
        UNIVERSAL,
        FIXED,
        CURVY_SLIDER,
        CURVY_PISTON,
        PLANE,
        POINT_TO_POINT
    };

    // Structures
    struct JointData {
        const NewtonWorld* m_world;
        unsigned int m_dof;
        int m_solver_model;
        JointType m_jtype;
        dFloat m_stiffness;
        dFloat m_max_angle_error;
        bool m_bodies_collidable;
        dFloat m_breaking_force;
        NewtonJoint* m_constraint;
        bool m_connected;
        const NewtonBody* m_parent;
        const NewtonBody* m_child;
        dMatrix m_pin_matrix;
        dMatrix m_local_matrix0;
        dMatrix m_local_matrix1;
        dMatrix m_local_matrix2;
        dVector m_tension1;
        dVector m_tension2;
        VALUE m_user_data;
        VALUE m_group;
        void* m_cj_data;
        NewtonUserBilateralCallback m_submit_constraints;
        NewtonUserBilateralGetInfoCallback m_get_info;
        //void (*m_submit_constraints)(const NewtonJoint *user_joint, dFloat timestep, int thread_index);
        void (*m_on_destroy)(JointData* joint_data);
        void (*m_on_connect)(JointData* joint_data);
        void (*m_on_disconnect)(JointData* joint_data);
        void (*m_on_collidable_changed)(JointData* joint_data);
        void (*m_on_stiffness_changed)(JointData* joint_data);
        void (*m_on_pin_matrix_changed)(JointData* joint_data);
        void (*m_adjust_pin_matrix_proc)(JointData* joint_data, dMatrix& pin_matrix);
        JointData(const NewtonWorld* world, unsigned int dof, const NewtonBody* parent, const dMatrix& pin_matrix, const VALUE& v_group) :
            m_world(world),
            m_dof(dof),
            m_solver_model(DEFAULT_SOLVER_MODEL),
            m_jtype(NONE),
            m_stiffness(DEFAULT_STIFFNESS),
            m_max_angle_error(5.0 * M_DEG_TO_RAD),
            m_bodies_collidable(DEFAULT_BODIES_COLLIDABLE),
            m_breaking_force(DEFAULT_BREAKING_FORCE),
            m_connected(false),
            m_parent(parent),
            m_pin_matrix(pin_matrix),
            m_user_data(Qnil),
            m_group(v_group),
            m_tension1(0.0),
            m_tension2(0.0)
        {
            m_constraint = nullptr;
            m_child = nullptr;
            m_cj_data = nullptr;
            m_submit_constraints = nullptr;
            m_get_info = nullptr;
            m_on_destroy = nullptr;
            m_on_connect = nullptr;
            m_on_disconnect = nullptr;
            m_on_collidable_changed = nullptr;
            m_on_stiffness_changed = nullptr;
            m_on_pin_matrix_changed = nullptr;
            m_adjust_pin_matrix_proc = nullptr;
        }
        ~JointData()
        {
        }
    };

    // Variables
    static std::map<JointData*, bool> s_valid_joints;
    static std::map<VALUE, std::map<JointData*, bool>> s_map_group_to_joints;

    // Callback Functions
    static void submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index);
    static void constraint_destructor(const NewtonJoint* joint);
    static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);

    static void on_destroy(JointData* joint_data);
    static void on_connect(JointData* joint_data);
    static void on_disconnect(JointData* joint_data);
    static void on_collidable_changed(JointData* joint_data);
    static void on_stiffness_changed(JointData* joint_data);
    static void on_pin_matrix_changed(JointData* joint_data);
    static void adjust_pin_matrix_proc(JointData* joint_data, dMatrix& pin_matrix);

    // Helper Functions
    static bool c_is_joint_valid(JointData* address);
    static VALUE c_joint_to_value(JointData* joint_data);
    static JointData* c_value_to_joint(VALUE v_joint);
    static JointData* c_value_to_joint2(VALUE v_joint, JointType joint_type);
    static void c_calculate_local_matrix(JointData* joint_data);
    static void c_calculate_global_matrix(JointData* joint_data, dMatrix& matrix0, dMatrix& matrix1);
    static void c_calculate_global_matrix2(JointData* joint_data, dMatrix& matrix0, dMatrix& matrix1, dMatrix& matrix2);
    static void c_calculate_global_parent_matrix(JointData* joint_data, dMatrix& parent_matrix);
    static void c_calculate_angle(const dVector& dir, const dVector& cosDir, const dVector& sinDir, dFloat& sinAngle, dFloat& cosAngle);
    static dFloat c_calculate_angle2(const dVector& dir, const dVector& cosDir, const dVector& sinDir, dFloat& sinAngle, dFloat& cosAngle);
    static dFloat c_calculate_angle2(const dVector& dir, const dVector& cosDir, const dVector& sinDir);
    static void c_get_pin_matrix(JointData* joint_data, dMatrix& matrix_out);
    static JointData* c_create(const NewtonWorld* world, const NewtonBody* parent, dMatrix pin_matrix, VALUE v_group);
    static void c_destroy(JointData* joint_data);

    // Ruby Functions
    static VALUE rbf_is_valid(VALUE self, VALUE v_joint);
    static VALUE rbf_create(VALUE self, VALUE v_world, VALUE v_parent, VALUE v_pin_matrix, VALUE v_group);
    static VALUE rbf_destroy(VALUE self, VALUE v_joint);
    static VALUE rbf_connect(VALUE self, VALUE v_joint, VALUE v_child);
    static VALUE rbf_disconnect(VALUE self, VALUE v_joint);
    static VALUE rbf_is_connected(VALUE self, VALUE v_joint);
    static VALUE rbf_get_world(VALUE self, VALUE v_joint);
    static VALUE rbf_get_group(VALUE self, VALUE v_joint);
    static VALUE rbf_get_dof(VALUE self, VALUE v_joint);
    static VALUE rbf_get_type(VALUE self, VALUE v_joint);
    static VALUE rbf_get_parent(VALUE self, VALUE v_joint);
    static VALUE rbf_get_child(VALUE self, VALUE v_joint);
    static VALUE rbf_get_pin_matrix(VALUE self, VALUE v_joint);
    static VALUE rbf_set_pin_matrix(VALUE self, VALUE v_joint, VALUE v_pin_matrix);
    static VALUE rbf_get_pin_matrix2(VALUE self, VALUE v_joint, VALUE v_mode);
    static VALUE rbf_bodies_collidable(VALUE self, VALUE v_joint);
    static VALUE rbf_set_bodies_collidable(VALUE self, VALUE v_joint, VALUE v_state);
    static VALUE rbf_get_stiffness(VALUE self, VALUE v_joint);
    static VALUE rbf_set_stiffness(VALUE self, VALUE v_joint, VALUE v_stiffness);
    static VALUE rbf_get_user_data(VALUE self, VALUE v_joint);
    static VALUE rbf_set_user_data(VALUE self, VALUE v_joint, VALUE v_user_data);
    static VALUE rbf_get_breaking_force(VALUE self, VALUE v_joint);
    static VALUE rbf_set_breaking_force(VALUE self, VALUE v_joint, VALUE v_force);
    static VALUE rbf_get_solver_model(VALUE self, VALUE v_joint);
    static VALUE rbf_set_solver_model(VALUE self, VALUE v_joint, VALUE v_solver_model);
    static VALUE rbf_get_tension1(VALUE self, VALUE v_joint);
    static VALUE rbf_get_tension2(VALUE self, VALUE v_joint);
    static VALUE rbf_get_joint_by_group(VALUE self, VALUE v_group);
    static VALUE rbf_get_joints_by_group(VALUE self, VALUE v_group);
    static VALUE rbf_get_joint_data_by_group(VALUE self, VALUE v_group);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_JOINT_H */
