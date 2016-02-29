#include "msp_joint_piston.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::Piston::DEFAULT_MIN = -10.0f;
const dFloat MSNewton::Piston::DEFAULT_MAX = 10.0f;
const bool MSNewton::Piston::DEFAULT_LIMITS_ENABLED = false;
const dFloat MSNewton::Piston::DEFAULT_LINEAR_RATE = 40.0f;
const dFloat MSNewton::Piston::DEFAULT_STRENGTH = 0.0f;
const dFloat MSNewton::Piston::DEFAULT_REDUCTION_RATIO = 0.1f;
const dFloat MSNewton::Piston::DEFAULT_CONTROLLER = 0.0f;
const bool MSNewton::Piston::DEFAULT_CONTROLLER_ENABLED = false;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Piston::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;

	// Calculate position of pivot points and Jacobian direction vectors in global space.
	dMatrix matrix0;
	dMatrix matrix1;
	Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

	const dVector& pos0 = matrix0.m_posit;
	dVector pos1(matrix1.m_posit + matrix1.m_right.Scale((pos0 - matrix1.m_posit) % matrix1.m_right));
	//dVector pos1(matrix1.m_posit);

	// Calculate position, velocity, and acceleration
	dFloat last_pos = cj_data->cur_pos;
	dFloat last_vel = cj_data->cur_vel;
	cj_data->cur_pos = matrix1.UntransformVector(matrix0.m_posit).m_z;
	cj_data->cur_vel = (cj_data->cur_pos - last_pos) / timestep;
	cj_data->cur_accel = (cj_data->cur_vel - last_vel) / timestep;

	// Restrict movement on the pivot point along two orthonormal directions perpendicular to pin direction.
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

	// Add three angular rows to restrict rotation around all axis.
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
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle(matrix0.m_front, matrix1.m_front, matrix0.m_right), &matrix0.m_right[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);*/

	// Get a point along the ping axis at some reasonable large distance from the pivot
	dVector q0(pos0 + matrix0.m_right.Scale(MIN_JOINT_PIN_LENGTH));
	dVector q1(pos1 + matrix1.m_right.Scale(MIN_JOINT_PIN_LENGTH));

	// Add two constraints row perpendicular to the pin
	NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_front[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Get a point along the ping axis at some reasonable large distance from the pivot
	dVector r0(pos0 + matrix0.m_front.Scale(MIN_JOINT_PIN_LENGTH));
	dVector r1(pos1 + matrix1.m_front.Scale(MIN_JOINT_PIN_LENGTH));

	// Add one constraint row perpendicular to the pin
	NewtonUserJointAddLinearRow(joint, &r0[0], &r1[0], &matrix0.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Add limits and friction
	if (cj_data->limits_enabled == true && cj_data->cur_pos < cj_data->min) {
		const dVector& s0 = matrix0.m_posit;
		dVector s1 = s0 + matrix1.m_right.Scale(cj_data->min + Joint::LINEAR_LIMIT_EPSILON - cj_data->cur_pos);
		NewtonUserJointAddLinearRow(joint, &s0[0], &s1[0], &matrix1.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else //if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->limits_enabled == true && cj_data->cur_pos > cj_data->max) {
		const dVector& s0 = matrix0.m_posit;
		dVector s1 = s0 + matrix1.m_right.Scale(cj_data->max - Joint::LINEAR_LIMIT_EPSILON - cj_data->cur_pos);
		NewtonUserJointAddLinearRow(joint, &s0[0], &s1[0], &matrix1.m_right[0]);
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else //if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		// Get relative linear velocity
		dVector veloc0(0.0f, 0.0f, 0.0f);
		dVector veloc1(0.0f, 0.0f, 0.0f);
		NewtonBodyGetVelocity(joint_data->child, &veloc0[0]);
		if (joint_data->parent != nullptr)
			NewtonBodyGetVelocity(joint_data->parent, &veloc1[0]);
		dFloat desired_vel;
		dFloat rel_veloc = (veloc0 - veloc1) % matrix0.m_right;
		if (cj_data->controller_enabled) {
			// Calculate relative position
			dFloat desired_pos = cj_data->limits_enabled ? Util::clamp(cj_data->controller, cj_data->min, cj_data->max) : cj_data->controller;
			dFloat rel_pos = desired_pos - cj_data->cur_pos;
			dFloat arel_pos = dAbs(rel_pos);
			// Calculate desired accel
			dFloat mar = cj_data->linear_rate * cj_data->reduction_ratio;
			dFloat ratio = (cj_data->linear_rate > EPSILON && cj_data->reduction_ratio > EPSILON && arel_pos < mar) ? arel_pos / mar : 1.0f;
			desired_vel = cj_data->linear_rate * ratio * dSign(rel_pos);
		}
		else
			desired_vel = 0.0f;
		dFloat desired_accel = (desired_vel - rel_veloc) / timestep;
		NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix0.m_posit[0], &matrix1.m_right[0]);
		NewtonUserJointSetRowAcceleration(joint, desired_accel);
		if (cj_data->strength > EPSILON) {
			BodyData* cbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
			dFloat power;
			if (cbody_data->bstatic == false && cbody_data->mass >= MIN_MASS)
				power = cbody_data->mass * cj_data->strength;
			else {
				BodyData* pbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
				power = (pbody_data->bstatic == false && pbody_data->mass >= MIN_MASS) ? pbody_data->mass * cj_data->strength : cj_data->strength;
			}
			NewtonUserJointSetRowMinimumFriction(joint, -power);
			NewtonUserJointSetRowMaximumFriction(joint, power);
		}
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	/*else {
		// Calculate relative position
		dFloat desired_pos = cj_data->limits_enabled ? Util::clamp(cj_data->controller, cj_data->min, cj_data->max) : cj_data->controller;
		dFloat rel_pos = desired_pos - cj_data->cur_pos;
		dFloat arel_pos = dAbs(rel_pos);
		// Calculate desired accel
		dFloat ratio = 1.0f;
		if (cj_data->damp > EPSILON2) {
			dFloat mar = cj_data->accel * cj_data->reduction_ratio / cj_data->damp;
			if (arel_pos < mar)
				ratio = arel_pos / mar;
		}
		dFloat desired_accel = cj_data->controller_enabled ? cj_data->accel * dSign(rel_pos) * ratio : 0.0f;
		// Get relative linear velocity
		dVector veloc0(0.0f, 0.0f, 0.0f);
		dVector veloc1(0.0f, 0.0f, 0.0f);
		NewtonBodyGetVelocity(joint_data->child, &veloc0[0]);
		if (joint_data->parent != nullptr)
			NewtonBodyGetVelocity(joint_data->parent, &veloc1[0]);
		dFloat rel_veloc = (veloc0 - veloc1) % matrix0.m_right;
		// Calculate the desired acceleration
		dFloat rel_accel = desired_accel - cj_data->damp * rel_veloc;
		// Set angular acceleration
		NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix0.m_posit[0], &matrix1.m_right[0]);
		NewtonUserJointSetRowAcceleration(joint, rel_accel);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}*/
	/*else if (cj_data->controller_enabled) {
		// Calculate relative position
		dFloat desired_pos = cj_data->limits_enabled ? Util::clamp(cj_data->controller, cj_data->min, cj_data->max) : cj_data->controller;
		dFloat rel_pos = desired_pos - cj_data->cur_pos;
		const dVector& s0 = matrix0.m_posit;
		dVector s1(s0 + matrix0.m_right.Scale(rel_pos));
		NewtonUserJointAddLinearRow(joint, &s0[0], &s1[0], &matrix0.m_right[0]);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness * Joint::STIFFNESS_RATIO);
		// Calculate linear acceleration
		dFloat step = cj_data->linear_rate * timestep;
		if (dAbs(rel_pos) > 2.0f * dAbs(step)) {
			dFloat desired_speed = dSign(rel_pos) * cj_data->linear_rate;
			dFloat current_speed = cj_data->cur_vel;
			dFloat accel = (desired_speed - current_speed) / timestep;
			if (dAbs(accel) > cj_data->max_accel)
				accel = cj_data->max_accel * dSign(accel);
			NewtonUserJointSetRowAcceleration(joint, accel);
		}
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		else //if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP * 0.1f);
	}
	else {
		NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix0.m_posit[0], &matrix0.m_right[0]);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness * Joint::STIFFNESS_RATIO);
		dFloat accel = -cj_data->cur_vel / timestep;
		if (dAbs(accel) > cj_data->max_accel)
			accel = cj_data->max_accel * dSign(accel);
		NewtonUserJointSetRowAcceleration(joint, accel);
	}*/
}

void MSNewton::Piston::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;

	info->m_minLinearDof[0] = -0.0f;
	info->m_maxLinearDof[0] = 0.0f;
	info->m_minLinearDof[1] = -0.0f;
	info->m_maxLinearDof[1] = 0.0f;

	if (cj_data->limits_enabled) {
		info->m_minLinearDof[2] = (cj_data->min - cj_data->cur_pos);
		info->m_minLinearDof[2] = (cj_data->max - cj_data->cur_pos);
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

void MSNewton::Piston::on_destroy(JointData* joint_data) {
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	delete cj_data;
}

void MSNewton::Piston::on_connect(JointData* joint_data) {
}

void MSNewton::Piston::on_disconnect(JointData* joint_data) {
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	cj_data->cur_pos = 0.0f;
	cj_data->cur_vel = 0.0f;
	cj_data->cur_accel = 0.0f;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::Piston::is_valid(VALUE self, VALUE v_joint) {
	JointData* address = (JointData*)Util::value_to_ll(v_joint);
	bool valid = Util::is_joint_valid(address);
	if (valid && address->jtype != JT_PISTON) valid = false;
	return Util::to_value(valid);
}

VALUE MSNewton::Piston::create(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_NONE);

	PistonData* cj_data = new PistonData;
	cj_data->cur_pos = 0.0f;
	cj_data->cur_vel = 0.0f;
	cj_data->cur_accel = 0.0f;
	cj_data->min = DEFAULT_MIN;
	cj_data->max = DEFAULT_MAX;
	cj_data->limits_enabled = DEFAULT_LIMITS_ENABLED;
	cj_data->linear_rate = DEFAULT_LINEAR_RATE;
	cj_data->strength = DEFAULT_STRENGTH;
	cj_data->reduction_ratio = DEFAULT_REDUCTION_RATIO;
	cj_data->controller = DEFAULT_CONTROLLER;
	cj_data->controller_enabled = DEFAULT_CONTROLLER_ENABLED;

	joint_data->dof = 6;
	joint_data->jtype = JT_PISTON;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_connect = on_connect;
	joint_data->on_disconnect = on_disconnect;

	return Util::to_value(joint_data);
}

VALUE MSNewton::Piston::get_cur_position(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_pos * world_data->inverse_scale);
}

VALUE MSNewton::Piston::get_cur_velocity(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_vel * world_data->inverse_scale);
}

VALUE MSNewton::Piston::get_cur_acceleration(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_accel * world_data->inverse_scale);
}

