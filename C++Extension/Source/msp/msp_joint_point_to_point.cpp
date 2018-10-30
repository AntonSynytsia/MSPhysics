#include "msp_joint_point_to_point.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::PointToPoint::DEFAULT_ACCEL(40.0f);
const dFloat MSP::PointToPoint::DEFAULT_DAMP(0.1f);
const dFloat MSP::PointToPoint::DEFAULT_STRENGTH(0.8f);
const int MSP::PointToPoint::DEFAULT_MODE(0);
const dFloat MSP::PointToPoint::DEFAULT_START_DISTANCE(0.0f);
const dFloat MSP::PointToPoint::DEFAULT_CONTROLLER(1.0f);


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSP::PointToPoint::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
	MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);

	dFloat inv_timestep = 1.0f / timestep;

	dMatrix matrix0, matrix1;
	MSP::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

	dVector p0(matrix0.m_posit + matrix0.m_right.Scale(cj_data->m_start_distance));
	const dVector& p1 = matrix1.m_posit;

	dVector veloc0(0.0f);
	dVector veloc1(0.0f);
	NewtonBodyGetVelocity(joint_data->m_child, &veloc0[0]);
	if (joint_data->m_parent != nullptr)
		NewtonBodyGetVelocity(joint_data->m_parent, &veloc1[0]);
	dVector rel_veloc(veloc0 - veloc1);

	dFloat stiffness = 0.999f - (1.0f - joint_data->m_stiffness_ratio * cj_data->m_strength) * Joint::DEFAULT_STIFFNESS_RANGE;

	if (cj_data->m_mode == 0) {
		dVector dir(p0 - p1);
		cj_data->m_cur_distance = Util::get_vector_magnitude(dir);
		if (cj_data->m_cur_distance > M_EPSILON)
			Util::scale_vector(dir, 1.0f / cj_data->m_cur_distance);
		else {
			dir.m_x = 0.0f;
			dir.m_y = 0.0f;
			dir.m_z = 1.0f;
		}
		dFloat offset = cj_data->m_cur_distance - cj_data->m_start_distance * cj_data->m_controller;
		dFloat dir_veloc = rel_veloc.DotProduct3(dir);
		dFloat des_accel = cj_data->m_accel * -offset - dir_veloc * inv_timestep * cj_data->m_damp;
		NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &dir[0]);
		NewtonUserJointSetRowAcceleration(joint, des_accel);
		NewtonUserJointSetRowStiffness(joint, stiffness);
	}
	else {
		dVector offset(matrix1.UntransformVector(p0));
		cj_data->m_cur_distance = Util::get_vector_magnitude(offset);
		offset.m_z -= cj_data->m_start_distance * cj_data->m_controller;
		dVector loc_vel(matrix1.UnrotateVector(rel_veloc));
		dVector des_accel(offset.Scale(-cj_data->m_accel) - loc_vel.Scale(inv_timestep * cj_data->m_damp));

		NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_front[0]);
		NewtonUserJointSetRowAcceleration(joint, des_accel.m_x);
		NewtonUserJointSetRowStiffness(joint, stiffness);

		NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_up[0]);
		NewtonUserJointSetRowAcceleration(joint, des_accel.m_y);
		NewtonUserJointSetRowStiffness(joint, stiffness);

		NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix1.m_right[0]);
		NewtonUserJointSetRowAcceleration(joint, des_accel.m_z);
		NewtonUserJointSetRowStiffness(joint, stiffness);
	}
}

void MSP::PointToPoint::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
}

void MSP::PointToPoint::on_destroy(MSP::Joint::JointData* joint_data) {
	delete (reinterpret_cast<PointToPointData*>(joint_data->m_cj_data));
}

void MSP::PointToPoint::on_connect(MSP::Joint::JointData* joint_data) {
}

void MSP::PointToPoint::on_disconnect(MSP::Joint::JointData* joint_data) {
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	cj_data->m_cur_distance = 0.0f;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::PointToPoint::rbf_is_valid(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
	return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::POINT_TO_POINT) ? Qtrue : Qfalse;
}

VALUE MSP::PointToPoint::rbf_create(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

	joint_data->m_dof = 3;
	joint_data->m_jtype = MSP::Joint::POINT_TO_POINT;
	joint_data->m_cj_data = new PointToPointData();
	joint_data->m_submit_constraints = submit_constraints;
	joint_data->m_get_info = get_info;
	joint_data->m_on_destroy = on_destroy;
	joint_data->m_on_connect = on_connect;
	joint_data->m_on_disconnect = on_disconnect;

	return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::PointToPoint::rbf_get_accel(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_accel);
}

VALUE MSP::PointToPoint::rbf_set_accel(VALUE self, VALUE v_joint, VALUE v_accel) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	cj_data->m_accel = Util::max_float(Util::value_to_dFloat(v_accel), 0.0f);
	return Qnil;
}

