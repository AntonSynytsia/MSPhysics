#ifndef MSP_PARTICLE_H
#define MSP_PARTICLE_H

#include "msp.h"

class MSP::Particle {
private:
	// Structures
	struct ParticleData
	{
		dVector m_position;
		dVector m_velocity;
		bool m_use_velocity;
		dVector m_gravity;
		bool m_use_gravity;
		dFloat m_velocity_damp;
		dFloat m_radius;
		dFloat m_scale;
		dVector m_color1;
		dVector m_color2;
		bool m_use_color2;
		dVector m_color;
		dFloat m_alpha1;
		dFloat m_alpha2;
		dFloat m_alpha;
		bool m_use_alpha2;
		dFloat m_fade;
		dFloat m_lifetime;
		dFloat m_rot_angle;
		unsigned int m_num_seg;
		dFloat m_cur_life;
		dFloat* m_pts;
	};

	// Variables
	static VALUE SYM_POSITION;
	static VALUE SYM_VELOCITY;
	static VALUE SYM_VELOCITY_DAMP;
	static VALUE SYM_GRAVITY;
	static VALUE SYM_RADIUS;
	static VALUE SYM_SCALE;
	static VALUE SYM_COLOR1;
	static VALUE SYM_COLOR2;
	static VALUE SYM_ALPHA1;
	static VALUE SYM_ALPHA2;
	static VALUE SYM_FADE;
	static VALUE SYM_LIFETIME;
	static VALUE SYM_NUM_SEG;
	static VALUE SYM_ROT_ANGLE;

	static VALUE V_GL_TRIANGLE_FAN;
	static VALUE V_GL_POLYGON;

	static VALUE s_reg_symbols;
	static std::set<ParticleData*> s_valid_particles;

	// Helper Functions
	static bool c_is_valid(ParticleData* address);
	static ParticleData* c_value_to_particle(VALUE v_address);
	static VALUE c_particle_to_value(ParticleData* address);
	static bool c_update_particle(ParticleData* data, dFloat timestep);
	static void c_draw_particle(ParticleData* data, VALUE v_view, VALUE v_bb, const dMatrix& camera_tra);
	static VALUE c_points_on_circle2d(const dVector& origin, dFloat radius, unsigned int num_seg, dFloat rot_angle);
	static VALUE c_points_on_circle3d(const dVector& origin, dFloat radius, const dVector& normal, unsigned int num_seg, dFloat rot_angle);

public:
	// Ruby Functions
	static VALUE rbf_is_valid(VALUE self, VALUE v_address);
	static VALUE rbf_create(VALUE self, VALUE v_opts);
	static VALUE rbf_update(VALUE self, VALUE v_address, VALUE v_timestep);
	static VALUE rbf_draw(VALUE self, VALUE v_address, VALUE v_view, VALUE v_bb);
	static VALUE rbf_destroy(VALUE self, VALUE v_address);
	static VALUE rbf_update_all(VALUE self, VALUE v_timestep);
	static VALUE rbf_draw_all(VALUE self, VALUE v_view, VALUE v_bb);
	static VALUE rbf_destroy_all(VALUE self);
	static VALUE rbf_get_size(VALUE self);

	//Main
	static void init_ruby(VALUE mC);
};

#endif	/* MSP_PARTICLE_H */
