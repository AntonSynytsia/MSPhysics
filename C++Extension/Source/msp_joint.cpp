#include "msp_joint.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables and Constants
 ///////////////////////////////////////////////////////////////////////////////
*/

const ConstraintType MSNewton::Joint::DEFAULT_CONSTRAINT_TYPE = CT_STANDARD;
const dFloat MSNewton::Joint::DEFAULT_STIFFNESS = 1.0f;
const bool MSNewton::Joint::DEFAULT_BODIES_COLLIDABLE = false;
const dFloat MSNewton::Joint::DEFAULT_BREAKING_FORCE = 0.0f;
const dFloat MSNewton::Joint::LINEAR_STIFF = 1.5e8f;
const dFloat MSNewton::Joint::LINEAR_DAMP = 0.1e8f;
const dFloat MSNewton::Joint::ANGULAR_STIFF = 1.5e8f;
const dFloat MSNewton::Joint::ANGULAR_DAMP = 0.5e8f;
const dFloat MSNewton::Joint::STIFFNESS_RATIO = 0.4f;
const dFloat MSNewton::Joint::STIFFNESS_RATIO2 = 0.01f;
const dFloat MSNewton::Joint::ANGULAR_LIMIT_EPSILON = 0.1f * DEG_TO_RAD;
const dFloat MSNewton::Joint::LINEAR_LIMIT_EPSILON = 0.001f;
const char* MSNewton::Joint::TYPE_NAME = "MSPhysicsJoint";
const dFloat MSNewton::Joint::CUSTOM_LARGE_VALUE = 1.0e15f;

std::map<NewtonSkeletonContainer*, bool> MSNewton::Joint::valid_skeletons;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Joint::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	if (timestep == 0.0f) return;
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	if (joint_data->submit_constraints != nullptr)
		joint_data->submit_constraints(joint, timestep, thread_index);
	// Destroy constraint if force exceeds particular limit.
	if (joint_data->breaking_force == 0.0f) return;
	for (int i = 0; i < NewtonUserJoinRowsCount(joint); ++i) {
		if (dAbs(NewtonUserJointGetRowForce(joint, i)) >= joint_data->breaking_force) {
			WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
			world_data->joints_to_disconnect.push_back(joint_data);
			return;
		}
	}
}

void MSNewton::Joint::constraint_destructor(const NewtonJoint* joint) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	on_disconnect(joint_data);
	joint_data->constraint = nullptr;
	joint_data->connected = false;
	joint_data->child = nullptr;
}

void MSNewton::Joint::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);

	strcpy(info->m_descriptionType, TYPE_NAME);

	info->m_attachBody_0 = joint_data->child;
	info->m_attachBody_1 = joint_data->parent;

	memcpy(info->m_attachmenMatrix_0, &joint_data->local_matrix0, sizeof(dMatrix));
	memcpy(info->m_attachmenMatrix_1, &joint_data->local_matrix1, sizeof(dMatrix));

	info->m_bodiesCollisionOn = joint_data->bodies_collidable;

	if (joint_data->get_info != nullptr)
		joint_data->get_info(joint, info);
}

void MSNewton::Joint::on_destroy(JointData* joint_data) {
	if (joint_data->on_destroy != nullptr)
		joint_data->on_destroy(joint_data);
}

void MSNewton::Joint::on_connect(JointData* joint_data) {
	if (joint_data->on_connect != nullptr)
		joint_data->on_connect(joint_data);
}

void MSNewton::Joint::on_disconnect(JointData* joint_data) {
	if (joint_data->on_disconnect != nullptr)
		joint_data->on_disconnect(joint_data);
}

void MSNewton::Joint::on_collidable_changed(JointData* joint_data) {
	if (joint_data->on_collidable_changed != nullptr)
		joint_data->on_collidable_changed(joint_data);
}

