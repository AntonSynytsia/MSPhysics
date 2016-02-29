#include "msp_music.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

std::map<Mix_Music*, MSPhysics::Music::MusicData*> MSPhysics::Music::valid_music;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

bool MSPhysics::Music::c_is_valid(Mix_Music* address) {
	return valid_music.find(address) != valid_music.end();
}

Mix_Music* MSPhysics::Music::c_value_to_music(VALUE v_address) {
	Mix_Music* address = (Mix_Music*)Util::value_to_ll(v_address);
	if (valid_music.find(address) == valid_music.end())
		rb_raise(rb_eTypeError, "Given address is not a reference to a valid music!");
	return address;
}

VALUE MSPhysics::Music::c_music_to_value(Mix_Music* address) {
	return Util::to_value((long long)address);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSPhysics::Music::is_valid(VALUE self, VALUE v_address) {
	return c_is_valid((Mix_Music*)Util::value_to_ll(v_address)) ? Qtrue : Qfalse;
}

VALUE MSPhysics::Music::create_from_dir(VALUE self, VALUE v_path) {
	Mix_Music* music = Mix_LoadMUS(Util::value_to_c_str(v_path));
	if (!music)
		rb_raise(rb_eTypeError, "Given path is not a reference to a valid music!");
	MusicData* data = new MusicData;
	data->name = nullptr;
	data->buffer = nullptr;
	data->rw = nullptr;
	valid_music[music] = data;
	return c_music_to_value(music);
}

VALUE MSPhysics::Music::create_from_buffer(VALUE self, VALUE v_buffer, VALUE v_buffer_size) {
	char* buffer = Util::value_to_c_str(v_buffer);
	int buffer_size = Util::value_to_int(v_buffer_size);
	char* allocated_buffer = new char[buffer_size];
	std::memcpy(allocated_buffer, buffer, buffer_size);
	SDL_RWops* rw = SDL_RWFromConstMem(allocated_buffer, buffer_size);
	if (!rw)
		rb_raise(rb_eTypeError, "Given buffer is not valid!");
	Mix_Music* music = Mix_LoadMUS_RW(rw, 0);
	if (!music)
		rb_raise(rb_eTypeError, "Failed to create music from given buffer!");
	MusicData* data = new MusicData;
	data->name = nullptr;
	data->buffer = allocated_buffer;
	data->rw = rw;
	valid_music[music] = data;
	return c_music_to_value(music);
}

VALUE MSPhysics::Music::destroy(VALUE self, VALUE v_address) {
	Mix_Music* music = c_value_to_music(v_address);
	Mix_FreeMusic(music);
	MusicData* data = valid_music[music];
	if (data->rw) SDL_RWclose(data->rw);
	if (data->name) delete[] data->name;
	if (data->buffer) delete[] data->buffer;
	delete data;
	valid_music.erase(music);
	return Qnil;
}

VALUE MSPhysics::Music::destroy_all(VALUE self) {
	for (std::map<Mix_Music*, MusicData*>::iterator it = valid_music.begin(); it != valid_music.end(); ++it) {
		Mix_FreeMusic(it->first);
		MusicData* data = it->second;
		if (data->rw) SDL_RWclose(data->rw);
		if (data->name) delete[] data->name;
		if (data->buffer) delete[] data->buffer;
		delete data;
	}
	unsigned int size = (unsigned int)valid_music.size();
	valid_music.clear();
	return Util::to_value(size);
}

VALUE MSPhysics::Music::get_name(VALUE self, VALUE v_address) {
	Mix_Music* music = c_value_to_music(v_address);
	MusicData* data = valid_music[music];
	return data->name == nullptr ? Qnil : Util::to_value(data->name);
}

VALUE MSPhysics::Music::set_name(VALUE self, VALUE v_address, VALUE v_name) {
	Mix_Music* music = c_value_to_music(v_address);
	MusicData* data = valid_music[music];
	wchar_t* name = Util::value_to_c_str2(v_name);
	if (data->name) delete[] data->name;
	data->name = name;
	return Util::to_value(data->name);
}

VALUE MSPhysics::Music::get_by_name(VALUE self, VALUE v_name) {
	const wchar_t* name = Util::value_to_c_str2(v_name);
	size_t len = wcslen(name);
	if (len == 0) {
		delete[] name;
		return Qnil;
	}
	for (std::map<Mix_Music*, MusicData*>::iterator it = valid_music.begin(); it != valid_music.end(); ++it)
		if (it->second->name != nullptr && wcscmp(it->second->name, name) == 0) {
			delete[] name;
			return c_music_to_value(it->first);
		}
	delete[] name;
	return Qnil;
}

VALUE MSPhysics::Music::get_all_music(VALUE self) {
	VALUE v_container = rb_ary_new2((long)valid_music.size());
	int count = 0;
	for (std::map<Mix_Music*, MusicData*>::iterator it = valid_music.begin(); it != valid_music.end(); ++it) {
		rb_ary_store(v_container, count, c_music_to_value(it->first));
		++count;
	}
	return v_container;
}

VALUE MSPhysics::Music::play(VALUE self, VALUE v_address, VALUE v_repeat) {
	Mix_Music* music = c_value_to_music(v_address);
	Mix_HaltMusic();
	Mix_SetMusicCMD(NULL);
	return Mix_PlayMusic(music, Util::value_to_int(v_repeat)) == 0 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Music::pause(VALUE self) {
	Mix_PauseMusic();
	return Qnil;
}

VALUE MSPhysics::Music::resume(VALUE self) {
	Mix_ResumeMusic();
	return Qnil;
}

VALUE MSPhysics::Music::stop(VALUE self) {
	Mix_HaltMusic();
	return Qnil;
}

VALUE MSPhysics::Music::fade_in(VALUE self, VALUE v_address, VALUE v_repeat, VALUE v_time) {
	Mix_Music* music = c_value_to_music(v_address);
	Mix_SetMusicCMD(NULL);
	return Mix_FadeInMusic(music, Util::value_to_int(v_repeat), Util::value_to_int(v_time)) == 0 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Music::fade_out(VALUE self, VALUE v_time) {
	return Mix_FadeOutMusic(Util::value_to_int(v_time)) == 1 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Music::get_volume(VALUE self) {
	return Util::to_value(Mix_VolumeMusic(-1));
}

VALUE MSPhysics::Music::set_volume(VALUE self, VALUE v_volume) {
	int vol = Util::clamp(Util::value_to_int(v_volume), 0, 128);
	return Util::to_value(Mix_VolumeMusic(vol));
}

VALUE MSPhysics::Music::is_playing(VALUE self) {
	return Mix_PlayingMusic() == 1 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Music::is_paused(VALUE self) {
	return Mix_PausedMusic() == 1 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Music::is_stopped(VALUE self) {
	return Mix_PlayingMusic() == 0 && Mix_PausedMusic() == 0 ? Qtrue : Qfalse;
}

VALUE MSPhysics::Music::is_fading(VALUE self) {
	return Mix_FadingMusic() != MIX_NO_FADING ? Qtrue : Qfalse;
}

VALUE MSPhysics::Music::get_type(VALUE self, VALUE v_address) {
	Mix_Music* music = c_value_to_music(v_address);
	return Util::to_value(Mix_GetMusicType(music));
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void Init_msp_music(VALUE mMSPhysics) {
	VALUE mMusic = rb_define_module_under(mMSPhysics, "Music");

	rb_define_module_function(mMusic, "is_valid?", VALUEFUNC(MSPhysics::Music::is_valid), 1);
	rb_define_module_function(mMusic, "create_from_dir", VALUEFUNC(MSPhysics::Music::create_from_dir), 1);
	rb_define_module_function(mMusic, "create_from_buffer", VALUEFUNC(MSPhysics::Music::create_from_buffer), 2);
	rb_define_module_function(mMusic, "destroy", VALUEFUNC(MSPhysics::Music::destroy), 1);
	rb_define_module_function(mMusic, "destroy_all", VALUEFUNC(MSPhysics::Music::destroy_all), 0);
	rb_define_module_function(mMusic, "get_name", VALUEFUNC(MSPhysics::Music::get_name), 1);
	rb_define_module_function(mMusic, "set_name", VALUEFUNC(MSPhysics::Music::set_name), 2);
	rb_define_module_function(mMusic, "get_by_name", VALUEFUNC(MSPhysics::Music::get_by_name), 1);
	rb_define_module_function(mMusic, "get_all_music", VALUEFUNC(MSPhysics::Music::get_all_music), 0);
	rb_define_module_function(mMusic, "play", VALUEFUNC(MSPhysics::Music::play), 2);
	rb_define_module_function(mMusic, "pause", VALUEFUNC(MSPhysics::Music::pause), 0);
	rb_define_module_function(mMusic, "resume", VALUEFUNC(MSPhysics::Music::resume), 0);
	rb_define_module_function(mMusic, "stop", VALUEFUNC(MSPhysics::Music::stop), 0);
	rb_define_module_function(mMusic, "fade_in", VALUEFUNC(MSPhysics::Music::fade_in), 3);
	rb_define_module_function(mMusic, "fade_out", VALUEFUNC(MSPhysics::Music::fade_out), 1);
	rb_define_module_function(mMusic, "get_volume", VALUEFUNC(MSPhysics::Music::get_volume), 0);
	rb_define_module_function(mMusic, "set_volume", VALUEFUNC(MSPhysics::Music::set_volume), 1);
	rb_define_module_function(mMusic, "is_playing?", VALUEFUNC(MSPhysics::Music::is_playing), 0);
	rb_define_module_function(mMusic, "is_paused?", VALUEFUNC(MSPhysics::Music::is_paused), 0);
	rb_define_module_function(mMusic, "is_stopped?", VALUEFUNC(MSPhysics::Music::is_stopped), 0);
	rb_define_module_function(mMusic, "is_fading?", VALUEFUNC(MSPhysics::Music::is_fading), 0);
	rb_define_module_function(mMusic, "get_type", VALUEFUNC(MSPhysics::Music::get_type), 1);
}