VALUE MSP::PointToPoint::rbf_get_damp(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_damp);
}

VALUE MSP::PointToPoint::rbf_set_damp(VALUE self, VALUE v_joint, VALUE v_damp) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	cj_data->m_damp = Util::clamp_float(Util::value_to_dFloat(v_damp), 0.0f, 1.0f);
	return Qnil;
}

VALUE MSP::PointToPoint::rbf_get_strength(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_strength);
}

VALUE MSP::PointToPoint::rbf_set_strength(VALUE self, VALUE v_joint, VALUE v_strength) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	cj_data->m_strength = Util::clamp_float(Util::value_to_dFloat(v_strength), 0.0f, 1.0f);
	return Qnil;
}

VALUE MSP::PointToPoint::rbf_get_mode(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_mode);
}

VALUE MSP::PointToPoint::rbf_set_mode(VALUE self, VALUE v_joint, VALUE v_mode) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	cj_data->m_mode = Util::value_to_int(v_mode) == 1 ? 1 : 0;
	return Qnil;
}

VALUE MSP::PointToPoint::rbf_get_start_distance(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_start_distance * M_INCH_TO_METER);
}

VALUE MSP::PointToPoint::rbf_set_start_distance(VALUE self, VALUE v_joint, VALUE v_distance) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	dFloat des_start_distance = Util::max_float(Util::value_to_dFloat(v_distance) * M_METER_TO_INCH, 0.0f);
	if (des_start_distance != cj_data->m_start_distance) {
		cj_data->m_start_distance = des_start_distance;
		if (joint_data->m_connected)
			NewtonBodySetSleepState(joint_data->m_child, 0);
	}
	return Qnil;
}

VALUE MSP::PointToPoint::rbf_get_controller(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_controller);
}

VALUE MSP::PointToPoint::rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	dFloat desired_controller = Util::max_float(Util::value_to_dFloat(v_controller), 0.0f);
	if (desired_controller != cj_data->m_controller) {
		cj_data->m_controller = desired_controller;
		if (joint_data->m_connected)
			NewtonBodySetSleepState(joint_data->m_child, 0);
	}
	return Qnil;
}

VALUE MSP::PointToPoint::rbf_get_cur_distance(VALUE self, VALUE v_joint) {
	MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::POINT_TO_POINT);
	PointToPointData* cj_data = reinterpret_cast<PointToPointData*>(joint_data->m_cj_data);
	return Util::to_value(cj_data->m_cur_distance * M_INCH_TO_METER);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSP::PointToPoint::init_ruby(VALUE mNewton) {
	VALUE mPointToPoint = rb_define_module_under(mNewton, "PointToPoint");

	rb_define_module_function(mPointToPoint, "is_valid?", VALUEFUNC(MSP::PointToPoint::rbf_is_valid), 1);
	rb_define_module_function(mPointToPoint, "create", VALUEFUNC(MSP::PointToPoint::rbf_create), 1);

	rb_define_module_function(mPointToPoint, "get_accel", VALUEFUNC(MSP::PointToPoint::rbf_get_accel), 1);
	rb_define_module_function(mPointToPoint, "set_accel", VALUEFUNC(MSP::PointToPoint::rbf_set_accel), 2);
	rb_define_module_function(mPointToPoint, "get_damp", VALUEFUNC(MSP::PointToPoint::rbf_get_damp), 1);
	rb_define_module_function(mPointToPoint, "set_damp", VALUEFUNC(MSP::PointToPoint::rbf_set_damp), 2);
	rb_define_module_function(mPointToPoint, "get_strength", VALUEFUNC(MSP::PointToPoint::rbf_get_strength), 1);
	rb_define_module_function(mPointToPoint, "set_strength", VALUEFUNC(MSP::PointToPoint::rbf_set_strength), 2);

	rb_define_module_function(mPointToPoint, "get_mode", VALUEFUNC(MSP::PointToPoint::rbf_get_mode), 1);
	rb_define_module_function(mPointToPoint, "set_mode", VALUEFUNC(MSP::PointToPoint::rbf_set_mode), 2);

	rb_define_module_function(mPointToPoint, "get_start_distance", VALUEFUNC(MSP::PointToPoint::rbf_get_start_distance), 1);
	rb_define_module_function(mPointToPoint, "set_start_distance", VALUEFUNC(MSP::PointToPoint::rbf_set_start_distance), 2);

	rb_define_module_function(mPointToPoint, "get_controller", VALUEFUNC(MSP::PointToPoint::rbf_get_controller), 1);
	rb_define_module_function(mPointToPoint, "set_controller", VALUEFUNC(MSP::PointToPoint::rbf_set_controller), 2);

	rb_define_module_function(mPointToPoint, "get_cur_distance", VALUEFUNC(MSP::PointToPoint::rbf_get_cur_distance), 1);
}
