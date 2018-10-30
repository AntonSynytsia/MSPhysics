#ifndef MSP_BODIES_H
#define MSP_BODIES_H

#include "msp.h"

class MSP::Bodies {
public:
	// Ruby Functions
	static VALUE rbf_aabb_overlap(VALUE self, VALUE v_body1, VALUE v_body2);
	static VALUE rbf_collidable(VALUE self, VALUE v_body1, VALUE v_body2);
	static VALUE rbf_touching(VALUE self, VALUE v_body1, VALUE v_body2);
	static VALUE rbf_get_closest_points(VALUE self, VALUE v_body1, VALUE v_body2);
	static VALUE rbf_get_force_in_between(VALUE self, VALUE v_body1, VALUE v_body2);

	// Main
	static void init_ruby(VALUE mNewton);
};

#endif	/* MSP_BODIES_H */
