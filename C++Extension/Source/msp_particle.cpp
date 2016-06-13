#include "msp_particle.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

std::map<MSPhysics::Particle::ParticleData*, MSPhysics::Particle::ParticleData*> MSPhysics::Particle::valid_particles;

const VALUE MSPhysics::Particle::V_GL_POLYGON = INT2FIX(9);
const VALUE MSPhysics::Particle::V_GL_TRIANGLE_FAN = INT2FIX(6);

const VALUE MSPhysics::Particle::position_sym = ID2SYM(rb_intern("position"));
const VALUE MSPhysics::Particle::velocity_sym = ID2SYM(rb_intern("velocity"));
const VALUE MSPhysics::Particle::velocity_damp_sym = ID2SYM(rb_intern("velocity_damp"));
const VALUE MSPhysics::Particle::radius_sym = ID2SYM(rb_intern("radius"));
const VALUE MSPhysics::Particle::scale_sym = ID2SYM(rb_intern("scale"));
const VALUE MSPhysics::Particle::color1_sym = ID2SYM(rb_intern("color1"));
const VALUE MSPhysics::Particle::color2_sym = ID2SYM(rb_intern("color2"));
const VALUE MSPhysics::Particle::alpha1_sym = ID2SYM(rb_intern("alpha1"));
const VALUE MSPhysics::Particle::alpha2_sym = ID2SYM(rb_intern("alpha2"));
const VALUE MSPhysics::Particle::fade_sym = ID2SYM(rb_intern("fade"));
const VALUE MSPhysics::Particle::lifetime_sym = ID2SYM(rb_intern("lifetime"));
const VALUE MSPhysics::Particle::num_seg_sym = ID2SYM(rb_intern("num_seg"));
const VALUE MSPhysics::Particle::rot_angle_sym = ID2SYM(rb_intern("rot_angle"));



