#include "msp_joint_universal.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
	Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::Universal::DEFAULT_MIN = -180.0f * DEG_TO_RAD;
const dFloat MSNewton::Universal::DEFAULT_MAX = 180.0f * DEG_TO_RAD;
const bool MSNewton::Universal::DEFAULT_LIMITS_ENABLED = false;
const dFloat MSNewton::Universal::DEFAULT_FRICTION = 0.0f;
const dFloat MSNewton::Universal::DEFAULT_CONTROLLER = 1.0f;


/*
 ///////////////////////////////////////////////////////////////////////////////
	Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Universal::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;

	// Calculate position of pivot points and Jacobian direction vectors in global space.
	dMatrix matrix0, matrix1, matrix2;
	MSNewton::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1, matrix2);

	// Restrict movement on the pivot point along all three orthonormal directions
	NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix0.m_front[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix0.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix0.m_right[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Construct an orthogonal coordinate system with these two vectors
	dMatrix matrix1_1;
	matrix1_1.m_front = matrix1.m_front;
	matrix1_1.m_up = matrix0.m_right * matrix1.m_front;
	matrix1_1.m_up = matrix1_1.m_up.Scale(1.0f / dSqrt(matrix1_1.m_up % matrix1_1.m_up));
	matrix1_1.m_right = matrix1_1.m_front * matrix1_1.m_up;

	// Override the normal right side  because the joint is too week due to centripetal accelerations
	dVector omega0(0.0f, 0.0f, 0.0f);
	dVector omega1(0.0f, 0.0f, 0.0f);
	NewtonBodyGetOmega(joint_data->child, &omega0[0]);
	if (joint_data->parent != nullptr)
		NewtonBodyGetOmega(joint_data->parent, &omega1[0]);
	dVector rel_omega(omega0 - omega1);

	dFloat angle = -MSNewton::Joint::c_calculate_angle(matrix0.m_right, matrix1_1.m_right, matrix1_1.m_up);
	dFloat omega = (rel_omega % matrix1_1.m_up);
	dFloat alpha_error = -(angle + omega * timestep) / (timestep * timestep);

	NewtonUserJointAddAngularRow(joint, -angle, &matrix1_1.m_up[0]);
	NewtonUserJointSetRowAcceleration(joint, alpha_error);
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	dFloat sin_angle1;
	dFloat cos_angle1;
	MSNewton::Joint::c_calculate_angle(matrix1_1.m_front, matrix0.m_front, matrix1_1.m_right, sin_angle1, cos_angle1);
	dFloat cur_angle1 = -cj_data->ai1->update(cos_angle1, sin_angle1);
	dFloat last_omega1 = cj_data->cur_omega1;
	cj_data->cur_omega1 = rel_omega % matrix0.m_right;
	cj_data->cur_accel1 = (cj_data->cur_omega1 - last_omega1) / timestep;

	dFloat sin_angle2;
	dFloat cos_angle2;
	MSNewton::Joint::c_calculate_angle(matrix1.m_right, matrix1_1.m_right, matrix1_1.m_front, sin_angle2, cos_angle2);
	dFloat cur_angle2 = -cj_data->ai1->update(cos_angle2, sin_angle2);
	dFloat last_omega2 = cj_data->cur_omega2;
	cj_data->cur_omega2 = rel_omega % matrix1.m_front;
	cj_data->cur_accel2 = (cj_data->cur_omega2 - last_omega2) / timestep;

	if (cj_data->limits1_enabled == true && cur_angle1 < cj_data->min1 - Joint::ANGULAR_LIMIT_EPSILON) {
		dFloat rel_angle = cur_angle1 - cj_data->min1;
		// Tell joint error will minimize the exceeded angle error
		NewtonUserJointAddAngularRow(joint, rel_angle, &matrix0.m_right[0]);
		// Allow the joint to move back freely
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->limits1_enabled == true && cur_angle1 > cj_data->max1 + Joint::ANGULAR_LIMIT_EPSILON) {
		dFloat rel_angle = cur_angle1 - cj_data->max1;
		// Tell joint error will minimize the exceeded angle error
		NewtonUserJointAddAngularRow(joint, rel_angle, &matrix0.m_right[0]);
		// Allow the joint to move back freely
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix0.m_right[0]);
		dFloat power = cj_data->friction * dAbs(cj_data->controller);
		/*BodyData* cbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
		if (cbody_data->bstatic == false && cbody_data->mass >= MIN_MASS)
			power *= cbody_data->mass;
		else {
			BodyData* pbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
			if (pbody_data->bstatic == false && pbody_data->mass >= MIN_MASS) power *= pbody_data->mass;
		}*/
		NewtonUserJointSetRowMinimumFriction(joint, -power);
		NewtonUserJointSetRowMaximumFriction(joint, power);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}

	if (cj_data->limits2_enabled == true && cur_angle2 < cj_data->min2 - Joint::ANGULAR_LIMIT_EPSILON) {
		dFloat rel_angle = cur_angle2 - cj_data->min2;
		// Tell joint error will minimize the exceeded angle error
		NewtonUserJointAddAngularRow(joint, rel_angle, &matrix0.m_front[0]);
		// Allow the joint to move back freely
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->limits2_enabled == true && cur_angle2 > cj_data->max2 + Joint::ANGULAR_LIMIT_EPSILON) {
		dFloat rel_angle = cur_angle2 - cj_data->max2;
		// Tell joint error will minimize the exceeded angle error
		NewtonUserJointAddAngularRow(joint, rel_angle, &matrix0.m_front[0]);
		// Allow the joint to move back freely
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix0.m_front[0]);
		dFloat power = cj_data->friction * dAbs(cj_data->controller);
		/*BodyData* cbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
		if (cbody_data->bstatic == false && cbody_data->mass >= MIN_MASS)
			power *= cbody_data->mass;
		else {
			BodyData* pbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
			if (pbody_data->bstatic == false && pbody_data->mass >= MIN_MASS) power *= pbody_data->mass;
		}*/
		NewtonUserJointSetRowMinimumFriction(joint, -power);
		NewtonUserJointSetRowMaximumFriction(joint, power);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
}

