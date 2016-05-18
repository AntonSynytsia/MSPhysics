#ifndef MSP_JOINT_H
#define MSP_JOINT_H

#include "msp_util.h"
#include "angular_integration.h"

namespace MSNewton {
	class Joint;
}

class MSNewton::Joint {
public:
	// Constants
	static const ConstraintType DEFAULT_CONSTRAINT_TYPE;
	static const dFloat DEFAULT_STIFFNESS;
	static const bool DEFAULT_BODIES_COLLIDABLE;
	static const dFloat DEFAULT_BREAKING_FORCE;
	static const dFloat LINEAR_STIFF;
	static const dFloat LINEAR_DAMP;
	static const dFloat ANGULAR_STIFF;
	static const dFloat ANGULAR_DAMP;
	static const dFloat STIFFNESS_RATIO;
	static const dFloat STIFFNESS_RATIO2;
	static const dFloat LINEAR_LIMIT_EPSILON;
	static const dFloat ANGULAR_LIMIT_EPSILON;
	static const char* TYPE_NAME;
	static const dFloat CUSTOM_LARGE_VALUE;

	// Variables
	static std::map<NewtonSkeletonContainer*, bool> valid_skeletons;
	static std::map<VALUE, std::map<JointData*, bool>> map_group_to_joints;

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index);
	static void constraint_destructor(const NewtonJoint* joint);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);

	static void on_destroy(JointData* joint_data);
	static void on_connect(JointData* joint_data);
	static void on_disconnect(JointData* joint_data);
	static void on_collidable_changed(JointData* joint_data);
	static void on_stiffness_changed(JointData* joint_data);
	static void on_pin_matrix_changed(JointData* joint_data);

	static void skeleton_destructor(const NewtonSkeletonContainer* const skeleton);

	// Helper Functions
	static void c_calculate_local_matrix(JointData* joint_data);
	static void c_calculate_global_matrix(JointData* joint_data, dMatrix& matrix0, dMatrix& matrix1);
	static dFloat c_calculate_angle(const dVector& dir, const dVector& cosDir, const dVector& sinDir, dFloat& sinAngle, dFloat& cosAngle);
	static dFloat c_calculate_angle(const dVector& dir, const dVector& cosDir, const dVector& sinDir);
	static JointData* c_create(const NewtonWorld* world, const NewtonBody* parent, dMatrix pin_matrix, unsigned int dof, VALUE v_group);
	static void c_destroy(JointData* joint_data);

	static bool c_skeleton_is_valid(NewtonSkeletonContainer* skeleton);
	static NewtonSkeletonContainer* c_value_to_skeleton(VALUE v_skeleton);
	static VALUE c_skeleton_to_value(NewtonSkeletonContainer* skeleton);


	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_joint);
	static VALUE create(VALUE self, VALUE v_world, VALUE v_parent, VALUE v_pin_matrix, VALUE v_dof, VALUE v_group);
	static VALUE destroy(VALUE self, VALUE v_joint);
	static VALUE connect(VALUE self, VALUE v_joint, VALUE v_child);
	static VALUE disconnect(VALUE self, VALUE v_joint);
	static VALUE is_connected(VALUE self, VALUE v_joint);
	static VALUE get_dof(VALUE self, VALUE v_joint);
	static VALUE get_type(VALUE self, VALUE v_joint);
	static VALUE get_constraint_type(VALUE self, VALUE v_joint);
	static VALUE set_constraint_type(VALUE self, VALUE v_joint, VALUE v_ctype);
	static VALUE get_parent(VALUE self, VALUE v_joint);
	static VALUE get_child(VALUE self, VALUE v_joint);
	static VALUE get_pin_matrix(VALUE self, VALUE v_joint);
	static VALUE set_pin_matrix(VALUE self, VALUE v_joint, VALUE v_pin_matrix);
	static VALUE bodies_collidable(VALUE self, VALUE v_joint);
	static VALUE set_bodies_collidable(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE get_stiffness(VALUE self, VALUE v_joint);
	static VALUE set_stiffness(VALUE self, VALUE v_joint, VALUE v_stiffness);
	static VALUE get_user_data(VALUE self, VALUE v_joint);
	static VALUE set_user_data(VALUE self, VALUE v_joint, VALUE v_user_data);
	static VALUE get_breaking_force(VALUE self, VALUE v_joint);
	static VALUE set_breaking_force(VALUE self, VALUE v_joint, VALUE v_force);

	static VALUE get_group(VALUE self, VALUE v_joint);
	static VALUE get_joint_by_group(VALUE self, VALUE v_group);
	static VALUE get_joints_by_group(VALUE self, VALUE v_group);
	static VALUE get_joint_data_by_group(VALUE self, VALUE v_group);
	static VALUE get_joint_datas_by_group(VALUE self, VALUE v_group);

	static VALUE skeleton_is_valid(VALUE self, VALUE v_skeleton);
	static VALUE skeleton_create(VALUE self, VALUE v_root_body);
	static VALUE skeleton_destroy(VALUE self, VALUE v_skeleton);
	static VALUE skeleton_attach_bone(VALUE self, VALUE v_skeleton, VALUE v_child_bone, VALUE v_parent_bone);
	static VALUE skeleton_finalize(VALUE self, VALUE v_skeleton);
	static VALUE skeleton_get_solver(VALUE self, VALUE v_skeleton);
	static VALUE skeleton_set_solver(VALUE self, VALUE v_skeleton, VALUE v_solver);
};

void Init_msp_joint(VALUE mNewton);

#endif	/* MSP_JOINT_H */
