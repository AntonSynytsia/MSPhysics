/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "msp_particle.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Particle::SYM_POSITION;
VALUE MSP::Particle::SYM_VELOCITY;
VALUE MSP::Particle::SYM_GRAVITY;
VALUE MSP::Particle::SYM_VELOCITY_DAMP;
VALUE MSP::Particle::SYM_RADIUS;
VALUE MSP::Particle::SYM_SCALE;
VALUE MSP::Particle::SYM_COLOR1;
VALUE MSP::Particle::SYM_COLOR2;
VALUE MSP::Particle::SYM_ALPHA1;
VALUE MSP::Particle::SYM_ALPHA2;
VALUE MSP::Particle::SYM_FADE;
VALUE MSP::Particle::SYM_LIFETIME;
VALUE MSP::Particle::SYM_NUM_SEG;
VALUE MSP::Particle::SYM_ROT_ANGLE;

VALUE MSP::Particle::V_GL_TRIANGLE_FAN;
VALUE MSP::Particle::V_GL_POLYGON;

VALUE MSP::Particle::s_reg_symbols;
std::set<MSP::Particle::ParticleData*> MSP::Particle::s_valid_particles;


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

bool MSP::Particle::c_is_valid(ParticleData* address) {
    return s_valid_particles.find(address) != s_valid_particles.end();
}

MSP::Particle::ParticleData* MSP::Particle::c_value_to_particle(VALUE v_address) {
    ParticleData* address = reinterpret_cast<ParticleData*>(Util::value_to_ull(v_address));
    if (s_valid_particles.find(address) == s_valid_particles.end())
        rb_raise(rb_eTypeError, "Given address doesn't reference a valid particle!");
    return address;
}

VALUE MSP::Particle::c_particle_to_value(ParticleData* address) {
    return rb_ull2inum(reinterpret_cast<unsigned long long>(address));
}

bool MSP::Particle::c_update_particle(ParticleData* data, dFloat timestep) {
    // Control radius
    data->m_radius *= data->m_scale;
    // Increment life
    data->m_cur_life += timestep;
    // Check if need to delete the particle
    if (data->m_radius < 0.01f || data->m_radius > 100000.0 || data->m_cur_life > data->m_lifetime) {
        s_valid_particles.erase(data);
        delete[] data->m_pts;
        delete data;
        return false;
    }
    // Calc life ratio
    dFloat ratio = data->m_cur_life / data->m_lifetime;
    // Transition color
    if (data->m_use_color2) {
        data->m_color.m_x = data->m_color1.m_x + (data->m_color2.m_x - data->m_color1.m_x) * ratio;
        data->m_color.m_y = data->m_color1.m_y + (data->m_color2.m_y - data->m_color1.m_y) * ratio;
        data->m_color.m_z = data->m_color1.m_z + (data->m_color2.m_z - data->m_color1.m_z) * ratio;
    }
    // Transition opacity
    if (data->m_use_alpha2) {
        if (data->m_fade < M_EPSILON)
            data->m_alpha = data->m_alpha1 + (data->m_alpha2 - data->m_alpha1) * ratio;
        else {
            dFloat fh = data->m_fade * 0.5f;
            if (ratio < fh) {
                dFloat fr = data->m_cur_life / (data->m_lifetime * fh);
                data->m_alpha = data->m_alpha1 * fr;
            }
            else if (ratio >= (1.0 - fh)) {
                dFloat fr = (data->m_lifetime - data->m_cur_life) / (data->m_lifetime * fh);
                data->m_alpha = data->m_alpha2 * fr;
            }
            else {
                dFloat fl = data->m_lifetime * data->m_fade;
                dFloat fr = (data->m_cur_life - fl * 0.5f) / (data->m_lifetime - fl);
                data->m_alpha = data->m_alpha1 + (data->m_alpha2 - data->m_alpha1) * fr;
            }
        }
    }
    else {
        if (data->m_fade < M_EPSILON)
            data->m_alpha = data->m_alpha1;
        else {
            dFloat fh = data->m_fade * 0.5f;
            if (ratio < fh) {
                dFloat fr = data->m_cur_life / (data->m_lifetime * fh);
                data->m_alpha = data->m_alpha1 * fr;
            }
            else if (ratio >= (1.0 - fh)) {
                dFloat fr = (data->m_lifetime - data->m_cur_life) / (data->m_lifetime * fh);
                data->m_alpha = data->m_alpha1 * fr;
            }
            else {
                data->m_alpha = data->m_alpha1;
            }
        }
    }
    // Control velocity and position
    if (data->m_use_velocity) {
        if (data->m_use_gravity)
            data->m_velocity += data->m_gravity.Scale(timestep);
        if (data->m_velocity_damp > M_EPSILON) {
            dFloat s = 1.0f - data->m_velocity_damp;
            data->m_velocity.m_x *= s;
            data->m_velocity.m_y *= s;
            data->m_velocity.m_z *= s;
        }
        data->m_position += data->m_velocity.Scale(timestep);
    }
    return true;
}