/*
 ///////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

bool MSPhysics::Particle::c_is_valid(ParticleData* address) {
	return valid_particles.find(address) != valid_particles.end();
}

MSPhysics::Particle::ParticleData* MSPhysics::Particle::c_value_to_particle(VALUE v_address) {
	ParticleData* address = (ParticleData*)Util::value_to_ll(v_address);
	if (valid_particles.find(address) == valid_particles.end())
		rb_raise(rb_eTypeError, "Given address is not a reference to a valid particle!");
	return address;
}

VALUE MSPhysics::Particle::c_particle_to_value(ParticleData* address) {
	return Util::to_value((long long)address);
}

bool MSPhysics::Particle::c_update_particle(ParticleData* data, dFloat timestep) {
	// Control radius
	data->radius *= data->scale;
	// Increment life
	data->cur_life += timestep;
	// Check if need to delete the particle
	if (data->radius < 0.01f || data->radius > 10000.0 || data->cur_life > data->lifetime) {
		valid_particles.erase(data);
		rb_gc_unregister_address(&data->v_color);
		delete[] data->pts;
		delete data;
		return false;
	}
	// Calc life ratio
	dFloat ratio = data->cur_life / data->lifetime;
	// Transition color
	if (data->use_color2) {
		data->color.m_x = data->color1.m_x + (data->color2.m_x - data->color1.m_x) * ratio;
		data->color.m_y = data->color1.m_y + (data->color2.m_y - data->color1.m_y) * ratio;
		data->color.m_z = data->color1.m_z + (data->color2.m_z - data->color1.m_z) * ratio;
		rb_funcall(data->v_color, INTERN_SRED, 1, Util::to_value((int)(data->color.m_x)));
		rb_funcall(data->v_color, INTERN_SGREEN, 1, Util::to_value((int)(data->color.m_y)));
		rb_funcall(data->v_color, INTERN_SBLUE, 1, Util::to_value((int)(data->color.m_z)));
	}
	// Transition opacity
	if (data->use_alpha2) {
		if (data->fade < EPSILON)
			data->alpha = data->alpha1 + (data->alpha2 - data->alpha1) * ratio;
		else {
			dFloat fh = data->fade * 0.5f;
			if (ratio < fh) {
				dFloat fr = data->cur_life / (data->lifetime * fh);
				data->alpha = data->alpha1 * fr;
			}
			else if (ratio >= (1.0 - fh)) {
				dFloat fr = (data->lifetime - data->cur_life) / (data->lifetime * fh);
				data->alpha = data->alpha2 * fr;
			}
			else {
				dFloat fl = data->lifetime * data->fade;
				dFloat fr = (data->cur_life - fl * 0.5f) / (data->lifetime - fl);
				data->alpha = data->alpha1 + (data->alpha2 - data->alpha1) * fr;
			}
		}
	}
	else {
		if (data->fade < EPSILON)
			data->alpha = data->alpha1;
		else {
			dFloat fh = data->fade * 0.5f;
			if (ratio < fh) {
				dFloat fr = data->cur_life / (data->lifetime * fh);
				data->alpha = data->alpha1 * fr;
			}
			else if (ratio >= (1.0 - fh)) {
				dFloat fr = (data->lifetime - data->cur_life) / (data->lifetime * fh);
				data->alpha = data->alpha1 * fr;
			}
			else {
				data->alpha = data->alpha1;
			}
		}
	}
	rb_funcall(data->v_color, INTERN_SALPHA, 1, Util::to_value(data->alpha));
	// Control velocity and position
	if (data->use_velocity) {
		if (data->use_gravity) {
			data->velocity.m_x += data->gravity.m_x * timestep;
			data->velocity.m_y += data->gravity.m_y * timestep;
			data->velocity.m_z += data->gravity.m_z * timestep;
		}
		if (data->velocity_damp != 0.0f) {
			dFloat s = 1.0f - data->velocity_damp;
			data->velocity.m_x *= s;
			data->velocity.m_y *= s;
			data->velocity.m_z *= s;
		}
		data->position.m_x += data->velocity.m_x * timestep;
		data->position.m_y += data->velocity.m_y * timestep;
		data->position.m_z += data->velocity.m_z * timestep;
	}
	return true;
}

void MSPhysics::Particle::c_draw_particle(ParticleData* data, VALUE v_view, VALUE v_bb, const dVector& camera_eye) {
	dVector normal(Util::vectors_identical(camera_eye, data->position) ? Z_AXIS : camera_eye - data->position);
	//VALUE v_pts = c_points_on_circle3d(data->position, data->radius, normal, data->num_seg, data->rot_angle);
	rb_funcall(v_bb, INTERN_ADD, 1, Util::point_to_value2(data->position));
	//rb_funcall(v_view, INTERN_SDRAWING_COLOR, 1, Util::color_to_value(data->color, data->alpha));
	rb_funcall(v_view, INTERN_SDRAWING_COLOR, 1, data->v_color);
	//rb_funcall(v_view, INTERN_DRAW, 2, V_GL_POLYGON, v_pts);

	/*VALUE v_pts = rb_ary_new2(data->num_seg);
	dMatrix cmatrix;
	Util::matrix_from_pin_dir(data->position, normal, cmatrix);
	dFloat offset = PI2 / data->num_seg;
	for (unsigned int i = 0; i < data->num_seg; ++i) {
		dFloat angle = data->rot_angle + i * offset;
		dVector pt(dCos(angle) * data->radius, dSin(angle) * data->radius, 0.0f);
		rb_ary_store(v_pts, i, Util::point_to_value2(cmatrix.TransformVector(pt)));
	}
	rb_funcall(v_view, INTERN_DRAW, 2, V_GL_POLYGON, v_pts);*/ // 42/45 FPS at 6

	/*VALUE v_pts = rb_ary_new2(data->num_seg+2);
	rb_ary_store(v_pts, 0, Util::point_to_value2(data->position));
	dMatrix cmatrix;
	Util::matrix_from_pin_dir(data->position, normal, cmatrix);
	for (unsigned int i = 0; i < data->num_seg; ++i) {
		dVector pt(data->pts[i*2] * data->radius, data->pts[i*2+1] * data->radius, 0.0f);
		rb_ary_store(v_pts, i+1, Util::point_to_value2(cmatrix.TransformVector(pt)));
	}
	rb_ary_store(v_pts, data->num_seg+1, rb_ary_entry(v_pts, 1));
	rb_funcall(v_view, INTERN_DRAW, 2, V_GL_TRIANGLE_FAN, v_pts);*/ // 42/45 at 6

	VALUE v_pts = rb_ary_new2(data->num_seg);
	dMatrix cmatrix;
	Util::matrix_from_pin_dir(data->position, normal, cmatrix);
	for (unsigned int i = 0; i < data->num_seg; ++i) {
		dVector pt(data->pts[i*2] * data->radius, data->pts[i*2+1] * data->radius, 0.0f);
		rb_ary_store(v_pts, i, Util::point_to_value2(cmatrix.TransformVector(pt)));
	}
	rb_funcall(v_view, INTERN_DRAW, 2, V_GL_POLYGON, v_pts); // 44/46 FPS at 6
}

