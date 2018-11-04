/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_JOYSTICK_H
#define MSP_JOYSTICK_H

#include "msp.h"
#include "SDL.h"

class MSP::Joystick {
private:
    // Variables
    static std::set<SDL_Joystick*> s_valid_joysticks;

    // Helper Functions
    static bool c_is_valid(SDL_Joystick* address);
    static SDL_Joystick* c_value_to_joystick(VALUE v_address);
    static VALUE c_joystick_to_value(SDL_Joystick* address);

public:
    // Ruby Functions
    static VALUE rbf_get_num_joysticks(VALUE self);
    static VALUE rbf_open(VALUE self, VALUE v_index);
    static VALUE rbf_close(VALUE self, VALUE v_joystick);
    static VALUE rbf_is_open(VALUE self, VALUE v_joystick);
    static VALUE rbf_get_open_joysticks(VALUE self);
    static VALUE rbf_close_all_joysticks(VALUE self);
    static VALUE rbf_update(VALUE self);
    static VALUE rbf_get_power_level(VALUE self, VALUE v_joystick);
    static VALUE rbf_get_name(VALUE self, VALUE v_joystick);
    static VALUE rbf_get_name_by_index(VALUE self, VALUE v_index);
    static VALUE rbf_get_num_axes(VALUE self, VALUE v_joystick);
    static VALUE rbf_get_num_balls(VALUE self, VALUE v_joystick);
    static VALUE rbf_get_num_buttons(VALUE self, VALUE v_joystick);
    static VALUE rbf_get_num_hats(VALUE self, VALUE v_joystick);
    static VALUE rbf_get_axis(VALUE self, VALUE v_joystick, VALUE v_index);
    static VALUE rbf_get_ball(VALUE self, VALUE v_joystick, VALUE v_index);
    static VALUE rbf_get_button(VALUE self, VALUE v_joystick, VALUE v_index);
    static VALUE rbf_get_hat(VALUE self, VALUE v_joystick, VALUE v_index);

    // Main
    static void init_ruby(VALUE mMSPhysics);
};

#endif  /* MSP_JOYSTICK_H */