void MSP::Particle::c_draw_particle(ParticleData* data, VALUE v_view, VALUE v_bb, const dMatrix& camera_tra) {
    dVector zaxis(camera_tra.m_posit - data->m_position);
    dFloat nmag = Util::get_vector_magnitude(zaxis);
    if (nmag < M_EPSILON)
        zaxis = camera_tra.m_right;
    else {
        dFloat r = 1.0f / nmag;
        zaxis.m_x *= r;
        zaxis.m_y *= r;
        zaxis.m_z *= r;
        zaxis.m_w = 0.0f;
    }
    //VALUE v_pts = c_points_on_circle3d(data->position, data->radius, normal, data->num_seg, data->rot_angle);
    //~rb_funcall(v_bb, Util::INTERN_ADD, 1, Util::point_to_value2(data->position));
    //rb_funcall(v_view, Util::INTERN_SDRAWING_COLOR, 1, Util::color_to_value(data->color, data->alpha));
    VALUE v_color = Util::color_to_value(data->m_color, data->m_alpha);
    rb_funcall(v_view, Util::INTERN_SDRAWING_COLOR, 1, v_color);
    //rb_funcall(v_view, Util::INTERN_DRAW, 2, V_GL_POLYGON, v_pts);

    /*VALUE v_pts = rb_ary_new2(data->num_seg);
    dMatrix cmatrix;
    Util::matrix_from_pin_dir(data->position, normal, cmatrix);
    dFloat offset = M_2PI / data->num_seg;
    for (unsigned int i = 0; i < data->num_seg; ++i) {
        dFloat angle = data->rot_angle + i * offset;
        dVector pt(dCos(angle) * data->radius, dSin(angle) * data->radius, 0.0f);
        rb_ary_store(v_pts, i, Util::point_to_value2(cmatrix.TransformVector(pt)));
    }
    rb_funcall(v_view, Util::INTERN_DRAW, 2, V_GL_POLYGON, v_pts);*/ // 42/45 FPS at 6

    /*VALUE v_pts = rb_ary_new2(data->num_seg+2);
    rb_ary_store(v_pts, 0, Util::point_to_value2(data->position));
    dMatrix cmatrix;
    Util::matrix_from_pin_dir(data->position, normal, cmatrix);
    for (unsigned int i = 0; i < data->num_seg; ++i) {
        dVector pt(data->pts[i*2] * data->radius, data->pts[i*2+1] * data->radius, 0.0f);
        rb_ary_store(v_pts, i+1, Util::point_to_value2(cmatrix.TransformVector(pt)));
    }
    rb_ary_store(v_pts, data->num_seg+1, rb_ary_entry(v_pts, 1));
    rb_funcall(v_view, Util::INTERN_DRAW, 2, V_GL_TRIANGLE_FAN, v_pts);*/ // 42/45 at 6

    VALUE v_pts = rb_ary_new2(data->m_num_seg);
    //dMatrix cmatrix;
    //Util::matrix_from_pin_dir(data->position, normal, cmatrix);
    dVector xaxis;
    if (dAbs(zaxis.m_z) > 0.9999995f) {
        //xaxis = Y_AXIS.CrossProduct(zaxis);
        xaxis.m_x = zaxis.m_z;
        xaxis.m_y = 0.0f;
        xaxis.m_z = -zaxis.m_x;
    }
    else {
        //xaxis = Z_AXIS.CrossProduct(zaxis);
        xaxis.m_x = -zaxis.m_y;
        xaxis.m_y = zaxis.m_x;
        xaxis.m_z = 0.0f;
    }
    dVector yaxis(zaxis.CrossProduct(xaxis));
    nmag = Util::get_vector_magnitude(xaxis);
    dFloat r = data->m_radius / nmag;
    xaxis.m_x *= r;
    xaxis.m_y *= r;
    xaxis.m_z *= r;
    xaxis.m_w = 0.0f;
    nmag = Util::get_vector_magnitude(yaxis);
    r = data->m_radius / nmag;
    yaxis.m_x *= r;
    yaxis.m_y *= r;
    yaxis.m_z *= r;
    yaxis.m_w = 0.0f;
    dMatrix cmatrix(xaxis, yaxis, zaxis, data->m_position);
    for (unsigned int i = 0; i < data->m_num_seg; ++i) {
        dVector pt(data->m_pts[i*2], data->m_pts[i*2+1], 0.0f);
        rb_ary_store(v_pts, i, Util::point_to_value(cmatrix.TransformVector(pt)));
    }
    rb_funcall(v_view, Util::INTERN_DRAW, 2, V_GL_POLYGON, v_pts); // 44/46 FPS at 6
}

