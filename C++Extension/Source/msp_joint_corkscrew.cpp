#include "msp_joint_corkscrew.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::Corkscrew::DEFAULT_MIN_POS = -10.0f;
const dFloat MSNewton::Corkscrew::DEFAULT_MAX_POS = 10.0f;
const bool MSNewton::Corkscrew::DEFAULT_LIN_LIMITS_ENABLED = false;
const dFloat MSNewton::Corkscrew::DEFAULT_LIN_FRICTION = 0.0f;
const dFloat MSNewton::Corkscrew::DEFAULT_MIN_ANG = -180.0f * DEG_TO_RAD;
const dFloat MSNewton::Corkscrew::DEFAULT_MAX_ANG = 180.0f * DEG_TO_RAD;
const bool MSNewton::Corkscrew::DEFAULT_ANG_LIMITS_ENABLED = false;
const dFloat MSNewton::Corkscrew::DEFAULT_ANG_FRICTION = 0.0f;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Corkscrew::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;

	// Calculate position of pivot points and Jacobian direction vectors in global space.
	dMatrix matrix0;
	dMatrix matrix1;
	Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

	const dVector& pos0 = matrix0.m_posit;
	dVector pos1(matrix1.m_posit + matrix1.m_right.Scale((pos0 - matrix1.m_posit) % matrix1.m_right));
	//dVector pos1(matrix1.m_posit);

	// Calculate position, velocity, and linear acceleration
	dFloat last_pos = cj_data->cur_pos;
	dFloat last_vel = cj_data->cur_vel;
	cj_data->cur_pos = matrix1.UntransformVector(matrix0.m_posit).m_z;
	cj_data->cur_vel = (cj_data->cur_pos - last_pos) / timestep;
	cj_data->cur_lin_accel = (cj_data->cur_vel - last_vel) / timestep;

	// Calculate angle, omega, and angular acceleration
	dFloat last_angle = cj_data->ai->get_angle();
	dFloat last_omega = cj_data->cur_omega;
	dFloat sin_angle;
	dFloat cos_angle;
	Joint::c_calculate_angle(matrix1.m_front, matrix0.m_front, matrix0.m_right, sin_angle, cos_angle);
	cj_data->ai->update(cos_angle, sin_angle);
	cj_data->cur_omega = (cj_data->ai->get_angle() - last_angle) / timestep;
	cj_data->cur_ang_accel = (cj_data->cur_omega - last_omega) / timestep;
	dFloat cur_angle = cj_data->ai->get_angle();

	// Restrict movement on axis perpendicular to the pin direction.
	NewtonUserJointAddLinearRow(joint, &pos0[0], &pos1[0], &matrix0.m_front[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddLinearRow(joint, &pos0[0], &pos1[0], &matrix0.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Add two constraints row perpendicular to the pin
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

	// Get a point along the pin axis at some reasonable large distance from the pivot
	dVector q0(pos0 + matrix0.m_right.Scale(MIN_JOINT_PIN_LENGTH));
	dVector q1(pos1 + matrix1.m_right.Scale(MIN_JOINT_PIN_LENGTH));

	// Add two constraints row perpendicular to the pin
	NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix1.m_front[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix1.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Add linear limits and friction
	if (cj_data->lin_limits_enabled == true && cj_data->cur_pos < cj_data->min_pos) {
		const dVector& s0 = matrix0.m_posit;
		dVector s1(s0 + matrix1.m_right.Scale(cj_data->min_pos + Joint::LINEAR_LIMIT_EPSILON - cj_data->cur_pos));
		NewtonUserJointAddLinearRow(joint, &s0[0], &s1[0], &matrix1.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else //if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->lin_limits_enabled == true && cj_data->cur_pos > cj_data->max_pos) {
		const dVector& s0 = matrix0.m_posit;
		dVector s1(s0 + matrix1.m_right.Scale(cj_data->max_pos - Joint::LINEAR_LIMIT_EPSILON - cj_data->cur_pos));
		NewtonUserJointAddLinearRow(joint, &s0[0], &s1[0], &matrix1.m_right[0]);
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else //if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix0.m_posit[0], &matrix1.m_right[0]);
		dFloat power = cj_data->lin_friction;
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

	// Add angular limits and friction.
	if (cj_data->ang_limits_enabled == true && (cur_angle - cj_data->min_ang) < -Joint::ANGULAR_LIMIT_EPSILON) {
		NewtonUserJointAddAngularRow(joint, cj_data->min_ang - cur_angle, &matrix0.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->ang_limits_enabled == true && (cur_angle - cj_data->max_ang) > Joint::ANGULAR_LIMIT_EPSILON) {
		NewtonUserJointAddAngularRow(joint, cj_data->max_ang - cur_angle, &matrix0.m_right[0]);
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix0.m_right[0]);
		dFloat power = cj_data->ang_friction;
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

void MSNewton::Corkscrew::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;

	info->m_minLinearDof[0] = -0.0f;
	info->m_maxLinearDof[0] = 0.0f;
	info->m_minLinearDof[1] = -0.0f;
	info->m_maxLinearDof[1] = 0.0f;

	if (cj_data->lin_limits_enabled) {
		info->m_minLinearDof[2] = (cj_data->min_pos - cj_data->cur_pos);
		info->m_minLinearDof[2] = (cj_data->max_pos - cj_data->cur_pos);
	}
	else {
		info->m_minLinearDof[2] = -Joint::CUSTOM_LARGE_VALUE;
		info->m_minLinearDof[2] = Joint::CUSTOM_LARGE_VALUE;
	}

	info->m_minAngularDof[0] = -0.0f;
	info->m_maxAngularDof[0] = 0.0f;
	info->m_minAngularDof[1] = -0.0f;
	info->m_maxAngularDof[1] = 0.0f;

	if (cj_data->ang_limits_enabled) {
		info->m_minAngularDof[2] = (cj_data->min_ang - cj_data->ai->get_angle()) * RAD_TO_DEG;
		info->m_maxAngularDof[2] = (cj_data->max_ang - cj_data->ai->get_angle()) * RAD_TO_DEG;
	}
	else {
		info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
		info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
	}
}

void MSNewton::Corkscrew::on_destroy(JointData* joint_data) {
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	delete cj_data;
}

void MSNewton::Corkscrew::on_connect(JointData* joint_data) {
}

void MSNewton::Corkscrew::on_disconnect(JointData* joint_data) {
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	cj_data->ai->set_angle(0.0f);
	cj_data->cur_pos = 0.0f;
	cj_data->cur_vel = 0.0f;
	cj_data->cur_omega = 0.0f;
	cj_data->cur_lin_accel = 0.0f;
	cj_data->cur_ang_accel = 0.0f;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::Corkscrew::is_valid(VALUE self, VALUE v_joint) {
	JointData* address = (JointData*)Util::value_to_ll(v_joint);
	bool valid = Util::is_joint_valid(address);
	if (valid && address->jtype != JT_CORKSCREW) valid = false;
	return Util::to_value(valid);
}

VALUE MSNewton::Corkscrew::create(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_NONE);

	CorkscrewData* cj_data = new CorkscrewData;
	cj_data->ai = new AngularIntegration();
	cj_data->cur_pos = 0.0f;
	cj_data->cur_vel = 0.0f;
	cj_data->cur_omega = 0.0f;
	cj_data->cur_lin_accel = 0.0f;
	cj_data->cur_ang_accel = 0.0f;
	cj_data->min_pos = DEFAULT_MIN_POS;
	cj_data->max_pos = DEFAULT_MAX_POS;
	cj_data->min_ang = DEFAULT_MIN_ANG;
	cj_data->max_ang = DEFAULT_MAX_ANG;
	cj_data->lin_friction = DEFAULT_LIN_FRICTION;
	cj_data->ang_friction = DEFAULT_ANG_FRICTION;
	cj_data->lin_limits_enabled = DEFAULT_LIN_LIMITS_ENABLED;
	cj_data->ang_limits_enabled = DEFAULT_ANG_LIMITS_ENABLED;

	joint_data->dof = 6;
	joint_data->jtype = JT_CORKSCREW;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_connect = on_connect;
	joint_data->on_disconnect = on_disconnect;

	return Util::to_value(joint_data);
}

VALUE MSNewton::Corkscrew::get_cur_position(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_pos * world_data->inverse_scale);
}

VALUE MSNewton::Corkscrew::get_cur_velocity(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_vel * world_data->inverse_scale);
}

VALUE MSNewton::Corkscrew::get_cur_linear_acceleration(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_lin_accel * world_data->inverse_scale);
}

VALUE MSNewton::Corkscrew::get_min_position(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->min_pos * world_data->inverse_scale);
}

VALUE MSNewton::Corkscrew::set_min_position(VALUE self, VALUE v_joint, VALUE v_min) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->min_pos = Util::value_to_dFloat(v_min) * world_data->scale;
	return Util::to_value(cj_data->min_pos * world_data->inverse_scale);
}

VALUE MSNewton::Corkscrew::get_max_position(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->max_pos * world_data->inverse_scale);
}

