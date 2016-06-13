#include "msp_joint_ball_and_socket.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::BallAndSocket::DEFAULT_MAX_CONE_ANGLE = 30.0f * DEG_TO_RAD;
const bool MSNewton::BallAndSocket::DEFAULT_CONE_LIMITS_ENABLED = false;
const dFloat MSNewton::BallAndSocket::DEFAULT_MIN_TWIST_ANGLE = -180.0f * DEG_TO_RAD;
const dFloat MSNewton::BallAndSocket::DEFAULT_MAX_TWIST_ANGLE = 180.0f * DEG_TO_RAD;
const bool MSNewton::BallAndSocket::DEFAULT_TWIST_LIMITS_ENABLED = false;
const dFloat MSNewton::BallAndSocket::DEFAULT_FRICTION = 0.0f;
const dFloat MSNewton::BallAndSocket::DEFAULT_CONTROLLER = 1.0f;

/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::BallAndSocket::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;

	// Calculate the position of the pivot point and the Jacobian direction vectors, in global space.
	dMatrix matrix0, matrix1, matrix2;
	MSNewton::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1, matrix2);

	// Calculate current cone angle.
	const dVector& cone_dir0 = matrix0.m_right;
	const dVector& cone_dir1 = matrix1.m_right;
	dFloat cur_cone_angle_cos = cone_dir0 % cone_dir1;
	cj_data->cur_cone_angle = dAcos(cur_cone_angle_cos);
	dVector lateral_dir;
	if (dAbs(cur_cone_angle_cos) > 0.99995f)
		lateral_dir = matrix0.m_up;
	else {
		lateral_dir = cone_dir0 * cone_dir1;
		Util::normalize_vector(lateral_dir);
	}
	// Calculate current twist angle.
	dFloat sin_angle;
	dFloat cos_angle;
	if (dAbs(cur_cone_angle_cos) > 0.99995f) {
		// No need to unrotate front vector if the current cone angle is near zero.
		MSNewton::Joint::c_calculate_angle(matrix1.m_front, matrix0.m_front, matrix0.m_right, sin_angle, cos_angle);
	}
	else {
		// Unrotate the front vector in order to calculate the twist angle.
		dVector front = Util::rotate_vector(matrix0.m_front, lateral_dir, -cj_data->cur_cone_angle);
		MSNewton::Joint::c_calculate_angle(matrix1.m_front, front, matrix1.m_right, sin_angle, cos_angle);
	}
	cj_data->twist_ai->update(cos_angle, sin_angle);
	dFloat cur_twist_angle = cj_data->twist_ai->get_angle();

	// Restrict the movement on the pivot point along all tree orthonormal directions.
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

	// Calculate friction
	dFloat power = cj_data->friction * dAbs(cj_data->controller);
	/*BodyData* cbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
	if (cbody_data->bstatic == false && cbody_data->mass >= MIN_MASS)
		power *= cbody_data->mass;
	else {
		BodyData* pbody_data = (BodyData*)NewtonBodyGetUserData(joint_data->child);
		if (pbody_data->bstatic == false && pbody_data->mass >= MIN_MASS) power *= pbody_data->mass;
	}*/

	// Handle cone angle
	if (cj_data->cone_limits_enabled == true && (cj_data->max_cone_angle < 1.0e-4f)) {
		// Handle in case joint being a hinge; max cone angle is near zero.
		NewtonUserJointAddAngularRow(joint, MSNewton::Joint::c_calculate_angle(matrix0.m_right, matrix1.m_right, matrix1.m_front), &matrix1.m_front[0]);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

		NewtonUserJointAddAngularRow(joint, MSNewton::Joint::c_calculate_angle(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_up[0]);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->cone_limits_enabled == true && cj_data->cur_cone_angle > cj_data->max_cone_angle) {
		// Handle in case current cone angle is greater than max cone angle
		NewtonUserJointAddAngularRow(joint, cj_data->cur_cone_angle - cj_data->max_cone_angle, &lateral_dir[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
		dVector front_dir = lateral_dir * matrix0.m_right;
		NewtonUserJointAddAngularRow(joint, 0.0f, &front_dir[0]);
		NewtonUserJointSetRowMinimumFriction(joint, -power);
		NewtonUserJointSetRowMaximumFriction(joint, power);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		// Handle in case limits are not necessary
		dVector front_dir = lateral_dir * matrix0.m_right;
		NewtonUserJointAddAngularRow(joint, 0.0f, &lateral_dir[0]);
		NewtonUserJointSetRowMinimumFriction(joint, -power);
		NewtonUserJointSetRowMaximumFriction(joint, power);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
		NewtonUserJointAddAngularRow(joint, 0.0f, &front_dir[0]);
		NewtonUserJointSetRowMinimumFriction(joint, -power);
		NewtonUserJointSetRowMaximumFriction(joint, power);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}

	// Handle twist angle
	if (cj_data->twist_limits_enabled == true && cj_data->min_twist_angle > cj_data->max_twist_angle) {
		// Handle in case min angle is greater than max
		NewtonUserJointAddAngularRow(joint, cur_twist_angle, &matrix0.m_right[0]);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->twist_limits_enabled == true && (cj_data->max_twist_angle - cj_data->min_twist_angle) < 1.0e-4f) {
		// Handle in case min angle is almost equal to max
		NewtonUserJointAddAngularRow(joint, cj_data->max_twist_angle - cur_twist_angle, &matrix0.m_right[0]);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->twist_limits_enabled == true && cur_twist_angle < cj_data->min_twist_angle - Joint::ANGULAR_LIMIT_EPSILON) {
		// Handle in case current twist angle is less than min
		NewtonUserJointAddAngularRow(joint, cj_data->min_twist_angle - cur_twist_angle, &matrix0.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cj_data->twist_limits_enabled == true && cur_twist_angle > cj_data->max_twist_angle + Joint::ANGULAR_LIMIT_EPSILON) {
		// Handle in case current twist angle is greater than max
		NewtonUserJointAddAngularRow(joint, cj_data->max_twist_angle - cur_twist_angle, &matrix0.m_right[0]);
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		// Handle in case limits are not necessary
		NewtonUserJointAddAngularRow(joint, 0.0f, &matrix0.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, -power);
		NewtonUserJointSetRowMaximumFriction(joint, power);
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

void MSNewton::BallAndSocket::on_disconnect(JointData* data) {
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	cj_data->cur_cone_angle = 0.0f;
	cj_data->twist_ai->set_angle(0.0f);
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
	cj_data->twist_ai = new AngularIntegration();
	cj_data->cone_limits_enabled = DEFAULT_CONE_LIMITS_ENABLED;
	cj_data->twist_limits_enabled = DEFAULT_TWIST_LIMITS_ENABLED;
	cj_data->cone_angle_cos = dCos(cj_data->max_cone_angle);
	cj_data->cone_angle_sin = dSin(cj_data->max_cone_angle);
	cj_data->cone_angle_half_cos = dCos(cj_data->max_cone_angle * 0.5f);
	cj_data->cone_angle_half_sin = dSin(cj_data->max_cone_angle * 0.5f);
	cj_data->friction = DEFAULT_FRICTION;
	cj_data->controller = DEFAULT_CONTROLLER;

	data->dof = 6;
	data->jtype = JT_BALL_AND_SOCKET;
	data->cj_data = cj_data;
	data->submit_constraints = submit_constraints;
	data->get_info = get_info;
	data->on_destroy = on_destroy;
	data->on_disconnect = on_disconnect;

	return Util::to_value(data);
}

VALUE MSNewton::BallAndSocket::get_max_cone_angle(VALUE self, VALUE v_joint) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	return Util::to_value(cj_data->max_cone_angle);
}

VALUE MSNewton::BallAndSocket::set_max_cone_angle(VALUE self, VALUE v_joint, VALUE v_angle) {
	JointData* data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)data->cj_data;
	cj_data->max_cone_angle = Util::clamp<dFloat>(Util::value_to_dFloat(v_angle), 0.0f, PI);
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

VALUE MSNewton::BallAndSocket::cone_limits_enabled(VALUE self, VALUE v_joint) {
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

VALUE MSNewton::BallAndSocket::twist_limits_enabled(VALUE self, VALUE v_joint) {
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
	return Util::to_value(cj_data->twist_ai->get_angle());
}

VALUE MSNewton::BallAndSocket::get_friction(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->friction * world_data->inverse_scale5);
}

VALUE MSNewton::BallAndSocket::set_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->friction = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_friction), 0.0f) * world_data->scale5;
	return Util::to_value(cj_data->friction * world_data->inverse_scale5);
}

VALUE MSNewton::BallAndSocket::get_controller(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;
	return Util::to_value(cj_data->controller);
}

VALUE MSNewton::BallAndSocket::set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_BALL_AND_SOCKET);
	BallAndSocketData* cj_data = (BallAndSocketData*)joint_data->cj_data;
	dFloat desired_controller = Util::value_to_dFloat(v_controller);
	if (cj_data->controller != desired_controller) {
		cj_data->controller = desired_controller;
		if (joint_data->connected)
			NewtonBodySetSleepState(joint_data->child, 0);
	}
	return Util::to_value(cj_data->controller);
}


