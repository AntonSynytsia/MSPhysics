#include "msp_joint_piston.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::Piston::DEFAULT_MIN = -10.0f;
const dFloat MSNewton::Piston::DEFAULT_MAX = 10.0f;
const bool MSNewton::Piston::DEFAULT_LIMITS_ENABLED = false;
const dFloat MSNewton::Piston::DEFAULT_RATE = 40.0f;
const dFloat MSNewton::Piston::DEFAULT_POWER = 0.0f;
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
	dMatrix matrix0, matrix1, matrix2;
	MSNewton::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1, matrix2);

	const dVector& pos0 = matrix0.m_posit;
	dVector pos1(matrix1.m_posit + matrix1.m_right.Scale((pos0 - matrix1.m_posit) % matrix1.m_right));

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
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Get a point along the ping axis at some reasonable large distance from the pivot
	dVector r0(pos0 + matrix0.m_front.Scale(MIN_JOINT_PIN_LENGTH));
	dVector r1(pos1 + matrix1.m_front.Scale(MIN_JOINT_PIN_LENGTH));

	// Add one constraint row perpendicular to the pin
	NewtonUserJointAddLinearRow(joint, &r0[0], &r1[0], &matrix0.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Add limits and friction
	if (cj_data->limits_enabled == true && cj_data->cur_pos < cj_data->min - Joint::LINEAR_LIMIT_EPSILON) {
		const dVector& s0 = matrix0.m_posit;
		dVector s1 = s0 + matrix1.m_right.Scale(cj_data->min - cj_data->cur_pos);
		NewtonUserJointAddLinearRow(joint, &s0[0], &s1[0], &matrix1.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->limits_enabled == true && cj_data->cur_pos > cj_data->max + Joint::LINEAR_LIMIT_EPSILON) {
		const dVector& s0 = matrix0.m_posit;
		dVector s1 = s0 + matrix1.m_right.Scale(cj_data->max - cj_data->cur_pos);
		NewtonUserJointAddLinearRow(joint, &s0[0], &s1[0], &matrix1.m_right[0]);
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		if (cj_data->controller_enabled) {
			// Get relative linear velocity
			dVector veloc0(0.0f, 0.0f, 0.0f);
			dVector veloc1(0.0f, 0.0f, 0.0f);
			NewtonBodyGetVelocity(joint_data->child, &veloc0[0]);
			if (joint_data->parent != nullptr)
				NewtonBodyGetVelocity(joint_data->parent, &veloc1[0]);
			dFloat rel_veloc = (veloc0 - veloc1) % matrix1.m_right;
			// Calculate relative position
			dFloat desired_pos = cj_data->limits_enabled ? Util::clamp(cj_data->controller, cj_data->min, cj_data->max) : cj_data->controller;
			dFloat rel_pos = desired_pos - cj_data->cur_pos;
			dFloat arel_pos = dAbs(rel_pos);
			// Calculate desired accel
			dFloat mar = cj_data->rate * cj_data->reduction_ratio;
			dFloat ratio = (cj_data->rate > EPSILON && cj_data->reduction_ratio > EPSILON && arel_pos < mar) ? arel_pos / mar : 1.0f;
			dFloat step = cj_data->rate * ratio * dSign(rel_pos) * timestep;
			if (dAbs(step) > arel_pos) step = rel_pos;
			dFloat desired_vel = step / timestep;
			dFloat desired_accel = (desired_vel - rel_veloc) / timestep;
			// Add linear row
			dVector point(matrix1.UntransformVector(matrix0.m_posit));
			point.m_z = step;
			point = matrix1.TransformVector(point);
			NewtonUserJointAddLinearRow(joint, &point[0], &matrix1.m_posit[0], &matrix1.m_right[0]);
			// Apply acceleration
			NewtonUserJointSetRowAcceleration(joint, desired_accel);
		}
		else {
			// Add linear row
			dVector point(matrix1.UntransformVector(matrix0.m_posit));
			point.m_z = 0.0f;
			point = matrix1.TransformVector(point);
			NewtonUserJointAddLinearRow(joint, &point[0], &matrix1.m_posit[0], &matrix1.m_right[0]);
		}
		if (cj_data->power == 0.0f) {
			NewtonUserJointSetRowMinimumFriction(joint, -Joint::CUSTOM_LARGE_VALUE);
			NewtonUserJointSetRowMaximumFriction(joint, Joint::CUSTOM_LARGE_VALUE);
		}
		else {
			NewtonUserJointSetRowMinimumFriction(joint, -cj_data->power);
			NewtonUserJointSetRowMaximumFriction(joint, cj_data->power);
		}
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
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

void MSNewton::Piston::on_disconnect(JointData* joint_data) {
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	cj_data->cur_pos = 0.0f;
	cj_data->cur_vel = 0.0f;
	cj_data->cur_accel = 0.0f;
}

void MSNewton::Piston::adjust_pin_matrix_proc(JointData* joint_data, dMatrix& pin_matrix) {
	dMatrix matrix;
	dVector centre;
	NewtonBodyGetMatrix(joint_data->child, &matrix[0][0]);
	NewtonBodyGetCentreOfMass(joint_data->child, &centre[0]);
	pin_matrix.m_posit = matrix.TransformVector(centre);
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
	cj_data->rate = DEFAULT_RATE;
	cj_data->power = DEFAULT_POWER;
	cj_data->reduction_ratio = DEFAULT_REDUCTION_RATIO;
	cj_data->controller = DEFAULT_CONTROLLER;
	cj_data->controller_enabled = DEFAULT_CONTROLLER_ENABLED;

	joint_data->dof = 6;
	joint_data->jtype = JT_PISTON;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_disconnect = on_disconnect;
	//~ joint_data->adjust_pin_matrix_proc = adjust_pin_matrix_proc;

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

VALUE MSNewton::Piston::limits_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	return Util::to_value(cj_data->limits_enabled);
}

VALUE MSNewton::Piston::get_rate(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->rate * world_data->inverse_scale);
}

VALUE MSNewton::Piston::set_rate(VALUE self, VALUE v_joint, VALUE v_rate) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->rate = Util::clamp_min(Util::value_to_dFloat(v_rate), 0.0f) * world_data->scale;
	return Util::to_value(cj_data->rate * world_data->inverse_scale);
}

VALUE MSNewton::Piston::get_power(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->power * world_data->inverse_scale4);
}

