#ifndef MSP_BODIES_H
#define MSP_BODIES_H

#include "msp_util.h"

namespace MSNewton {
	class Bodies;
}

class MSNewton::Bodies {
public:
	// Ruby Functions
	static VALUE aabb_overlap(VALUE self, VALUE v_body1, VALUE v_body2);
	static VALUE collidable(VALUE self, VALUE v_body1, VALUE v_body2);
	static VALUE touching(VALUE self, VALUE v_body1, VALUE v_body2);
	static VALUE get_closest_points(VALUE self, VALUE v_body1, VALUE v_body2);
	static VALUE get_force_in_between(VALUE self, VALUE v_body1, VALUE v_body2);
};

void Init_msp_bodies(VALUE mNewton);

#endif	/* MSP_BODIES_H */