VALUE MSP::Particle::c_points_on_circle2d(const dVector& origin, dFloat radius, unsigned int num_seg, dFloat rot_angle) {
    dFloat offset = M_SPI * (dFloat)(2.0) / num_seg;
    dFloat angle = rot_angle * M_DEG_TO_RAD;
    VALUE v_pts = rb_ary_new2(num_seg);
    for (unsigned int i = 0; i < num_seg; ++i) {
        dVector pt(origin.m_x + dCos(angle) * radius, origin.m_y + dSin(angle) * radius, origin.m_z);
        rb_ary_store(v_pts, i, Util::point_to_value(pt));
        angle += offset;
    }
    return v_pts;
}

VALUE MSP::Particle::c_points_on_circle3d(const dVector& origin, dFloat radius, const dVector& normal, unsigned int num_seg, dFloat rot_angle) {
    dMatrix cmatrix;
    Util::matrix_from_pin_dir(origin, normal, cmatrix);
    dFloat offset = M_SPI * (dFloat)(2.0) / num_seg;
    dFloat angle = rot_angle * M_DEG_TO_RAD;
    VALUE v_pts = rb_ary_new2(num_seg);
    for (unsigned int i = 0; i < num_seg; ++i) {
        dVector pt(dCos(angle) * radius, dSin(angle) * radius, 0.0f);
        rb_ary_store(v_pts, i, Util::point_to_value(cmatrix.TransformVector(pt)));
        angle += offset;
    }
    return v_pts;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Particle::rbf_is_valid(VALUE self, VALUE v_address) {
    return c_is_valid(reinterpret_cast<ParticleData*>(Util::value_to_ull(v_address))) ? Qtrue : Qfalse;
}

VALUE MSP::Particle::rbf_create(VALUE self, VALUE v_opts) {
    if (TYPE(v_opts) != T_HASH)
        rb_raise(rb_eTypeError, "Expected a hash with particle options.");

    ParticleData tdata;
    VALUE val;

    // Position
    val = rb_hash_aref(v_opts, SYM_POSITION);
    tdata.m_position = (val != Qnil) ? Util::value_to_point(val) : Util::ORIGIN;
    // Velocity
    val = rb_hash_aref(v_opts, SYM_VELOCITY);
    if (val != Qnil) {
        tdata.m_velocity = Util::value_to_vector(val);
        tdata.m_use_velocity    = true;
    }
    else
        tdata.m_use_velocity = false;
    // Velocity damp
    val = rb_hash_aref(v_opts, SYM_VELOCITY_DAMP);
    tdata.m_velocity_damp = (val != Qnil) ? Util::clamp_float(Util::value_to_dFloat(val), 0.0f, 1.0f) : 0.0f;
    // Gravity
    val = rb_hash_aref(v_opts, SYM_GRAVITY);
    if (val != Qnil) {
        tdata.m_gravity = Util::value_to_vector(val);
        tdata.m_use_gravity = true;
    }
    else
        tdata.m_use_gravity = false;
    // Radius
    val = rb_hash_aref(v_opts, SYM_RADIUS);
    tdata.m_radius = (val != Qnil) ? Util::clamp_float(Util::value_to_dFloat(val), 0.01f, 10000.0f) : 1.0f;
    // Scale
    val = rb_hash_aref(v_opts, SYM_SCALE);
    tdata.m_scale = (val != Qnil) ? Util::clamp_float(Util::value_to_dFloat(val), 0.001f, 1000.0f) : 1.01f;
    // Color1
    val = rb_hash_aref(v_opts, SYM_COLOR1);
    tdata.m_color1 = (val != Qnil) ? Util::value_to_color(val) : dVector(100.0f, 100.0f, 100.0f, 1.0f);
    // Color2
    val = rb_hash_aref(v_opts, SYM_COLOR2);
    if (val != Qnil) {
        tdata.m_color2 = Util::value_to_color(val);
        tdata.m_use_color2 = true;
    }
    else
        tdata.m_use_color2 = false;
    // Alpha1
    val = rb_hash_aref(v_opts, SYM_ALPHA1);
    tdata.m_alpha1 = (val != Qnil) ? Util::clamp_float(Util::value_to_dFloat(val), 0.0f, 1.0f) : 1.0f;
    // Alpha2
    val = rb_hash_aref(v_opts, SYM_ALPHA2);
    if (val != Qnil) {
        tdata.m_alpha2 = Util::clamp_float(Util::value_to_dFloat(val), 0.0f, 1.0f);
        tdata.m_use_alpha2 = true;
    }
    else
        tdata.m_use_alpha2 = false;
    // Fade
    val = rb_hash_aref(v_opts, SYM_FADE);
    tdata.m_fade = (val != Qnil) ? Util::clamp_float(Util::value_to_dFloat(val), 0.0f, 1.0f) : 1.0f;
    // Lifetime
    val = rb_hash_aref(v_opts, SYM_LIFETIME);
    tdata.m_lifetime = (val != Qnil) ? Util::max_float(Util::value_to_dFloat(val), M_EPSILON) : 2.0f;
    // Number of segments
    val = rb_hash_aref(v_opts, SYM_NUM_SEG);
    tdata.m_num_seg = (val != Qnil) ? Util::clamp_uint(Util::value_to_uint(val), 3, 120) : 16;
    // Rotate angle
    val = rb_hash_aref(v_opts, SYM_ROT_ANGLE);
    tdata.m_rot_angle = (val != Qnil) ? Util::value_to_dFloat(val) * M_DEG_TO_RAD : 0.0f;

    ParticleData* data = new ParticleData(tdata);
    // Preset current color
    data->m_color = data->m_color1;
    data->m_alpha = data->m_fade < M_EPSILON ? data->m_alpha1 : 0.0f;
    // Preset current life
    data->m_cur_life = 0.0f;
    // Calculate normal points on circle
    data->m_pts = new dFloat[data->m_num_seg*2];
    dFloat offset = M_SPI * (dFloat)(2.0) / data->m_num_seg;
    dFloat angle = data->m_rot_angle;
    for (unsigned int i = 0; i < data->m_num_seg; ++i) {
        data->m_pts[i*2] = dCos(angle);
        data->m_pts[i*2+1] = dSin(angle);
        angle += offset;
    }

    s_valid_particles.insert(data);
    return c_particle_to_value(data);
}

VALUE MSP::Particle::rbf_update(VALUE self, VALUE v_address, VALUE v_timestep) {
    ParticleData* data = c_value_to_particle(v_address);
    dFloat timestep = Util::value_to_dFloat(v_timestep);
    return Util::to_value(c_update_particle(data, timestep));
}

VALUE MSP::Particle::rbf_draw(VALUE self, VALUE v_address, VALUE v_view, VALUE v_bb) {
    ParticleData* data = c_value_to_particle(v_address);
    VALUE v_camera = rb_funcall(v_view, Util::INTERN_CAMERA, 0);
    VALUE v_eye = rb_funcall(v_camera, Util::INTERN_EYE, 0);
    VALUE v_xaxis = rb_funcall(v_camera, Util::INTERN_XAXIS, 0); // camera side
    VALUE v_yaxis = rb_funcall(v_camera, Util::INTERN_YAXIS, 0); // camera up
    VALUE v_zaxis = rb_funcall(v_camera, Util::INTERN_ZAXIS, 0); // camera front
    dMatrix camera_tra(Util::value_to_vector(v_xaxis), Util::value_to_vector(v_yaxis), Util::value_to_vector(v_zaxis), Util::value_to_vector(v_eye));
    c_draw_particle(data, v_view, v_bb, camera_tra);
    return Qnil;
}

VALUE MSP::Particle::rbf_destroy(VALUE self, VALUE v_address) {
    ParticleData* data = c_value_to_particle(v_address);
    s_valid_particles.erase(data);
    delete[] data->m_pts;
    delete data;
    return Qnil;
}

VALUE MSP::Particle::rbf_update_all(VALUE self, VALUE v_timestep) {
    dFloat timestep = Util::value_to_dFloat(v_timestep);
    for (std::set<ParticleData*>::iterator it = s_valid_particles.begin(); it != s_valid_particles.end();) {
        ParticleData* data = *it;
        ++it;
        c_update_particle(data, timestep);
    }
    return Qnil;
}

VALUE MSP::Particle::rbf_draw_all(VALUE self, VALUE v_view, VALUE v_bb) {
    VALUE v_camera = rb_funcall(v_view, Util::INTERN_CAMERA, 0);
    VALUE v_eye = rb_funcall(v_camera, Util::INTERN_EYE, 0);
    VALUE v_xaxis = rb_funcall(v_camera, Util::INTERN_XAXIS, 0); // camera side
    VALUE v_yaxis = rb_funcall(v_camera, Util::INTERN_YAXIS, 0); // camera up
    VALUE v_zaxis = rb_funcall(v_camera, Util::INTERN_ZAXIS, 0); // camera front
    dMatrix camera_tra(Util::value_to_vector(v_xaxis), Util::value_to_vector(v_yaxis), Util::value_to_vector(v_zaxis), Util::value_to_vector(v_eye));
    std::map<dFloat, std::vector<ParticleData*>> sorted_data;
    for (std::set<ParticleData*>::iterator it = s_valid_particles.begin(); it != s_valid_particles.end(); ++it) {
        ParticleData* data = *it;
        dVector pv(data->m_position - camera_tra.m_posit);
        if (pv.DotProduct3(camera_tra.m_right) > M_EPSILON) {
            dFloat dist = Util::get_vector_magnitude2(pv);
            sorted_data[dist].push_back(data);
        }
    }
    for (std::map<dFloat, std::vector<ParticleData*>>::reverse_iterator it = sorted_data.rbegin(); it != sorted_data.rend(); ++it) {
        for (std::vector<ParticleData*>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            c_draw_particle(*it2, v_view, v_bb, camera_tra);
    }
    return Qnil;
}

VALUE MSP::Particle::rbf_destroy_all(VALUE self) {
    for (std::set<ParticleData*>::iterator it = s_valid_particles.begin(); it != s_valid_particles.end(); ++it) {
        ParticleData* data = *it;
        delete[] data->m_pts;
        delete data;
    }
    s_valid_particles.clear();
    return Qnil;
}

VALUE MSP::Particle::rbf_get_size(VALUE self) {
    return Util::to_value(s_valid_particles.size());
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Particle::init_ruby(VALUE mC) {
    SYM_POSITION        = ID2SYM(rb_intern("position"));
    SYM_VELOCITY        = ID2SYM(rb_intern("velocity"));
    SYM_GRAVITY         = ID2SYM(rb_intern("gravity"));
    SYM_VELOCITY_DAMP   = ID2SYM(rb_intern("velocity_damp"));
    SYM_RADIUS          = ID2SYM(rb_intern("radius"));
    SYM_SCALE           = ID2SYM(rb_intern("scale"));
    SYM_COLOR1          = ID2SYM(rb_intern("color1"));
    SYM_COLOR2          = ID2SYM(rb_intern("color2"));
    SYM_ALPHA1          = ID2SYM(rb_intern("alpha1"));
    SYM_ALPHA2          = ID2SYM(rb_intern("alpha2"));
    SYM_FADE            = ID2SYM(rb_intern("fade"));
    SYM_LIFETIME        = ID2SYM(rb_intern("lifetime"));
    SYM_NUM_SEG         = ID2SYM(rb_intern("num_seg"));
    SYM_ROT_ANGLE       = ID2SYM(rb_intern("rot_angle"));

    V_GL_TRIANGLE_FAN = INT2FIX(6);
    V_GL_POLYGON = INT2FIX(9);

    s_reg_symbols = rb_ary_new();
    rb_gc_register_address(&s_reg_symbols);

    rb_ary_push(s_reg_symbols, SYM_POSITION);
    rb_ary_push(s_reg_symbols, SYM_VELOCITY);
    rb_ary_push(s_reg_symbols, SYM_GRAVITY);
    rb_ary_push(s_reg_symbols, SYM_VELOCITY_DAMP);
    rb_ary_push(s_reg_symbols, SYM_RADIUS);
    rb_ary_push(s_reg_symbols, SYM_SCALE);
    rb_ary_push(s_reg_symbols, SYM_COLOR1);
    rb_ary_push(s_reg_symbols, SYM_COLOR2);
    rb_ary_push(s_reg_symbols, SYM_ALPHA1);
    rb_ary_push(s_reg_symbols, SYM_ALPHA2);
    rb_ary_push(s_reg_symbols, SYM_FADE);
    rb_ary_push(s_reg_symbols, SYM_LIFETIME);
    rb_ary_push(s_reg_symbols, SYM_NUM_SEG);
    rb_ary_push(s_reg_symbols, SYM_ROT_ANGLE);

    rb_ary_push(s_reg_symbols, V_GL_TRIANGLE_FAN);
    rb_ary_push(s_reg_symbols, V_GL_POLYGON);

    VALUE mParticle = rb_define_module_under(mC, "Particle");

    rb_define_module_function(mParticle, "is_valid?", VALUEFUNC(MSP::Particle::rbf_is_valid), 1);
    rb_define_module_function(mParticle, "create", VALUEFUNC(MSP::Particle::rbf_create), 1);
    rb_define_module_function(mParticle, "update", VALUEFUNC(MSP::Particle::rbf_update), 2);
    rb_define_module_function(mParticle, "draw", VALUEFUNC(MSP::Particle::rbf_draw), 3);
    rb_define_module_function(mParticle, "destroy", VALUEFUNC(MSP::Particle::rbf_destroy), 1);
    rb_define_module_function(mParticle, "update_all", VALUEFUNC(MSP::Particle::rbf_update_all), 1);
    rb_define_module_function(mParticle, "draw_all", VALUEFUNC(MSP::Particle::rbf_draw_all), 2);
    rb_define_module_function(mParticle, "destroy_all", VALUEFUNC(MSP::Particle::rbf_destroy_all), 0);
    rb_define_module_function(mParticle, "size", VALUEFUNC(MSP::Particle::rbf_get_size), 0);
}
