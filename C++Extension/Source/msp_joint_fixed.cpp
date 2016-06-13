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
	dMatrix matrix0, matrix1, matrix2;
	MSNewton::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1, matrix2);

	const dVector& p0 = matrix0.m_posit;
	const dVector& p1 = matrix1.m_posit;
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

void MSNewton::Fixed::adjust_pin_matrix_proc(JointData* joint_data, dMatrix& pin_matrix) {
	dMatrix matrix;
	dVector ccentre;
	NewtonBodyGetMatrix(joint_data->child, &matrix[0][0]);
	NewtonBodyGetCentreOfMass(joint_data->child, &ccentre[0]);
	ccentre = matrix.TransformVector(ccentre);
	if (joint_data->parent != nullptr) {
		/*NewtonBodyGetMatrix(joint_data->parent, &matrix[0][0]);
		dVector pcentre;
		NewtonBodyGetCentreOfMass(joint_data->parent, &pcentre[0]);
		pcentre = matrix.TransformVector(pcentre);
		pin_matrix.m_posit.m_x = (ccentre.m_x + pcentre.m_x) * 0.5f;
		pin_matrix.m_posit.m_y = (ccentre.m_y + pcentre.m_y) * 0.5f;
		pin_matrix.m_posit.m_z = (ccentre.m_z + pcentre.m_z) * 0.5f;*/
	}
	else {
		pin_matrix.m_posit = ccentre;
	}
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
	//~ joint_data->adjust_pin_matrix_proc = adjust_pin_matrix_proc;

	return Util::to_value(joint_data);
}


void Init_msp_fixed(VALUE mNewton) {
	VALUE mFixed = rb_define_module_under(mNewton, "Fixed");

	rb_define_module_function(mFixed, "is_valid?", VALUEFUNC(MSNewton::Fixed::is_valid), 1);
	rb_define_module_function(mFixed, "create", VALUEFUNC(MSNewton::Fixed::create), 1);
}
