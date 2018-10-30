#include "msp_sdl_mixer.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::SDLMixer::rbf_init(VALUE self, VALUE v_flags) {
	int flags = Util::value_to_int(v_flags);
	int initted = Mix_Init(flags);
	return ((initted & flags) == flags) ? Qtrue : Qfalse;
}

VALUE MSP::SDLMixer::rbf_quit(VALUE self) {
	Mix_Quit();
	return Qnil;
}

VALUE MSP::SDLMixer::rbf_get_error(VALUE self) {
	return Util::to_value(Mix_GetError());
}

VALUE MSP::SDLMixer::rbf_open_audio(VALUE self, VALUE v_frequency, VALUE v_format, VALUE v_channels, VALUE v_chunk_size) {
	int frequency = Util::value_to_int(v_frequency);
	unsigned int format = Util::value_to_uint(v_format);
	int channels = Util::value_to_int(v_channels);
	int chunk_size = Util::value_to_int(v_chunk_size);
	return Mix_OpenAudio(frequency, (Uint16)format, channels, chunk_size) == 0 ? Qtrue : Qfalse;
}

VALUE MSP::SDLMixer::rbf_close_audio(VALUE self) {
	Mix_CloseAudio();
	return Qnil;
}

VALUE MSP::SDLMixer::rbf_allocate_channels(VALUE self, VALUE v_num_channels) {
	return Util::to_value(Mix_AllocateChannels(Util::value_to_int(v_num_channels)));
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSP::SDLMixer::init_ruby(VALUE mMSPhysics) {
	VALUE mMixer = rb_define_module_under(mMSPhysics, "Mixer");

	rb_define_module_function(mMixer, "init", VALUEFUNC(MSP::SDLMixer::rbf_init), 1);
	rb_define_module_function(mMixer, "quit", VALUEFUNC(MSP::SDLMixer::rbf_quit), 0);
	rb_define_module_function(mMixer, "get_error", VALUEFUNC(MSP::SDLMixer::rbf_get_error), 0);
	rb_define_module_function(mMixer, "open_audio", VALUEFUNC(MSP::SDLMixer::rbf_open_audio), 4);
	rb_define_module_function(mMixer, "close_audio", VALUEFUNC(MSP::SDLMixer::rbf_close_audio), 0);
	rb_define_module_function(mMixer, "allocate_channels", VALUEFUNC(MSP::SDLMixer::rbf_allocate_channels), 1);

	rb_define_const(mMixer, "INIT_FLAC", Util::to_value(MIX_INIT_FLAC));
	rb_define_const(mMixer, "INIT_MOD", Util::to_value(MIX_INIT_MOD));
	rb_define_const(mMixer, "INIT_MODPLUG", Util::to_value(MIX_INIT_MODPLUG));
	rb_define_const(mMixer, "INIT_MP3", Util::to_value(MIX_INIT_MP3));
	rb_define_const(mMixer, "INIT_OGG", Util::to_value(MIX_INIT_OGG));
	rb_define_const(mMixer, "INIT_FLUIDSYNTH", Util::to_value(MIX_INIT_FLUIDSYNTH));
	rb_define_const(mMixer, "INIT_EVERYTHING", Util::to_value(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MODPLUG | MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_FLUIDSYNTH));
	//rb_define_const(mMixer, "INIT_EVERYTHING", Util::to_value(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_FLUIDSYNTH));
	rb_define_const(mMixer, "INIT_SUPPORTED", Util::to_value(MIX_INIT_FLAC | MIX_INIT_MODPLUG | MIX_INIT_MP3 | MIX_INIT_OGG));
	//rb_define_const(mMixer, "INIT_SUPPORTED", Util::to_value(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_OGG));
	rb_define_const(mMixer, "DEFAULT_FREQUENCY", Util::to_value(MIX_DEFAULT_FREQUENCY));
	rb_define_const(mMixer, "DEFAULT_FORMAT", Util::to_value(MIX_DEFAULT_FORMAT));
	rb_define_const(mMixer, "DEFAULT_CHANNELS", Util::to_value(MIX_DEFAULT_CHANNELS));

	atexit(Mix_Quit);
}
