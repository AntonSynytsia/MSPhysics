#include "msp_joint_hinge.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::Hinge::DEFAULT_MIN = -180.0f * DEG_TO_RAD;
const dFloat MSNewton::Hinge::DEFAULT_MAX = 180.0f * DEG_TO_RAD;
const bool MSNewton::Hinge::DEFAULT_LIMITS_ENABLED = false;
const bool MSNewton::Hinge::DEFAULT_STRONG_MODE_ENABLED = true;
const dFloat MSNewton::Hinge::DEFAULT_FRICTION = 0.0f;
const dFloat MSNewton::Hinge::DEFAULT_ACCEL = 40.0f;
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
	dMatrix matrix0, matrix1, matrix2;
	MSNewton::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1, matrix2);

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
	/*NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle(matrix0.m_right, matrix1.m_right, matrix0.m_front), &matrix0.m_front[0]);
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
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);*/

	// Add two more rows for a more robust angular constraint.
	// Get a point along the pin axis at some reasonable large distance from the pivot.
	dVector q0(matrix0.m_posit + matrix0.m_right.Scale(MIN_JOINT_PIN_LENGTH));
	dVector q1(matrix1.m_posit + matrix1.m_right.Scale(MIN_JOINT_PIN_LENGTH));

	// Add two constraints row perpendicular to the pin vector.
	dVector q2(q0 + matrix0.m_front.Scale((q1 - q0) % matrix0.m_front));
	NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix1.m_front[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	q2 = q0 + matrix0.m_up.Scale((q1 - q0) % matrix0.m_up);
	NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix1.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Check if need to re-enable limits.
	if (cj_data->temp_disable_limits == true && cur_angle >= cj_data->min && cur_angle <= cj_data->max)
		cj_data->temp_disable_limits = false;

	// Add limits and friction.
	if (cj_data->limits_enabled == true && cur_angle < cj_data->min - Joint::ANGULAR_LIMIT_EPSILON && cj_data->temp_disable_limits == false) {
		NewtonUserJointAddAngularRow(joint, cj_data->min - cur_angle, &matrix0.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->limits_enabled == true && cur_angle > cj_data->max + Joint::ANGULAR_LIMIT_EPSILON && cj_data->temp_disable_limits == false) {
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
		if (cj_data->strong_mode_enabled) {
			dFloat accel = NewtonCalculateSpringDamperAcceleration(timestep, cj_data->accel, cur_angle, cj_data->damp, cj_data->cur_omega);
			NewtonUserJointSetRowAcceleration(joint, accel);
			NewtonUserJointSetRowStiffness(joint, -joint_data->stiffness);
		}
		else {
			NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
			NewtonUserJointSetRowSpringDamperAcceleration(joint, cj_data->accel, cj_data->damp);
		}
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
}

void MSNewton::Hinge::sc_submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	//HingeData* cj_data = (HingeData*)joint_data->cj_data;

	dMatrix matrix0, matrix1, matrix2, matrix3;
	MSNewton::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1, matrix2);

	dVector com1, com2;
	NewtonBodyGetMatrix(joint_data->child, &matrix3[0][0]);
	NewtonBodyGetCentreOfMass(joint_data->child, &com1[0]);
	com1 = matrix3.TransformVector(com1); // Actual centre of mass in global space
	com2 = matrix0.UntransformVector(com1); // Expected centre of mass in local space.

	Util::rotate_matrix_to_dir(matrix0, matrix1.m_right, matrix3);
	matrix3.m_posit = matrix1.m_posit;
	com2 = matrix3.TransformVector(com2); // Expected centre of mass in global space.

	NewtonUserJointAddLinearRow(joint, &com1[0], &com2[0], &matrix1.m_right[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
}

void MSNewton::Hinge::sc_get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
	//JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	//HingeData* cj_data = (HingeData*)joint_data->cj_data;
}

void  MSNewton::Hinge::sc_destructor(const NewtonJoint* joint) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->sc = nullptr;
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

	/*cj_data->sc = NewtonConstraintCreateUserJoint(joint_data->world, 6, sc_submit_constraints, sc_get_info, joint_data->child, joint_data->parent);
	NewtonJointSetCollisionState(cj_data->sc, joint_data->bodies_collidable ? 1 : 0);
	NewtonJointSetStiffness(cj_data->sc, joint_data->stiffness);
	NewtonJointSetUserData(cj_data->sc, joint_data);
	NewtonJointSetDestructor(cj_data->sc, sc_destructor);*/
}

void MSNewton::Hinge::on_disconnect(JointData* joint_data) {
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->ai->set_angle(0.0f);
	cj_data->cur_omega = 0.0f;
	cj_data->cur_accel = 0.0f;
	if (cj_data->sc != nullptr)
		NewtonDestroyJoint(joint_data->world, cj_data->sc);
}

