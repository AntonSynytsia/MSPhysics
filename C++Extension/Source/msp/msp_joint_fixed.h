#ifndef MSP_FIXED_H
#define MSP_FIXED_H

#include "msp.h"
#include "msp_joint.h"

class MSP::Fixed {
private:
	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);

public:
	// Ruby Functions
	static VALUE rbf_is_valid(VALUE self, VALUE v_joint);
	static VALUE rbf_create(VALUE self, VALUE v_joint);

	// Main
	static void init_ruby(VALUE mNewton);
};

#endif	/* MSP_FIXED_H */
