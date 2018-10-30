#ifndef MSP_MUSIC_H
#define	MSP_MUSIC_H

#include "msp.h"
#include "SDL.h"
#include "SDL_mixer.h"

class MSP::Music {
public:
	// Structures
	struct MusicData {
		wchar_t* m_name;
		char* m_buffer;
		SDL_RWops* m_rw;
		MusicData(wchar_t* name, char* buffer, SDL_RWops* rw) :
			m_name(name),
			m_buffer(buffer),
			m_rw(rw)
		{
		}
	};

private:
	// Variables
	static std::map<Mix_Music*, MusicData*> s_valid_music;

	// Helper Functions
	static bool c_is_valid(Mix_Music* address);
	static Mix_Music* c_value_to_music(VALUE v_address);
	static VALUE c_music_to_value(Mix_Music* address);

public:
	// Ruby Functions
	static VALUE rbf_is_valid(VALUE self, VALUE v_address);
	static VALUE rbf_create_from_dir(VALUE self, VALUE v_path);
	static VALUE rbf_create_from_buffer(VALUE self, VALUE v_buffer, VALUE v_buffer_size);
	static VALUE rbf_destroy(VALUE self, VALUE v_address);
	static VALUE rbf_destroy_all(VALUE self);
	static VALUE rbf_get_name(VALUE self, VALUE v_address);
	static VALUE rbf_set_name(VALUE self, VALUE v_address, VALUE v_name);
	static VALUE rbf_get_by_name(VALUE self, VALUE v_name);
	static VALUE rbf_get_all_music(VALUE self);
	static VALUE rbf_play(VALUE self, VALUE v_address, VALUE v_repeat);
	static VALUE rbf_pause(VALUE self);
	static VALUE rbf_resume(VALUE self);
	static VALUE rbf_stop(VALUE self);
	static VALUE rbf_fade_in(VALUE self, VALUE v_address, VALUE v_repeat, VALUE v_time);
	static VALUE rbf_fade_out(VALUE self, VALUE v_time);
	static VALUE rbf_get_volume(VALUE self);
	static VALUE rbf_set_volume(VALUE self, VALUE v_volume);
	static VALUE rbf_is_playing(VALUE self);
	static VALUE rbf_is_paused(VALUE self);
	static VALUE rbf_is_stopped(VALUE self);
	static VALUE rbf_is_fading(VALUE self);
	static VALUE rbf_get_type(VALUE self, VALUE v_address);
	
	// Main
	static void init_ruby(VALUE mMSPhysics);
};

#endif	/* MSP_MUSIC_H */