VALUE MSNewton::Piston::get_min(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->min * world_data->inverse_scale);
}

VALUE MSNewton::Piston::set_min(VALUE self, VALUE v_joint, VALUE v_min) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->min = Util::value_to_dFloat(v_min) * world_data->scale;
	return Util::to_value(cj_data->min * world_data->inverse_scale);
}

VALUE MSNewton::Piston::get_max(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->max * world_data->inverse_scale);
}

VALUE MSNewton::Piston::set_max(VALUE self, VALUE v_joint, VALUE v_max) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->max = Util::value_to_dFloat(v_max) * world_data->scale;
	return Util::to_value(cj_data->max * world_data->inverse_scale);
}

VALUE MSNewton::Piston::enable_limits(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	cj_data->limits_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->limits_enabled);
}

VALUE MSNewton::Piston::are_limits_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	return Util::to_value(cj_data->limits_enabled);
}

VALUE MSNewton::Piston::get_linear_rate(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->linear_rate * world_data->inverse_scale);
}

VALUE MSNewton::Piston::set_linear_rate(VALUE self, VALUE v_joint, VALUE v_linear_rate) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->linear_rate = Util::clamp_min(Util::value_to_dFloat(v_linear_rate), 0.0f) * world_data->scale;
	return Util::to_value(cj_data->linear_rate * world_data->inverse_scale);
}

