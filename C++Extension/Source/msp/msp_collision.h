#ifndef MSP_COLLISION_H
#define MSP_COLLISION_H

#include "msp.h"

class MSP::Collision {
private:
	// Constants
	static const dFloat MIN_SIZE;
	static const dFloat MAX_SIZE;

public:
	// Structures
	struct CollisionData {
		dVector m_scale;
		CollisionData()
			: m_scale(1.0f, 1.0f, 1.0f, 1.0f)
		{
		}
		CollisionData(const dVector& scale)
			: m_scale(scale)
		{
		}
		CollisionData(dFloat x, dFloat y, dFloat z)
			: m_scale(x, y, z, 1.0f)
		{
		}
	};

	// Variables
	static std::map<const NewtonCollision*, CollisionData*> s_valid_collisions;

	// Helper Functions
	static bool c_is_collision_valid(const NewtonCollision* address);
	static const NewtonCollision* c_value_to_collision(VALUE v_collision);
	static VALUE c_collision_to_value(const NewtonCollision* collision);
	static bool c_is_collision_convex(const NewtonCollision* collision);

	// Ruby Functions
	static VALUE rbf_create_null(VALUE self, VALUE v_world);
	static VALUE rbf_create_box(VALUE self, VALUE v_world, VALUE v_width, VALUE v_height, VALUE v_depth, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_sphere(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_scaled_sphere(VALUE self, VALUE v_world, VALUE v_width, VALUE v_height, VALUE v_depth, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_cone(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_scaled_cone(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_cylinder(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_scaled_cylinder(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_capsule(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_scaled_capsule(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_total_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_tapered_capsule(VALUE self, VALUE v_world, VALUE v_radius0, VALUE v_radius1, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_tapered_cylinder(VALUE self, VALUE v_world, VALUE v_radius0, VALUE v_radius1, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_chamfer_cylinder(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_scaled_chamfer_cylinder(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_convex_hull(VALUE self, VALUE v_world, VALUE v_vertices, VALUE v_tolerance, VALUE v_id, VALUE v_offset_matrix);
	static VALUE rbf_create_compound(VALUE self, VALUE v_world, VALUE v_convex_collisions, VALUE v_id);
	static VALUE rbf_create_compound_from_cd(
		VALUE self,
		VALUE v_world,
		VALUE v_polygons,
		VALUE v_max_concavity,
		VALUE v_back_face_distance_factor,
		VALUE v_max_hull_count,
		VALUE v_max_vertices_per_hull,
		VALUE v_hull_tolerance,
		VALUE v_id);
	static VALUE rbf_create_static_mesh(VALUE self, VALUE v_world, VALUE v_polygons, VALUE v_optimize, VALUE v_id);
	static VALUE rbf_get_type(VALUE self, VALUE v_collision);
	static VALUE rbf_get_scale(VALUE self, VALUE v_collision);
	static VALUE rbf_set_scale(VALUE self, VALUE v_collision, VALUE v_scale);
	static VALUE rbf_is_valid(VALUE self, VALUE v_collision);
	static VALUE rbf_destroy(VALUE self, VALUE v_collision);

	// Main
	static void init_ruby(VALUE mNewton);
};

#endif	/* MSP_COLLISION_H */
