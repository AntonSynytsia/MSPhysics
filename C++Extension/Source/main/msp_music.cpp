/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_music.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

std::map<Mix_Music*, MSP::Music::MusicData*> MSP::Music::s_valid_music;


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

bool MSP::Music::c_is_valid(Mix_Music* address) {
    return s_valid_music.find(address) != s_valid_music.end();
}

Mix_Music* MSP::Music::c_value_to_music(VALUE v_address) {
    Mix_Music* address = reinterpret_cast<Mix_Music*>(Util::value_to_ull(v_address));
    if (s_valid_music.find(address) == s_valid_music.end())
        rb_raise(rb_eTypeError, "Given address is not a reference to a valid music!");
    return address;
}

VALUE MSP::Music::c_music_to_value(Mix_Music* address) {
    return rb_ull2inum(reinterpret_cast<unsigned long long>(address));
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Music::rbf_is_valid(VALUE self, VALUE v_address) {
    return c_is_valid(reinterpret_cast<Mix_Music*>(Util::value_to_ull(v_address))) ? Qtrue : Qfalse;
}

VALUE MSP::Music::rbf_create_from_dir(VALUE self, VALUE v_path) {
    Mix_Music* music = Mix_LoadMUS(Util::value_to_c_str(v_path));
    if (!music)
        rb_raise(rb_eTypeError, "Given path is not a reference to a valid music!");
    s_valid_music[music] = new MusicData(nullptr, nullptr, nullptr);
    return c_music_to_value(music);
}

VALUE MSP::Music::rbf_create_from_buffer(VALUE self, VALUE v_buffer, VALUE v_buffer_size) {
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
    s_valid_music[music] = new MusicData(nullptr, allocated_buffer, rw);
    return c_music_to_value(music);
}

VALUE MSP::Music::rbf_destroy(VALUE self, VALUE v_address) {
    Mix_Music* music = c_value_to_music(v_address);
    Mix_FreeMusic(music);
    MusicData* data = s_valid_music[music];
    if (data->m_rw) SDL_RWclose(data->m_rw);
    if (data->m_name) delete[] data->m_name;
    if (data->m_buffer) delete[] data->m_buffer;
    delete data;
    s_valid_music.erase(music);
    return Qnil;
}

VALUE MSP::Music::rbf_destroy_all(VALUE self) {
    for (std::map<Mix_Music*, MusicData*>::iterator it = s_valid_music.begin(); it != s_valid_music.end(); ++it) {
        Mix_FreeMusic(it->first);
        MusicData* data = it->second;
        if (data->m_rw) SDL_RWclose(data->m_rw);
        if (data->m_name) delete[] data->m_name;
        if (data->m_buffer) delete[] data->m_buffer;
        delete data;
    }
    unsigned int size = (unsigned int)s_valid_music.size();
    s_valid_music.clear();
    return Util::to_value(size);
}

VALUE MSP::Music::rbf_get_name(VALUE self, VALUE v_address) {
    Mix_Music* music = c_value_to_music(v_address);
    MusicData* data = s_valid_music[music];
    return data->m_name == nullptr ? Qnil : Util::to_value(data->m_name);
}

VALUE MSP::Music::rbf_set_name(VALUE self, VALUE v_address, VALUE v_name) {
    Mix_Music* music = c_value_to_music(v_address);
    MusicData* data = s_valid_music[music];
    wchar_t* name = Util::value_to_c_str2(v_name);
    if (data->m_name) delete[] data->m_name;
    data->m_name = name;
    return Util::to_value(data->m_name);
}

VALUE MSP::Music::rbf_get_by_name(VALUE self, VALUE v_name) {
    const wchar_t* name = Util::value_to_c_str2(v_name);
    size_t len(wcslen(name));
    if (len == 0) {
        delete[] name;
        return Qnil;
    }
    for (std::map<Mix_Music*, MusicData*>::iterator it = s_valid_music.begin(); it != s_valid_music.end(); ++it)
        if (it->second->m_name != nullptr && wcscmp(it->second->m_name, name) == 0) {
            delete[] name;
            return c_music_to_value(it->first);
        }
    delete[] name;
    return Qnil;
}

VALUE MSP::Music::rbf_get_all_music(VALUE self) {
    VALUE v_container = rb_ary_new2((unsigned int)s_valid_music.size());
    int count = 0;
    for (std::map<Mix_Music*, MusicData*>::iterator it = s_valid_music.begin(); it != s_valid_music.end(); ++it) {
        rb_ary_store(v_container, count, c_music_to_value(it->first));
        ++count;
    }
    return v_container;
}

VALUE MSP::Music::rbf_play(VALUE self, VALUE v_address, VALUE v_repeat) {
    Mix_Music* music = c_value_to_music(v_address);
    Mix_HaltMusic();
    Mix_SetMusicCMD(NULL);
    return Mix_PlayMusic(music, Util::value_to_int(v_repeat)) == 0 ? Qtrue : Qfalse;
}

VALUE MSP::Music::rbf_pause(VALUE self) {
    Mix_PauseMusic();
    return Qnil;
}

VALUE MSP::Music::rbf_resume(VALUE self) {
    Mix_ResumeMusic();
    return Qnil;
}

VALUE MSP::Music::rbf_stop(VALUE self) {
    Mix_HaltMusic();
    return Qnil;
}

VALUE MSP::Music::rbf_fade_in(VALUE self, VALUE v_address, VALUE v_repeat, VALUE v_time) {
    Mix_Music* music = c_value_to_music(v_address);
    Mix_SetMusicCMD(NULL);
    return Mix_FadeInMusic(music, Util::value_to_int(v_repeat), Util::value_to_int(v_time)) == 0 ? Qtrue : Qfalse;
}

VALUE MSP::Music::rbf_fade_out(VALUE self, VALUE v_time) {
    return Mix_FadeOutMusic(Util::value_to_int(v_time)) == 1 ? Qtrue : Qfalse;
}

VALUE MSP::Music::rbf_get_volume(VALUE self) {
    return Util::to_value(Mix_VolumeMusic(-1));
}

VALUE MSP::Music::rbf_set_volume(VALUE self, VALUE v_volume) {
    int vol = Util::clamp_int(Util::value_to_int(v_volume), 0, 128);
    return Util::to_value(Mix_VolumeMusic(vol));
}

VALUE MSP::Music::rbf_is_playing(VALUE self) {
    return Mix_PlayingMusic() == 1 ? Qtrue : Qfalse;
}

VALUE MSP::Music::rbf_is_paused(VALUE self) {
    return Mix_PausedMusic() == 1 ? Qtrue : Qfalse;
}

VALUE MSP::Music::rbf_is_stopped(VALUE self) {
    return Mix_PlayingMusic() == 0 && Mix_PausedMusic() == 0 ? Qtrue : Qfalse;
}

VALUE MSP::Music::rbf_is_fading(VALUE self) {
    return Mix_FadingMusic() != MIX_NO_FADING ? Qtrue : Qfalse;
}

VALUE MSP::Music::rbf_get_type(VALUE self, VALUE v_address) {
    Mix_Music* music = c_value_to_music(v_address);
    return Util::to_value(Mix_GetMusicType(music));
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Music::init_ruby(VALUE mMSPhysics) {
    VALUE mMusic = rb_define_module_under(mMSPhysics, "Music");

    rb_define_module_function(mMusic, "is_valid?", VALUEFUNC(MSP::Music::rbf_is_valid), 1);
    rb_define_module_function(mMusic, "create_from_dir", VALUEFUNC(MSP::Music::rbf_create_from_dir), 1);
    rb_define_module_function(mMusic, "create_from_buffer", VALUEFUNC(MSP::Music::rbf_create_from_buffer), 2);
    rb_define_module_function(mMusic, "destroy", VALUEFUNC(MSP::Music::rbf_destroy), 1);
    rb_define_module_function(mMusic, "destroy_all", VALUEFUNC(MSP::Music::rbf_destroy_all), 0);
    rb_define_module_function(mMusic, "get_name", VALUEFUNC(MSP::Music::rbf_get_name), 1);
    rb_define_module_function(mMusic, "set_name", VALUEFUNC(MSP::Music::rbf_set_name), 2);
    rb_define_module_function(mMusic, "get_by_name", VALUEFUNC(MSP::Music::rbf_get_by_name), 1);
    rb_define_module_function(mMusic, "get_all_music", VALUEFUNC(MSP::Music::rbf_get_all_music), 0);
    rb_define_module_function(mMusic, "play", VALUEFUNC(MSP::Music::rbf_play), 2);
    rb_define_module_function(mMusic, "pause", VALUEFUNC(MSP::Music::rbf_pause), 0);
    rb_define_module_function(mMusic, "resume", VALUEFUNC(MSP::Music::rbf_resume), 0);
    rb_define_module_function(mMusic, "stop", VALUEFUNC(MSP::Music::rbf_stop), 0);
    rb_define_module_function(mMusic, "fade_in", VALUEFUNC(MSP::Music::rbf_fade_in), 3);
    rb_define_module_function(mMusic, "fade_out", VALUEFUNC(MSP::Music::rbf_fade_out), 1);
    rb_define_module_function(mMusic, "get_volume", VALUEFUNC(MSP::Music::rbf_get_volume), 0);
    rb_define_module_function(mMusic, "set_volume", VALUEFUNC(MSP::Music::rbf_set_volume), 1);
    rb_define_module_function(mMusic, "is_playing?", VALUEFUNC(MSP::Music::rbf_is_playing), 0);
    rb_define_module_function(mMusic, "is_paused?", VALUEFUNC(MSP::Music::rbf_is_paused), 0);
    rb_define_module_function(mMusic, "is_stopped?", VALUEFUNC(MSP::Music::rbf_is_stopped), 0);
    rb_define_module_function(mMusic, "is_fading?", VALUEFUNC(MSP::Music::rbf_is_fading), 0);
    rb_define_module_function(mMusic, "get_type", VALUEFUNC(MSP::Music::rbf_get_type), 1);
}