void MSNewton::Joint::on_stiffness_changed(JointData* joint_data) {
	if (joint_data->on_stiffness_changed != nullptr)
		joint_data->on_stiffness_changed(joint_data);
}

void MSNewton::Joint::on_pin_matrix_changed(JointData* joint_data) {
	if (joint_data->on_pin_matrix_changed != nullptr)
		joint_data->on_pin_matrix_changed(joint_data);
}

void MSNewton::Joint::skeleton_destructor(const NewtonSkeletonContainer* const skeleton) {
	valid_skeletons.erase((NewtonSkeletonContainer*)skeleton);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Joint::c_calculate_local_matrix(JointData* joint_data) {
	dMatrix pin_matrix;
	dMatrix matrix0;
	dMatrix matrix1;
	NewtonBodyGetMatrix(joint_data->child, &matrix0[0][0]);
	if (joint_data->parent != nullptr) {
		NewtonBodyGetMatrix(joint_data->parent, &matrix1[0][0]);
		pin_matrix = joint_data->pin_matrix * matrix1;
	}
	else
		pin_matrix = joint_data->pin_matrix;
	joint_data->local_matrix0 = pin_matrix * matrix0.Inverse();
	joint_data->local_matrix1 = joint_data->parent != nullptr ? pin_matrix * matrix1.Inverse() : pin_matrix;
}

void MSNewton::Joint::c_calculate_global_matrix(JointData* joint_data, dMatrix& matrix0, dMatrix& matrix1) {
	dMatrix child_matrix;
	NewtonBodyGetMatrix(joint_data->child, &child_matrix[0][0]);
	dMatrix parent_matrix;
	if (joint_data->parent != nullptr)
		NewtonBodyGetMatrix(joint_data->parent, &parent_matrix[0][0]);
	matrix0 = joint_data->local_matrix0 * child_matrix;
	matrix1 = joint_data->parent != nullptr ? joint_data->local_matrix1 * parent_matrix : joint_data->local_matrix1;
}

dFloat MSNewton::Joint::c_calculate_angle(const dVector& dir, const dVector& cosDir, const dVector& sinDir, dFloat& sinAngle, dFloat& cosAngle) {
	cosAngle = dir % cosDir;
	sinAngle = (dir * cosDir) % sinDir;
	return dAtan2(sinAngle, cosAngle);
}

dFloat MSNewton::Joint::c_calculate_angle(const dVector& dir, const dVector& cosDir, const dVector& sinDir) {
	dFloat sinAngle;
	dFloat cosAngle;
	return c_calculate_angle(dir, cosDir, sinDir, sinAngle, cosAngle);
}

JointData* MSNewton::Joint::c_create(const NewtonWorld* world, const NewtonBody* parent, dMatrix pin_matrix, unsigned int dof) {
	if (parent != nullptr) {
		dMatrix parent_matrix;
		NewtonBodyGetMatrix(parent, &parent_matrix[0][0]);
		pin_matrix = pin_matrix * parent_matrix.Inverse();
	}
	JointData* joint_data = new JointData;
	joint_data->world = world;
	joint_data->dof = dof;
	joint_data->jtype = JT_NONE;
	joint_data->ctype = DEFAULT_CONSTRAINT_TYPE;
	joint_data->stiffness = DEFAULT_STIFFNESS;
	joint_data->bodies_collidable = DEFAULT_BODIES_COLLIDABLE;
	joint_data->breaking_force = DEFAULT_BREAKING_FORCE;
	joint_data->constraint = nullptr;
	joint_data->connected = false;
	joint_data->parent = parent;
	joint_data->child = nullptr;
	joint_data->pin_matrix = pin_matrix;
	joint_data->local_matrix0;
	joint_data->local_matrix1;
	joint_data->user_data = rb_ary_new();
	joint_data->cj_data = nullptr;
	joint_data->submit_constraints = nullptr;
	joint_data->get_info = nullptr;
	joint_data->on_destroy = nullptr;
	joint_data->on_connect = nullptr;
	joint_data->on_disconnect = nullptr;
	joint_data->on_collidable_changed = nullptr;
	joint_data->on_stiffness_changed = nullptr;
	joint_data->on_pin_matrix_changed = nullptr;

	rb_gc_register_address(&joint_data->user_data);
	valid_joints[joint_data] = true;
	return joint_data;
}

void MSNewton::Joint::c_destroy(JointData* joint_data) {
	valid_joints.erase(joint_data);
	if (joint_data->connected)
		NewtonDestroyJoint(joint_data->world, joint_data->constraint);
	on_destroy(joint_data);
	rb_ary_clear(joint_data->user_data);
	rb_gc_unregister_address(&joint_data->user_data);
	#ifndef RUBY_VERSION18
		rb_ary_free(joint_data->user_data);
	#endif
	delete joint_data;
}


bool MSNewton::Joint::c_skeleton_is_valid(NewtonSkeletonContainer* skeleton) {
	return valid_skeletons.find(skeleton) != valid_skeletons.end();
}

NewtonSkeletonContainer* MSNewton::Joint::c_value_to_skeleton(VALUE v_value) {
	NewtonSkeletonContainer* address = (NewtonSkeletonContainer*)NUM2LL(v_value);
	if (validate_objects == true && valid_skeletons.find(address) == valid_skeletons.end())
		rb_raise(rb_eTypeError, "Given address doesn't reference a valid skeleton container!");
	return address;
}

VALUE MSNewton::Joint::c_skeleton_to_value(NewtonSkeletonContainer* skeleton) {
	return rb_ll2inum((long long)skeleton);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::Joint::is_valid(VALUE self, VALUE v_joint) {
	return Util::is_joint_valid((JointData*)Util::value_to_ll(v_joint)) ? Qtrue : Qfalse;
}

VALUE MSNewton::Joint::create(VALUE self, VALUE v_world, VALUE v_parent, VALUE v_pin_matrix, VALUE v_dof) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	const NewtonBody* parent = v_parent == Qnil ? nullptr : Util::value_to_body(v_parent);
	dMatrix pin_matrix = Util::value_to_matrix(v_pin_matrix, world_data->scale);
	Util::extract_matrix_scale(pin_matrix);
	unsigned int dof = Util::value_to_uint(v_dof);
	if (parent != nullptr) {
		const NewtonWorld* parent_world = NewtonBodyGetWorld(parent);
		if (parent_world != world)
			rb_raise(rb_eTypeError, "Given parent body is not from the preset world!");
	}
	JointData* joint_data = c_create(world, parent, pin_matrix, dof);
	return Util::to_value(joint_data);
}

VALUE MSNewton::Joint::destroy(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	c_destroy(joint_data);
	return Qnil;
}

VALUE MSNewton::Joint::connect(VALUE self, VALUE v_joint, VALUE v_child) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	const NewtonBody* child = Util::value_to_body(v_child);
	if (joint_data->connected) return Qfalse;
	const NewtonWorld* child_world = NewtonBodyGetWorld(child);
	if (child_world != joint_data->world)
		rb_raise(rb_eTypeError, "Given child body is not from the preset world!");
	if (joint_data->parent != nullptr && !Util::is_body_valid(joint_data->parent))
		joint_data->parent = nullptr;
	if (child == joint_data->parent)
		rb_raise(rb_eTypeError, "Using same body as parent and child is not allowed!");
	joint_data->child = child;
	c_calculate_local_matrix(joint_data);
	joint_data->constraint = NewtonConstraintCreateUserJoint(joint_data->world, joint_data->dof, submit_constraints, get_info, joint_data->child, joint_data->parent);
	NewtonJointSetCollisionState(joint_data->constraint, joint_data->bodies_collidable ? 1 : 0);
	NewtonJointSetStiffness(joint_data->constraint, joint_data->stiffness);
	NewtonJointSetUserData(joint_data->constraint, joint_data);
	NewtonJointSetDestructor(joint_data->constraint, constraint_destructor);
	joint_data->connected = true;
	on_connect(joint_data);
	return Qtrue;
}

