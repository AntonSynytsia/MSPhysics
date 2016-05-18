#ifndef MSP_PARTICLE_H
#define MSP_PARTICLE_H

#include "msp_util.h"

namespace MSPhysics {
	class Particle;
}

class MSPhysics::Particle {
public:
	// Structures
	typedef struct ParticleData
	{
		dVector position;
		dVector velocity;
		bool use_velocity;
		dVector gravity;
		bool use_gravity;
		dFloat velocity_damp;
		dFloat radius;
		dFloat scale;
		dVector color1;
		dVector color2;
		bool use_color2;
		dVector color;
		dFloat alpha1;
		dFloat alpha2;
		dFloat alpha;
		VALUE v_color;
		bool use_alpha2;
		dFloat fade;
		dFloat lifetime;
		dFloat rot_angle;
		unsigned int num_seg;
		dFloat cur_life;
		dFloat* pts;
	} ParticleData;

private:
	// Variables
	static std::map<ParticleData*, ParticleData*> valid_particles;

	static const VALUE V_GL_POLYGON;
	static const VALUE V_GL_TRIANGLE_FAN;

	static const VALUE position_sym;
	static const VALUE velocity_sym;
	static const VALUE velocity_damp_sym;
	static const VALUE radius_sym;
	static const VALUE scale_sym;
	static const VALUE color1_sym;
	static const VALUE color2_sym;
	static const VALUE alpha1_sym;
	static const VALUE alpha2_sym;
	static const VALUE fade_sym;
	static const VALUE lifetime_sym;
	static const VALUE num_seg_sym;
	static const VALUE rot_angle_sym;

	// Helper Functions
	static bool c_is_valid(ParticleData* address);
	static ParticleData* c_value_to_particle(VALUE v_address);
	static VALUE c_particle_to_value(ParticleData* address);
	static bool c_update_particle(ParticleData* data, dFloat timestep);
	static void c_draw_particle(ParticleData* data, VALUE v_view, VALUE v_bb, const dVector& camera_eye);
	static VALUE c_points_on_circle2d(const dVector& origin, dFloat radius, unsigned int num_seg, dFloat rot_angle);
	static VALUE c_points_on_circle3d(const dVector& origin, dFloat radius, const dVector& normal, unsigned int num_seg, dFloat rot_angle);

public:
	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_address);
	static VALUE create(VALUE self, VALUE v_opts, VALUE v_timestep);
	static VALUE update(VALUE self, VALUE v_address, VALUE v_timestep);
	static VALUE draw(VALUE self, VALUE v_address, VALUE v_view, VALUE v_bb);
	static VALUE destroy(VALUE self, VALUE v_address);
	static VALUE update_all(VALUE self, VALUE v_timestep);
	static VALUE draw_all(VALUE self, VALUE v_view, VALUE v_bb);
	static VALUE destroy_all(VALUE self);
	static VALUE get_size(VALUE self);
};

void Init_msp_particle(VALUE mC);

#endif	/* MSP_PARTICLE_H */
