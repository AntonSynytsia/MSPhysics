#include "msp_joint_fixed.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Fixed::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);

	// Calculate position of pivot points and Jacobian direction vectors in global space.
	dMatrix matrix0;
	dMatrix matrix1;
	Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1);

	dVector p0(matrix0.m_posit);
	dVector p1(matrix1.m_posit);
	// Get a point along the pin axis at some reasonable large distance from the pivot.
	dVector q0(p0 + matrix0.m_right.Scale(MIN_JOINT_PIN_LENGTH));
	dVector q1(p1 + matrix1.m_right.Scale(MIN_JOINT_PIN_LENGTH));
	// Get the ankle point.
	dVector r0(p0 + matrix0.m_front.Scale(MIN_JOINT_PIN_LENGTH));
	dVector r1(p1 + matrix1.m_front.Scale(MIN_JOINT_PIN_LENGTH));

	// Restrict movement on the pivot point along all three orthonormal directions
	NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_front[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix0.m_right[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Restrict rotation along all three orthonormal directions
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

	NewtonUserJointAddLinearRow(joint, &r0[0], &r1[0], &matrix0.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
}

void MSNewton::Fixed::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
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
	info->m_minAngularDof[2] = -0.0f;
	info->m_maxAngularDof[2] = 0.0f;
}

void MSNewton::Fixed::on_destroy(JointData* joint_data) {
	FixedData* cj_data = (FixedData*)joint_data->cj_data;
	delete cj_data;
}

void MSNewton::Fixed::on_connect(JointData* joint_data) {
}

void MSNewton::Fixed::on_disconnect(JointData* joint_data) {
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::Fixed::is_valid(VALUE self, VALUE v_joint) {
	JointData* address = (JointData*)Util::value_to_ll(v_joint);
	bool valid = Util::is_joint_valid(address);
	if (valid && address->jtype != JT_FIXED) valid = false;
	return Util::to_value(valid);
}

VALUE MSNewton::Fixed::create(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_NONE);

	FixedData* cj_data = new FixedData;

	joint_data->dof = 6;
	joint_data->jtype = JT_FIXED;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_connect = on_connect;
	joint_data->on_disconnect = on_disconnect;

	return Util::to_value(joint_data);
}


void Init_msp_fixed(VALUE mNewton) {
	VALUE mFixed = rb_define_module_under(mNewton, "Fixed");

	rb_define_module_function(mFixed, "is_valid?", VALUEFUNC(MSNewton::Fixed::is_valid), 1);
	rb_define_module_function(mFixed, "create", VALUEFUNC(MSNewton::Fixed::create), 1);
}