VALUE MSNewton::Corkscrew::set_max_position(VALUE self, VALUE v_joint, VALUE v_max) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->max_pos = Util::value_to_dFloat(v_max) * world_data->scale;
	return Util::to_value(cj_data->max_pos * world_data->inverse_scale);
}

VALUE MSNewton::Corkscrew::enable_linear_limits(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	cj_data->lin_limits_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->lin_limits_enabled);
}

VALUE MSNewton::Corkscrew::linear_limits_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	return Util::to_value(cj_data->lin_limits_enabled);
}

VALUE MSNewton::Corkscrew::get_linear_friction(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	return Util::to_value(cj_data->lin_friction);
}

VALUE MSNewton::Corkscrew::set_linear_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	cj_data->lin_friction = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_friction), 0.0f);
	return Util::to_value(cj_data->lin_friction);
}

VALUE MSNewton::Corkscrew::get_cur_angle(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	return Util::to_value(cj_data->ai->get_angle());
}

VALUE MSNewton::Corkscrew::get_cur_omega(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	return Util::to_value(cj_data->cur_omega);
}

VALUE MSNewton::Corkscrew::get_cur_angular_acceleration(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	return Util::to_value(cj_data->cur_ang_accel);
}

VALUE MSNewton::Corkscrew::get_min_angle(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	return Util::to_value(cj_data->min_ang);
}

