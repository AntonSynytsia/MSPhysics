#include "msp_gear.h"
#include "msp_joint.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Gear::DEFAULT_RATIO(1.0f);


/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

std::set<MSP::Gear::GearData*> MSP::Gear::s_valid_gears;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

bool MSP::Gear::c_is_gear_valid(GearData* address) {
	return s_valid_gears.find(address) != s_valid_gears.end();
}

VALUE MSP::Gear::c_gear_to_value(GearData* gear_data) {
	return rb_ull2inum(reinterpret_cast<unsigned long long>(gear_data));
}

MSP::Gear::GearData* MSP::Gear::c_value_to_gear(VALUE v_gear) {
	GearData* address = reinterpret_cast<GearData*>(rb_num2ull(v_gear));
	if (Util::s_validate_objects && s_valid_gears.find(address) == s_valid_gears.end())
		rb_raise(rb_eTypeError, "Given address doesn't reference a valid gear!");
	return address;
}

MSP::Gear::GearData* MSP::Gear::c_create(const NewtonWorld* world, MSP::Joint::JointData* joint_data1, MSP::Joint::JointData* joint_data2) {
	if (joint_data1->m_world != world || joint_data2->m_world != world)
		rb_raise(rb_eTypeError, "One or more of the given joints don't associate with the given world!");
	if (joint_data1 == joint_data2)
		rb_raise(rb_eTypeError, "The given joints may not reference the same joint!");
	if (!c_are_joints_gearable(joint_data1, joint_data2) && !c_are_joints_gearable(joint_data2, joint_data1))
		rb_raise(rb_eTypeError, "Cannot create gear between %s and %s!", MSP::Joint::JOINT_NAMES[joint_data1->m_jtype], MSP::Joint::JOINT_NAMES[joint_data2->m_jtype]);
	GearData* gear_data = new GearData(world, joint_data1, joint_data2, 0.0f, 0.0f);
	s_valid_gears.insert(gear_data);
	return gear_data;
}

void MSP::Gear::c_destroy(GearData* gear_data) {
	if (s_valid_gears.find(gear_data) != s_valid_gears.end())
		s_valid_gears.erase(gear_data);
	delete gear_data;
}

bool MSP::Gear::c_are_joints_gearable(MSP::Joint::JointData* joint_data1, MSP::Joint::JointData* joint_data2) {
	switch (joint_data1->m_jtype) {
		case MSP::Joint::HINGE:
			switch (joint_data2->m_jtype) {
				case MSP::Joint::HINGE:
				case MSP::Joint::MOTOR:
				case MSP::Joint::SERVO:
				case MSP::Joint::SLIDER:
				case MSP::Joint::PISTON:
				case MSP::Joint::SPRING:
				case MSP::Joint::CURVY_SLIDER:
				case MSP::Joint::CURVY_PISTON:
					return true;
			}
			break;
		case MSP::Joint::SLIDER:
			switch (joint_data2->m_jtype) {
				case MSP::Joint::MOTOR:
				case MSP::Joint::SERVO:
				case MSP::Joint::SLIDER:
				case MSP::Joint::PISTON:
				case MSP::Joint::SPRING:
				case MSP::Joint::CURVY_SLIDER:
				case MSP::Joint::CURVY_PISTON:
					return true;
			}
			break;
		case MSP::Joint::CURVY_SLIDER:
			switch (joint_data2->m_jtype) {
				case MSP::Joint::MOTOR:
				case MSP::Joint::SERVO:
				case MSP::Joint::PISTON:
				case MSP::Joint::SPRING:
				case MSP::Joint::CURVY_SLIDER:
				case MSP::Joint::CURVY_PISTON:
					return true;
			}
			break;
	}
	return false;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Gear::rbf_is_valid(VALUE self, VALUE v_gear) {
	return c_is_gear_valid(reinterpret_cast<GearData*>(Util::value_to_ull(v_gear))) ? Qtrue : Qfalse;
}

VALUE MSP::Gear::rbf_create(VALUE self, VALUE v_world, VALUE v_joint1, VALUE v_joint2) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	MSP::Joint::JointData* joint_data1 = MSP::Joint::c_value_to_joint(v_joint1);
	MSP::Joint::JointData* joint_data2 = MSP::Joint::c_value_to_joint(v_joint2);
	return c_gear_to_value(c_create(world, joint_data1, joint_data2));
}

VALUE MSP::Gear::rbf_destroy(VALUE self, VALUE v_gear) {
	GearData* gear_data = c_value_to_gear(v_gear);
	c_destroy(gear_data);
	return Qnil;
}

VALUE MSP::Gear::rbf_get_user_data(VALUE self, VALUE v_gear) {
	GearData* gear_data = c_value_to_gear(v_gear);
	return gear_data->m_user_data;
}

VALUE MSP::Gear::rbf_set_user_data(VALUE self, VALUE v_gear, VALUE v_user_data) {
	GearData* gear_data = c_value_to_gear(v_gear);
	gear_data->m_user_data = v_user_data;
	return Qnil;
}

VALUE MSP::Gear::rbf_get_ratio(VALUE self, VALUE v_gear) {
	GearData* gear_data = c_value_to_gear(v_gear);
	return Util::to_value(gear_data->m_ratio);
}

VALUE MSP::Gear::rbf_set_ratio(VALUE self, VALUE v_gear, VALUE v_ratio) {
	GearData* gear_data = c_value_to_gear(v_gear);
	gear_data->m_ratio = Util::value_to_dFloat(v_ratio);
	return Qnil;
}

VALUE MSP::Gear::rbf_get_joint1(VALUE self, VALUE v_gear) {
	GearData* gear_data = c_value_to_gear(v_gear);
	return MSP::Joint::c_joint_to_value(gear_data->m_joint_data1);
}

VALUE MSP::Gear::rbf_get_joint2(VALUE self, VALUE v_gear) {
	GearData* gear_data = c_value_to_gear(v_gear);
	return MSP::Joint::c_joint_to_value(gear_data->m_joint_data2);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSP::Gear::init_ruby(VALUE mNewton) {
	VALUE mGear = rb_define_module_under(mNewton, "Gear");

	rb_define_module_function(mGear, "is_valid?", VALUEFUNC(MSP::Gear::rbf_is_valid), 1);
	rb_define_module_function(mGear, "create", VALUEFUNC(MSP::Gear::rbf_create), 3);
	rb_define_module_function(mGear, "destroy", VALUEFUNC(MSP::Gear::rbf_destroy), 1);
	rb_define_module_function(mGear, "get_user_data", VALUEFUNC(MSP::Gear::rbf_get_user_data), 1);
	rb_define_module_function(mGear, "set_user_data", VALUEFUNC(MSP::Gear::rbf_set_user_data), 2);
	rb_define_module_function(mGear, "get_ratio", VALUEFUNC(MSP::Gear::rbf_get_ratio), 1);
	rb_define_module_function(mGear, "set_ratio", VALUEFUNC(MSP::Gear::rbf_set_ratio), 2);
	rb_define_module_function(mGear, "get_joint1", VALUEFUNC(MSP::Gear::rbf_get_joint1), 1);
	rb_define_module_function(mGear, "get_joint2", VALUEFUNC(MSP::Gear::rbf_get_joint2), 1);
}
