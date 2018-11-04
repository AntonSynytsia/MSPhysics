/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_SDL_H
#define MSP_SDL_H

#include "msp.h"
#include "SDL.h"

class MSP::SDL {
public:
    // Structures
    struct compare_c_str {
        bool operator()(const char* a, const char* b) {
            return std::strcmp(a, b) < 0;
        }
    };

    // Ruby Function
    static VALUE rbf_init(VALUE self, VALUE v_flags);
    static VALUE rbf_init_sub_system(VALUE self, VALUE v_flags);
    static VALUE rbf_quit(VALUE self);
    static VALUE rbf_quit_sub_system(VALUE self, VALUE v_flags);
    static VALUE rbf_is_init(VALUE self, VALUE v_flags);
    static VALUE rbf_get_error(VALUE self);

    /*static VALUE rbf_get_key_state(VALUE self, VALUE v_vk);

    static VALUE rbf_name_to_key_code(VALUE self, VALUE v_key_name);
    static VALUE rbf_key_code_to_name(VALUE self, VALUE v_key_code);

    static VALUE rbf_name_to_scan_code(VALUE self, VALUE v_key_name);
    static VALUE rbf_scan_code_to_name(VALUE self, VALUE v_scan_code);

    static VALUE rbf_key_code_to_scan_code(VALUE self, VALUE v_key_code);
    static VALUE rbf_scan_code_to_key_code(VALUE self, VALUE v_scan_code);

    static VALUE rbf_get_global_mouse_state(VALUE self);
    static VALUE rbf_get_mouse_state(VALUE self);

    static VALUE rbf_get_global_cursor_pos(VALUE self);
    static VALUE rbf_get_cursor_pos(VALUE self);

    static VALUE rbf_set_global_cursor_pos(VALUE self, VALUE v_x, VALUE v_y);
    static VALUE rbf_set_cursor_pos(VALUE self, VALUE v_x, VALUE v_y);

    static VALUE rbf_show_cursor(VALUE self, VALUE v_state);
    static VALUE rbf_cursor_visible(VALUE self);

    static VALUE rbf_pump_events(VALUE self);*/

    // Main
    static void init_ruby(VALUE mMSPhysics);
};

#endif  /* MSP_SDL_H */
