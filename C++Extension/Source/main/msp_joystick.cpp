/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "msp_joystick.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

std::set<SDL_Joystick*> MSP::Joystick::s_valid_joysticks;


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

bool MSP::Joystick::c_is_valid(SDL_Joystick* address) {
    return s_valid_joysticks.find(address) != s_valid_joysticks.end();
}

SDL_Joystick* MSP::Joystick::c_value_to_joystick(VALUE v_address) {
    SDL_Joystick* address = reinterpret_cast<SDL_Joystick*>(Util::value_to_ull(v_address));
    if (s_valid_joysticks.find(address) == s_valid_joysticks.end())
        rb_raise(rb_eTypeError, "Given address is not a reference to a valid joystick!");
    return address;
}

VALUE MSP::Joystick::c_joystick_to_value(SDL_Joystick* address) {
    return rb_ull2inum(reinterpret_cast<unsigned long long>(address));
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Joystick::rbf_get_num_joysticks(VALUE self) {
    return Util::to_value(SDL_NumJoysticks());
}

VALUE MSP::Joystick::rbf_open(VALUE self, VALUE v_index) {
    int index = Util::value_to_int(v_index);
    SDL_Joystick* address = SDL_JoystickOpen(index);
    if (address == NULL) return Qnil;
    s_valid_joysticks.insert(address);
    return c_joystick_to_value(address);
}

VALUE MSP::Joystick::rbf_close(VALUE self, VALUE v_joystick) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    bool state = (SDL_JoystickGetAttached(address) == SDL_TRUE);
    if (state) SDL_JoystickClose(address);
    s_valid_joysticks.erase(address);
    return Util::to_value(state);
}

VALUE MSP::Joystick::rbf_is_open(VALUE self, VALUE v_joystick) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    return Util::to_value(SDL_JoystickGetAttached(address) == SDL_TRUE);
}

VALUE MSP::Joystick::rbf_get_open_joysticks(VALUE self) {
    VALUE v_joysticks = rb_ary_new();
    for(std::set<SDL_Joystick*>::iterator it = s_valid_joysticks.begin(); it != s_valid_joysticks.end(); ++it) {
        if (SDL_JoystickGetAttached(*it) == SDL_TRUE)
            rb_ary_push(v_joysticks, c_joystick_to_value(*it));
    }
    return v_joysticks;
}

VALUE MSP::Joystick::rbf_close_all_joysticks(VALUE self) {
    int count = 0;
    for(std::set<SDL_Joystick*>::iterator it = s_valid_joysticks.begin(); it != s_valid_joysticks.end(); ++it) {
        if (SDL_JoystickGetAttached(*it) == SDL_TRUE) {
            SDL_JoystickClose(*it);
            ++count;
        }
    }
    s_valid_joysticks.clear();
    return Util::to_value(count);
}

VALUE MSP::Joystick::rbf_update(VALUE self) {
    SDL_JoystickUpdate();
    return Qnil;
}

VALUE MSP::Joystick::rbf_get_power_level(VALUE self, VALUE v_joystick) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    return Util::to_value(SDL_JoystickCurrentPowerLevel(address));
}

VALUE MSP::Joystick::rbf_get_name(VALUE self, VALUE v_joystick) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    const char* name = SDL_JoystickName(address);
    if (name == NULL)
        return Qnil;
    else
        return Util::to_value(name);
}

VALUE MSP::Joystick::rbf_get_name_by_index(VALUE self, VALUE v_index) {
    int index = Util::value_to_int(v_index);
    const char* name = SDL_JoystickNameForIndex(index);
    if (name == NULL)
        return Qnil;
    else
        return Util::to_value(name);
}

VALUE MSP::Joystick::rbf_get_num_axes(VALUE self, VALUE v_joystick) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    return Util::to_value(SDL_JoystickNumAxes(address));
}

VALUE MSP::Joystick::rbf_get_num_balls(VALUE self, VALUE v_joystick) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    return Util::to_value(SDL_JoystickNumBalls(address));
}

VALUE MSP::Joystick::rbf_get_num_buttons(VALUE self, VALUE v_joystick) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    return Util::to_value(SDL_JoystickNumButtons(address));
}

VALUE MSP::Joystick::rbf_get_num_hats(VALUE self, VALUE v_joystick) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    return Util::to_value(SDL_JoystickNumHats(address));
}

VALUE MSP::Joystick::rbf_get_axis(VALUE self, VALUE v_joystick, VALUE v_index) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    int index = Util::value_to_int(v_index);
    return Util::to_value((int)SDL_JoystickGetAxis(address, index));
}