void MSNewton::Hinge::adjust_pin_matrix_proc(JointData* joint_data, dMatrix& pin_matrix) {
	dMatrix matrix;
	dVector centre;
	NewtonBodyGetMatrix(joint_data->child, &matrix[0][0]);
	NewtonBodyGetCentreOfMass(joint_data->child, &centre[0]);
	centre = matrix.TransformVector(centre);
	centre = pin_matrix.UntransformVector(centre);
	dVector point(0.0f, 0.0f, centre.m_z);
	pin_matrix.m_posit = pin_matrix.TransformVector(point);
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
	cj_data->strong_mode_enabled = DEFAULT_STRONG_MODE_ENABLED;
	cj_data->friction = DEFAULT_FRICTION;
	cj_data->accel = DEFAULT_ACCEL;
	cj_data->damp = DEFAULT_DAMP;
	cj_data->rotate_back_enabled = DEFAULT_ROTATE_BACK_ENABLED;
	cj_data->start_angle = DEFAULT_START_ANGLE;
	cj_data->controller = DEFAULT_CONTROLLER;
	cj_data->desired_start_angle = cj_data->start_angle * cj_data->controller;
	cj_data->temp_disable_limits = true;
	cj_data->sc = nullptr;

	joint_data->dof = 6;
	joint_data->jtype = JT_HINGE;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_connect = on_connect;
	joint_data->on_disconnect = on_disconnect;
	//~ joint_data->adjust_pin_matrix_proc = adjust_pin_matrix_proc;

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

VALUE MSNewton::Hinge::limits_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->limits_enabled);
}

VALUE MSNewton::Hinge::enable_strong_mode(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->strong_mode_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->strong_mode_enabled);
}

VALUE MSNewton::Hinge::strong_mode_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->strong_mode_enabled);
}

VALUE MSNewton::Hinge::get_friction(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->friction * world_data->inverse_scale5);
}

VALUE MSNewton::Hinge::set_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->friction = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_friction), 0.0f) * world_data->scale5;
	return Util::to_value(cj_data->friction * world_data->inverse_scale5);
}

VALUE MSNewton::Hinge::get_accel(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	return Util::to_value(cj_data->accel);
}

VALUE MSNewton::Hinge::set_accel(VALUE self, VALUE v_joint, VALUE v_accel) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_HINGE);
	HingeData* cj_data = (HingeData*)joint_data->cj_data;
	cj_data->accel = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_accel), 0.0f);
	return Util::to_value(cj_data->accel);
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

VALUE MSNewton::Hinge::rotate_back_enabled(VALUE self, VALUE v_joint) {
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
	rb_define_module_function(mHinge, "limits_enabled?", VALUEFUNC(MSNewton::Hinge::limits_enabled), 1);
	rb_define_module_function(mHinge, "enable_strong_mode", VALUEFUNC(MSNewton::Hinge::enable_strong_mode), 2);
	rb_define_module_function(mHinge, "strong_mode_enabled?", VALUEFUNC(MSNewton::Hinge::strong_mode_enabled), 1);
	rb_define_module_function(mHinge, "get_friction", VALUEFUNC(MSNewton::Hinge::get_friction), 1);
	rb_define_module_function(mHinge, "set_friction", VALUEFUNC(MSNewton::Hinge::set_friction), 2);
	rb_define_module_function(mHinge, "get_accel", VALUEFUNC(MSNewton::Hinge::get_accel), 1);
	rb_define_module_function(mHinge, "set_accel", VALUEFUNC(MSNewton::Hinge::set_accel), 2);
	rb_define_module_function(mHinge, "get_damp", VALUEFUNC(MSNewton::Hinge::get_damp), 1);
	rb_define_module_function(mHinge, "set_damp", VALUEFUNC(MSNewton::Hinge::set_damp), 2);
	rb_define_module_function(mHinge, "enable_rotate_back", VALUEFUNC(MSNewton::Hinge::enable_rotate_back), 2);
	rb_define_module_function(mHinge, "rotate_back_enabled?", VALUEFUNC(MSNewton::Hinge::rotate_back_enabled), 1);
	rb_define_module_function(mHinge, "get_start_angle", VALUEFUNC(MSNewton::Hinge::get_start_angle), 1);
	rb_define_module_function(mHinge, "set_start_angle", VALUEFUNC(MSNewton::Hinge::set_start_angle), 2);
	rb_define_module_function(mHinge, "get_controller", VALUEFUNC(MSNewton::Hinge::get_controller), 1);
	rb_define_module_function(mHinge, "set_controller", VALUEFUNC(MSNewton::Hinge::set_controller), 2);
}