VALUE MSNewton::Joint::disconnect(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	if (!joint_data->connected) return Qfalse;
	NewtonDestroyJoint(joint_data->world, joint_data->constraint);
	return Qtrue;
}

VALUE MSNewton::Joint::is_connected(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	return Util::to_value(joint_data->connected);
}

VALUE MSNewton::Joint::get_dof(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	return Util::to_value(joint_data->dof);
}

VALUE MSNewton::Joint::get_type(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	return Util::to_value(joint_data->jtype);
}

VALUE MSNewton::Joint::get_constraint_type(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	return Util::to_value(joint_data->ctype);
}

VALUE MSNewton::Joint::set_constraint_type(VALUE self, VALUE v_joint, VALUE v_ctype) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	joint_data->ctype = (ConstraintType)Util::clamp(Util::value_to_int(v_ctype), 0, 2);
	return Util::to_value(joint_data->ctype);
}

VALUE MSNewton::Joint::get_parent(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	if (joint_data->parent != nullptr && !Util::is_body_valid(joint_data->parent))
		joint_data->parent = nullptr;
	return joint_data->parent ? Util::to_value(joint_data->parent) : Qnil;
}

VALUE MSNewton::Joint::get_child(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	return joint_data->child ? Util::to_value(joint_data->child) : Qnil;
}

