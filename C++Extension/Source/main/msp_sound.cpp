/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_sound.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

std::map<Mix_Chunk*, MSP::Sound::SoundData*> MSP::Sound::s_valid_sounds;
std::map<int, MSP::Sound::SoundData2*> MSP::Sound::s_registered_channels;


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

bool MSP::Sound::c_is_valid(Mix_Chunk* address) {
    return s_valid_sounds.find(address) != s_valid_sounds.end();
}

Mix_Chunk* MSP::Sound::c_value_to_sound(VALUE v_address) {
    Mix_Chunk* address = reinterpret_cast<Mix_Chunk*>(Util::value_to_ull(v_address));
    if (s_valid_sounds.find(address) == s_valid_sounds.end())
        rb_raise(rb_eTypeError, "Given address is not a reference to a valid sound!");
    return address;
}

VALUE MSP::Sound::c_sound_to_value(Mix_Chunk* address) {
    return rb_ull2inum(reinterpret_cast<unsigned long long>(address));
}

bool MSP::Sound::c_unregister_channel_effect(int channel) {
    if (s_registered_channels.find(channel) == s_registered_channels.end())
        return false;
    delete s_registered_channels[channel];
    s_registered_channels.erase(channel);
    return true;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Sound::rbf_is_valid(VALUE self, VALUE v_address) {
    return c_is_valid(reinterpret_cast<Mix_Chunk*>(Util::value_to_ull(v_address))) ? Qtrue : Qfalse;
}

VALUE MSP::Sound::rbf_create_from_dir(VALUE self, VALUE v_path) {
    Mix_Chunk* sound = Mix_LoadWAV(Util::value_to_c_str(v_path));
    if (!sound)
        rb_raise(rb_eTypeError, "Given path is not a reference to a valid sound!");
    s_valid_sounds[sound] = new SoundData(nullptr, nullptr, nullptr);
    return c_sound_to_value(sound);
}

VALUE MSP::Sound::rbf_create_from_buffer(VALUE self, VALUE v_buffer, VALUE v_buffer_size) {
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
    s_valid_sounds[sound] = new SoundData(nullptr, allocated_buffer, rw);
    return c_sound_to_value(sound);
}

VALUE MSP::Sound::rbf_destroy(VALUE self, VALUE v_address) {
    Mix_Chunk* sound = c_value_to_sound(v_address);
    std::map<int, SoundData2*>::iterator it(s_registered_channels.begin());
    while (it != s_registered_channels.end()) {;
        if (sound == Mix_GetChunk(it->first))
            s_registered_channels.erase(it++);
        else
            ++it;
    }
    Mix_FreeChunk(sound);
    SoundData* data = s_valid_sounds[sound];
    if (data->m_rw) SDL_RWclose(data->m_rw);
    if (data->m_name) delete[] data->m_name;
    if (data->m_buffer) delete[] data->m_buffer;
    delete data;
    s_valid_sounds.erase(sound);
    return Qnil;
}

VALUE MSP::Sound::rbf_destroy_all(VALUE self) {
    for (std::map<int, SoundData2*>::iterator it = s_registered_channels.begin(); it != s_registered_channels.end(); ++it)
        delete it->second;
    s_registered_channels.clear();
    for (std::map<Mix_Chunk*, SoundData*>::iterator it = s_valid_sounds.begin(); it != s_valid_sounds.end(); ++it) {
        Mix_FreeChunk(it->first);
        SoundData* data = it->second;
        if (data->m_rw) SDL_RWclose(data->m_rw);
        if (data->m_name) delete[] data->m_name;
        if (data->m_buffer) delete[] data->m_buffer;
        delete data;
    }
    unsigned int size = (unsigned int)s_valid_sounds.size();
    s_valid_sounds.clear();
    return Util::to_value(size);
}

VALUE MSP::Sound::rbf_get_name(VALUE self, VALUE v_address) {
    Mix_Chunk* sound = c_value_to_sound(v_address);
    SoundData* data = s_valid_sounds[sound];
    return data->m_name == nullptr ? Qnil : Util::to_value(data->m_name);
}

VALUE MSP::Sound::rbf_set_name(VALUE self, VALUE v_address, VALUE v_name) {
    Mix_Chunk* sound = c_value_to_sound(v_address);
    SoundData* data = s_valid_sounds[sound];
    wchar_t* name = Util::value_to_c_str2(v_name);
    if (data->m_name) delete[] data->m_name;
    data->m_name = name;
    return Util::to_value(data->m_name);
}

VALUE MSP::Sound::rbf_get_by_name(VALUE self, VALUE v_name) {
    const wchar_t* name = Util::value_to_c_str2(v_name);
    size_t len(wcslen(name));
    if (len == 0) {
        delete[] name;
        return Qnil;
    }
    for (std::map<Mix_Chunk*, SoundData*>::iterator it = s_valid_sounds.begin(); it != s_valid_sounds.end(); ++it)
        if (it->second->m_name != nullptr && wcscmp(it->second->m_name, name) == 0) {
            delete[] name;
            return c_sound_to_value(it->first);
        }
    delete[] name;
    return Qnil;
}

VALUE MSP::Sound::rbf_get_all_sounds(VALUE self) {
    VALUE v_container = rb_ary_new2((unsigned int)s_valid_sounds.size());
    int count = 0;
    for (std::map<Mix_Chunk*, SoundData*>::iterator it = s_valid_sounds.begin(); it != s_valid_sounds.end(); ++it) {
        rb_ary_store(v_container, count, c_sound_to_value(it->first));
        ++count;
    }
    return v_container;
}

VALUE MSP::Sound::rbf_get_sound(VALUE self, VALUE v_channel) {
    int channel = Util::max_int(Util::value_to_int(v_channel), 0);
    Mix_Chunk* sound = Mix_GetChunk(channel);
    if (Mix_Playing(channel) == 1 && c_is_valid(sound))
        return c_sound_to_value(sound);
    else
        return Qnil;
}

VALUE MSP::Sound::rbf_play(VALUE self, VALUE v_address, VALUE v_channel, VALUE v_repeat) {
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

VALUE MSP::Sound::rbf_pause(VALUE self, VALUE v_channel) {
    Mix_Pause(Util::value_to_int(v_channel));
    return Qnil;
}

VALUE MSP::Sound::rbf_resume(VALUE self, VALUE v_channel) {
    Mix_Resume(Util::value_to_int(v_channel));
    return Qnil;
}

VALUE MSP::Sound::rbf_stop(VALUE self, VALUE v_channel) {
    Mix_HaltChannel(Util::value_to_int(v_channel));
    return Qnil;
}

VALUE MSP::Sound::rbf_fade_in(VALUE self, VALUE v_address, VALUE v_channel, VALUE v_repeat, VALUE v_time) {
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

VALUE MSP::Sound::rbf_fade_out(VALUE self, VALUE v_channel, VALUE v_time) {
    int channel = Util::value_to_int(v_channel);
    int time = Util::value_to_int(v_time);
    return Util::to_value( Mix_FadeOutChannel(channel, time) );
}

VALUE MSP::Sound::rbf_get_volume(VALUE self, VALUE v_channel) {
    int channel = Util::value_to_int(v_channel);
    return Util::to_value( Mix_Volume(channel, -1) );
}

VALUE MSP::Sound::rbf_set_volume(VALUE self, VALUE v_channel, VALUE v_volume) {
    int channel = Util::value_to_int(v_channel);
    int volume = Util::clamp_int(Util::value_to_int(v_volume), 0, 128);
    return Util::to_value( Mix_Volume(channel, volume) );
}

VALUE MSP::Sound::rbf_is_playing(VALUE self, VALUE v_channel) {
    int channel = Util::max_int(Util::value_to_int(v_channel), 0);
    return Mix_Playing(channel) == 1 && Mix_Paused(channel) == 0 ? Qtrue : Qfalse;
}

VALUE MSP::Sound::rbf_is_paused(VALUE self, VALUE v_channel) {
    int channel = Util::max_int(Util::value_to_int(v_channel), 0);
    return Mix_Playing(channel) == 1 && Mix_Paused(channel) == 1 ? Qtrue : Qfalse;
}

VALUE MSP::Sound::rbf_is_stopped(VALUE self, VALUE v_channel) {
    int channel = Util::max_int(Util::value_to_int(v_channel), 0);
    return Mix_Playing(channel) == 0 ? Qtrue : Qfalse;
}

VALUE MSP::Sound::rbf_is_fading(VALUE self, VALUE v_channel) {
    int channel = Util::max_int(Util::value_to_int(v_channel), 0);
    return Mix_FadingChannel(channel) != MIX_NO_FADING ? Qtrue : Qfalse;
}

VALUE MSP::Sound::rbf_set_panning(VALUE self, VALUE v_channel, VALUE v_left_range, VALUE v_right_range) {
    int channel = Util::max_int(Util::value_to_int(v_channel), 0);
    int left = Util::clamp_int(Util::value_to_int(v_left_range), 0, 255);
    int right = Util::clamp_int(Util::value_to_int(v_right_range), 0, 255);
    return Mix_SetPanning(channel, (Uint8)left, (Uint8)right) != 0 ? Qtrue : Qfalse;
}

VALUE MSP::Sound::rbf_set_distance(VALUE self, VALUE v_channel, VALUE v_distance) {
    int channel = Util::max_int(Util::value_to_int(v_channel), 0);
    int distance = Util::clamp_int(Util::value_to_int(v_distance), 0, 255);
    return Mix_SetDistance(channel, (Uint8)distance) != 0 ? Qtrue : Qfalse;
}

VALUE MSP::Sound::rbf_set_position(VALUE self, VALUE v_channel, VALUE v_angle, VALUE v_distance) {
    int channel = Util::max_int(Util::value_to_int(v_channel), 0);
    int angle = Util::value_to_int(v_angle);
    int distance = Util::clamp_int(Util::value_to_int(v_distance), 0, 255);
    return Mix_SetPosition(channel, (Sint16)angle, (Uint8)distance) != 0 ? Qtrue : Qfalse;
}

VALUE MSP::Sound::rbf_unregister_effects(VALUE self, VALUE v_channel)
{
    int channel = Util::max_int(Util::value_to_int(v_channel), 0);
    c_unregister_channel_effect(channel);
    return Mix_UnregisterAllEffects(channel) != 0 ? Qtrue : Qfalse;
}

VALUE MSP::Sound::rbf_set_position_3d(VALUE self, VALUE v_channel, VALUE v_position, VALUE v_max_hearing_range) {
    int channel = Util::max_int(Util::value_to_int(v_channel), 0);
    dVector position(Util::value_to_point(v_position));
    double max_hearing_range = Util::max_double(Util::value_to_double(v_max_hearing_range), 1.0);
    Mix_Chunk* sound = Mix_GetChunk(channel);
    if (Mix_Playing(channel) == 0 || !c_is_valid(sound))
        return Qfalse;
    c_unregister_channel_effect(channel);
    s_registered_channels[channel] = new SoundData2(position, max_hearing_range);
    return Qtrue;
}

VALUE MSP::Sound::rbf_update_effects(VALUE self) {
    if (s_registered_channels.empty())
        return Qfalse;
    VALUE v_model = rb_funcall(Util::SU_SKETCHUP, Util::INTERN_ACTIVE_MODEL, 0);
    VALUE v_view = rb_funcall(v_model, Util::INTERN_ACTIVE_VIEW, 0);
    VALUE v_cam = rb_funcall(v_view, Util::INTERN_CAMERA, 0);
    dVector eye(Util::value_to_point( rb_funcall(v_cam, Util::INTERN_EYE, 0) ));
    dVector xaxis(Util::value_to_vector( rb_funcall(v_cam, Util::INTERN_XAXIS, 0) ));
    dVector yaxis(Util::value_to_vector( rb_funcall(v_cam, Util::INTERN_YAXIS, 0) ));
    dVector zaxis(Util::value_to_vector( rb_funcall(v_cam, Util::INTERN_ZAXIS, 0) ));
    dMatrix matrix(zaxis, xaxis, yaxis, eye);
    std::map<int, SoundData2*>::iterator it(s_registered_channels.begin());
    while (it != s_registered_channels.end()) {
        Mix_Chunk* sound = Mix_GetChunk(it->first);
        if (Mix_Playing(it->first) == 0 || !c_is_valid(sound)) {
            s_registered_channels.erase(it++);
            continue;
        }
        dVector pos(matrix.UntransformVector(it->second->m_position));
        double dist = sqrt(pos.m_x*pos.m_x + pos.m_y*pos.m_y + pos.m_z*pos.m_z);
        if (dist > it->second->m_max_hearing_range) {
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
        if (pos.m_y < 0) angle = M_SPI * (dFloat)(2.0) - angle;
        Sint16 deg_angle(Sint16(angle * M_RAD_TO_DEG));
        Uint8 vol(Uint8(dist * 255.0 / it->second->m_max_hearing_range));
        Mix_SetPosition(it->first, deg_angle, vol);
        ++it;
    }
    return Qtrue;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Sound::init_ruby(VALUE mMSPhysics) {
    VALUE mSound = rb_define_module_under(mMSPhysics, "Sound");

    rb_define_module_function(mSound, "is_valid?", VALUEFUNC(MSP::Sound::rbf_is_valid), 1);
    rb_define_module_function(mSound, "create_from_dir", VALUEFUNC(MSP::Sound::rbf_create_from_dir), 1);
    rb_define_module_function(mSound, "create_from_buffer", VALUEFUNC(MSP::Sound::rbf_create_from_buffer), 2);
    rb_define_module_function(mSound, "destroy", VALUEFUNC(MSP::Sound::rbf_destroy), 1);
    rb_define_module_function(mSound, "destroy_all", VALUEFUNC(MSP::Sound::rbf_destroy_all), 0);
    rb_define_module_function(mSound, "get_name", VALUEFUNC(MSP::Sound::rbf_get_name), 1);
    rb_define_module_function(mSound, "set_name", VALUEFUNC(MSP::Sound::rbf_set_name), 2);
    rb_define_module_function(mSound, "get_by_name", VALUEFUNC(MSP::Sound::rbf_get_by_name), 1);
    rb_define_module_function(mSound, "get_all_sounds", VALUEFUNC(MSP::Sound::rbf_get_all_sounds), 0);
    rb_define_module_function(mSound, "get_sound", VALUEFUNC(MSP::Sound::rbf_get_sound), 1);
    rb_define_module_function(mSound, "play", VALUEFUNC(MSP::Sound::rbf_play), 3);
    rb_define_module_function(mSound, "pause", VALUEFUNC(MSP::Sound::rbf_pause), 1);
    rb_define_module_function(mSound, "resume", VALUEFUNC(MSP::Sound::rbf_resume), 1);
    rb_define_module_function(mSound, "stop", VALUEFUNC(MSP::Sound::rbf_stop), 1);
    rb_define_module_function(mSound, "fade_in", VALUEFUNC(MSP::Sound::rbf_fade_in), 4);
    rb_define_module_function(mSound, "fade_out", VALUEFUNC(MSP::Sound::rbf_fade_out), 2);
    rb_define_module_function(mSound, "get_volume", VALUEFUNC(MSP::Sound::rbf_get_volume), 1);
    rb_define_module_function(mSound, "set_volume", VALUEFUNC(MSP::Sound::rbf_set_volume), 2);
    rb_define_module_function(mSound, "is_playing?", VALUEFUNC(MSP::Sound::rbf_is_playing), 1);
    rb_define_module_function(mSound, "is_paused?", VALUEFUNC(MSP::Sound::rbf_is_paused), 1);
    rb_define_module_function(mSound, "is_stopped?", VALUEFUNC(MSP::Sound::rbf_is_stopped), 1);
    rb_define_module_function(mSound, "is_fading?", VALUEFUNC(MSP::Sound::rbf_is_fading), 1);
    rb_define_module_function(mSound, "set_panning", VALUEFUNC(MSP::Sound::rbf_set_panning), 3);
    rb_define_module_function(mSound, "set_distance", VALUEFUNC(MSP::Sound::rbf_set_distance), 2);
    rb_define_module_function(mSound, "set_position", VALUEFUNC(MSP::Sound::rbf_set_position), 3);
    rb_define_module_function(mSound, "unregister_effects", VALUEFUNC(MSP::Sound::rbf_unregister_effects), 1);
    rb_define_module_function(mSound, "set_position_3d", VALUEFUNC(MSP::Sound::rbf_set_position_3d), 3);
    rb_define_module_function(mSound, "update_effects", VALUEFUNC(MSP::Sound::rbf_update_effects), 0);
}