VALUE MSNewton::Piston::set_power(VALUE self, VALUE v_joint, VALUE v_power) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_PISTON);
	PistonData* cj_data = (PistonData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->power = Util::clamp_min(Util::value_to_dFloat(v_power), 0.0f) * world_data->scale4;
	return Util::to_value(cj_data->power * world_data->inverse_scale4);
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
	rb_define_module_function(mPiston, "limits_enabled?", VALUEFUNC(MSNewton::Piston::limits_enabled), 1);
	rb_define_module_function(mPiston, "get_rate", VALUEFUNC(MSNewton::Piston::get_rate), 1);
	rb_define_module_function(mPiston, "set_rate", VALUEFUNC(MSNewton::Piston::set_rate), 2);
	rb_define_module_function(mPiston, "get_power", VALUEFUNC(MSNewton::Piston::get_power), 1);
	rb_define_module_function(mPiston, "set_power", VALUEFUNC(MSNewton::Piston::set_power), 2);
	rb_define_module_function(mPiston, "get_reduction_ratio", VALUEFUNC(MSNewton::Piston::get_reduction_ratio), 1);
	rb_define_module_function(mPiston, "set_reduction_ratio", VALUEFUNC(MSNewton::Piston::set_reduction_ratio), 2);
	rb_define_module_function(mPiston, "get_controller", VALUEFUNC(MSNewton::Piston::get_controller), 1);
	rb_define_module_function(mPiston, "set_controller", VALUEFUNC(MSNewton::Piston::set_controller), 2);
}
