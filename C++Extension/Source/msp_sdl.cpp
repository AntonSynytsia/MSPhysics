#include "msp_sdl.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSPhysics::SDL::init(VALUE self, VALUE v_flags) {
	return SDL_Init(Util::value_to_uint(v_flags)) == 0 ? Qtrue : Qfalse;
}

VALUE MSPhysics::SDL::quit(VALUE self) {
	SDL_Quit();
	return Qnil;
}

VALUE MSPhysics::SDL::get_error(VALUE self) {
	return Util::to_value(SDL_GetError());
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void Init_msp_sdl(VALUE mMSPhysics) {
	VALUE mSDL = rb_define_module_under(mMSPhysics, "SDL");

	rb_define_module_function(mSDL, "init", VALUEFUNC(MSPhysics::SDL::init), 1);
	rb_define_module_function(mSDL, "quit", VALUEFUNC(MSPhysics::SDL::quit), 0);
	rb_define_module_function(mSDL, "get_error", VALUEFUNC(MSPhysics::SDL::get_error), 0);

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