VALUE MSNewton::Corkscrew::set_min_angle(VALUE self, VALUE v_joint, VALUE v_min) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	cj_data->min_ang = Util::value_to_dFloat(v_min);
	return Util::to_value(cj_data->min_ang);
}

VALUE MSNewton::Corkscrew::get_max_angle(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	return Util::to_value(cj_data->max_ang);
}

VALUE MSNewton::Corkscrew::set_max_angle(VALUE self, VALUE v_joint, VALUE v_max) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	cj_data->max_ang = Util::value_to_dFloat(v_max);
	return Util::to_value(cj_data->max_ang);
}

VALUE MSNewton::Corkscrew::enable_angular_limits(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	cj_data->ang_limits_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->ang_limits_enabled);
}

VALUE MSNewton::Corkscrew::angular_limits_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	return Util::to_value(cj_data->ang_limits_enabled);
}

VALUE MSNewton::Corkscrew::get_angular_friction(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	return Util::to_value(cj_data->ang_friction);
}

VALUE MSNewton::Corkscrew::set_angular_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CORKSCREW);
	CorkscrewData* cj_data = (CorkscrewData*)joint_data->cj_data;
	cj_data->ang_friction = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_friction), 0.0f);
	return Util::to_value(cj_data->ang_friction);
}


void Init_msp_corkscrew(VALUE mNewton) {
	VALUE mCorkscrew = rb_define_module_under(mNewton, "Corkscrew");

	rb_define_module_function(mCorkscrew, "is_valid?", VALUEFUNC(MSNewton::Corkscrew::is_valid), 1);
	rb_define_module_function(mCorkscrew, "create", VALUEFUNC(MSNewton::Corkscrew::create), 1);
	rb_define_module_function(mCorkscrew, "get_cur_position", VALUEFUNC(MSNewton::Corkscrew::get_cur_position), 1);
	rb_define_module_function(mCorkscrew, "get_cur_velocity", VALUEFUNC(MSNewton::Corkscrew::get_cur_velocity), 1);
	rb_define_module_function(mCorkscrew, "get_cur_linaer_acceleration", VALUEFUNC(MSNewton::Corkscrew::get_cur_linear_acceleration), 1);
	rb_define_module_function(mCorkscrew, "get_min_position", VALUEFUNC(MSNewton::Corkscrew::get_min_position), 1);
	rb_define_module_function(mCorkscrew, "set_min_position", VALUEFUNC(MSNewton::Corkscrew::set_min_position), 2);
	rb_define_module_function(mCorkscrew, "get_max_position", VALUEFUNC(MSNewton::Corkscrew::get_max_position), 1);
	rb_define_module_function(mCorkscrew, "set_max_position", VALUEFUNC(MSNewton::Corkscrew::set_max_position), 2);
	rb_define_module_function(mCorkscrew, "enable_linear_limits", VALUEFUNC(MSNewton::Corkscrew::enable_linear_limits), 2);
	rb_define_module_function(mCorkscrew, "linear_limits_enabled?", VALUEFUNC(MSNewton::Corkscrew::linear_limits_enabled), 1);
	rb_define_module_function(mCorkscrew, "get_linear_friction", VALUEFUNC(MSNewton::Corkscrew::get_linear_friction), 1);
	rb_define_module_function(mCorkscrew, "set_linear_friction", VALUEFUNC(MSNewton::Corkscrew::set_linear_friction), 2);
	rb_define_module_function(mCorkscrew, "get_cur_angle", VALUEFUNC(MSNewton::Corkscrew::get_cur_angle), 1);
	rb_define_module_function(mCorkscrew, "get_cur_omega", VALUEFUNC(MSNewton::Corkscrew::get_cur_omega), 1);
	rb_define_module_function(mCorkscrew, "get_cur_angular_acceleration", VALUEFUNC(MSNewton::Corkscrew::get_cur_angular_acceleration), 1);
	rb_define_module_function(mCorkscrew, "get_min_angle", VALUEFUNC(MSNewton::Corkscrew::get_min_angle), 1);
	rb_define_module_function(mCorkscrew, "set_min_angle", VALUEFUNC(MSNewton::Corkscrew::set_min_angle), 2);
	rb_define_module_function(mCorkscrew, "get_max_angle", VALUEFUNC(MSNewton::Corkscrew::get_max_angle), 1);
	rb_define_module_function(mCorkscrew, "set_max_angle", VALUEFUNC(MSNewton::Corkscrew::set_max_angle), 2);
	rb_define_module_function(mCorkscrew, "enable_angular_limits", VALUEFUNC(MSNewton::Corkscrew::enable_angular_limits), 2);
	rb_define_module_function(mCorkscrew, "angular_limits_enabled?", VALUEFUNC(MSNewton::Corkscrew::angular_limits_enabled), 1);
	rb_define_module_function(mCorkscrew, "get_angular_friction", VALUEFUNC(MSNewton::Corkscrew::get_angular_friction), 1);
	rb_define_module_function(mCorkscrew, "set_angular_friction", VALUEFUNC(MSNewton::Corkscrew::set_angular_friction), 2);
}
