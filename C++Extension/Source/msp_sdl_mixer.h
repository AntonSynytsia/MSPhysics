#ifndef MSP_SDL_MIXER_H
#define	MSP_SDL_MIXER_H

#include "msp_util.h"
#include "SDL.h"
#include "SDL_mixer.h"

namespace MSPhysics {
	class Mixer;
}

class MSPhysics::Mixer {
public:
	// Ruby Function
	static VALUE init(VALUE self, VALUE v_flags);
	static VALUE quit(VALUE self);
	static VALUE get_error(VALUE self);
	static VALUE open_audio(VALUE self, VALUE v_frequency, VALUE v_format, VALUE v_channels, VALUE v_chunk_size);
	static VALUE close_audio(VALUE self);
	static VALUE allocate_channels(VALUE self, VALUE v_num_channels);
};

void Init_msp_sdl_mixer(VALUE mMSPhysics);

#endif	/* MSP_SDL_MIXER_H */
