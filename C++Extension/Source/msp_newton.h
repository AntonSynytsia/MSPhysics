#ifndef MSP_NEWTON_H
#define MSP_NEWTON_H

#include "msp_util.h"

namespace MSPhysics {
	class Newton;
}

class MSPhysics::Newton {
public:
	// Ruby Functions
	static VALUE get_version(VALUE self);
	static VALUE get_float_size(VALUE self);
	static VALUE get_memory_used(VALUE self);
	static VALUE get_all_worlds(VALUE self);
	static VALUE get_all_bodies(VALUE self);
	static VALUE get_all_joints(VALUE self);
	static VALUE enable_object_validation(VALUE self, VALUE v_state);
	static VALUE is_object_validation_enabled(VALUE self);
};

void Init_msp_newton(VALUE mNewton);

#endif  /* MSP_NEWTON_H */
