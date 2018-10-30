#include "msp_sdl.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::SDL::rbf_init(VALUE self, VALUE v_flags) {
	return SDL_Init(Util::value_to_uint(v_flags)) == 0 ? Qtrue : Qfalse;
}

VALUE MSP::SDL::rbf_init_sub_system(VALUE self, VALUE v_flags) {
	return SDL_InitSubSystem(Util::value_to_uint(v_flags)) == 0 ? Qtrue : Qfalse;
}

VALUE MSP::SDL::rbf_quit_sub_system(VALUE self, VALUE v_flags) {
	SDL_QuitSubSystem(Util::value_to_uint(v_flags));
	return Qnil;
}

VALUE MSP::SDL::rbf_quit(VALUE self) {
	SDL_Quit();
	return Qnil;
}

VALUE MSP::SDL::rbf_is_init(VALUE self, VALUE v_flags) {
	return SDL_WasInit(Util::value_to_uint(v_flags)) != 0 ? Qtrue : Qfalse;
}

VALUE MSP::SDL::rbf_get_error(VALUE self) {
	return Util::to_value(SDL_GetError());
}

/*VALUE MSP::SDL::rbf_get_key_state(VALUE self, VALUE v_vk) {
	int type = TYPE(v_vk);
	SDL_Scancode scan_code = SDL_SCANCODE_UNKNOWN;
	if (type == T_FLOAT || type == T_BIGNUM || type == T_FIXNUM) {
		SDL_Keycode key_code = (SDL_Keycode)Util::value_to_int(v_vk);
		scan_code = SDL_GetScancodeFromKey(key_code);
	}
	else {
		const char* key_name = Util::value_to_c_str(v_vk);
		scan_code = SDL_GetScancodeFromName(key_name);
	}
	if (scan_code == SDL_SCANCODE_UNKNOWN)
		return Qnil;//Util::to_value(0);
	else {
		const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
		return Util::to_value((unsigned char)keyboard_state[scan_code]);
	}
}

VALUE MSP::SDL::rbf_name_to_key_code(VALUE self, VALUE v_key_name) {
	const char* key_name = Util::value_to_c_str(v_key_name);
	SDL_Keycode key_code = SDL_GetKeyFromName(key_name);
	return Util::to_value((int)key_code);
}

VALUE MSP::SDL::rbf_key_code_to_name(VALUE self, VALUE v_key_code) {
	SDL_Keycode key_code = (SDL_Keycode)Util::value_to_int(v_key_code);
	const char* key_name = SDL_GetKeyName(key_code);
	return Util::to_value(key_name);
}

VALUE MSP::SDL::rbf_name_to_scan_code(VALUE self, VALUE v_key_name) {
	const char* key_name = Util::value_to_c_str(v_key_name);
	SDL_Scancode scan_code = SDL_GetScancodeFromName(key_name);
	return Util::to_value((int)scan_code);
}

VALUE MSP::SDL::rbf_scan_code_to_name(VALUE self, VALUE v_scan_code) {
	SDL_Scancode scan_code = (SDL_Scancode)Util::value_to_int(v_scan_code);
	const char* key_name = SDL_GetScancodeName(scan_code);
	return Util::to_value(key_name);
}

VALUE MSP::SDL::rbf_key_code_to_scan_code(VALUE self, VALUE v_key_code) {
	SDL_Keycode key_code = (SDL_Keycode)Util::value_to_int(v_key_code);
	SDL_Scancode scan_code = SDL_GetScancodeFromKey(key_code);
	return Util::to_value((int)scan_code);
}

VALUE MSP::SDL::rbf_scan_code_to_key_code(VALUE self, VALUE v_scan_code) {
	SDL_Scancode scan_code = (SDL_Scancode)Util::value_to_int(v_scan_code);
	SDL_Keycode key_code = SDL_GetKeyFromScancode(scan_code);
	return Util::to_value((int)key_code);
}

VALUE MSP::SDL::rbf_get_global_mouse_state(VALUE self) {
	Uint32 state = SDL_GetGlobalMouseState(NULL, NULL);
	return Util::to_value((int)SDL_BUTTON(state));
}

VALUE MSP::SDL::rbf_get_mouse_state(VALUE self) {
	Uint32 state = SDL_GetMouseState(NULL, NULL);
	return Util::to_value((int)state);
}

VALUE MSP::SDL::rbf_get_global_cursor_pos(VALUE self) {
	int x, y;
	SDL_GetGlobalMouseState(&x, &y);
	return rb_ary_new3(2, Util::to_value(x), Util::to_value(y));
}

VALUE MSP::SDL::rbf_get_cursor_pos(VALUE self) {
	int x, y;
	SDL_GetMouseState(&x, &y);
	return rb_ary_new3(2, Util::to_value(x), Util::to_value(y));
}

VALUE MSP::SDL::rbf_set_global_cursor_pos(VALUE self, VALUE v_x, VALUE v_y) {
	int state = SDL_WarpMouseGlobal(Util::value_to_int(v_x), Util::value_to_int(v_y));
	return state == 0 ? Qtrue : Qfalse;
}

VALUE MSP::SDL::rbf_set_cursor_pos(VALUE self, VALUE v_x, VALUE v_y) {
	SDL_WarpMouseInWindow(SDL_GetMouseFocus(), Util::value_to_int(v_x), Util::value_to_int(v_y));
	return Qnil;
}

VALUE MSP::SDL::rbf_show_cursor(VALUE self, VALUE v_state) {
	SDL_ShowCursor(Util::value_to_bool(v_state) ? SDL_ENABLE : SDL_DISABLE);
	return Qnil;
}

VALUE MSP::SDL::rbf_cursor_visible(VALUE self) {
	return SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE ? Qtrue : Qfalse;
}

VALUE MSP::SDL::rbf_pump_events(VALUE self) {
	SDL_PumpEvents();
	return Qnil;
}*/


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSP::SDL::init_ruby(VALUE mMSPhysics) {
	VALUE mSDL = rb_define_module_under(mMSPhysics, "SDL");

	rb_define_module_function(mSDL, "init", VALUEFUNC(MSP::SDL::rbf_init), 1);
	rb_define_module_function(mSDL, "init_sub_system", VALUEFUNC(MSP::SDL::rbf_init_sub_system), 1);
	rb_define_module_function(mSDL, "quit", VALUEFUNC(MSP::SDL::rbf_quit), 0);
	rb_define_module_function(mSDL, "quit_sub_system", VALUEFUNC(MSP::SDL::rbf_quit_sub_system), 1);
	rb_define_module_function(mSDL, "is_init?", VALUEFUNC(MSP::SDL::rbf_is_init), 1);
	rb_define_module_function(mSDL, "get_error", VALUEFUNC(MSP::SDL::rbf_get_error), 0);

	/*rb_define_module_function(mSDL, "get_key_state", VALUEFUNC(MSP::SDL::rbf_get_key_state), 1);

	rb_define_module_function(mSDL, "name_to_key_code", VALUEFUNC(MSP::SDL::rbf_name_to_key_code), 1);
	rb_define_module_function(mSDL, "key_code_to_name", VALUEFUNC(MSP::SDL::rbf_key_code_to_name), 1);

	rb_define_module_function(mSDL, "name_to_scan_code", VALUEFUNC(MSP::SDL::rbf_name_to_scan_code), 1);
	rb_define_module_function(mSDL, "scan_code_to_name", VALUEFUNC(MSP::SDL::rbf_scan_code_to_name), 1);

	rb_define_module_function(mSDL, "key_code_to_scan_code", VALUEFUNC(MSP::SDL::rbf_key_code_to_scan_code), 1);
	rb_define_module_function(mSDL, "scan_code_to_key_code", VALUEFUNC(MSP::SDL::rbf_scan_code_to_key_code), 1);

	rb_define_module_function(mSDL, "get_global_mouse_state", VALUEFUNC(MSP::SDL::rbf_get_global_mouse_state), 0);
	rb_define_module_function(mSDL, "get_mouse_state", VALUEFUNC(MSP::SDL::rbf_get_mouse_state), 0);

	rb_define_module_function(mSDL, "get_global_cursor_pos", VALUEFUNC(MSP::SDL::rbf_get_global_cursor_pos), 0);
	rb_define_module_function(mSDL, "get_cursor_pos", VALUEFUNC(MSP::SDL::rbf_get_cursor_pos), 0);
	rb_define_module_function(mSDL, "set_global_cursor_pos", VALUEFUNC(MSP::SDL::rbf_set_global_cursor_pos), 2);
	rb_define_module_function(mSDL, "set_cursor_pos", VALUEFUNC(MSP::SDL::rbf_set_cursor_pos), 2);

	rb_define_module_function(mSDL, "show_cursor", VALUEFUNC(MSP::SDL::rbf_show_cursor), 1);
	rb_define_module_function(mSDL, "cursor_visible?", VALUEFUNC(MSP::SDL::rbf_cursor_visible), 0);

	rb_define_module_function(mSDL, "pump_events", VALUEFUNC(MSP::SDL::rbf_pump_events), 0);*/


	rb_define_const(mSDL, "INIT_TIMER", Util::to_value(SDL_INIT_TIMER));
	rb_define_const(mSDL, "INIT_AUDIO", Util::to_value(SDL_INIT_AUDIO));
	rb_define_const(mSDL, "INIT_VIDEO", Util::to_value(SDL_INIT_VIDEO));
	rb_define_const(mSDL, "INIT_JOYSTICK", Util::to_value(SDL_INIT_JOYSTICK));
	rb_define_const(mSDL, "INIT_HAPTIC", Util::to_value(SDL_INIT_HAPTIC));
	rb_define_const(mSDL, "INIT_GAMECONTROLLER", Util::to_value(SDL_INIT_GAMECONTROLLER));
	rb_define_const(mSDL, "INIT_EVENTS", Util::to_value(SDL_INIT_EVENTS));
	rb_define_const(mSDL, "INIT_NOPARACHUTE", Util::to_value(SDL_INIT_NOPARACHUTE));
	rb_define_const(mSDL, "INIT_EVERYTHING", Util::to_value(SDL_INIT_EVERYTHING));

	atexit(SDL_Quit);
}
