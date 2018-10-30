#ifndef MSP_SOUND_H
#define MSP_SOUND_H

#include "msp.h"
#include "SDL.h"
#include "SDL_mixer.h"

class MSP::Sound {
public:
	// Structures
	struct SoundData {
		wchar_t* m_name;
		char* m_buffer;
		SDL_RWops* m_rw;
		SoundData(wchar_t* name, char* buffer, SDL_RWops* rw) :
			m_name(name),
			m_buffer(buffer),
			m_rw(rw)
		{
		}
	};

	struct SoundData2 {
		dVector m_position;
		double m_max_hearing_range;
		SoundData2(const dVector& position, double max_hearing_range) :
			m_position(position),
			m_max_hearing_range(max_hearing_range)
		{
		}
	};

private:
	// Variables
	static std::map< Mix_Chunk*, SoundData*> s_valid_sounds;
	static std::map<int, SoundData2*> s_registered_channels;

	// Helper Functions
	static bool c_is_valid(Mix_Chunk* address);
	static Mix_Chunk* c_value_to_sound(VALUE v_address);
	static VALUE c_sound_to_value(Mix_Chunk* address);
	static bool c_unregister_channel_effect(int channel);

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
	static VALUE rbf_get_all_sounds(VALUE self);
	static VALUE rbf_get_sound(VALUE self, VALUE v_channel);
	static VALUE rbf_play(VALUE self, VALUE v_address, VALUE v_channel, VALUE v_repeat);
	static VALUE rbf_pause(VALUE self, VALUE v_channel);
	static VALUE rbf_resume(VALUE self, VALUE v_channel);
	static VALUE rbf_stop(VALUE self, VALUE v_channel);
	static VALUE rbf_fade_in(VALUE self, VALUE v_address, VALUE v_channel, VALUE v_repeat, VALUE v_time);
	static VALUE rbf_fade_out(VALUE self, VALUE v_channel, VALUE v_time);
	static VALUE rbf_get_volume(VALUE self, VALUE v_channel);
	static VALUE rbf_set_volume(VALUE self, VALUE v_channel, VALUE v_volume);
	static VALUE rbf_is_playing(VALUE self, VALUE v_channel);
	static VALUE rbf_is_paused(VALUE self, VALUE v_channel);
	static VALUE rbf_is_stopped(VALUE self, VALUE v_channel);
	static VALUE rbf_is_fading(VALUE self, VALUE v_channel);
	static VALUE rbf_set_panning(VALUE self, VALUE v_channel, VALUE v_left_range, VALUE v_right_range);
	static VALUE rbf_set_distance(VALUE self, VALUE v_channel, VALUE v_distance);
	static VALUE rbf_set_position(VALUE self, VALUE v_channel, VALUE v_angle, VALUE v_distance);
	static VALUE rbf_unregister_effects(VALUE self, VALUE v_channel);
	static VALUE rbf_set_position_3d(VALUE self, VALUE v_channel, VALUE v_position, VALUE v_max_hearing_range);
	static VALUE rbf_update_effects(VALUE self);

	// Main
	static void init_ruby(VALUE mMSPhysics);
};

#endif	/* MSP_SOUND_H */
