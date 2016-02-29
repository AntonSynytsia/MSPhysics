#ifndef MSP_SOUND_H
#define MSP_SOUND_H

#include "msp_util.h"
#include "SDL.h"
#include "SDL_mixer.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace MSPhysics {
	class Sound;
}

class MSPhysics::Sound {
public:
	// Structures
	typedef struct SoundData {
		wchar_t* name;
		char* buffer;
		SDL_RWops* rw;
	} SoundData;

	typedef struct SoundData2 {
		dVector position;
		double max_hearing_range;
	} SoundData2;

private:
	// Variables
	static std::map< Mix_Chunk*, SoundData*> valid_sounds;
	static std::map<int, SoundData2*> registered_channels;

	// Helper Functions
	static bool c_is_valid(Mix_Chunk* address);
	static Mix_Chunk* c_value_to_sound(VALUE v_address);
	static VALUE c_sound_to_value(Mix_Chunk* address);
	static bool c_unregister_channel_effect(int channel);

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
	static VALUE get_all_sounds(VALUE self);
	static VALUE get_sound(VALUE self, VALUE v_channel);
	static VALUE play(VALUE self, VALUE v_address, VALUE v_channel, VALUE v_repeat);
	static VALUE pause(VALUE self, VALUE v_channel);
	static VALUE resume(VALUE self, VALUE v_channel);
	static VALUE stop(VALUE self, VALUE v_channel);
	static VALUE fade_in(VALUE self, VALUE v_address, VALUE v_channel, VALUE v_repeat, VALUE v_time);
	static VALUE fade_out(VALUE self, VALUE v_channel, VALUE v_time);
	static VALUE get_volume(VALUE self, VALUE v_channel);
	static VALUE set_volume(VALUE self, VALUE v_channel, VALUE v_volume);
	static VALUE is_playing(VALUE self, VALUE v_channel);
	static VALUE is_paused(VALUE self, VALUE v_channel);
	static VALUE is_stopped(VALUE self, VALUE v_channel);
	static VALUE is_fading(VALUE self, VALUE v_channel);
	static VALUE set_panning(VALUE self, VALUE v_channel, VALUE v_left_range, VALUE v_right_range);
	static VALUE set_distance(VALUE self, VALUE v_channel, VALUE v_distance);
	static VALUE set_position(VALUE self, VALUE v_channel, VALUE v_angle, VALUE v_distance);
	static VALUE unregister_effects(VALUE self, VALUE v_channel);
	static VALUE set_position_3d(VALUE self, VALUE v_channel, VALUE v_position, VALUE v_max_hearing_range);
	static VALUE update_effects(VALUE self);
};

void Init_msp_sound(VALUE mMSPhysics);

#endif	/* MSP_SOUND_H */
