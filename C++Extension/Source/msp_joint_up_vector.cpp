#include "msp_joint_up_vector.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dVector MSNewton::UpVector::DEFAULT_PIN_DIR(0.0f, 0.0f, 1.0f);
const dFloat MSNewton::UpVector::DEFAULT_ACCEL = 40.0f;
const dFloat MSNewton::UpVector::DEFAULT_DAMP = 10.0f;
const bool MSNewton::UpVector::DEFAULT_DAMPER_ENABLED = false;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::UpVector::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;

	// Calculate position of pivot points and Jacobian direction vectors in global space.
	dMatrix matrix0;
	dMatrix matrix1;
	Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);
	dMatrix pin_matrix = cj_data->pin_matrix * matrix1;

	// If the body is rotated by some amount, there is a plane of rotation.
	dVector res = matrix0.m_right + pin_matrix.m_right;
	dVector lateral_dir;
	if (dAbs(res.m_x) < EPSILON2 && dAbs(res.m_y) < EPSILON2 && dAbs(res.m_z) < EPSILON2)
		lateral_dir = pin_matrix.m_up;
	else
		lateral_dir = matrix0.m_right * pin_matrix.m_right;
	dFloat mag = lateral_dir % lateral_dir;
	if (mag > EPSILON) {
		// If the side vector is not zero, it means the body has rotated
		mag = dSqrt(mag);
		lateral_dir = lateral_dir.Scale(1.0f / mag);
		dFloat angle = Joint::c_calculate_angle(matrix0.m_right, pin_matrix.m_right, lateral_dir);
		// Add an angular constraint to correct the error angle.
		NewtonUserJointAddAngularRow(joint, angle, &lateral_dir[0]);
		if (cj_data->damper_enabled)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, cj_data->accel, cj_data->damp);
		NewtonUserJointSetRowStiffness(joint, -joint_data->stiffness);

		// In theory only one correction is needed, but this produces instability as the body may move sideway.
		// A lateral correction prevent this from happening.
		dVector front_dir(lateral_dir * pin_matrix.m_right);
		NewtonUserJointAddAngularRow(joint, 0.0f, &front_dir[0]);
		if (cj_data->damper_enabled)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, cj_data->accel, cj_data->damp);
		NewtonUserJointSetRowStiffness(joint, -joint_data->stiffness);
	}
	else {
		// If the angle error is very small, then two angular corrections along the plane axis do the trick.
		NewtonUserJointAddAngularRow(joint, 0.0f, &pin_matrix.m_front[0]);
		if (cj_data->damper_enabled)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, cj_data->accel, cj_data->damp);
		NewtonUserJointSetRowStiffness(joint, -joint_data->stiffness);

		NewtonUserJointAddAngularRow(joint, 0.0f, &pin_matrix.m_up[0]);
		if (cj_data->damper_enabled)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, cj_data->accel, cj_data->damp);
		NewtonUserJointSetRowStiffness(joint, -joint_data->stiffness);
	}
}

void MSNewton::UpVector::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
	//JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	//UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;

	info->m_minLinearDof[0] = -Joint::CUSTOM_LARGE_VALUE;
	info->m_maxLinearDof[0] = Joint::CUSTOM_LARGE_VALUE;
	info->m_minLinearDof[1] = -Joint::CUSTOM_LARGE_VALUE;
	info->m_maxLinearDof[1] = Joint::CUSTOM_LARGE_VALUE;
	info->m_minLinearDof[2] = -Joint::CUSTOM_LARGE_VALUE;
	info->m_maxLinearDof[2] = Joint::CUSTOM_LARGE_VALUE;

	info->m_minAngularDof[0] = -0.0f;
	info->m_maxAngularDof[0] = 0.0f;
	info->m_minAngularDof[1] = -0.0f;
	info->m_maxAngularDof[1] = 0.0f;
	info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
	info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
}

void MSNewton::UpVector::on_destroy(JointData* joint_data) {
	UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;
	delete cj_data;
}

void MSNewton::UpVector::on_connect(JointData* joint_data) {
}

