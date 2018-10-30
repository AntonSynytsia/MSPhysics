#ifndef MSP_SDL_MIXER_H
#define	MSP_SDL_MIXER_H

#include "msp.h"
#include "SDL.h"
#include "SDL_mixer.h"

class MSP::SDLMixer {
public:
	// Ruby Function
	static VALUE rbf_init(VALUE self, VALUE v_flags);
	static VALUE rbf_quit(VALUE self);
	static VALUE rbf_get_error(VALUE self);
	static VALUE rbf_open_audio(VALUE self, VALUE v_frequency, VALUE v_format, VALUE v_channels, VALUE v_chunk_size);
	static VALUE rbf_close_audio(VALUE self);
	static VALUE rbf_allocate_channels(VALUE self, VALUE v_num_channels);

	// Main
	static void init_ruby(VALUE mMSPhysics);
};

#endif	/* MSP_SDL_MIXER_H */
