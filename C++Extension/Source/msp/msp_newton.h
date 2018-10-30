#ifndef MSP_NEWTON_H
#define MSP_NEWTON_H

#include "msp.h"

class MSP::Newton {
public:
	// Ruby Functions
	static VALUE rbf_get_version(VALUE self);
	static VALUE rbf_get_float_size(VALUE self);
	static VALUE rbf_get_memory_used(VALUE self);
	static VALUE rbf_get_all_worlds(VALUE self);
	static VALUE rbf_get_all_bodies(VALUE self);
	static VALUE rbf_get_all_joints(VALUE self);
	static VALUE rbf_get_all_gears(VALUE self);
	static VALUE rbf_enable_object_validation(VALUE self, VALUE v_state);
	static VALUE rbf_is_object_validation_enabled(VALUE self);

	// Main
	static void init_ruby(VALUE mNewton);
};

#endif	/* MSP_NEWTON_H */