void MSNewton::Universal::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;

	info->m_minLinearDof[0] = -0.0f;
	info->m_maxLinearDof[0] = 0.0f;
	info->m_minLinearDof[1] = -0.0f;
	info->m_maxLinearDof[1] = 0.0f;
	info->m_minLinearDof[2] = -0.0f;
	info->m_maxLinearDof[2] = 0.0f;

	if (cj_data->limits2_enabled) {
		info->m_minAngularDof[0] = (cj_data->min2 - cj_data->ai2->get_angle()) * RAD_TO_DEG;
		info->m_maxAngularDof[0] = (cj_data->max2 - cj_data->ai2->get_angle()) * RAD_TO_DEG;
	}
	else {
		info->m_minAngularDof[0] = -Joint::CUSTOM_LARGE_VALUE;
		info->m_maxAngularDof[0] = Joint::CUSTOM_LARGE_VALUE;
	}

	info->m_minAngularDof[1] = -0.0f;
	info->m_maxAngularDof[1] = 0.0f;

	if (cj_data->limits1_enabled) {
		info->m_minAngularDof[2] = (cj_data->min1 - cj_data->ai1->get_angle()) * RAD_TO_DEG;
		info->m_maxAngularDof[2] = (cj_data->max1 - cj_data->ai1->get_angle()) * RAD_TO_DEG;
	}
	else {
		info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
		info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
	}
}

void MSNewton::Universal::on_destroy(JointData* joint_data) {
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	delete cj_data;
}