VALUE MSPhysics::Particle::c_points_on_circle2d(const dVector& origin, dFloat radius, unsigned int num_seg, dFloat rot_angle) {
	dFloat ra = rot_angle * DEG_TO_RAD;
	dFloat offset = PI * 2.0f / num_seg;
	VALUE v_pts = rb_ary_new2(num_seg);
	for (unsigned int i = 0; i < num_seg; ++i) {
		dFloat angle = ra + i * offset;
		dVector pt(origin.m_x + dCos(angle) * radius, origin.m_y + dSin(angle) * radius, origin.m_z);
		rb_ary_store(v_pts, i, Util::point_to_value2(pt));
	}
	return v_pts;
}

VALUE MSPhysics::Particle::c_points_on_circle3d(const dVector& origin, dFloat radius, const dVector& normal, unsigned int num_seg, dFloat rot_angle) {
	dMatrix cmatrix;
	Util::matrix_from_pin_dir(origin, normal, cmatrix);
	dFloat ra = rot_angle * DEG_TO_RAD;
	dFloat offset = PI * 2.0f / num_seg;
	VALUE v_pts = rb_ary_new2(num_seg);
	for (unsigned int i = 0; i < num_seg; ++i) {
		dFloat angle = ra + i * offset;
		dVector pt(dCos(angle) * radius, dSin(angle) * radius, 0.0f);
		rb_ary_store(v_pts, i, Util::point_to_value2(cmatrix.TransformVector(pt)));
	}
	return v_pts;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSPhysics::Particle::is_valid(VALUE self, VALUE v_address) {
	return c_is_valid((ParticleData*)Util::value_to_ll(v_address)) ? Qtrue : Qfalse;
}

VALUE MSPhysics::Particle::create(VALUE self, VALUE v_opts, VALUE v_timestep) {
	if (TYPE(v_opts) != T_HASH)
		rb_raise(rb_eTypeError, "Expected a hash with particle options.");
	dFloat timestep = Util::value_to_dFloat(v_timestep);

	ParticleData tdata;
	VALUE val;

	// Position
	val = rb_hash_aref(v_opts, position_sym);
	tdata.position = (val != Qnil) ? Util::value_to_point2(val) : dVector(0,0,0);
	// Velocity
	val = rb_hash_aref(v_opts, velocity_sym);
	if (val != Qnil) {
		tdata.velocity = Util::value_to_vector(val);
		tdata.use_velocity = true;
	}
	else
		tdata.use_velocity = false;
	// Velocity damp
	val = rb_hash_aref(v_opts, velocity_damp_sym);
	tdata.velocity_damp = (val != Qnil) ? Util::clamp<dFloat>(Util::value_to_dFloat(val), 0.0f, 1.0f) : 0.0f;
	// Gravity
	val = rb_hash_aref(v_opts, velocity_sym);
	if (val != Qnil) {
		tdata.gravity = Util::value_to_vector(val);
		tdata.use_gravity = true;
	}
	else
		tdata.use_gravity = false;
	// Radius
	val = rb_hash_aref(v_opts, radius_sym);
	tdata.radius = (val != Qnil) ? Util::clamp<dFloat>(Util::value_to_dFloat(val), 0.01f, 10000.0f) : 1.0f;
	// Scale
	val = rb_hash_aref(v_opts, scale_sym);
	tdata.scale = (val != Qnil) ? Util::clamp<dFloat>(Util::value_to_dFloat(val), 0.001f, 1000.0f) : 1.01f;
	// Color1
	val = rb_hash_aref(v_opts, color1_sym);
	tdata.color1 = (val != Qnil) ? Util::value_to_color(val) : dVector(100,100,100,255);
	// Color2
	val = rb_hash_aref(v_opts, color2_sym);
	if (val != Qnil) {
		tdata.color2 = Util::value_to_color(val);
		tdata.use_color2 = true;
	}
	else
		tdata.use_color2 = false;
	// Alpha1
	val = rb_hash_aref(v_opts, alpha1_sym);
	tdata.alpha1 = (val != Qnil) ? Util::clamp<dFloat>(Util::value_to_dFloat(val), 0.0f, 1.0f) : 1.0f;
	// Alpha2
	val = rb_hash_aref(v_opts, alpha2_sym);
	if (val != Qnil) {
		tdata.alpha2 = Util::clamp<dFloat>(Util::value_to_dFloat(val), 0.0f, 1.0f);
		tdata.use_alpha2 = true;
	}
	else
		tdata.use_alpha2 = false;
	// Fade
	val = rb_hash_aref(v_opts, fade_sym);
	tdata.fade = (val != Qnil) ? Util::clamp<dFloat>(Util::value_to_dFloat(val), 0.0f, 1.0f) : 1.0f;
	// Lifetime
	val = rb_hash_aref(v_opts, lifetime_sym);
	tdata.lifetime = (val != Qnil) ? Util::clamp_min<dFloat>(Util::value_to_dFloat(val) * timestep, EPSILON) : 2.0f;
	// Number of segments
	val = rb_hash_aref(v_opts, num_seg_sym);
	tdata.num_seg = (val != Qnil) ? Util::clamp<unsigned int>(Util::value_to_uint(val), 3, 120) : 16;
	// Rotate angle
	val = rb_hash_aref(v_opts, rot_angle_sym);
	tdata.rot_angle = (val != Qnil) ? Util::value_to_dFloat(val) * DEG_TO_RAD : 0.0f;

	ParticleData* data = new ParticleData(tdata);
	// Preset current color
	data->color = dVector(data->color1);
	data->alpha = data->fade < EPSILON ? data->alpha1 : 0.0f;
	data->v_color = Util::color_to_value(data->color, data->alpha);
	rb_gc_register_address(&data->v_color);
	// Preset current life
	data->cur_life = 0.0f;
	// Calculate normal points on circle
	data->pts = new dFloat[data->num_seg*2];
	dFloat offset = PI2 / data->num_seg;
	for (unsigned int i = 0; i < data->num_seg; ++i) {
		dFloat angle = data->rot_angle + i * offset;
		data->pts[i*2] = dCos(angle);
		data->pts[i*2+1] = dSin(angle);
	}

	valid_particles[data] = data;
	return c_particle_to_value(data);
}

VALUE MSPhysics::Particle::update(VALUE self, VALUE v_address, VALUE v_timestep) {
	ParticleData* data = c_value_to_particle(v_address);
	dFloat timestep = Util::value_to_dFloat(v_timestep);
	return Util::to_value(c_update_particle(data, timestep));
}

VALUE MSPhysics::Particle::draw(VALUE self, VALUE v_address, VALUE v_view, VALUE v_bb) {
	ParticleData* data = c_value_to_particle(v_address);
	VALUE v_camera = rb_funcall(v_view, INTERN_CAMERA, 0);
	VALUE v_eye = rb_funcall(v_camera, INTERN_EYE, 0);
	dVector eye = Util::value_to_point2(v_eye);
	c_draw_particle(data, v_view, v_bb, eye);
	return Qnil;
}

VALUE MSPhysics::Particle::destroy(VALUE self, VALUE v_address) {
	ParticleData* data = c_value_to_particle(v_address);
	valid_particles.erase(data);
	rb_gc_unregister_address(&data->v_color);
	delete[] data->pts;
	delete data;
	return Qnil;
}

VALUE MSPhysics::Particle::update_all(VALUE self, VALUE v_timestep) {
	dFloat timestep = Util::value_to_dFloat(v_timestep);
	for (std::map<ParticleData*, ParticleData*>::iterator it = valid_particles.begin(); it != valid_particles.end();) {
		ParticleData* data = it->first;
		++it;
		c_update_particle(data, timestep);
	}
	return Qnil;
}

VALUE MSPhysics::Particle::draw_all(VALUE self, VALUE v_view, VALUE v_bb) {
	VALUE v_camera = rb_funcall(v_view, INTERN_CAMERA, 0);
	VALUE v_eye = rb_funcall(v_camera, INTERN_EYE, 0);
	dVector eye = Util::value_to_point2(v_eye);
	std::map<dFloat, std::vector<ParticleData*>> sorted_data;
	for (std::map<ParticleData*, ParticleData*>::iterator it = valid_particles.begin(); it != valid_particles.end(); ++it) {
		ParticleData* data = it->first;
		dFloat dist = Util::get_vector_magnitude(eye - data->position);
		sorted_data[dist].push_back(data);
	}
	for (std::map<dFloat, std::vector<ParticleData*>>::reverse_iterator it = sorted_data.rbegin(); it != sorted_data.rend(); ++it) {
		for (std::vector<ParticleData*>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
			c_draw_particle(*it2, v_view, v_bb, eye);
	}
	sorted_data.clear();
	return Qnil;
}

VALUE MSPhysics::Particle::destroy_all(VALUE self) {
	for (std::map<ParticleData*, ParticleData*>::iterator it = valid_particles.begin(); it != valid_particles.end(); ++it) {
		ParticleData* data = it->first;
		rb_gc_unregister_address(&data->v_color);
		delete[] data->pts;
		delete data;
	}
	valid_particles.clear();
	return Qnil;
}

VALUE MSPhysics::Particle::get_size(VALUE self) {
	return Util::to_value(valid_particles.size());
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void Init_msp_particle(VALUE mMSPhysics) {
	VALUE mParticle = rb_define_module_under(mMSPhysics, "Particle");

	rb_define_module_function(mParticle, "is_valid?", VALUEFUNC(MSPhysics::Particle::is_valid), 1);
	rb_define_module_function(mParticle, "create", VALUEFUNC(MSPhysics::Particle::create), 2);
	rb_define_module_function(mParticle, "update", VALUEFUNC(MSPhysics::Particle::update), 2);
	rb_define_module_function(mParticle, "draw", VALUEFUNC(MSPhysics::Particle::draw), 3);
	rb_define_module_function(mParticle, "destroy", VALUEFUNC(MSPhysics::Particle::destroy), 1);
	rb_define_module_function(mParticle, "update_all", VALUEFUNC(MSPhysics::Particle::update_all), 1);
	rb_define_module_function(mParticle, "draw_all", VALUEFUNC(MSPhysics::Particle::draw_all), 2);
	rb_define_module_function(mParticle, "destroy_all", VALUEFUNC(MSPhysics::Particle::destroy_all), 0);
	rb_define_module_function(mParticle, "size", VALUEFUNC(MSPhysics::Particle::get_size), 0);
}