VALUE MSNewton::Piston::get_strength(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	return Util::to_value(cj_data->strength);
}

VALUE MSNewton::Piston::set_strength(VALUE self, VALUE v_joint, VALUE v_strength) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	cj_data->strength = Util::clamp_min(Util::value_to_dFloat(v_strength), 0.0f);
	return Util::to_value(cj_data->strength);
}

VALUE MSNewton::Piston::get_reduction_ratio(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	return Util::to_value(cj_data->reduction_ratio);
}

VALUE MSNewton::Piston::set_reduction_ratio(VALUE self, VALUE v_joint, VALUE v_reduction_ratio) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	cj_data->reduction_ratio = Util::clamp(Util::value_to_dFloat(v_reduction_ratio), 0.0f, 1.0f);
	return Util::to_value(cj_data->reduction_ratio);
}

VALUE MSNewton::Piston::get_controller(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return (cj_data->controller_enabled ? Util::to_value(cj_data->controller * world_data->inverse_scale) : Qnil);
}

VALUE MSNewton::Piston::set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	if (v_controller == Qnil) {
		if (cj_data->controller_enabled == true) {
			cj_data->controller_enabled = false;
			if (joint_data->connected)
				NewtonBodySetSleepState(joint_data->child, 0);
		}
		return Qnil;
	}
	else {
		dFloat controller = Util::value_to_dFloat(v_controller) * world_data->scale;
		if (cj_data->controller_enabled == false || controller != cj_data->controller) {
			cj_data->controller = controller;
			cj_data->controller_enabled = true;
			if (joint_data->connected)
				NewtonBodySetSleepState(joint_data->child, 0);
		}
		return Util::to_value(cj_data->controller * world_data->inverse_scale);
	}
}