void MSNewton::Universal::on_disconnect(JointData* joint_data) {
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	cj_data->ai1->set_angle(0.0f);
	cj_data->cur_omega1 = 0.0f;
	cj_data->cur_accel1 = 0.0f;
	cj_data->ai2->set_angle(0.0f);
	cj_data->cur_omega2 = 0.0f;
	cj_data->cur_accel2 = 0.0f;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
	Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::Universal::is_valid(VALUE self, VALUE v_joint) {
	JointData* address = (JointData*)Util::value_to_ll(v_joint);
	bool valid = Util::is_joint_valid(address);
	if (valid && address->jtype != JT_UNIVERSAL) valid = false;
	return Util::to_value(valid);
}

VALUE MSNewton::Universal::create(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_NONE);

	UniversalData* cj_data = new UniversalData;
	cj_data->ai1 = new AngularIntegration();
	cj_data->cur_omega1 = 0.0f;
	cj_data->cur_accel1 = 0.0f;
	cj_data->min1 = DEFAULT_MIN;
	cj_data->max1 = DEFAULT_MAX;
	cj_data->limits1_enabled = DEFAULT_LIMITS_ENABLED;
	cj_data->ai2 = new AngularIntegration();
	cj_data->cur_omega2 = 0.0f;
	cj_data->cur_accel2 = 0.0f;
	cj_data->min2 = DEFAULT_MIN;
	cj_data->max2 = DEFAULT_MAX;
	cj_data->limits2_enabled = DEFAULT_LIMITS_ENABLED;
	cj_data->friction = DEFAULT_FRICTION;
	cj_data->controller = DEFAULT_CONTROLLER;

	joint_data->dof = 6;
	joint_data->jtype = JT_UNIVERSAL;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_disconnect = on_disconnect;

	return Util::to_value(joint_data);
}

VALUE MSNewton::Universal::get_cur_angle1(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->ai1->get_angle());
}

VALUE MSNewton::Universal::get_cur_omega1(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->cur_omega1);
}

VALUE MSNewton::Universal::get_cur_acceleration1(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->cur_accel1);
}

VALUE MSNewton::Universal::get_min1(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->min1);
}

VALUE MSNewton::Universal::set_min1(VALUE self, VALUE v_joint, VALUE v_min) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	cj_data->min1 = Util::value_to_dFloat(v_min);
	return Util::to_value(cj_data->min1);
}

VALUE MSNewton::Universal::get_max1(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->max1);
}

VALUE MSNewton::Universal::set_max1(VALUE self, VALUE v_joint, VALUE v_max) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	cj_data->max1 = Util::value_to_dFloat(v_max);
	return Util::to_value(cj_data->max1);
}

VALUE MSNewton::Universal::enable_limits1(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	cj_data->limits1_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->limits1_enabled);
}

VALUE MSNewton::Universal::limits1_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->limits1_enabled);
}


VALUE MSNewton::Universal::get_cur_angle2(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->ai2->get_angle());
}

VALUE MSNewton::Universal::get_cur_omega2(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->cur_omega2);
}

VALUE MSNewton::Universal::get_cur_acceleration2(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->cur_accel2);
}

VALUE MSNewton::Universal::get_min2(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->min2);
}

VALUE MSNewton::Universal::set_min2(VALUE self, VALUE v_joint, VALUE v_min) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	cj_data->min2 = Util::value_to_dFloat(v_min);
	return Util::to_value(cj_data->min2);
}

VALUE MSNewton::Universal::get_max2(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->max2);
}

VALUE MSNewton::Universal::set_max2(VALUE self, VALUE v_joint, VALUE v_max) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	cj_data->max2 = Util::value_to_dFloat(v_max);
	return Util::to_value(cj_data->max2);
}

VALUE MSNewton::Universal::enable_limits2(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	cj_data->limits2_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->limits2_enabled);
}

VALUE MSNewton::Universal::limits2_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->limits2_enabled);
}


VALUE MSNewton::Universal::get_friction(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->friction * world_data->inverse_scale5);
}

VALUE MSNewton::Universal::set_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->friction = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_friction), 0.0f) * world_data->scale5;
	return Util::to_value(cj_data->friction * world_data->inverse_scale5);
}

