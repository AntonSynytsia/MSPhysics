/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_newton.h"
#include "msp_world.h"
#include "msp_body.h"
#include "msp_joint.h"
#include "msp_gear.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Newton::rbf_get_version(VALUE self) {
    //return rb_sprintf("%d.%d", NEWTON_MAJOR_VERSION, NEWTON_MINOR_VERSION);
    char version_str[16];
    sprintf(version_str, "%d.%d", NEWTON_MAJOR_VERSION, NEWTON_MINOR_VERSION);
    return rb_str_new2(version_str);
}

VALUE MSP::Newton::rbf_get_float_size(VALUE self) {
    return Util::to_value( NewtonWorldFloatSize() );
}

VALUE MSP::Newton::rbf_get_memory_used(VALUE self) {
    return Util::to_value( NewtonGetMemoryUsed() );
}

VALUE MSP::Newton::rbf_get_all_worlds(VALUE self) {
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_worlds = rb_ary_new();
    for (std::map<const NewtonWorld*, MSP::World::WorldData*>::iterator it = MSP::World::valid_worlds.begin(); it != MSP::World::valid_worlds.end(); ++it) {
        VALUE v_address = MSP::World::c_world_to_value(it->first);
        if (proc_given) {
            MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(it->first));
            VALUE v_user_data = rb_ary_entry(world_data->m_user_info, 1);
            VALUE v_result = rb_yield_values(2, v_address, v_user_data);
            if (v_result != Qnil) rb_ary_push(v_worlds, v_result);
        }
        else
            rb_ary_push(v_worlds, v_address);
    }
    return v_worlds;
}

VALUE MSP::Newton::rbf_get_all_bodies(VALUE self) {
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_bodies = rb_ary_new();
    for (std::set<const NewtonBody*>::iterator it = MSP::Body::s_valid_bodies.begin(); it != MSP::Body::s_valid_bodies.end(); ++it) {
        const NewtonBody* body = *it;
        VALUE v_address = MSP::Body::c_body_to_value(body);
        if (proc_given) {
            MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body));
            VALUE v_user_data = rb_ary_entry(body_data->m_user_data, 0);
            VALUE v_result = rb_yield_values(2, v_address, v_user_data);
            if (v_result != Qnil)
                rb_ary_push(v_bodies, v_result);
        }
        else
            rb_ary_push(v_bodies, v_address);
    }
    return v_bodies;
}

VALUE MSP::Newton::rbf_get_all_joints(VALUE self) {
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_joints = rb_ary_new();
    for (std::map<MSP::Joint::JointData*, bool>::iterator it = MSP::Joint::s_valid_joints.begin(); it != MSP::Joint::s_valid_joints.end(); ++it) {
        VALUE v_address = MSP::Joint::c_joint_to_value(it->first);
        if (proc_given) {
            VALUE v_user_data = rb_ary_entry(it->first->m_user_data, 0);
            VALUE v_result = rb_yield_values(2, v_address, v_user_data);
            if (v_result != Qnil)
                rb_ary_push(v_joints, v_result);
        }
        else
            rb_ary_push(v_joints, v_address);
    }
    return v_joints;
}

VALUE MSP::Newton::rbf_get_all_gears(VALUE self) {
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_gears = rb_ary_new();
    for (std::set<MSP::Gear::GearData*>::iterator it = MSP::Gear::s_valid_gears.begin(); it != MSP::Gear::s_valid_gears.end(); ++it) {
        MSP::Gear::GearData* object = *it;
        VALUE v_address = MSP::Gear::c_gear_to_value(object);
        if (proc_given) {
            VALUE v_user_data = rb_ary_entry(object->m_user_data, 0);
            VALUE v_result = rb_yield_values(2, v_address, v_user_data);
            if (v_result != Qnil) rb_ary_push(v_gears, v_result);
        }
        else
            rb_ary_push(v_gears, v_address);
    }
    return v_gears;
}

VALUE MSP::Newton::rbf_enable_object_validation(VALUE self, VALUE v_state) {
    Util::s_validate_objects = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Newton::rbf_is_object_validation_enabled(VALUE self) {
    return Util::to_value(Util::s_validate_objects);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Newton::init_ruby(VALUE mNewton) {
    rb_define_module_function(mNewton, "get_version", VALUEFUNC(MSP::Newton::rbf_get_version), 0);
    rb_define_module_function(mNewton, "get_float_size", VALUEFUNC(MSP::Newton::rbf_get_float_size), 0);
    rb_define_module_function(mNewton, "get_memory_used", VALUEFUNC(MSP::Newton::rbf_get_memory_used), 0);
    rb_define_module_function(mNewton, "get_all_worlds", VALUEFUNC(MSP::Newton::rbf_get_all_worlds), 0);
    rb_define_module_function(mNewton, "get_all_bodies", VALUEFUNC(MSP::Newton::rbf_get_all_bodies), 0);
    rb_define_module_function(mNewton, "get_all_joints", VALUEFUNC(MSP::Newton::rbf_get_all_joints), 0);
    rb_define_module_function(mNewton, "get_all_gears", VALUEFUNC(MSP::Newton::rbf_get_all_gears), 0);
    rb_define_module_function(mNewton, "enable_object_validation", VALUEFUNC(MSP::Newton::rbf_enable_object_validation), 1);
    rb_define_module_function(mNewton, "is_object_validation_enabled?", VALUEFUNC(MSP::Newton::rbf_is_object_validation_enabled), 0);
}