void Init_msp_ball_and_socket(VALUE mNewton) {
	VALUE mBallAndSocket = rb_define_module_under(mNewton, "BallAndSocket");

	rb_define_module_function(mBallAndSocket, "is_valid?", VALUEFUNC(MSNewton::BallAndSocket::is_valid), 1);
	rb_define_module_function(mBallAndSocket, "create", VALUEFUNC(MSNewton::BallAndSocket::create), 1);
	rb_define_module_function(mBallAndSocket, "get_max_cone_angle", VALUEFUNC(MSNewton::BallAndSocket::get_max_cone_angle), 1);
	rb_define_module_function(mBallAndSocket, "set_max_cone_angle", VALUEFUNC(MSNewton::BallAndSocket::set_max_cone_angle), 2);
	rb_define_module_function(mBallAndSocket, "enable_cone_limits", VALUEFUNC(MSNewton::BallAndSocket::enable_cone_limits), 2);
	rb_define_module_function(mBallAndSocket, "cone_limits_enabled?", VALUEFUNC(MSNewton::BallAndSocket::cone_limits_enabled), 1);
	rb_define_module_function(mBallAndSocket, "get_min_twist_angle", VALUEFUNC(MSNewton::BallAndSocket::get_min_twist_angle), 1);
	rb_define_module_function(mBallAndSocket, "set_min_twist_angle", VALUEFUNC(MSNewton::BallAndSocket::set_min_twist_angle), 2);
	rb_define_module_function(mBallAndSocket, "get_max_twist_angle", VALUEFUNC(MSNewton::BallAndSocket::get_max_twist_angle), 1);
	rb_define_module_function(mBallAndSocket, "set_max_twist_angle", VALUEFUNC(MSNewton::BallAndSocket::set_max_twist_angle), 2);
	rb_define_module_function(mBallAndSocket, "enable_twist_limits", VALUEFUNC(MSNewton::BallAndSocket::enable_twist_limits), 2);
	rb_define_module_function(mBallAndSocket, "twist_limits_enabled?", VALUEFUNC(MSNewton::BallAndSocket::twist_limits_enabled), 1);
	rb_define_module_function(mBallAndSocket, "get_cur_cone_angle", VALUEFUNC(MSNewton::BallAndSocket::get_cur_cone_angle), 1);
	rb_define_module_function(mBallAndSocket, "get_cur_twist_angle", VALUEFUNC(MSNewton::BallAndSocket::get_cur_twist_angle), 1);
	rb_define_module_function(mBallAndSocket, "get_friction", VALUEFUNC(MSNewton::BallAndSocket::get_friction), 1);
	rb_define_module_function(mBallAndSocket, "set_friction", VALUEFUNC(MSNewton::BallAndSocket::set_friction), 2);
	rb_define_module_function(mBallAndSocket, "get_controller", VALUEFUNC(MSNewton::BallAndSocket::get_controller), 1);
	rb_define_module_function(mBallAndSocket, "set_controller", VALUEFUNC(MSNewton::BallAndSocket::set_controller), 2);
}
