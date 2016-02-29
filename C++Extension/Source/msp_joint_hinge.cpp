#include "msp_joint_hinge.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::Hinge::DEFAULT_MIN = -180.0f * DEG_TO_RAD;
const dFloat MSNewton::Hinge::DEFAULT_MAX = 180.0f * DEG_TO_RAD;
const bool MSNewton::Hinge::DEFAULT_LIMITS_ENABLED = false;
const dFloat MSNewton::Hinge::DEFAULT_FRICTION = 0.0f;
const dFloat MSNewton::Hinge::DEFAULT_STIFF = 40.0f;
const dFloat MSNewton::Hinge::DEFAULT_DAMP = 10.0f;
const bool MSNewton::Hinge::DEFAULT_ROTATE_BACK_ENABLED = false;
const dFloat MSNewton::Hinge::DEFAULT_START_ANGLE = 0.0f;
const dFloat MSNewton::Hinge::DEFAULT_CONTROLLER = 1.0f;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Hinge::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;

	// Calculate position of pivot points and Jacobian direction vectors in global space.
	dMatrix matrix0;
	dMatrix matrix1;
	Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

	// Get linear velocity
	dVector veloc0(0.0f, 0.0f, 0.0f);
	dVector veloc1(0.0f, 0.0f, 0.0f);
	NewtonBodyGetVelocity(joint_data->child, &veloc0[0]);
	if (joint_data->parent != nullptr)
		NewtonBodyGetVelocity(joint_data->parent, &veloc1[0]);

	// Get angluar velocity
	dVector omega0(0.0f, 0.0f, 0.0f);
	dVector omega1(0.0f, 0.0f, 0.0f);
	NewtonBodyGetOmega(joint_data->child, &omega0[0]);
	if (joint_data->parent != nullptr)
		NewtonBodyGetOmega(joint_data->parent, &omega1[0]);

	// Calculate angle, omega, and acceleration.
	dFloat last_angle = cj_data->ai->get_angle();
	dFloat last_omega = cj_data->cur_omega;
	dFloat sin_angle;
	dFloat cos_angle;
	Joint::c_calculate_angle(matrix1.m_front, matrix0.m_front, matrix0.m_right, sin_angle, cos_angle);
	cj_data->ai->update(cos_angle, sin_angle);
	cj_data->cur_omega = (cj_data->ai->get_angle() - last_angle) / timestep;
	cj_data->cur_accel = (cj_data->cur_omega - last_omega) / timestep;
	dFloat cur_angle = cj_data->ai->get_angle() - cj_data->start_angle * cj_data->controller;

	// Restrict movement on the pivot point along all tree orthonormal directions.
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

	// Add two rows to restrict rotation around the the axis perpendicular to the rotation axis.
	NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle(matrix0.m_right, matrix1.m_right, matrix0.m_front), &matrix0.m_front[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle(matrix0.m_right, matrix1.m_right, matrix0.m_up), &matrix0.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	/*// Add two more rows for a more robust angular constraint.
	// Get a point along the pin axis at some reasonable large distance from the pivot.
	dVector q0(matrix0.m_posit + matrix0.m_right.Scale(MIN_JOINT_PIN_LENGTH));
	dVector q1(matrix1.m_posit + matrix1.m_right.Scale(MIN_JOINT_PIN_LENGTH));

	// Add two constraints row perpendicular to the pin vector.
	dVector q2(q0 + matrix0.m_front.Scale((q1 - q0) % matrix0.m_front));
	NewtonUserJointAddLinearRow(joint, &q0[0], &q2[0], &matrix0.m_front[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	q2 = q0 + matrix0.m_up.Scale((q1 - q0) % matrix0.m_up);
	NewtonUserJointAddLinearRow(joint, &q0[0], &q2[0], &matrix0.m_up[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);*/

	// Check if need to re-enable limits.
	if (cj_data->temp_disable_limits == true && cur_angle >= cj_data->min && cur_angle <= cj_data->max)
		cj_data->temp_disable_limits = false;

	// Add limits and friction.
	if (cj_data->limits_enabled == true && (cur_angle - cj_data->min) < -Joint::ANGULAR_LIMIT_EPSILON && cj_data->temp_disable_limits == false) {
		NewtonUserJointAddAngularRow(joint, cj_data->min - cur_angle, &matrix0.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->limits_enabled == true && (cur_angle - cj_data->max) > Joint::ANGULAR_LIMIT_EPSILON && cj_data->temp_disable_limits == false) {
		NewtonUserJointAddAngularRow(joint, cj_data->max - cur_angle, &matrix0.m_right[0]);
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->rotate_back_enabled) {
		NewtonUserJointAddAngularRow(joint, -cur_angle, &matrix0.m_right[0]);
		NewtonUserJointSetRowSpringDamperAcceleration(joint, cj_data->stiff, cj_data->damp);
		/*dFloat rel_omega = (omega0 - omega1) % matrix0.m_right;
		dFloat rel_accel = -cur_angle / timestep * cj_data->stiff - cj_data->damp * rel_omega;
		NewtonUserJointSetRowAcceleration(joint, rel_accel);*/
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness * Joint::STIFFNESS_RATIO);
	}
	else {
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix0.m_right[0]);
		dFloat power = cj_data->friction * dAbs(cj_data->controller) * DEG_TO_RAD;
		BodyData* cbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
		if (cbody_data->bstatic == false && cbody_data->mass >= MIN_MASS)
			power *= cbody_data->mass;
		else {
			BodyData* pbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
			if (pbody_data->bstatic == false && pbody_data->mass >= MIN_MASS) power *= pbody_data->mass;
		}
		NewtonUserJointSetRowMinimumFriction(joint, -power);
		NewtonUserJointSetRowMaximumFriction(joint, power);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
}

