#include "msp_joint_motor.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Motor::DEFAULT_ACCEL(1.0f);
const dFloat MSP::Motor::DEFAULT_DAMP(0.5f);
const bool MSP::Motor::DEFAULT_FREE_ROTATE_ENABLED(false);
const dFloat MSP::Motor::DEFAULT_CONTROLLER(1.0f);


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSP::Motor::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
	MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);

	dFloat inv_timestep = 1.0f / timestep;

	// Calculate position of pivot points and Jacobian direction vectors in global space.
	dMatrix matrix0, matrix1;
	MSP::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

	// Calculate angle, omega, and acceleration.
	dFloat last_angle = cj_data->m_ai->get_angle();
	dFloat last_omega = cj_data->m_cur_omega;
	dFloat sin_angle, cos_angle;
	Joint::c_calculate_angle(matrix1.m_front, matrix0.m_front, matrix0.m_right, sin_angle, cos_angle);
	cj_data->m_ai->update(cos_angle, sin_angle);
	cj_data->m_cur_omega = (cj_data->m_ai->get_angle() - last_angle) * inv_timestep;
	cj_data->m_cur_alpha = (cj_data->m_cur_omega - last_omega) * inv_timestep;

	const dVector& p0 = matrix0.m_posit;
	const dVector& p1 = matrix1.m_posit;

	// Restrict movement on axes perpendicular to the pin direction.
	NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_front[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

	NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_up[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

	NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_right[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

	// Restriction rotation along the two axis perpendicular to pin direction.
	/*dVector q0(p0 + matrix0.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
	dVector q1(p1 + matrix1.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));

	NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_front[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

	NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_up[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);*/
	NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_front), &matrix1.m_front[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

	NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_up[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

	// Add accel and damp
	dFloat desired_accel = cj_data->m_accel * cj_data->m_controller;
	if (cj_data->m_free_rotate_enabled && desired_accel == 0.0f) {
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix1.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
	}
	else {
		// Calculate the desired acceleration
		dFloat rel_accel = desired_accel - cj_data->m_damp * cj_data->m_cur_omega;
		// Set angular acceleration
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix1.m_right[0]);
		NewtonUserJointSetRowAcceleration(joint, rel_accel);
		NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
	}
}

void MSP::Motor::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
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

void MSP::Motor::on_destroy(MSP::Joint::JointData* joint_data) {
	delete (reinterpret_cast<MotorData*>(joint_data->m_cj_data));
}

void MSP::Motor::on_disconnect(MSP::Joint::JointData* joint_data) {
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	cj_data->m_ai->set_angle(0.0f);
	cj_data->m_cur_omega = 0.0f;
	cj_data->m_cur_alpha = 0.0f;
}

void MSP::Motor::adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix) {
	dMatrix matrix;
	dVector centre;
	NewtonBodyGetMatrix(joint_data->m_child, &matrix[0][0]);
	NewtonBodyGetCentreOfMass(joint_data->m_child, &centre[0]);
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

VALUE MSP::Motor::rbf_is_valid(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
	return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::MOTOR) ? Qtrue : Qfalse;
}

VALUE MSP::Motor::rbf_create(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

	joint_data->m_dof = 6;
	joint_data->m_jtype = MSP::Joint::MOTOR;
	joint_data->m_cj_data = new MotorData();
	joint_data->m_submit_constraints = submit_constraints;
	joint_data->m_get_info = get_info;
	joint_data->m_on_destroy = on_destroy;
	joint_data->m_on_disconnect = on_disconnect;
	//~ joint_data->adjust_pin_matrix_proc = adjust_pin_matrix_proc;

	return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::Motor::rbf_get_cur_angle(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_ai->get_angle());
}

VALUE MSP::Motor::rbf_get_cur_omega(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_cur_omega);
}

VALUE MSP::Motor::rbf_get_cur_alpha(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_cur_alpha);
}

VALUE MSP::Motor::rbf_get_accel(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_accel);
}

VALUE MSP::Motor::rbf_set_accel(VALUE self, VALUE v_joint, VALUE v_accel) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	cj_data->m_accel = Util::value_to_dFloat(v_accel);
	return Qnil;
}

VALUE MSP::Motor::rbf_get_damp(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_damp);
}

VALUE MSP::Motor::rbf_set_damp(VALUE self, VALUE v_joint, VALUE v_damp) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	cj_data->m_damp = Util::max_float(Util::value_to_dFloat(v_damp), 0.0f);
	return Qnil;
}

VALUE MSP::Motor::rbf_enable_free_rotate(VALUE self, VALUE v_joint, VALUE v_state) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	cj_data->m_free_rotate_enabled = Util::value_to_bool(v_state);
	return Qnil;
}

VALUE MSP::Motor::rbf_is_free_rotate_enabled(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_free_rotate_enabled);
}

VALUE MSP::Motor::rbf_get_controller(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_controller);
}

VALUE MSP::Motor::rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::MOTOR);
	MotorData* cj_data = reinterpret_cast<MotorData*>(joint_data->m_cj_data);
	dFloat controller = Util::value_to_dFloat(v_controller);
	if (controller != cj_data->m_controller) {
		cj_data->m_controller = controller;
		if (joint_data->m_connected)
			NewtonBodySetSleepState(joint_data->m_child, 0);
	}
	return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSP::Motor::init_ruby(VALUE mNewton) {
	VALUE mMotor = rb_define_module_under(mNewton, "Motor");

	rb_define_module_function(mMotor, "is_valid?", VALUEFUNC(MSP::Motor::rbf_is_valid), 1);
	rb_define_module_function(mMotor, "create", VALUEFUNC(MSP::Motor::rbf_create), 1);
	rb_define_module_function(mMotor, "get_cur_angle", VALUEFUNC(MSP::Motor::rbf_get_cur_angle), 1);
	rb_define_module_function(mMotor, "get_cur_omega", VALUEFUNC(MSP::Motor::rbf_get_cur_omega), 1);
	rb_define_module_function(mMotor, "get_cur_alpha", VALUEFUNC(MSP::Motor::rbf_get_cur_alpha), 1);
	rb_define_module_function(mMotor, "get_accel", VALUEFUNC(MSP::Motor::rbf_get_accel), 1);
	rb_define_module_function(mMotor, "set_accel", VALUEFUNC(MSP::Motor::rbf_set_accel), 2);
	rb_define_module_function(mMotor, "get_damp", VALUEFUNC(MSP::Motor::rbf_get_damp), 1);
	rb_define_module_function(mMotor, "set_damp", VALUEFUNC(MSP::Motor::rbf_set_damp), 2);
	rb_define_module_function(mMotor, "enable_free_rotate", VALUEFUNC(MSP::Motor::rbf_enable_free_rotate), 2);
	rb_define_module_function(mMotor, "is_free_rotate_enabled?", VALUEFUNC(MSP::Motor::rbf_is_free_rotate_enabled), 1);
	rb_define_module_function(mMotor, "get_controller", VALUEFUNC(MSP::Motor::rbf_get_controller), 1);
	rb_define_module_function(mMotor, "set_controller", VALUEFUNC(MSP::Motor::rbf_set_controller), 2);
}