VALUE MSNewton::Joint::get_pin_matrix(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	if (joint_data->parent != nullptr && !Util::is_body_valid(joint_data->parent))
		joint_data->parent = nullptr;
	dMatrix pin_matrix;
	if (joint_data->parent != nullptr) {
		dMatrix parent_matrix;
		NewtonBodyGetMatrix(joint_data->parent, &parent_matrix[0][0]);
		pin_matrix = joint_data->pin_matrix * parent_matrix;
	}
	else
		pin_matrix = joint_data->pin_matrix;
	return Util::matrix_to_value(pin_matrix, world_data->inverse_scale);
}

VALUE MSNewton::Joint::set_pin_matrix(VALUE self, VALUE v_joint, VALUE v_pin_matrix) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	dMatrix pin_matrix = Util::value_to_matrix(v_pin_matrix, world_data->scale);
	Util::extract_matrix_scale(pin_matrix);
	if (joint_data->parent != nullptr && !Util::is_body_valid(joint_data->parent))
		joint_data->parent = nullptr;
	if (joint_data->parent	!= nullptr) {
		dMatrix parent_matrix;
		NewtonBodyGetMatrix(joint_data->parent, &parent_matrix[0][0]);
		joint_data->pin_matrix = pin_matrix * parent_matrix.Inverse();
	}
	else
		joint_data->pin_matrix = pin_matrix;
	if (joint_data->connected) {
		c_calculate_local_matrix(joint_data);
		on_pin_matrix_changed(joint_data);
	}
	return Util::matrix_to_value(pin_matrix, world_data->inverse_scale);
}

VALUE MSNewton::Joint::bodies_collidable(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	return Util::to_value(joint_data->bodies_collidable);
}

VALUE MSNewton::Joint::set_bodies_collidable(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	joint_data->bodies_collidable = Util::value_to_bool(v_state);
	if (joint_data->constraint != nullptr)
		NewtonJointSetCollisionState(joint_data->constraint, joint_data->bodies_collidable ? 1 : 0);
	on_collidable_changed(joint_data);
	return Util::to_value(joint_data->bodies_collidable);
}

VALUE MSNewton::Joint::get_stiffness(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	return Util::to_value(joint_data->stiffness);
}

