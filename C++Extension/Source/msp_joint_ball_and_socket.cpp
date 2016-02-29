#include "msp_joint_ball_and_socket.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::BallAndSocket::DEFAULT_STIFF = 40.0f;
const dFloat MSNewton::BallAndSocket::DEFAULT_DAMP = 10.0f;
const bool MSNewton::BallAndSocket::DEFAULT_DAMPER_ENABLED = false;
const dFloat MSNewton::BallAndSocket::DEFAULT_MAX_CONE_ANGLE = 30.0f * DEG_TO_RAD;
const bool MSNewton::BallAndSocket::DEFAULT_CONE_LIMITS_ENABLED = false;
const dFloat MSNewton::BallAndSocket::DEFAULT_MIN_TWIST_ANGLE = -180.0f * DEG_TO_RAD;
const dFloat MSNewton::BallAndSocket::DEFAULT_MAX_TWIST_ANGLE = 180.0f * DEG_TO_RAD;
const bool MSNewton::BallAndSocket::DEFAULT_TWIST_LIMITS_ENABLED = false;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::BallAndSocket::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;

	dMatrix matrix0;
	dMatrix matrix1;

	// Calculate the position of the pivot point and the Jacobian direction vectors, in global space.
	MSNewton::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);
	dMatrix matrix2 = Util::rotate_matrix_to_dir(matrix0, matrix1.m_right);

	// Restrict the movement on the pivot point along all tree orthonormal directions.
	NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix1.m_front[0]);
	NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix1.m_up[0]);
	NewtonUserJointAddLinearRow(joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix1.m_right[0]);

	// Calculate current angles.
	dFloat sin_angle;
	dFloat cos_angle;
	MSNewton::Joint::c_calculate_angle(matrix1.m_front, matrix2.m_front, matrix1.m_right, sin_angle, cos_angle);
	cj_data->twist_ai.update(cos_angle, sin_angle);
	dFloat cur_twist_angle = cj_data->twist_ai.get_angle();

	const dVector& cone_dir0 = matrix0.m_right;
	const dVector& cone_dir1 = matrix1.m_right;
	dFloat cone_angle_cos = cone_dir0 % cone_dir1;
	cj_data->cur_cone_angle = dAcos(cone_angle_cos);

	// Handle cone angle
	if (cj_data->cone_limits_enabled && cj_data->cone_angle_cos > 0.9999f) {
		NewtonUserJointAddAngularRow(joint, MSNewton::Joint::c_calculate_angle(matrix0.m_right, matrix1.m_right, matrix1.m_front), &matrix1.m_front[0]);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

		NewtonUserJointAddAngularRow(joint, MSNewton::Joint::c_calculate_angle(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_up[0]);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->cone_limits_enabled && cone_angle_cos <= cj_data->cone_angle_cos) {
		dVector lateral_dir(cone_dir0 * cone_dir1);
		dFloat mag2 = lateral_dir % lateral_dir;
		lateral_dir = lateral_dir.Scale(1.0f / dSqrt(mag2));
		dQuaternion rot(cj_data->cone_angle_half_cos, lateral_dir.m_x * cj_data->cone_angle_half_sin, lateral_dir.m_y * cj_data->cone_angle_half_sin, lateral_dir.m_z * cj_data->cone_angle_half_sin);
		dVector front_dir(rot.UnrotateVector(cone_dir1));
		dVector up_dir(lateral_dir * front_dir);

		NewtonUserJointAddAngularRow(joint, 0.0f, &up_dir[0]);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

		NewtonUserJointAddAngularRow(joint, MSNewton::Joint::c_calculate_angle(cone_dir0, front_dir, lateral_dir), &lateral_dir[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}

	// Handle twist angle
	if (cj_data->twist_limits_enabled == true && cur_twist_angle < cj_data->min_twist_angle) {
		dFloat rel_angle = cj_data->min_twist_angle - cur_twist_angle;
		NewtonUserJointAddAngularRow(joint, rel_angle, &matrix0.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->twist_limits_enabled == true && cur_twist_angle > cj_data->max_twist_angle) {
		dFloat rel_angle = cj_data->max_twist_angle - cur_twist_angle;
		NewtonUserJointAddAngularRow(joint, rel_angle, &matrix0.m_right[0]);
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
}

void MSNewton::BallAndSocket::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
	//JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	//BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;

	info->m_minLinearDof[0] = -0.0f;
	info->m_maxLinearDof[0] = 0.0f;
	info->m_minLinearDof[1] = -0.0f;
	info->m_maxLinearDof[1] = 0.0f;;
	info->m_minLinearDof[2] = -0.0f;
	info->m_maxLinearDof[2] = 0.0f;

	info->m_minAngularDof[0] = -Joint::CUSTOM_LARGE_VALUE;
	info->m_minAngularDof[0] = Joint::CUSTOM_LARGE_VALUE;
	info->m_minAngularDof[1] = -Joint::CUSTOM_LARGE_VALUE;
	info->m_minAngularDof[1] = Joint::CUSTOM_LARGE_VALUE;
	info->m_minAngularDof[2] = -Joint::CUSTOM_LARGE_VALUE;
	info->m_minAngularDof[2] = Joint::CUSTOM_LARGE_VALUE;
}

void MSNewton::BallAndSocket::on_destroy(JointData* data) {
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	delete cj_data;
}

void MSNewton::BallAndSocket::on_connect(JointData* data) {
}

void MSNewton::BallAndSocket::on_disconnect(JointData* data) {
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	cj_data->cur_cone_angle = 0.0f;
	cj_data->twist_ai.set_angle(0.0f);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::BallAndSocket::is_valid(VALUE self, VALUE v_joint) {
	JointData* address = (JointData*)Util::value_to_ll(v_joint);
	bool valid = Util::is_joint_valid(address);
	if (valid && address->jtype != JT_BALL_AND_SOCKET) valid = false;
	return Util::to_value(valid);
}

VALUE MSNewton::BallAndSocket::create(VALUE self, VALUE v_joint) {
	JointData* data = Util::value_to_joint2(v_joint, JT_NONE);

	BallAndSocketData* cj_data = new BallAndSocketData;
	cj_data->max_cone_angle = DEFAULT_MAX_CONE_ANGLE;
	cj_data->min_twist_angle = DEFAULT_MIN_TWIST_ANGLE;
	cj_data->max_twist_angle = DEFAULT_MAX_TWIST_ANGLE;
	cj_data->cur_cone_angle = 0.0f;
	cj_data->twist_ai.set_angle(0.0f);
	cj_data->cone_limits_enabled = DEFAULT_CONE_LIMITS_ENABLED;
	cj_data->twist_limits_enabled = DEFAULT_TWIST_LIMITS_ENABLED;
	cj_data->stiff = DEFAULT_STIFF;
	cj_data->damp = DEFAULT_DAMP;
	cj_data->damper_enabled = DEFAULT_DAMPER_ENABLED;
	cj_data->cone_angle_cos = dCos(cj_data->max_cone_angle);
	cj_data->cone_angle_sin = dSin(cj_data->max_cone_angle);
	cj_data->cone_angle_half_cos = dCos(cj_data->max_cone_angle * 0.5f);
	cj_data->cone_angle_half_sin = dSin(cj_data->max_cone_angle * 0.5f);

	data->dof = 6;
	data->jtype = JT_BALL_AND_SOCKET;
	data->cj_data = cj_data;
	data->submit_constraints = submit_constraints;
	data->get_info = get_info;
	data->on_destroy = on_destroy;
	data->on_connect = on_connect;
	data->on_disconnect = on_disconnect;

	return Util::to_value(data);
}

VALUE MSNewton::BallAndSocket::get_stiff(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;
	return Util::to_value(cj_data->stiff);
}

VALUE MSNewton::BallAndSocket::set_stiff(VALUE self, VALUE v_joint, VALUE v_stiff) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;
	cj_data->stiff = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_stiff), 0.0f);
	return Util::to_value(cj_data->stiff);
}

VALUE MSNewton::BallAndSocket::get_damp(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;
	return Util::to_value(cj_data->damp);
}

VALUE MSNewton::BallAndSocket::set_damp(VALUE self, VALUE v_joint, VALUE v_damp) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;
	cj_data->damp = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_damp), 0.0f);
	return Util::to_value(cj_data->damp);
}

