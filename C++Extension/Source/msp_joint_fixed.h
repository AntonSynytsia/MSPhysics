#ifndef MSP_FIXED_H
#define MSP_FIXED_H

#include "msp_util.h"
#include "msp_joint.h"

namespace MSNewton {
	class Fixed;
}

class MSNewton::Fixed {
private:
	// Variables

public:
	// Structures
	typedef struct FixedData
	{
	} FixedData;

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(JointData* joint_data);
	static void adjust_pin_matrix_proc(JointData* joint_data, dMatrix& pin_matrix);

	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_joint);
	static VALUE create(VALUE self, VALUE v_joint);
};

void Init_msp_fixed(VALUE mNewton);

#endif	/* MSP_FIXED_H */