VALUE MSNewton::Joint::set_stiffness(VALUE self, VALUE v_joint, VALUE v_stiffness) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	joint_data->stiffness = Util::clamp<dFloat>(Util::value_to_dFloat(v_stiffness), 0.0f, 1.0f);
	if (joint_data->constraint != nullptr)
		NewtonJointSetStiffness(joint_data->constraint, joint_data->stiffness);
	on_stiffness_changed(joint_data);
	return Util::to_value(joint_data->stiffness);
}

VALUE MSNewton::Joint::get_user_data(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	if (RARRAY_LEN(joint_data->user_data) == 0) return Qnil;
	return rb_ary_entry(joint_data->user_data, 0);
}

VALUE MSNewton::Joint::set_user_data(VALUE self, VALUE v_joint, VALUE v_user_data) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	if (v_user_data == Qnil)
		rb_ary_clear(joint_data->user_data);
	else
		rb_ary_store(joint_data->user_data, 0, v_user_data);
	return Qnil;
}

VALUE MSNewton::Joint::get_breaking_force(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(joint_data->breaking_force * world_data->inverse_scale3);
}

VALUE MSNewton::Joint::set_breaking_force(VALUE self, VALUE v_joint, VALUE v_force) {
	JointData* joint_data = Util::value_to_joint(v_joint);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	joint_data->breaking_force = Util::clamp_min(Util::value_to_dFloat(v_force), 0.0f) * world_data->scale3;
	return Util::to_value(joint_data->breaking_force * world_data->inverse_scale3);
}


VALUE MSNewton::Joint::skeleton_is_valid(VALUE self, VALUE v_skeleton) {
	return c_skeleton_is_valid((NewtonSkeletonContainer*)Util::value_to_ll(v_skeleton)) ? Qtrue : Qfalse;
}

VALUE MSNewton::Joint::skeleton_create(VALUE self, VALUE v_root_bone) {
	const NewtonBody* root_bone = Util::value_to_body(v_root_bone);
	const NewtonWorld* world = NewtonBodyGetWorld(root_bone);
	NewtonSkeletonContainer* skeleton = NewtonSkeletonContainerCreate((NewtonWorld*)world, (NewtonBody*)root_bone, skeleton_destructor);
	valid_skeletons[skeleton] = true;
	return c_skeleton_to_value(skeleton);
}

VALUE MSNewton::Joint::skeleton_destroy(VALUE self, VALUE v_skeleton) {
	NewtonSkeletonContainerDelete(c_value_to_skeleton(v_skeleton));
	return Qnil;
}

VALUE MSNewton::Joint::skeleton_attach_bone(VALUE self, VALUE v_skeleton, VALUE v_child_bone, VALUE v_parent_bone) {
	void* bone = NewtonSkeletonContainerAttachBone(c_value_to_skeleton(v_skeleton), (NewtonBody*)Util::value_to_body(v_child_bone), (NewtonBody*)Util::value_to_body(v_parent_bone));
	return rb_ll2inum((long long)bone);
}

VALUE MSNewton::Joint::skeleton_finalize(VALUE self, VALUE v_skeleton) {
	NewtonSkeletonContainerFinalize(c_value_to_skeleton(v_skeleton));
	return Qnil;
}

VALUE MSNewton::Joint::skeleton_get_solver(VALUE self, VALUE v_skeleton) {
	return Util::to_value(NewtonSkeletonGetSolverMode(c_value_to_skeleton(v_skeleton)));
}

VALUE MSNewton::Joint::skeleton_set_solver(VALUE self, VALUE v_skeleton, VALUE v_solver) {
	NewtonSkeletonContainer* skeleton = c_value_to_skeleton(v_skeleton);
	NewtonSkeletonSetSolverMode(skeleton, Util::value_to_int(v_solver));
	return Util::to_value(NewtonSkeletonGetSolverMode(skeleton));
}