VALUE MSNewton::Universal::get_controller(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	return Util::to_value(cj_data->controller);
}

VALUE MSNewton::Universal::set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UNIVERSAL);
	UniversalData* cj_data = (UniversalData*)joint_data->cj_data;
	dFloat desired_controller = Util::value_to_dFloat(v_controller);
	if (cj_data->controller != desired_controller) {
		cj_data->controller = desired_controller;
		if (joint_data->connected)
			NewtonBodySetSleepState(joint_data->child, 0);
	}
	return Util::to_value(cj_data->controller);
}


void Init_msp_universal(VALUE mNewton) {
	VALUE mUniversal = rb_define_module_under(mNewton, "Universal");

	rb_define_module_function(mUniversal, "is_valid?", VALUEFUNC(MSNewton::Universal::is_valid), 1);
	rb_define_module_function(mUniversal, "create", VALUEFUNC(MSNewton::Universal::create), 1);
	rb_define_module_function(mUniversal, "get_cur_angle1", VALUEFUNC(MSNewton::Universal::get_cur_angle1), 1);
	rb_define_module_function(mUniversal, "get_cur_omega1", VALUEFUNC(MSNewton::Universal::get_cur_omega1), 1);
	rb_define_module_function(mUniversal, "get_cur_acceleration1", VALUEFUNC(MSNewton::Universal::get_cur_acceleration1), 1);
	rb_define_module_function(mUniversal, "get_min1", VALUEFUNC(MSNewton::Universal::get_min1), 1);
	rb_define_module_function(mUniversal, "set_min1", VALUEFUNC(MSNewton::Universal::set_min1), 2);
	rb_define_module_function(mUniversal, "get_max1", VALUEFUNC(MSNewton::Universal::get_max1), 1);
	rb_define_module_function(mUniversal, "set_max1", VALUEFUNC(MSNewton::Universal::set_max1), 2);
	rb_define_module_function(mUniversal, "enable_limits1", VALUEFUNC(MSNewton::Universal::enable_limits1), 2);
	rb_define_module_function(mUniversal, "limits1_enabled?", VALUEFUNC(MSNewton::Universal::limits1_enabled), 1);

	rb_define_module_function(mUniversal, "get_cur_angle2", VALUEFUNC(MSNewton::Universal::get_cur_angle2), 1);
	rb_define_module_function(mUniversal, "get_cur_omega2", VALUEFUNC(MSNewton::Universal::get_cur_omega2), 1);
	rb_define_module_function(mUniversal, "get_cur_acceleration2", VALUEFUNC(MSNewton::Universal::get_cur_acceleration2), 1);
	rb_define_module_function(mUniversal, "get_min2", VALUEFUNC(MSNewton::Universal::get_min2), 1);
	rb_define_module_function(mUniversal, "set_min2", VALUEFUNC(MSNewton::Universal::set_min2), 2);
	rb_define_module_function(mUniversal, "get_max2", VALUEFUNC(MSNewton::Universal::get_max2), 1);
	rb_define_module_function(mUniversal, "set_max2", VALUEFUNC(MSNewton::Universal::set_max2), 2);
	rb_define_module_function(mUniversal, "enable_limits2", VALUEFUNC(MSNewton::Universal::enable_limits2), 2);
	rb_define_module_function(mUniversal, "limits2_enabled?", VALUEFUNC(MSNewton::Universal::limits2_enabled), 1);

	rb_define_module_function(mUniversal, "get_friction", VALUEFUNC(MSNewton::Universal::get_friction), 1);
	rb_define_module_function(mUniversal, "set_friction", VALUEFUNC(MSNewton::Universal::set_friction), 2);
	rb_define_module_function(mUniversal, "get_controller", VALUEFUNC(MSNewton::Universal::get_controller), 1);
	rb_define_module_function(mUniversal, "set_controller", VALUEFUNC(MSNewton::Universal::set_controller), 2);
}
