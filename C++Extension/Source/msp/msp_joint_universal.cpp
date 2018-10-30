#include "msp_joint_universal.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
	Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Universal::DEFAULT_MIN(-180.0f * M_DEG_TO_RAD);
const dFloat MSP::Universal::DEFAULT_MAX(180.0f * M_DEG_TO_RAD);
const bool MSP::Universal::DEFAULT_LIMITS_ENABLED(false);
const dFloat MSP::Universal::DEFAULT_FRICTION(0.0f);
const dFloat MSP::Universal::DEFAULT_CONTROLLER(1.0f);


/*
 ///////////////////////////////////////////////////////////////////////////////
	Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSP::Universal::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
	MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);

	dFloat inv_timestep = 1.0f / timestep;

	// Calculate position of pivot points and Jacobian direction vectors in global space.
	dMatrix matrix0, matrix1;
	MSP::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

	// Restrict movement on the pivot point along all three orthonormal directions
	NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix0.m_front[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

	NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix0.m_up[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

	NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix0.m_right[0]);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

	// Construct an orthogonal coordinate system with these two vectors
	dMatrix matrix1_1;
	matrix1_1.m_front = matrix1.m_front;
	matrix1_1.m_up = matrix0.m_right.CrossProduct(matrix1.m_front);
	matrix1_1.m_up = matrix1_1.m_up.Scale(1.0f / dSqrt(matrix1_1.m_up.DotProduct3(matrix1_1.m_up)));
	matrix1_1.m_right = matrix1_1.m_front.CrossProduct(matrix1_1.m_up);

	// Override the normal right side because the joint is too week due two centripetal accelerations
	dVector omega0(0.0f);
	dVector omega1(0.0f);
	NewtonBodyGetOmega(joint_data->m_child, &omega0[0]);
	if (joint_data->m_parent != nullptr)
		NewtonBodyGetOmega(joint_data->m_parent, &omega1[0]);
	dVector rel_omega(omega0 - omega1);

	dFloat angle = -MSP::Joint::c_calculate_angle2(matrix0.m_right, matrix1_1.m_right, matrix1_1.m_up);
	dFloat omega = rel_omega.DotProduct3(matrix1_1.m_up);
	dFloat alpha_error = -(angle + omega * timestep) / (timestep * timestep);

	NewtonUserJointAddAngularRow(joint, -angle, &matrix1_1.m_up[0]);
	NewtonUserJointSetRowAcceleration(joint, alpha_error);
	NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

	dFloat sin_angle1, cos_angle1;
	MSP::Joint::c_calculate_angle(matrix1_1.m_front, matrix0.m_front, matrix1_1.m_right, sin_angle1, cos_angle1);
	dFloat cur_angle1 = cj_data->m_ai1->update(cos_angle1, sin_angle1);
	dFloat last_omega1 = cj_data->m_cur_omega1;
	cj_data->m_cur_omega1 = rel_omega.DotProduct3(matrix1_1.m_right);
	cj_data->m_cur_alpha1 = (cj_data->m_cur_omega1 - last_omega1) * inv_timestep;

	dFloat sin_angle2, cos_angle2;
	MSP::Joint::c_calculate_angle(matrix1.m_right, matrix1_1.m_right, matrix1_1.m_front, sin_angle2, cos_angle2);
	dFloat cur_angle2 = cj_data->m_ai2->update(cos_angle2, sin_angle2);
	dFloat last_omega2 = cj_data->m_cur_omega2;
	cj_data->m_cur_omega2 = rel_omega.DotProduct3(matrix1_1.m_front);
	cj_data->m_cur_alpha2 = (cj_data->m_cur_omega2 - last_omega2) * inv_timestep;

	// Add angular limits and friction 1.
	bool bcontinue = false;
	if (cj_data->m_limits1_enabled) {
		if (cj_data->m_min1 > cj_data->m_max1) {
			NewtonUserJointAddAngularRow(joint, (cj_data->m_min1 + cj_data->m_max1) * 0.5f - cur_angle1, &matrix0.m_right[0]);
			NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
		}
		else if (cj_data->m_max1 - cj_data->m_min1 < Joint::ANGULAR_LIMIT_EPSILON2) {
			NewtonUserJointAddAngularRow(joint, -cur_angle1, &matrix0.m_right[0]);
			NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
		}
		else if (cur_angle1 < cj_data->m_min1) {
			NewtonUserJointAddAngularRow(joint, cj_data->m_min1 - cur_angle1 + Joint::ANGULAR_LIMIT_EPSILON, &matrix0.m_right[0]);
			NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
			NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
		}
		else if (cur_angle1 > cj_data->m_max1) {
			NewtonUserJointAddAngularRow(joint, cj_data->m_max1 - cur_angle1 - Joint::ANGULAR_LIMIT_EPSILON, &matrix0.m_right[0]);
			NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
			NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
		}
		else
			bcontinue = true;
	}
	else
		bcontinue = true;
	if (bcontinue) {
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix0.m_right[0]);
		NewtonUserJointSetRowAcceleration(joint, -cj_data->m_cur_omega1 * inv_timestep);
		dFloat power = cj_data->m_friction * dAbs(cj_data->m_controller);
		NewtonUserJointSetRowMinimumFriction(joint, -power);
		NewtonUserJointSetRowMaximumFriction(joint, power);
		NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
	}

	// Add angular limits and friction 2.
	bcontinue = false;
	if (cj_data->m_limits2_enabled) {
		if (cj_data->m_min2 > cj_data->m_max2) {
			NewtonUserJointAddAngularRow(joint, (cj_data->m_min2 + cj_data->m_max2) * 0.5f - cur_angle2, &matrix0.m_front[0]);
			NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
		}
		else if (cj_data->m_max2 - cj_data->m_min2 < Joint::ANGULAR_LIMIT_EPSILON2) {
			NewtonUserJointAddAngularRow(joint, -cur_angle2, &matrix0.m_front[0]);
			NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
		}
		else if (cur_angle2 < cj_data->m_min2) {
			NewtonUserJointAddAngularRow(joint, cj_data->m_min2 - cur_angle2 + Joint::ANGULAR_LIMIT_EPSILON, &matrix0.m_front[0]);
			NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
			NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
		}
		else if (cur_angle2 > cj_data->m_max2) {
			NewtonUserJointAddAngularRow(joint, cj_data->m_max2 - cur_angle2 - Joint::ANGULAR_LIMIT_EPSILON, &matrix0.m_front[0]);
			NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
			NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
		}
		else
			bcontinue = true;
	}
	else
		bcontinue = true;
	if (bcontinue) {
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix0.m_front[0]);
		NewtonUserJointSetRowAcceleration(joint, -cj_data->m_cur_omega2 * inv_timestep);
		dFloat power = cj_data->m_friction * dAbs(cj_data->m_controller);
		NewtonUserJointSetRowMinimumFriction(joint, -power);
		NewtonUserJointSetRowMaximumFriction(joint, power);
		NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
	}
}

void MSP::Universal::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
	MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);

	info->m_minLinearDof[0] = -0.0f;
	info->m_maxLinearDof[0] = 0.0f;
	info->m_minLinearDof[1] = -0.0f;
	info->m_maxLinearDof[1] = 0.0f;
	info->m_minLinearDof[2] = -0.0f;
	info->m_maxLinearDof[2] = 0.0f;

	if (cj_data->m_limits2_enabled) {
		info->m_minAngularDof[0] = (cj_data->m_min2 - cj_data->m_ai2->get_angle()) * M_RAD_TO_DEG;
		info->m_maxAngularDof[0] = (cj_data->m_max2 - cj_data->m_ai2->get_angle()) * M_RAD_TO_DEG;
	}
	else {
		info->m_minAngularDof[0] = -Joint::CUSTOM_LARGE_VALUE;
		info->m_maxAngularDof[0] = Joint::CUSTOM_LARGE_VALUE;
	}

	info->m_minAngularDof[1] = -0.0f;
	info->m_maxAngularDof[1] = 0.0f;

	if (cj_data->m_limits1_enabled) {
		info->m_minAngularDof[2] = (cj_data->m_min1 - cj_data->m_ai1->get_angle()) * M_RAD_TO_DEG;
		info->m_maxAngularDof[2] = (cj_data->m_max1 - cj_data->m_ai1->get_angle()) * M_RAD_TO_DEG;
	}
	else {
		info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
		info->m_maxAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
	}
}

void MSP::Universal::on_destroy(MSP::Joint::JointData* joint_data) {
	delete (reinterpret_cast<UniversalData*>(joint_data->m_cj_data));
}

void MSP::Universal::on_disconnect(MSP::Joint::JointData* joint_data) {
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	cj_data->m_ai1->set_angle(0.0f);
	cj_data->m_cur_omega1 = 0.0f;
	cj_data->m_cur_alpha1 = 0.0f;
	cj_data->m_ai2->set_angle(0.0f);
	cj_data->m_cur_omega2 = 0.0f;
	cj_data->m_cur_alpha2 = 0.0f;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
	Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Universal::rbf_is_valid(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
	return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::UNIVERSAL) ? Qtrue : Qfalse;
}

VALUE MSP::Universal::rbf_create(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);
	joint_data->m_dof = 6;
	joint_data->m_jtype = MSP::Joint::UNIVERSAL;
	joint_data->m_cj_data = new UniversalData();
	joint_data->m_submit_constraints = submit_constraints;
	joint_data->m_get_info = get_info;
	joint_data->m_on_destroy = on_destroy;
	joint_data->m_on_disconnect = on_disconnect;

	return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::Universal::rbf_get_cur_angle1(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_ai1->get_angle());
}

VALUE MSP::Universal::rbf_get_cur_omega1(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_cur_omega1);
}

VALUE MSP::Universal::rbf_get_cur_alpha1(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_cur_alpha1);
}

VALUE MSP::Universal::rbf_get_min1(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_min1);
}

VALUE MSP::Universal::rbf_set_min1(VALUE self, VALUE v_joint, VALUE v_min) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	cj_data->m_min1 = Util::value_to_dFloat(v_min);
	return Qnil;
}

VALUE MSP::Universal::rbf_get_max1(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_max1);
}

VALUE MSP::Universal::rbf_set_max1(VALUE self, VALUE v_joint, VALUE v_max) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	cj_data->m_max1 = Util::value_to_dFloat(v_max);
	return Qnil;
}

VALUE MSP::Universal::rbf_enable_limits1(VALUE self, VALUE v_joint, VALUE v_state) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	cj_data->m_limits1_enabled = Util::value_to_bool(v_state);
	return Qnil;
}

VALUE MSP::Universal::rbf_limits1_enabled(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_limits1_enabled);
}


VALUE MSP::Universal::rbf_get_cur_angle2(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_ai2->get_angle());
}

VALUE MSP::Universal::rbf_get_cur_omega2(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_cur_omega2);
}

VALUE MSP::Universal::rbf_get_cur_alpha2(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_cur_alpha2);
}

VALUE MSP::Universal::rbf_get_min2(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_min2);
}

VALUE MSP::Universal::rbf_set_min2(VALUE self, VALUE v_joint, VALUE v_min) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	cj_data->m_min2 = Util::value_to_dFloat(v_min);
	return Qnil;
}

VALUE MSP::Universal::rbf_get_max2(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_max2);
}

VALUE MSP::Universal::rbf_set_max2(VALUE self, VALUE v_joint, VALUE v_max) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	cj_data->m_max2 = Util::value_to_dFloat(v_max);
	return Qnil;
}

VALUE MSP::Universal::rbf_enable_limits2(VALUE self, VALUE v_joint, VALUE v_state) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	cj_data->m_limits2_enabled = Util::value_to_bool(v_state);
	return Qnil;
}

VALUE MSP::Universal::rbf_limits2_enabled(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_limits2_enabled);
}


VALUE MSP::Universal::rbf_get_friction(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_friction * M_INCH2_TO_METER2);
}

VALUE MSP::Universal::rbf_set_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	cj_data->m_friction = Util::max_float(Util::value_to_dFloat(v_friction) * M_METER2_TO_INCH2, 0.0f);
	return Qnil;
}

VALUE MSP::Universal::rbf_get_controller(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_controller);
}

VALUE MSP::Universal::rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::UNIVERSAL);
	UniversalData* cj_data = reinterpret_cast<UniversalData*>(joint_data->m_cj_data);
	dFloat desired_controller = Util::value_to_dFloat(v_controller);
	if (cj_data->m_controller != desired_controller) {
		cj_data->m_controller = desired_controller;
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

void MSP::Universal::init_ruby(VALUE mNewton) {
	VALUE mUniversal = rb_define_module_under(mNewton, "Universal");

	rb_define_module_function(mUniversal, "is_valid?", VALUEFUNC(MSP::Universal::rbf_is_valid), 1);
	rb_define_module_function(mUniversal, "create", VALUEFUNC(MSP::Universal::rbf_create), 1);
	rb_define_module_function(mUniversal, "get_cur_angle1", VALUEFUNC(MSP::Universal::rbf_get_cur_angle1), 1);
	rb_define_module_function(mUniversal, "get_cur_omega1", VALUEFUNC(MSP::Universal::rbf_get_cur_omega1), 1);
	rb_define_module_function(mUniversal, "get_cur_alpha1", VALUEFUNC(MSP::Universal::rbf_get_cur_alpha1), 1);
	rb_define_module_function(mUniversal, "get_min1", VALUEFUNC(MSP::Universal::rbf_get_min1), 1);
	rb_define_module_function(mUniversal, "set_min1", VALUEFUNC(MSP::Universal::rbf_set_min1), 2);
	rb_define_module_function(mUniversal, "get_max1", VALUEFUNC(MSP::Universal::rbf_get_max1), 1);
	rb_define_module_function(mUniversal, "set_max1", VALUEFUNC(MSP::Universal::rbf_set_max1), 2);
	rb_define_module_function(mUniversal, "enable_limits1", VALUEFUNC(MSP::Universal::rbf_enable_limits1), 2);
	rb_define_module_function(mUniversal, "limits1_enabled?", VALUEFUNC(MSP::Universal::rbf_limits1_enabled), 1);

	rb_define_module_function(mUniversal, "get_cur_angle2", VALUEFUNC(MSP::Universal::rbf_get_cur_angle2), 1);
	rb_define_module_function(mUniversal, "get_cur_omega2", VALUEFUNC(MSP::Universal::rbf_get_cur_omega2), 1);
	rb_define_module_function(mUniversal, "get_cur_alpha2", VALUEFUNC(MSP::Universal::rbf_get_cur_alpha2), 1);
	rb_define_module_function(mUniversal, "get_min2", VALUEFUNC(MSP::Universal::rbf_get_min2), 1);
	rb_define_module_function(mUniversal, "set_min2", VALUEFUNC(MSP::Universal::rbf_set_min2), 2);
	rb_define_module_function(mUniversal, "get_max2", VALUEFUNC(MSP::Universal::rbf_get_max2), 1);
	rb_define_module_function(mUniversal, "set_max2", VALUEFUNC(MSP::Universal::rbf_set_max2), 2);
	rb_define_module_function(mUniversal, "enable_limits2", VALUEFUNC(MSP::Universal::rbf_enable_limits2), 2);
	rb_define_module_function(mUniversal, "limits2_enabled?", VALUEFUNC(MSP::Universal::rbf_limits2_enabled), 1);

	rb_define_module_function(mUniversal, "get_friction", VALUEFUNC(MSP::Universal::rbf_get_friction), 1);
	rb_define_module_function(mUniversal, "set_friction", VALUEFUNC(MSP::Universal::rbf_set_friction), 2);
	rb_define_module_function(mUniversal, "get_controller", VALUEFUNC(MSP::Universal::rbf_get_controller), 1);
	rb_define_module_function(mUniversal, "set_controller", VALUEFUNC(MSP::Universal::rbf_set_controller), 2);
}
