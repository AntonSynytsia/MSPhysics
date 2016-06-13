#ifndef MSP_UP_VECTOR_H
#define MSP_UP_VECTOR_H

#include "msp_util.h"
#include "msp_joint.h"

namespace MSNewton {
	class UpVector;
}

class MSNewton::UpVector {
private:
	// Variables
	static const dVector DEFAULT_PIN_DIR;
	static const dFloat DEFAULT_ACCEL;
	static const dFloat DEFAULT_DAMP;
	static const bool DEFAULT_DAMPER_ENABLED;

public:
	// Structures
	typedef struct UpVectorData
	{
		dVector pin_dir;
		dMatrix pin_matrix;
		dFloat accel;
		dFloat damp;
		bool damper_enabled;
	} UpVectorData;

	// Callback Functions
	static void submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index);
	static void get_info(const NewtonJoint* const joint, NewtonJointRecord* const info);
	static void on_destroy(JointData* joint_data);

	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_joint);
	static VALUE create(VALUE self, VALUE v_joint);
	static VALUE get_pin_dir(VALUE self, VALUE v_joint);
	static VALUE set_pin_dir(VALUE self, VALUE v_joint, VALUE v_pin_dir);
	static VALUE get_accel(VALUE self, VALUE v_joint);
	static VALUE set_accel(VALUE self, VALUE v_joint, VALUE v_accel);
	static VALUE get_damp(VALUE self, VALUE v_joint);
	static VALUE set_damp(VALUE self, VALUE v_joint, VALUE v_damp);
	static VALUE enable_damper(VALUE self, VALUE v_joint, VALUE v_state);
	static VALUE damper_enabled(VALUE self, VALUE v_joint);
};

void Init_msp_up_vector(VALUE mNewton);

#endif	/* MSP_UP_VECTOR_H */