VALUE MSNewton::BallAndSocket::enable_damper(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;
	cj_data->damper_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->damper_enabled);
}

VALUE MSNewton::BallAndSocket::is_damper_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;
	return Util::to_value(cj_data->damper_enabled);
}

VALUE MSNewton::BallAndSocket::get_max_cone_angle(VALUE self, VALUE v_joint) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	return Util::to_value(cj_data->max_cone_angle);
}

VALUE MSNewton::BallAndSocket::set_max_cone_angle(VALUE self, VALUE v_joint, VALUE v_angle) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	cj_data->max_cone_angle = Util::value_to_dFloat(v_angle);
	cj_data->cone_angle_cos = dCos(cj_data->max_cone_angle);
	cj_data->cone_angle_sin = dSin(cj_data->max_cone_angle);
	cj_data->cone_angle_half_cos = dCos(cj_data->max_cone_angle * 0.5f);
	cj_data->cone_angle_half_sin = dSin(cj_data->max_cone_angle * 0.5f);
	return Util::to_value(cj_data->max_cone_angle);
}

VALUE MSNewton::BallAndSocket::enable_cone_limits(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	cj_data->cone_limits_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->cone_limits_enabled);
}

VALUE MSNewton::BallAndSocket::are_cone_limits_enabled(VALUE self, VALUE v_joint) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	return Util::to_value(cj_data->cone_limits_enabled);
}