void MSNewton::Hinge::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;

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

	if (cj_data->limits_enabled) {
		info->m_minAngularDof[2] = (cj_data->min - cj_data->ai->get_angle()) * RAD_TO_DEG;
		info->m_maxAngularDof[2] = (cj_data->max - cj_data->ai->get_angle()) * RAD_TO_DEG;
	}
	else {
		info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
		info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
	}
}

void MSNewton::Hinge::on_destroy(JointData* joint_data) {
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	delete cj_data->ai;
	delete cj_data;
}

void MSNewton::Hinge::on_connect(JointData* joint_data) {
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->desired_start_angle = cj_data->start_angle * cj_data->controller;
	cj_data->temp_disable_limits = true;
}

void MSNewton::Hinge::on_disconnect(JointData* joint_data) {
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->ai->set_angle(0.0f);
	cj_data->cur_omega = 0.0f;
	cj_data->cur_accel = 0.0f;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::Hinge::is_valid(VALUE self, VALUE v_joint) {
	JointData* address = (JointData*)Util::value_to_ll(v_joint);
	bool valid = Util::is_joint_valid(address);
	if (valid && address->jtype != JT_HINGE) valid = false;
	return Util::to_value(valid);
}

VALUE MSNewton::Hinge::create(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_NONE);

	HingeData* cj_data = new HingeData;
	cj_data->ai = new AngularIntegration();
	cj_data->cur_omega = 0.0f;
	cj_data->cur_accel = 0.0f;
	cj_data->min = DEFAULT_MIN;
	cj_data->max = DEFAULT_MAX;
	cj_data->limits_enabled = DEFAULT_LIMITS_ENABLED;
	cj_data->friction = DEFAULT_FRICTION;
	cj_data->stiff = DEFAULT_STIFF;
	cj_data->damp = DEFAULT_DAMP;
	cj_data->rotate_back_enabled = DEFAULT_ROTATE_BACK_ENABLED;
	cj_data->start_angle = DEFAULT_START_ANGLE;
	cj_data->controller = DEFAULT_CONTROLLER;
	cj_data->desired_start_angle = cj_data->start_angle * cj_data->controller;
	cj_data->temp_disable_limits = true;

	joint_data->dof = 6;
	joint_data->jtype = JT_HINGE;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_connect = on_connect;
	joint_data->on_disconnect = on_disconnect;

	return Util::to_value(joint_data);
}

VALUE MSNewton::Hinge::get_cur_angle(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->ai->get_angle() - cj_data->start_angle * cj_data->controller);
}

VALUE MSNewton::Hinge::get_cur_omega(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->cur_omega);
}

VALUE MSNewton::Hinge::get_cur_acceleration(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->cur_accel);
}

VALUE MSNewton::Hinge::get_min(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->min);
}

VALUE MSNewton::Hinge::set_min(VALUE self, VALUE v_joint, VALUE v_min) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->min = Util::value_to_dFloat(v_min);
	return Util::to_value(cj_data->min);
}

VALUE MSNewton::Hinge::get_max(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->max);
}

VALUE MSNewton::Hinge::set_max(VALUE self, VALUE v_joint, VALUE v_max) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->max = Util::value_to_dFloat(v_max);
	return Util::to_value(cj_data->max);
}

VALUE MSNewton::Hinge::enable_limits(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->limits_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->limits_enabled);
}

VALUE MSNewton::Hinge::are_limits_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->limits_enabled);
}

VALUE MSNewton::Hinge::get_friction(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->friction);
}

VALUE MSNewton::Hinge::set_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->friction = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_friction), 0.0f);
	return Util::to_value(cj_data->friction);
}

VALUE MSNewton::Hinge::get_stiff(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->stiff);
}

VALUE MSNewton::Hinge::set_stiff(VALUE self, VALUE v_joint, VALUE v_stiff) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->stiff = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_stiff), 0.0f);
	return Util::to_value(cj_data->stiff);
}

