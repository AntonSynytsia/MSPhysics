#ifndef MSP_JOYSTICK_H
#define MSP_JOYSTICK_H

#include "msp_util.h"
#include "SDL.h"

namespace MSPhysics {
	class Joystick;
}

class MSPhysics::Joystick {
private:
	// Variables
	static std::map<SDL_Joystick*, bool> valid_joysticks;

	// Helper Functions
	static bool c_is_valid(SDL_Joystick* address);
	static SDL_Joystick* c_value_to_joystick(VALUE v_address);
	static VALUE c_joystick_to_value(SDL_Joystick* address);

public:
	// Ruby Functions
	static VALUE get_num_joysticks(VALUE self);
	static VALUE open(VALUE self, VALUE v_index);
	static VALUE close(VALUE self, VALUE v_joystick);
	static VALUE is_open(VALUE self, VALUE v_joystick);
	static VALUE get_open_joysticks(VALUE self);
	static VALUE close_all_joysticks(VALUE self);
	static VALUE update(VALUE self);
	static VALUE get_name(VALUE self, VALUE v_joystick);
	static VALUE get_name_by_index(VALUE self, VALUE v_index);
	static VALUE get_num_axes(VALUE self, VALUE v_joystick);
	static VALUE get_num_balls(VALUE self, VALUE v_joystick);
	static VALUE get_num_buttons(VALUE self, VALUE v_joystick);
	static VALUE get_num_hats(VALUE self, VALUE v_joystick);
	static VALUE get_axis(VALUE self, VALUE v_joystick, VALUE v_index);
	static VALUE get_ball(VALUE self, VALUE v_joystick, VALUE v_index);
	static VALUE get_button(VALUE self, VALUE v_joystick, VALUE v_index);
	static VALUE get_hat(VALUE self, VALUE v_joystick, VALUE v_index);
};

void Init_msp_joystick(VALUE mMSPhysics);

#endif	/* MSP_JOYSTICK_H */