void MSNewton::UpVector::on_disconnect(JointData* joint_data) {
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::UpVector::is_valid(VALUE self, VALUE v_joint) {
	JointData* address = (JointData*)Util::value_to_ll(v_joint);
	bool valid = Util::is_joint_valid(address);
	if (valid && address->jtype != JT_UP_VECTOR) valid = false;
	return Util::to_value(valid);
}

VALUE MSNewton::UpVector::create(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_NONE);
	UpVectorData* cj_data = new UpVectorData;
	cj_data->pin_dir = DEFAULT_PIN_DIR;
	cj_data->pin_matrix = Util::matrix_from_pin_dir(ORIGIN, cj_data->pin_dir);
	cj_data->accel = DEFAULT_ACCEL;
	cj_data->damp = DEFAULT_DAMP;
	cj_data->damper_enabled = DEFAULT_DAMPER_ENABLED;

	joint_data->dof = 6;
	joint_data->jtype = JT_UP_VECTOR;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_connect = on_connect;
	joint_data->on_disconnect = on_disconnect;

	return Util::to_value(joint_data);
}

VALUE MSNewton::UpVector::get_pin_dir(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UP_VECTOR);
	UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;
	return Util::vector_to_value(cj_data->pin_dir);
}

VALUE MSNewton::UpVector::set_pin_dir(VALUE self, VALUE v_joint, VALUE v_pin_dir) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UP_VECTOR);
	UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;
	dVector pin_dir = Util::value_to_vector(v_pin_dir);
	if (Util::get_vector_magnitude(pin_dir) < EPSILON)
		rb_raise(rb_eTypeError, "Zero lengthed vectors are not allowed!");
	Util::normalize_vector(pin_dir);
	if (!Util::vectors_identical(pin_dir, cj_data->pin_dir)) {
		cj_data->pin_dir = pin_dir;
		cj_data->pin_matrix = Util::matrix_from_pin_dir(ORIGIN, cj_data->pin_dir);
		if (joint_data->connected)
			NewtonBodySetSleepState(joint_data->child, 0);
	}
	return Util::vector_to_value(cj_data->pin_dir);
}

VALUE MSNewton::UpVector::get_accel(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UP_VECTOR);
	UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;
	return Util::to_value(cj_data->accel);
}

VALUE MSNewton::UpVector::set_accel(VALUE self, VALUE v_joint, VALUE v_accel) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UP_VECTOR);
	UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;
	cj_data->accel = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_accel), 0.0f);
	return Util::to_value(cj_data->accel);
}

VALUE MSNewton::UpVector::get_damp(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UP_VECTOR);
	UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;
	return Util::to_value(cj_data->damp);
}

VALUE MSNewton::UpVector::set_damp(VALUE self, VALUE v_joint, VALUE v_damp) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UP_VECTOR);
	UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;
	cj_data->damp = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_damp), 0.0f);
	return Util::to_value(cj_data->damp);
}

VALUE MSNewton::UpVector::enable_damper(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UP_VECTOR);
	UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;
	cj_data->damper_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->damper_enabled);
}

VALUE MSNewton::UpVector::is_damper_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_UP_VECTOR);
	UpVectorData* cj_data = (UpVectorData*)joint_data->cj_data;
	return Util::to_value(cj_data->damper_enabled);
}


void Init_msp_up_vector(VALUE mNewton) {
	VALUE mUpVector = rb_define_module_under(mNewton, "UpVector");

	rb_define_module_function(mUpVector, "is_valid?", VALUEFUNC(MSNewton::UpVector::is_valid), 1);
	rb_define_module_function(mUpVector, "create", VALUEFUNC(MSNewton::UpVector::create), 1);
	rb_define_module_function(mUpVector, "get_pin_dir", VALUEFUNC(MSNewton::UpVector::get_pin_dir), 1);
	rb_define_module_function(mUpVector, "set_pin_dir", VALUEFUNC(MSNewton::UpVector::set_pin_dir), 2);
	rb_define_module_function(mUpVector, "get_accel", VALUEFUNC(MSNewton::UpVector::get_accel), 1);
	rb_define_module_function(mUpVector, "set_accel", VALUEFUNC(MSNewton::UpVector::set_accel), 2);
	rb_define_module_function(mUpVector, "get_damp", VALUEFUNC(MSNewton::UpVector::get_damp), 1);
	rb_define_module_function(mUpVector, "set_damp", VALUEFUNC(MSNewton::UpVector::set_damp), 2);
	rb_define_module_function(mUpVector, "enable_damper", VALUEFUNC(MSNewton::UpVector::enable_damper), 2);
	rb_define_module_function(mUpVector, "is_damper_enabled?", VALUEFUNC(MSNewton::UpVector::is_damper_enabled), 1);
}