VALUE MSNewton::Hinge::get_damp(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->damp);
}

VALUE MSNewton::Hinge::set_damp(VALUE self, VALUE v_joint, VALUE v_damp) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->damp = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_damp), 0.0f);
	return Util::to_value(cj_data->damp);
}

VALUE MSNewton::Hinge::enable_rotate_back(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->rotate_back_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->rotate_back_enabled);
}

VALUE MSNewton::Hinge::is_rotate_back_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->rotate_back_enabled);
}

VALUE MSNewton::Hinge::get_start_angle(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->start_angle);
}

VALUE MSNewton::Hinge::set_start_angle(VALUE self, VALUE v_joint, VALUE v_angle) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->start_angle = Util::value_to_dFloat(v_angle);
	dFloat desired_start_angle = cj_data->start_angle * cj_data->controller;
	if (cj_data->desired_start_angle != desired_start_angle) {
		cj_data->temp_disable_limits = true;
		cj_data->desired_start_angle = desired_start_angle;
		if (joint_data->connected)
			NewtonBodySetSleepState(joint_data->child, 0);
	}
	return Util::to_value(cj_data->start_angle);
}

VALUE MSNewton::Hinge::get_controller(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->controller);
}

VALUE MSNewton::Hinge::set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->controller = Util::value_to_dFloat(v_controller);
	dFloat desired_start_angle = cj_data->start_angle * cj_data->controller;
	if (cj_data->desired_start_angle != desired_start_angle) {
		cj_data->temp_disable_limits = true;
		cj_data->desired_start_angle = desired_start_angle;
		if (joint_data->connected)
			NewtonBodySetSleepState(joint_data->child, 0);
	}
	return Util::to_value(cj_data->controller);
}


void Init_msp_hinge(VALUE mNewton) {
	VALUE mHinge = rb_define_module_under(mNewton, "Hinge");

	rb_define_module_function(mHinge, "is_valid?", VALUEFUNC(MSNewton::Hinge::is_valid), 1);
	rb_define_module_function(mHinge, "create", VALUEFUNC(MSNewton::Hinge::create), 1);
	rb_define_module_function(mHinge, "get_cur_angle", VALUEFUNC(MSNewton::Hinge::get_cur_angle), 1);
	rb_define_module_function(mHinge, "get_cur_omega", VALUEFUNC(MSNewton::Hinge::get_cur_omega), 1);
	rb_define_module_function(mHinge, "get_cur_acceleration", VALUEFUNC(MSNewton::Hinge::get_cur_acceleration), 1);
	rb_define_module_function(mHinge, "get_min", VALUEFUNC(MSNewton::Hinge::get_min), 1);
	rb_define_module_function(mHinge, "set_min", VALUEFUNC(MSNewton::Hinge::set_min), 2);
	rb_define_module_function(mHinge, "get_max", VALUEFUNC(MSNewton::Hinge::get_max), 1);
	rb_define_module_function(mHinge, "set_max", VALUEFUNC(MSNewton::Hinge::set_max), 2);
	rb_define_module_function(mHinge, "enable_limits", VALUEFUNC(MSNewton::Hinge::enable_limits), 2);
	rb_define_module_function(mHinge, "are_limits_enabled?", VALUEFUNC(MSNewton::Hinge::are_limits_enabled), 1);
	rb_define_module_function(mHinge, "get_friction", VALUEFUNC(MSNewton::Hinge::get_friction), 1);
	rb_define_module_function(mHinge, "set_friction", VALUEFUNC(MSNewton::Hinge::set_friction), 2);
	rb_define_module_function(mHinge, "get_stiff", VALUEFUNC(MSNewton::Hinge::get_stiff), 1);
	rb_define_module_function(mHinge, "set_stiff", VALUEFUNC(MSNewton::Hinge::set_stiff), 2);
	rb_define_module_function(mHinge, "get_damp", VALUEFUNC(MSNewton::Hinge::get_damp), 1);
	rb_define_module_function(mHinge, "set_damp", VALUEFUNC(MSNewton::Hinge::set_damp), 2);
	rb_define_module_function(mHinge, "enable_rotate_back", VALUEFUNC(MSNewton::Hinge::enable_rotate_back), 2);
	rb_define_module_function(mHinge, "is_rotate_back_enabled?", VALUEFUNC(MSNewton::Hinge::is_rotate_back_enabled), 1);
	rb_define_module_function(mHinge, "get_start_angle", VALUEFUNC(MSNewton::Hinge::get_start_angle), 1);
	rb_define_module_function(mHinge, "set_start_angle", VALUEFUNC(MSNewton::Hinge::set_start_angle), 2);
	rb_define_module_function(mHinge, "get_controller", VALUEFUNC(MSNewton::Hinge::get_controller), 1);
	rb_define_module_function(mHinge, "set_controller", VALUEFUNC(MSNewton::Hinge::set_controller), 2);
}
