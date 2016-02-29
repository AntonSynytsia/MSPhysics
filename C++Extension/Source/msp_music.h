#ifndef MSP_MUSIC_H
#define	MSP_MUSIC_H

#include "msp_util.h"
#include "SDL.h"
#include "SDL_mixer.h"

namespace MSPhysics {
	class Music;
};

class MSPhysics::Music {
public:
	// Structures
	typedef struct MusicData {
		wchar_t* name;
		char* buffer;
		SDL_RWops* rw;
	} MusicData;

private:
	// Variables
	static std::map<Mix_Music*, MusicData*> valid_music;

	// Helper Functions
	static bool c_is_valid(Mix_Music* address);
	static Mix_Music* c_value_to_music(VALUE v_address);
	static VALUE c_music_to_value(Mix_Music* address);

public:
	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_address);
	static VALUE create_from_dir(VALUE self, VALUE v_path);
	static VALUE create_from_buffer(VALUE self, VALUE v_buffer, VALUE v_buffer_size);
	static VALUE destroy(VALUE self, VALUE v_address);
	static VALUE destroy_all(VALUE self);
	static VALUE get_name(VALUE self, VALUE v_address);
	static VALUE set_name(VALUE self, VALUE v_address, VALUE v_name);
	static VALUE get_by_name(VALUE self, VALUE v_name);
	static VALUE get_all_music(VALUE self);
	static VALUE play(VALUE self, VALUE v_address, VALUE v_repeat);
	static VALUE pause(VALUE self);
	static VALUE resume(VALUE self);
	static VALUE stop(VALUE self);
	static VALUE fade_in(VALUE self, VALUE v_address, VALUE v_repeat, VALUE v_time);
	static VALUE fade_out(VALUE self, VALUE v_time);
	static VALUE get_volume(VALUE self);
	static VALUE set_volume(VALUE self, VALUE v_volume);
	static VALUE is_playing(VALUE self);
	static VALUE is_paused(VALUE self);
	static VALUE is_stopped(VALUE self);
	static VALUE is_fading(VALUE self);
	static VALUE get_type(VALUE self, VALUE v_address);
};

void Init_msp_music(VALUE mMSPhysics);

#endif	/* MSP_MUSIC_H */
