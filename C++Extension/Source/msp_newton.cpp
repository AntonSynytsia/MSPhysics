#include "msp_newton.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSPhysics::Newton::get_version(VALUE self) {
	//return rb_sprintf("%d.%d", NEWTON_MAJOR_VERSION, NEWTON_MINOR_VERSION);
	char version_str[10];
	sprintf(version_str, "%d.%d", NEWTON_MAJOR_VERSION, NEWTON_MINOR_VERSION);
	return rb_str_new2(version_str);
}

VALUE MSPhysics::Newton::get_float_size(VALUE self) {
	return Util::to_value( NewtonWorldFloatSize() );
}

VALUE MSPhysics::Newton::get_memory_used(VALUE self) {
	return Util::to_value( NewtonGetMemoryUsed() );
}

VALUE MSPhysics::Newton::get_all_worlds(VALUE self) {
	VALUE v_worlds = rb_ary_new2((long)valid_worlds.size());
	int count = 0;
	for (std::map<const NewtonWorld*, bool>::iterator it = valid_worlds.begin(); it != valid_worlds.end(); ++it) {
		rb_ary_store(v_worlds, count, Util::to_value(it->first));
		++count;
	}
	return v_worlds;
}

VALUE MSPhysics::Newton::get_all_bodies(VALUE self) {
	VALUE v_bodies = rb_ary_new2((long)valid_bodies.size());
	int count = 0;
	for (std::map<const NewtonBody*, bool>::iterator it = valid_bodies.begin(); it != valid_bodies.end(); ++it) {
		rb_ary_store(v_bodies, count, Util::to_value(it->first));
		++count;
	}
	return v_bodies;
}

VALUE MSPhysics::Newton::get_all_joints(VALUE self) {
	VALUE v_joints = rb_ary_new2((long)valid_bodies.size());
	int count = 0;
	for (std::map<JointData*, bool>::iterator it = valid_joints.begin(); it != valid_joints.end(); ++it) {
		rb_ary_store(v_joints, count, Util::to_value(it->first));
		++count;
	}
	return v_joints;
}

VALUE MSPhysics::Newton::enable_object_validation(VALUE self, VALUE v_state) {
	validate_objects = Util::value_to_bool(v_state);
	return Util::to_value(validate_objects);
}

VALUE MSPhysics::Newton::is_object_validation_enabled(VALUE self) {
	return Util::to_value(validate_objects);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void Init_msp_newton(VALUE mNewton) {
	rb_define_module_function(mNewton, "get_version", VALUEFUNC(MSPhysics::Newton::get_version), 0);
	rb_define_module_function(mNewton, "get_float_size", VALUEFUNC(MSPhysics::Newton::get_float_size), 0);
	rb_define_module_function(mNewton, "get_memory_used", VALUEFUNC(MSPhysics::Newton::get_memory_used), 0);
	rb_define_module_function(mNewton, "get_all_worlds", VALUEFUNC(MSPhysics::Newton::get_all_worlds), 0);
	rb_define_module_function(mNewton, "get_all_bodies", VALUEFUNC(MSPhysics::Newton::get_all_bodies), 0);
	rb_define_module_function(mNewton, "get_all_joints", VALUEFUNC(MSPhysics::Newton::get_all_joints), 0);
	rb_define_module_function(mNewton, "enable_object_validation", VALUEFUNC(MSPhysics::Newton::enable_object_validation), 1);
	rb_define_module_function(mNewton, "is_object_validation_enabled?", VALUEFUNC(MSPhysics::Newton::is_object_validation_enabled), 0);
}