void Init_msp_joint(VALUE mNewton) {
	VALUE mJoint = rb_define_module_under(mNewton, "Joint");

	rb_define_module_function(mJoint, "is_valid?", VALUEFUNC(MSNewton::Joint::is_valid), 1);
	rb_define_module_function(mJoint, "create", VALUEFUNC(MSNewton::Joint::create), 4);
	rb_define_module_function(mJoint, "destroy", VALUEFUNC(MSNewton::Joint::destroy), 1);
	rb_define_module_function(mJoint, "connect", VALUEFUNC(MSNewton::Joint::connect), 2);
	rb_define_module_function(mJoint, "disconnect", VALUEFUNC(MSNewton::Joint::disconnect), 1);
	rb_define_module_function(mJoint, "is_connected?", VALUEFUNC(MSNewton::Joint::is_connected), 1);
	rb_define_module_function(mJoint, "get_dof", VALUEFUNC(MSNewton::Joint::get_dof), 1);
	rb_define_module_function(mJoint, "get_type", VALUEFUNC(MSNewton::Joint::get_type), 1);
	rb_define_module_function(mJoint, "get_constraint_type", VALUEFUNC(MSNewton::Joint::get_constraint_type), 1);
	rb_define_module_function(mJoint, "set_constraint_type", VALUEFUNC(MSNewton::Joint::set_constraint_type), 2);
	rb_define_module_function(mJoint, "get_parent", VALUEFUNC(MSNewton::Joint::get_parent), 1);
	rb_define_module_function(mJoint, "get_child", VALUEFUNC(MSNewton::Joint::get_child), 1);
	rb_define_module_function(mJoint, "get_pin_matrix", VALUEFUNC(MSNewton::Joint::get_pin_matrix), 1);
	rb_define_module_function(mJoint, "set_pin_matrix", VALUEFUNC(MSNewton::Joint::set_pin_matrix), 2);
	rb_define_module_function(mJoint, "bodies_collidable?", VALUEFUNC(MSNewton::Joint::bodies_collidable), 1);
	rb_define_module_function(mJoint, "set_bodies_collidable", VALUEFUNC(MSNewton::Joint::set_bodies_collidable), 2);
	rb_define_module_function(mJoint, "get_stiffness", VALUEFUNC(MSNewton::Joint::get_stiffness), 1);
	rb_define_module_function(mJoint, "set_stiffness", VALUEFUNC(MSNewton::Joint::set_stiffness), 2);
	rb_define_module_function(mJoint, "get_user_data", VALUEFUNC(MSNewton::Joint::get_user_data), 1);
	rb_define_module_function(mJoint, "set_user_data", VALUEFUNC(MSNewton::Joint::set_user_data), 2);
	rb_define_module_function(mJoint, "get_breaking_force", VALUEFUNC(MSNewton::Joint::get_breaking_force), 1);
	rb_define_module_function(mJoint, "set_breaking_force", VALUEFUNC(MSNewton::Joint::set_breaking_force), 2);

	rb_define_module_function(mJoint, "skeleton_is_valid?", VALUEFUNC(MSNewton::Joint::skeleton_is_valid), 1);
	rb_define_module_function(mJoint, "skeleton_create", VALUEFUNC(MSNewton::Joint::skeleton_create), 1);
	rb_define_module_function(mJoint, "skeleton_destroy", VALUEFUNC(MSNewton::Joint::skeleton_destroy), 1);
	rb_define_module_function(mJoint, "skeleton_attach_bone", VALUEFUNC(MSNewton::Joint::skeleton_attach_bone), 3);
	rb_define_module_function(mJoint, "skeleton_finalize", VALUEFUNC(MSNewton::Joint::skeleton_finalize), 1);
	rb_define_module_function(mJoint, "skeleton_get_solver", VALUEFUNC(MSNewton::Joint::skeleton_get_solver), 1);
	rb_define_module_function(mJoint, "skeleton_set_solver", VALUEFUNC(MSNewton::Joint::skeleton_set_solver), 2);
}