void Init_msp_piston(VALUE mNewton) {
	VALUE mPiston = rb_define_module_under(mNewton, "Piston");

	rb_define_module_function(mPiston, "is_valid?", VALUEFUNC(MSNewton::Piston::is_valid), 1);
	rb_define_module_function(mPiston, "create", VALUEFUNC(MSNewton::Piston::create), 1);
	rb_define_module_function(mPiston, "get_cur_position", VALUEFUNC(MSNewton::Piston::get_cur_position), 1);
	rb_define_module_function(mPiston, "get_cur_velocity", VALUEFUNC(MSNewton::Piston::get_cur_velocity), 1);
	rb_define_module_function(mPiston, "get_cur_acceleration", VALUEFUNC(MSNewton::Piston::get_cur_acceleration), 1);
	rb_define_module_function(mPiston, "get_min", VALUEFUNC(MSNewton::Piston::get_min), 1);
	rb_define_module_function(mPiston, "set_min", VALUEFUNC(MSNewton::Piston::set_min), 2);
	rb_define_module_function(mPiston, "get_max", VALUEFUNC(MSNewton::Piston::get_max), 1);
	rb_define_module_function(mPiston, "set_max", VALUEFUNC(MSNewton::Piston::set_max), 2);
	rb_define_module_function(mPiston, "enable_limits", VALUEFUNC(MSNewton::Piston::enable_limits), 2);
	rb_define_module_function(mPiston, "are_limits_enabled?", VALUEFUNC(MSNewton::Piston::are_limits_enabled), 1);
	rb_define_module_function(mPiston, "get_linear_rate", VALUEFUNC(MSNewton::Piston::get_linear_rate), 1);
	rb_define_module_function(mPiston, "set_linear_rate", VALUEFUNC(MSNewton::Piston::set_linear_rate), 2);
	rb_define_module_function(mPiston, "get_strength", VALUEFUNC(MSNewton::Piston::get_strength), 1);
	rb_define_module_function(mPiston, "set_strength", VALUEFUNC(MSNewton::Piston::set_strength), 2);
	rb_define_module_function(mPiston, "get_reduction_ratio", VALUEFUNC(MSNewton::Piston::get_reduction_ratio), 1);
	rb_define_module_function(mPiston, "set_reduction_ratio", VALUEFUNC(MSNewton::Piston::set_reduction_ratio), 2);
	rb_define_module_function(mPiston, "get_controller", VALUEFUNC(MSNewton::Piston::get_controller), 1);
	rb_define_module_function(mPiston, "set_controller", VALUEFUNC(MSNewton::Piston::set_controller), 2);
}