VALUE MSP::Joystick::rbf_get_ball(VALUE self, VALUE v_joystick, VALUE v_index) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    int index = Util::value_to_int(v_index);
    int delta_x, delta_y;
    if (SDL_JoystickGetBall(address, index, &delta_x, &delta_y) == 0)
        return rb_ary_new3(2, Util::to_value(delta_x), Util::to_value(delta_y));
    else
        return rb_ary_new3(2, Util::to_value(0), Util::to_value(0));
}

VALUE MSP::Joystick::rbf_get_button(VALUE self, VALUE v_joystick, VALUE v_index) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    int index = Util::value_to_int(v_index);
    return Util::to_value((int)SDL_JoystickGetButton(address, index));
}

VALUE MSP::Joystick::rbf_get_hat(VALUE self, VALUE v_joystick, VALUE v_index) {
    SDL_Joystick* address = c_value_to_joystick(v_joystick);
    int index = Util::value_to_int(v_index);
    return Util::to_value((unsigned int)SDL_JoystickGetHat(address, index));
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Joystick::init_ruby(VALUE mMSPhysics) {
    VALUE mJoystick = rb_define_module_under(mMSPhysics, "Joystick");

    rb_define_module_function(mJoystick, "get_num_joysticks", VALUEFUNC(MSP::Joystick::rbf_get_num_joysticks), 0);
    rb_define_module_function(mJoystick, "open", VALUEFUNC(MSP::Joystick::rbf_open), 1);
    rb_define_module_function(mJoystick, "close", VALUEFUNC(MSP::Joystick::rbf_close), 1);
    rb_define_module_function(mJoystick, "is_open?", VALUEFUNC(MSP::Joystick::rbf_is_open), 1);
    rb_define_module_function(mJoystick, "get_open_joysticks", VALUEFUNC(MSP::Joystick::rbf_get_open_joysticks), 0);
    rb_define_module_function(mJoystick, "close_all_joysticks", VALUEFUNC(MSP::Joystick::rbf_close_all_joysticks), 0);
    rb_define_module_function(mJoystick, "update", VALUEFUNC(MSP::Joystick::rbf_update), 0);
    rb_define_module_function(mJoystick, "get_power_level", VALUEFUNC(MSP::Joystick::rbf_get_power_level), 1);
    rb_define_module_function(mJoystick, "get_name", VALUEFUNC(MSP::Joystick::rbf_get_name), 1);
    rb_define_module_function(mJoystick, "get_name_by_index", VALUEFUNC(MSP::Joystick::rbf_get_name_by_index), 1);
    rb_define_module_function(mJoystick, "get_num_axes", VALUEFUNC(MSP::Joystick::rbf_get_num_axes), 1);
    rb_define_module_function(mJoystick, "get_num_balls", VALUEFUNC(MSP::Joystick::rbf_get_num_balls), 1);
    rb_define_module_function(mJoystick, "get_num_buttons", VALUEFUNC(MSP::Joystick::rbf_get_num_buttons), 1);
    rb_define_module_function(mJoystick, "get_num_hats", VALUEFUNC(MSP::Joystick::rbf_get_num_hats), 1);
    rb_define_module_function(mJoystick, "get_axis", VALUEFUNC(MSP::Joystick::rbf_get_axis), 2);
    rb_define_module_function(mJoystick, "get_ball", VALUEFUNC(MSP::Joystick::rbf_get_ball), 2);
    rb_define_module_function(mJoystick, "get_button", VALUEFUNC(MSP::Joystick::rbf_get_button), 2);
    rb_define_module_function(mJoystick, "get_hat", VALUEFUNC(MSP::Joystick::rbf_get_hat), 2);

    rb_define_const(mJoystick, "HAT_CENTERED", Util::to_value(SDL_HAT_CENTERED));
    rb_define_const(mJoystick, "HAT_UP", Util::to_value(SDL_HAT_UP));
    rb_define_const(mJoystick, "HAT_DOWN", Util::to_value(SDL_HAT_DOWN));
    rb_define_const(mJoystick, "HAT_LEFT", Util::to_value(SDL_HAT_LEFT));
    rb_define_const(mJoystick, "HAT_RIGHT", Util::to_value(SDL_HAT_RIGHT));
    rb_define_const(mJoystick, "HAT_LEFTUP", Util::to_value(SDL_HAT_LEFTUP));
    rb_define_const(mJoystick, "HAT_LEFTDOWN", Util::to_value(SDL_HAT_LEFTDOWN));
    rb_define_const(mJoystick, "HAT_RIGHTUP", Util::to_value(SDL_HAT_RIGHTUP));
    rb_define_const(mJoystick, "HAT_RIGHTDOWN", Util::to_value(SDL_HAT_RIGHTDOWN));
}
