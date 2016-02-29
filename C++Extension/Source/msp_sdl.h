#ifndef MSP_SDL_H
#define	MSP_SDL_H

#include "msp_util.h"
#include "SDL.h"

namespace MSPhysics {
	class SDL;
}

class MSPhysics::SDL {
public:
	// Ruby Function
	static VALUE init(VALUE self, VALUE v_flags);
	static VALUE quit(VALUE self);
	static VALUE get_error(VALUE self);
};

void Init_msp_sdl(VALUE mMSPhysics);

#endif	/* MSP_SDL_H */
