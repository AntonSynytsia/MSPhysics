#include "msp_sound.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

std::map<Mix_Chunk*, MSPhysics::Sound::SoundData*> MSPhysics::Sound::valid_sounds;
std::map<int, MSPhysics::Sound::SoundData2*> MSPhysics::Sound::registered_channels;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

bool MSPhysics::Sound::c_is_valid(Mix_Chunk* address) {
	return valid_sounds.find(address) != valid_sounds.end();
}

Mix_Chunk* MSPhysics::Sound::c_value_to_sound(VALUE v_address) {
	Mix_Chunk* address = (Mix_Chunk*)Util::value_to_ll(v_address);
	if (valid_sounds.find(address) == valid_sounds.end())
		rb_raise(rb_eTypeError, "Given address is not a reference to a valid sound!");
	return address;
}

VALUE MSPhysics::Sound::c_sound_to_value(Mix_Chunk* address) {
	return Util::to_value((long long)address);
}

bool MSPhysics::Sound::c_unregister_channel_effect(int channel) {
	if (registered_channels.find(channel) == registered_channels.end())
		return false;
	delete registered_channels[channel];
	registered_channels.erase(channel);
	return true;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSPhysics::Sound::is_valid(VALUE self, VALUE v_address) {
	return c_is_valid((Mix_Chunk*)Util::value_to_ll(v_address)) ? Qtrue : Qfalse;
}

VALUE MSPhysics::Sound::create_from_dir(VALUE self, VALUE v_path) {
	Mix_Chunk* sound = Mix_LoadWAV(Util::value_to_c_str(v_path));
	if (!sound)
		rb_raise(rb_eTypeError, "Given path is not a reference to a valid sound!");
	SoundData* data = new SoundData;
	data->name = nullptr;
	data->buffer = nullptr;
	data->rw = nullptr;
	valid_sounds[sound] = data;
	return c_sound_to_value(sound);
}

VALUE MSPhysics::Sound::create_from_buffer(VALUE self, VALUE v_buffer, VALUE v_buffer_size) {
	char* buffer = Util::value_to_c_str(v_buffer);
	int buffer_size = Util::value_to_int(v_buffer_size);
	char* allocated_buffer = new char[buffer_size];
	std::memcpy(allocated_buffer, buffer, buffer_size);
	SDL_RWops* rw = SDL_RWFromConstMem(allocated_buffer, buffer_size);
	if (!rw)
		rb_raise(rb_eTypeError, "Given buffer is not valid!");
	Mix_Chunk* sound = Mix_LoadWAV_RW(rw, 0);
	if (!sound)
		rb_raise(rb_eTypeError, "Failed to create sound from given buffer!");
	SoundData* data = new SoundData;
	data->name = nullptr;
	data->buffer = allocated_buffer;
	data->rw = rw;
	valid_sounds[sound] = data;
	return c_sound_to_value(sound);
}

VALUE MSPhysics::Sound::destroy(VALUE self, VALUE v_address) {
	Mix_Chunk* sound = c_value_to_sound(v_address);
	std::map<int, SoundData2*>::iterator it = registered_channels.begin();
	while (it != registered_channels.end()) {
		if (sound == Mix_GetChunk(it->first))
			registered_channels.erase(it++);
		else
			++it;
	}
	Mix_FreeChunk(sound);
	SoundData* data = valid_sounds[sound];
	if (data->rw) (data->rw);
	if (data->name) delete[] data->name;
	if (data->buffer) delete[] data->buffer;
	delete data;
	valid_sounds.erase(sound);
	return Qnil;
}

VALUE MSPhysics::Sound::destroy_all(VALUE self) {
	for (std::map<int, SoundData2*>::iterator it = registered_channels.begin(); it != registered_channels.end(); ++it)
		delete it->second;
	registered_channels.clear();
	for (std::map<Mix_Chunk*, SoundData*>::iterator it = valid_sounds.begin(); it != valid_sounds.end(); ++it) {
		Mix_FreeChunk((Mix_Chunk*)it->first);
		SoundData* data = it->second;
		if (data->rw) SDL_RWclose(data->rw);
		if (data->name) delete[] data->name;
		if (data->buffer) delete[] data->buffer;
		delete data;
	}
	unsigned int size = (unsigned int)valid_sounds.size();
	valid_sounds.clear();
	return Util::to_value(size);
}

VALUE MSPhysics::Sound::get_name(VALUE self, VALUE v_address) {
	Mix_Chunk* sound = c_value_to_sound(v_address);
	SoundData* data = valid_sounds[sound];
	return data->name == nullptr ? Qnil : Util::to_value(data->name);
}

VALUE MSPhysics::Sound::set_name(VALUE self, VALUE v_address, VALUE v_name) {
	Mix_Chunk* sound = c_value_to_sound(v_address);
	SoundData* data = valid_sounds[sound];
	wchar_t* name = Util::value_to_c_str2(v_name);
	if (data->name) delete[] data->name;
	data->name = name;
	return Util::to_value(data->name);
}

VALUE MSPhysics::Sound::get_by_name(VALUE self, VALUE v_name) {
	const wchar_t* name = Util::value_to_c_str2(v_name);
	size_t len = wcslen(name);
	if (len == 0) {
		delete[] name;
		return Qnil;
	}
	for (std::map<Mix_Chunk*, SoundData*>::iterator it = valid_sounds.begin(); it != valid_sounds.end(); ++it)
		if (it->second->name != nullptr && wcscmp(it->second->name, name) == 0) {
			delete[] name;
			return c_sound_to_value(it->first);
		}
	delete[] name;
	return Qnil;
}

VALUE MSPhysics::Sound::get_all_sounds(VALUE self) {
	VALUE v_container = rb_ary_new2((long)valid_sounds.size());
	int count = 0;
	for (std::map<Mix_Chunk*, SoundData*>::iterator it = valid_sounds.begin(); it != valid_sounds.end(); ++it) {
		rb_ary_store(v_container, count, c_sound_to_value(it->first));
		++count;
	}
	return v_container;
}

VALUE MSPhysics::Sound::get_sound(VALUE self, VALUE v_channel) {
	int channel = Util::clamp_min(Util::value_to_int(v_channel), 0);
	Mix_Chunk* sound = Mix_GetChunk(channel);
	if (Mix_Playing(channel) == 1 && c_is_valid(sound) == true)
		return c_sound_to_value(sound);
	else
		return Qnil;
}

VALUE MSPhysics::Sound::play(VALUE self, VALUE v_address, VALUE v_channel, VALUE v_repeat) {
	Mix_Chunk* sound = c_value_to_sound(v_address);
	int channel = Util::value_to_int(v_channel);
	int repeat = Util::value_to_int(v_repeat);
	int res = Mix_PlayChannel(channel, sound, repeat);
	if (res == -1)
		return Qnil;
	else {
		c_unregister_channel_effect(res);
		return Util::to_value(res);
	}
}

VALUE MSPhysics::Sound::pause(VALUE self, VALUE v_channel) {
	Mix_Pause(Util::value_to_int(v_channel));
	return Qnil;
}

VALUE MSPhysics::Sound::resume(VALUE self, VALUE v_channel) {
	Mix_Resume(Util::value_to_int(v_channel));
	return Qnil;
}

VALUE MSPhysics::Sound::stop(VALUE self, VALUE v_channel) {
	Mix_HaltChannel(Util::value_to_int(v_channel));
	return Qnil;
}

VALUE MSPhysics::Sound::fade_in(VALUE self, VALUE v_address, VALUE v_channel, VALUE v_repeat, VALUE v_time) {
	Mix_Chunk* sound = c_value_to_sound(v_address);
	int channel = Util::value_to_int(v_channel);
	int repeat = Util::value_to_int(v_repeat);
	int time = Util::value_to_int(v_time);
	int res = Mix_FadeInChannel(channel, sound, repeat, time);
	if (res == -1)
		return Qnil;
	else {
		c_unregister_channel_effect(res);
		return Util::to_value(res);
	}
}

VALUE MSPhysics::Sound::fade_out(VALUE self, VALUE v_channel, VALUE v_time) {
	int channel = Util::value_to_int(v_channel);
	int time = Util::value_to_int(v_time);
	return Util::to_value( Mix_FadeOutChannel(channel, time) );
}

VALUE MSPhysics::Sound::get_volume(VALUE self, VALUE v_channel) {
	int channel = Util::value_to_int(v_channel);
	return Util::to_value( Mix_Volume(channel, -1) );
}

VALUE MSPhysics::Sound::set_volume(VALUE self, VALUE v_channel, VALUE v_volume) {
	int channel = Util::value_to_int(v_channel);
	int volume = Util::clamp(Util::value_to_int(v_volume), 0, 128);
	return Util::to_value( Mix_Volume(channel, volume) );
}

VALUE MSPhysics::Sound::is_playing(VALUE self, VALUE v_channel) {
	int channel = Util::clamp_min(Util::value_to_int(v_channel), 0);
	return Mix_Playing(channel) == 1 && Mix_Paused(channel) == 0 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Sound::is_paused(VALUE self, VALUE v_channel) {
	int channel = Util::clamp_min(Util::value_to_int(v_channel), 0);
	return Mix_Playing(channel) == 1 && Mix_Paused(channel) == 1 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Sound::is_stopped(VALUE self, VALUE v_channel) {
	int channel = Util::clamp_min(Util::value_to_int(v_channel), 0);
	return Mix_Playing(channel) == 0 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Sound::is_fading(VALUE self, VALUE v_channel) {
	int channel = Util::clamp_min(Util::value_to_int(v_channel), 0);
	return Mix_FadingChannel(channel) != MIX_NO_FADING ? Qtrue : Qfalse;
}

VALUE MSPhysics::Sound::set_panning(VALUE self, VALUE v_channel, VALUE v_left_range, VALUE v_right_range) {
	int channel = Util::clamp_min(Util::value_to_int(v_channel), 0);
	int left = Util::clamp(Util::value_to_int(v_left_range), 0, 255);
	int right = Util::clamp(Util::value_to_int(v_right_range), 0, 255);
	return Mix_SetPanning(channel, (Uint8)left, (Uint8)right) != 0 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Sound::set_distance(VALUE self, VALUE v_channel, VALUE v_distance) {
	int channel = Util::clamp_min(Util::value_to_int(v_channel), 0);
	int distance = Util::clamp(Util::value_to_int(v_distance), 0, 255);
	return Mix_SetDistance(channel, (Uint8)distance) != 0 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Sound::set_position(VALUE self, VALUE v_channel, VALUE v_angle, VALUE v_distance) {
	int channel = Util::clamp_min(Util::value_to_int(v_channel), 0);
	int angle = Util::value_to_int(v_angle);
	int distance = Util::clamp(Util::value_to_int(v_distance), 0, 255);
	return Mix_SetPosition(channel, (Sint16)angle, (Uint8)distance) != 0 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Sound::unregister_effects(VALUE self, VALUE v_channel)
{
	int channel = Util::clamp_min(Util::value_to_int(v_channel), 0);
	c_unregister_channel_effect(channel);
	return Mix_UnregisterAllEffects(channel) != 0 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Sound::set_position_3d(VALUE self, VALUE v_channel, VALUE v_position, VALUE v_max_hearing_range) {
	int channel = Util::clamp_min(Util::value_to_int(v_channel), 0);
	dVector position = Util::value_to_point(v_position);
	double max_hearing_range = Util::clamp_min<double>(Util::value_to_double(v_max_hearing_range), 1);
	Mix_Chunk* sound = Mix_GetChunk(channel);
	if (Mix_Playing(channel) == 0 || c_is_valid(sound) == false)
		return Qfalse;
	c_unregister_channel_effect(channel);
	SoundData2* data = new SoundData2;
	data->position = position;
	data->max_hearing_range = max_hearing_range;
	registered_channels[channel] = data;
	return Qtrue;
}

VALUE MSPhysics::Sound::update_effects(VALUE self) {
	if (registered_channels.empty())
		return Qfalse;
	VALUE v_model = rb_funcall(suSketchup, INTERN_ACTIVE_MODEL, 0);
	VALUE v_view = rb_funcall(v_model, INTERN_ACTIVE_VIEW, 0);
	VALUE v_cam = rb_funcall(v_view, INTERN_CAMERA, 0);
	dVector eye = Util::value_to_point( rb_funcall(v_cam, INTERN_EYE, 0) );
	dVector xaxis = Util::value_to_vector( rb_funcall(v_cam, INTERN_XAXIS, 0) );
	dVector yaxis = Util::value_to_vector( rb_funcall(v_cam, INTERN_YAXIS, 0) );
	dVector zaxis = Util::value_to_vector( rb_funcall(v_cam, INTERN_ZAXIS, 0) );
	dMatrix matrix(zaxis, xaxis, yaxis, eye);
	std::map<int, SoundData2*>::iterator it = registered_channels.begin();
	while (it != registered_channels.end()) {
		Mix_Chunk* sound = Mix_GetChunk(it->first);
		if (Mix_Playing(it->first) == 0 || c_is_valid(sound) == false) {
			registered_channels.erase(it++);
			continue;
		}
		dVector pos = matrix.UntransformVector(it->second->position);
		double dist = sqrt(pos.m_x*pos.m_x + pos.m_y*pos.m_y + pos.m_z*pos.m_z);
		if (dist > it->second->max_hearing_range) {
			Mix_SetPosition(it->first, (Sint16)0, (Uint8)255);
			++it;
			continue;
		}
		double h = sqrt(pos.m_x*pos.m_x + pos.m_y*pos.m_y);
		if (h < 1.0e-4f) {
			Mix_SetPosition(it->first, (Sint16)0, (Uint8)0);
			++it;
			continue;
		}
		double angle = acos(pos.m_x / h);
		if (pos.m_y < 0) angle = M_PI*2 - angle;
		Sint16 deg_angle = Sint16(angle * 57.2957795);
		Uint8 vol = Uint8(dist * 255 / it->second->max_hearing_range);
		Mix_SetPosition(it->first, deg_angle, vol);
		++it;
	}
	return Qtrue;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void Init_msp_sound(VALUE mMSPhysics) {
	VALUE mSound = rb_define_module_under(mMSPhysics, "Sound");

	rb_define_module_function(mSound, "is_valid?", VALUEFUNC(MSPhysics::Sound::is_valid), 1);
	rb_define_module_function(mSound, "create_from_dir", VALUEFUNC(MSPhysics::Sound::create_from_dir), 1);
	rb_define_module_function(mSound, "create_from_buffer", VALUEFUNC(MSPhysics::Sound::create_from_buffer), 2);
	rb_define_module_function(mSound, "destroy", VALUEFUNC(MSPhysics::Sound::destroy), 1);
	rb_define_module_function(mSound, "destroy_all", VALUEFUNC(MSPhysics::Sound::destroy_all), 0);
	rb_define_module_function(mSound, "get_name", VALUEFUNC(MSPhysics::Sound::get_name), 1);
	rb_define_module_function(mSound, "set_name", VALUEFUNC(MSPhysics::Sound::set_name), 2);
	rb_define_module_function(mSound, "get_by_name", VALUEFUNC(MSPhysics::Sound::get_by_name), 1);
	rb_define_module_function(mSound, "get_all_sounds", VALUEFUNC(MSPhysics::Sound::get_all_sounds), 0);
	rb_define_module_function(mSound, "get_sound", VALUEFUNC(MSPhysics::Sound::get_sound), 1);
	rb_define_module_function(mSound, "play", VALUEFUNC(MSPhysics::Sound::play), 3);
	rb_define_module_function(mSound, "pause", VALUEFUNC(MSPhysics::Sound::pause), 1);
	rb_define_module_function(mSound, "resume", VALUEFUNC(MSPhysics::Sound::resume), 1);
	rb_define_module_function(mSound, "stop", VALUEFUNC(MSPhysics::Sound::stop), 1);
	rb_define_module_function(mSound, "fade_in", VALUEFUNC(MSPhysics::Sound::fade_in), 4);
	rb_define_module_function(mSound, "fade_out", VALUEFUNC(MSPhysics::Sound::fade_out), 2);
	rb_define_module_function(mSound, "get_volume", VALUEFUNC(MSPhysics::Sound::get_volume), 1);
	rb_define_module_function(mSound, "set_volume", VALUEFUNC(MSPhysics::Sound::set_volume), 2);
	rb_define_module_function(mSound, "is_playing?", VALUEFUNC(MSPhysics::Sound::is_playing), 1);
	rb_define_module_function(mSound, "is_paused?", VALUEFUNC(MSPhysics::Sound::is_paused), 1);
	rb_define_module_function(mSound, "is_stopped?", VALUEFUNC(MSPhysics::Sound::is_stopped), 1);
	rb_define_module_function(mSound, "is_fading?", VALUEFUNC(MSPhysics::Sound::is_fading), 1);
	rb_define_module_function(mSound, "set_panning", VALUEFUNC(MSPhysics::Sound::set_panning), 3);
	rb_define_module_function(mSound, "set_distance", VALUEFUNC(MSPhysics::Sound::set_distance), 2);
	rb_define_module_function(mSound, "set_position", VALUEFUNC(MSPhysics::Sound::set_position), 3);
	rb_define_module_function(mSound, "unregister_effects", VALUEFUNC(MSPhysics::Sound::unregister_effects), 1);
	rb_define_module_function(mSound, "set_position_3d", VALUEFUNC(MSPhysics::Sound::set_position_3d), 3);
	rb_define_module_function(mSound, "update_effects", VALUEFUNC(MSPhysics::Sound::update_effects), 0);
}