VALUE MSNewton::BallAndSocket::get_min_twist_angle(VALUE self, VALUE v_joint) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	return Util::to_value(cj_data->min_twist_angle);
}

VALUE MSNewton::BallAndSocket::set_min_twist_angle(VALUE self, VALUE v_joint, VALUE v_angle) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	cj_data->min_twist_angle = Util::value_to_dFloat(v_angle);
	return Util::to_value(cj_data->min_twist_angle);
}

VALUE MSNewton::BallAndSocket::get_max_twist_angle(VALUE self, VALUE v_joint) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	return Util::to_value(cj_data->max_twist_angle);
}

VALUE MSNewton::BallAndSocket::set_max_twist_angle(VALUE self, VALUE v_joint, VALUE v_angle) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	cj_data->max_twist_angle = Util::value_to_dFloat(v_angle);
	return Util::to_value(cj_data->max_twist_angle);
}

VALUE MSNewton::BallAndSocket::enable_twist_limits(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	cj_data->twist_limits_enabled = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->twist_limits_enabled);
}

VALUE MSNewton::BallAndSocket::are_twist_limits_enabled(VALUE self, VALUE v_joint) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	return Util::to_value(cj_data->twist_limits_enabled);
}

VALUE MSNewton::BallAndSocket::get_cur_cone_angle(VALUE self, VALUE v_joint) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	return Util::to_value(cj_data->cur_cone_angle);
}

VALUE MSNewton::BallAndSocket::get_cur_twist_angle(VALUE self, VALUE v_joint) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	return Util::to_value(cj_data->twist_ai.get_angle());
}


void Init_msp_ball_and_socket(VALUE mNewton) {
	VALUE mBallAndSocket = rb_define_module_under(mNewton, "BallAndSocket");

	rb_define_module_function(mBallAndSocket, "is_valid?", VALUEFUNC(MSNewton::BallAndSocket::is_valid), 1);
	rb_define_module_function(mBallAndSocket, "create", VALUEFUNC(MSNewton::BallAndSocket::create), 1);
	rb_define_module_function(mBallAndSocket, "get_stiff", VALUEFUNC(MSNewton::BallAndSocket::get_stiff), 1);
	rb_define_module_function(mBallAndSocket, "set_stiff", VALUEFUNC(MSNewton::BallAndSocket::set_stiff), 2);
	rb_define_module_function(mBallAndSocket, "get_damp", VALUEFUNC(MSNewton::BallAndSocket::get_damp), 1);
	rb_define_module_function(mBallAndSocket, "set_damp", VALUEFUNC(MSNewton::BallAndSocket::set_damp), 2);
	rb_define_module_function(mBallAndSocket, "enable_damper", VALUEFUNC(MSNewton::BallAndSocket::enable_damper), 2);
	rb_define_module_function(mBallAndSocket, "is_damper_enabled?", VALUEFUNC(MSNewton::BallAndSocket::is_damper_enabled), 1);
	rb_define_module_function(mBallAndSocket, "get_max_cone_angle", VALUEFUNC(MSNewton::BallAndSocket::get_max_cone_angle), 1);
	rb_define_module_function(mBallAndSocket, "set_max_cone_angle", VALUEFUNC(MSNewton::BallAndSocket::set_max_cone_angle), 2);
	rb_define_module_function(mBallAndSocket, "enable_cone_limits", VALUEFUNC(MSNewton::BallAndSocket::enable_cone_limits), 2);
	rb_define_module_function(mBallAndSocket, "are_cone_limits_enabled?", VALUEFUNC(MSNewton::BallAndSocket::are_cone_limits_enabled), 1);
	rb_define_module_function(mBallAndSocket, "get_min_twist_angle", VALUEFUNC(MSNewton::BallAndSocket::get_min_twist_angle), 1);
	rb_define_module_function(mBallAndSocket, "set_min_twist_angle", VALUEFUNC(MSNewton::BallAndSocket::set_min_twist_angle), 2);
	rb_define_module_function(mBallAndSocket, "get_max_twist_angle", VALUEFUNC(MSNewton::BallAndSocket::get_max_twist_angle), 1);
	rb_define_module_function(mBallAndSocket, "set_max_twist_angle", VALUEFUNC(MSNewton::BallAndSocket::set_max_twist_angle), 2);
	rb_define_module_function(mBallAndSocket, "enable_twist_limits", VALUEFUNC(MSNewton::BallAndSocket::enable_twist_limits), 2);
	rb_define_module_function(mBallAndSocket, "are_twist_limits_enabled?", VALUEFUNC(MSNewton::BallAndSocket::are_twist_limits_enabled), 1);
	rb_define_module_function(mBallAndSocket, "get_cur_cone_angle", VALUEFUNC(MSNewton::BallAndSocket::get_cur_cone_angle), 1);
	rb_define_module_function(mBallAndSocket, "get_cur_twist_angle", VALUEFUNC(MSNewton::BallAndSocket::get_cur_twist_angle), 1);
}
