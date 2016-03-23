#include "msp_joint_motor.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::Motor::DEFAULT_ACCEL = 1.0f;
const dFloat MSNewton::Motor::DEFAULT_DAMP = 0.5f;
const bool MSNewton::Motor::DEFAULT_FREE_ROTATE_ENABLED = false;
const dFloat MSNewton::Motor::DEFAULT_CONTROLLER = 1.0f;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Motor::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;

	// Calculate position of pivot points and Jacobian direction vectors in global space.
	dMatrix matrix0;
	dMatrix matrix1;
	Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

	// Calculate angle, omega, and acceleration.
	dFloat last_angle = cj_data->ai->get_angle();
	dFloat last_omega = cj_data->cur_omega;
	dFloat sin_angle;
	dFloat cos_angle;
	Joint::c_calculate_angle(matrix1.m_front, matrix0.m_front, matrix0.m_right, sin_angle, cos_angle);
	cj_data->ai->update(cos_angle, sin_angle);
	cj_data->cur_omega = (cj_data->ai->get_angle() - last_angle) / timestep;
	cj_data->cur_accel = (cj_data->cur_omega - last_omega) / timestep;

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
	NewtonUserJointAddLinearRow(joint, &q0[0], &q2[0], &matrix1.m_front[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	q2 = q0 + matrix0.m_up.Scale((q1 - q0) % matrix0.m_up);
	NewtonUserJointAddLinearRow(joint, &q0[0], &q2[0], &matrix1.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Add accel and damp
	dFloat desired_accel = cj_data->accel * cj_data->controller;
	if (cj_data->free_rotate_enabled == true && desired_accel == 0.0f) {
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix0.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		// Get relative angular velocity
		dVector omega0(0.0f, 0.0f, 0.0f);
		dVector omega1(0.0f, 0.0f, 0.0f);
		NewtonBodyGetOmega(joint_data->child, &omega0[0]);
		if (joint_data->parent != nullptr)
			NewtonBodyGetOmega(joint_data->parent, &omega1[0]);
		dFloat rel_omega = (omega0 - omega1) % matrix0.m_right;
		// Calculate the desired acceleration
		dFloat rel_accel = desired_accel - cj_data->damp * rel_omega;
		// Set angular acceleration
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix0.m_right[0]);
		NewtonUserJointSetRowAcceleration(joint, rel_accel);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
}

void MSNewton::Motor::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
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
	info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
	info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
}

void MSNewton::Motor::on_destroy(JointData* joint_data) {
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	delete cj_data->ai;
	delete cj_data;
}

void MSNewton::Motor::on_connect(JointData* joint_data) {
}

void MSNewton::Motor::on_disconnect(JointData* joint_data) {
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	cj_data->ai->set_angle(0.0f);
	cj_data->cur_omega = 0.0f;
	cj_data->cur_accel = 0.0f;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::Motor::is_valid(VALUE self, VALUE v_joint) {
	JointData* address = (JointData*)Util::value_to_ll(v_joint);
	bool valid = Util::is_joint_valid(address);
	if (valid && address->jtype != JT_MOTOR) valid = false;
	return Util::to_value(valid);
}

VALUE MSNewton::Motor::create(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_NONE);

	MotorData* cj_data = new MotorData;
	cj_data->accel = DEFAULT_ACCEL;
	cj_data->damp = DEFAULT_DAMP;
	cj_data->ai = new AngularIntegration();
	cj_data->cur_omega = 0.0f;
	cj_data->cur_accel = 0.0f;
	cj_data->free_rotate_enabled = DEFAULT_FREE_ROTATE_ENABLED;
	cj_data->controller = DEFAULT_CONTROLLER;

	joint_data->dof = 6;
	joint_data->jtype = JT_MOTOR;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_connect = on_connect;
	joint_data->on_disconnect = on_disconnect;

	return Util::to_value(joint_data);
}

VALUE MSNewton::Motor::get_cur_angle(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	return Util::to_value(cj_data->ai->get_angle());
}

VALUE MSNewton::Motor::get_cur_omega(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	return Util::to_value(cj_data->cur_omega);
}

VALUE MSNewton::Motor::get_cur_acceleration(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	return Util::to_value(cj_data->cur_accel);
}

VALUE MSNewton::Motor::get_accel(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	return Util::to_value(cj_data->accel);
}

VALUE MSNewton::Motor::set_accel(VALUE self, VALUE v_joint, VALUE v_accel) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	cj_data->accel = Util::value_to_dFloat(v_accel);
	return Util::to_value(cj_data->accel);
}

VALUE MSNewton::Motor::get_damp(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	return Util::to_value(cj_data->damp);
}

VALUE MSNewton::Motor::set_damp(VALUE self, VALUE v_joint, VALUE v_damp) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	cj_data->damp = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_damp), 0.0f);
	return Util::to_value(cj_data->damp);
}

VALUE MSNewton::Motor::enable_free_rotate(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	cj_data->free_rotate_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->free_rotate_enabled);
}

VALUE MSNewton::Motor::is_free_rotate_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	return Util::to_value(cj_data->free_rotate_enabled);
}

VALUE MSNewton::Motor::get_controller(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	return Util::to_value(cj_data->controller);
}

VALUE MSNewton::Motor::set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_MOTOR);
	MotorData* cj_data = (MotorData*)joint_data->cj_data;
	dFloat controller = Util::value_to_dFloat(v_controller);
	if (controller != cj_data->controller) {
		cj_data->controller = controller;
		if (joint_data->connected)
			NewtonBodySetSleepState(joint_data->child, 0);
	}
	return Util::to_value(cj_data->controller);
}


void Init_msp_motor(VALUE mNewton) {
	VALUE mMotor = rb_define_module_under(mNewton, "Motor");

	rb_define_module_function(mMotor, "is_valid?", VALUEFUNC(MSNewton::Motor::is_valid), 1);
	rb_define_module_function(mMotor, "create", VALUEFUNC(MSNewton::Motor::create), 1);
	rb_define_module_function(mMotor, "get_cur_angle", VALUEFUNC(MSNewton::Motor::get_cur_angle), 1);
	rb_define_module_function(mMotor, "get_cur_omega", VALUEFUNC(MSNewton::Motor::get_cur_omega), 1);
	rb_define_module_function(mMotor, "get_cur_acceleration", VALUEFUNC(MSNewton::Motor::get_cur_acceleration), 1);
	rb_define_module_function(mMotor, "get_accel", VALUEFUNC(MSNewton::Motor::get_accel), 1);
	rb_define_module_function(mMotor, "set_accel", VALUEFUNC(MSNewton::Motor::set_accel), 2);
	rb_define_module_function(mMotor, "get_damp", VALUEFUNC(MSNewton::Motor::get_damp), 1);
	rb_define_module_function(mMotor, "set_damp", VALUEFUNC(MSNewton::Motor::set_damp), 2);
	rb_define_module_function(mMotor, "enable_free_rotate", VALUEFUNC(MSNewton::Motor::enable_free_rotate), 2);
	rb_define_module_function(mMotor, "is_free_rotate_enabled?", VALUEFUNC(MSNewton::Motor::is_free_rotate_enabled), 1);
	rb_define_module_function(mMotor, "get_controller", VALUEFUNC(MSNewton::Motor::get_controller), 1);
	rb_define_module_function(mMotor, "set_controller", VALUEFUNC(MSNewton::Motor::set_controller), 2);
}
