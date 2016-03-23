#include "msp_joint_slider.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::Slider::DEFAULT_MIN = -10.0f;
const dFloat MSNewton::Slider::DEFAULT_MAX = 10.0f;
const bool MSNewton::Slider::DEFAULT_LIMITS_ENABLED = false;
const dFloat MSNewton::Slider::DEFAULT_FRICTION = 0.0f;
const dFloat MSNewton::Slider::DEFAULT_CONTROLLER = 1.0f;

/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Slider::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;

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
	if (cj_data->limits_enabled == true && cj_data->cur_pos < cj_data->min) {
		const dVector& s0 = matrix0.m_posit;
		dVector s1(s0 + matrix1.m_right.Scale(cj_data->min + Joint::LINEAR_LIMIT_EPSILON - cj_data->cur_pos));
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
		dVector s1(s0 + matrix1.m_right.Scale(cj_data->max - Joint::LINEAR_LIMIT_EPSILON - cj_data->cur_pos));
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
		BodyData* cbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
		dFloat power = cj_data->friction * cj_data->controller;
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

void MSNewton::Slider::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;

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

void MSNewton::Slider::on_destroy(JointData* joint_data) {
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	delete cj_data;
}

void MSNewton::Slider::on_connect(JointData* joint_data) {
}

void MSNewton::Slider::on_disconnect(JointData* joint_data) {
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	cj_data->cur_pos = 0.0f;
	cj_data->cur_vel = 0.0f;
	cj_data->cur_accel = 0.0f;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::Slider::is_valid(VALUE self, VALUE v_joint) {
	JointData* address = (JointData*)Util::value_to_ll(v_joint);
	bool valid = Util::is_joint_valid(address);
	if (valid && address->jtype != JT_SLIDER) valid = false;
	return Util::to_value(valid);
}

VALUE MSNewton::Slider::create(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_NONE);

	SliderData* cj_data = new SliderData;
	cj_data->cur_pos = 0.0f;
	cj_data->cur_vel = 0.0f;
	cj_data->cur_accel = 0.0f;
	cj_data->min = DEFAULT_MIN;
	cj_data->max = DEFAULT_MAX;
	cj_data->limits_enabled = DEFAULT_LIMITS_ENABLED;
	cj_data->friction = DEFAULT_FRICTION;
	cj_data->controller = DEFAULT_CONTROLLER;

	joint_data->dof = 6;
	joint_data->jtype = JT_SLIDER;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_connect = on_connect;
	joint_data->on_disconnect = on_disconnect;

	return Util::to_value(joint_data);
}

VALUE MSNewton::Slider::get_cur_position(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_pos * world_data->inverse_scale);
}

VALUE MSNewton::Slider::get_cur_velocity(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_vel * world_data->inverse_scale);
}

VALUE MSNewton::Slider::get_cur_acceleration(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_accel * world_data->inverse_scale);
}

VALUE MSNewton::Slider::get_min(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->min * world_data->inverse_scale);
}

VALUE MSNewton::Slider::set_min(VALUE self, VALUE v_joint, VALUE v_min) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->min = Util::value_to_dFloat(v_min) * world_data->scale;
	return Util::to_value(cj_data->min * world_data->inverse_scale);
}

VALUE MSNewton::Slider::get_max(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->max * world_data->inverse_scale);
}

VALUE MSNewton::Slider::set_max(VALUE self, VALUE v_joint, VALUE v_max) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->max = Util::value_to_dFloat(v_max) * world_data->scale;
	return Util::to_value(cj_data->max * world_data->inverse_scale);
}

VALUE MSNewton::Slider::enable_limits(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	cj_data->limits_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->limits_enabled);
}

VALUE MSNewton::Slider::limits_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	return Util::to_value(cj_data->limits_enabled);
}

VALUE MSNewton::Slider::get_friction(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	return Util::to_value(cj_data->friction);
}

VALUE MSNewton::Slider::set_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	cj_data->friction = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_friction), 0.0f);
	return Util::to_value(cj_data->friction);
}

VALUE MSNewton::Slider::get_controller(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	return Util::to_value(cj_data->controller);
}

VALUE MSNewton::Slider::set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_SLIDER);
	SliderData* cj_data = (SliderData*)joint_data->cj_data;
	dFloat controller = Util::value_to_dFloat(v_controller);
	if (controller != cj_data->controller) {
		cj_data->controller = controller;
		if (joint_data->connected) NewtonBodySetSleepState(joint_data->child, 0);
	}
	return Util::to_value(cj_data->controller);
}


void Init_msp_slider(VALUE mNewton) {
	VALUE mSlider = rb_define_module_under(mNewton, "Slider");

	rb_define_module_function(mSlider, "is_valid?", VALUEFUNC(MSNewton::Slider::is_valid), 1);
	rb_define_module_function(mSlider, "create", VALUEFUNC(MSNewton::Slider::create), 1);
	rb_define_module_function(mSlider, "get_cur_position", VALUEFUNC(MSNewton::Slider::get_cur_position), 1);
	rb_define_module_function(mSlider, "get_cur_velocity", VALUEFUNC(MSNewton::Slider::get_cur_velocity), 1);
	rb_define_module_function(mSlider, "get_cur_acceleration", VALUEFUNC(MSNewton::Slider::get_cur_acceleration), 1);
	rb_define_module_function(mSlider, "get_min", VALUEFUNC(MSNewton::Slider::get_min), 1);
	rb_define_module_function(mSlider, "set_min", VALUEFUNC(MSNewton::Slider::set_min), 2);
	rb_define_module_function(mSlider, "get_max", VALUEFUNC(MSNewton::Slider::get_max), 1);
	rb_define_module_function(mSlider, "set_max", VALUEFUNC(MSNewton::Slider::set_max), 2);
	rb_define_module_function(mSlider, "enable_limits", VALUEFUNC(MSNewton::Slider::enable_limits), 2);
	rb_define_module_function(mSlider, "limits_enabled?", VALUEFUNC(MSNewton::Slider::limits_enabled), 1);
	rb_define_module_function(mSlider, "get_friction", VALUEFUNC(MSNewton::Slider::get_friction), 1);
	rb_define_module_function(mSlider, "set_friction", VALUEFUNC(MSNewton::Slider::set_friction), 2);
	rb_define_module_function(mSlider, "get_controller", VALUEFUNC(MSNewton::Slider::get_controller), 1);
	rb_define_module_function(mSlider, "set_controller", VALUEFUNC(MSNewton::Slider::set_controller), 2);
}
