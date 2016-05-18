#ifndef MSP_COLLISION_H
#define MSP_COLLISION_H

#include "msp_util.h"

namespace MSNewton {
	class Collision;
}

class MSNewton::Collision {
private:
	// Constants
	static const dFloat MIN_SIZE;
	static const dFloat MAX_SIZE;

public:
	// Ruby Functions
	static VALUE create_null(VALUE self, VALUE v_world);
	static VALUE create_box(VALUE self, VALUE v_world, VALUE v_width, VALUE v_height, VALUE v_depth, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_sphere(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_scaled_sphere(VALUE self, VALUE v_world, VALUE v_width, VALUE v_height, VALUE v_depth, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_cone(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_scaled_cone(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_cylinder(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_scaled_cylinder(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_capsule(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_scaled_capsule(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_total_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_tapered_capsule(VALUE self, VALUE v_world, VALUE v_radius0, VALUE v_radius1, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_tapered_cylinder(VALUE self, VALUE v_world, VALUE v_radius0, VALUE v_radius1, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_chamfer_cylinder(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_scaled_chamfer_cylinder(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_convex_hull(VALUE self, VALUE v_world, VALUE v_vertices, VALUE v_tolerance, VALUE v_id, VALUE v_offset_matrix);
	static VALUE create_compound(VALUE self, VALUE v_world, VALUE v_convex_collisions, VALUE v_id);
	static VALUE create_compound_from_cd1(
		VALUE self,
		VALUE v_world,
		VALUE v_polygons,
		VALUE v_max_concavity,
		VALUE v_back_face_distance_factor,
		VALUE v_max_hull_count,
		VALUE v_max_vertices_per_hull,
		VALUE v_id);
	static VALUE create_compound_from_cd2(
		VALUE self,
		VALUE v_world,
		VALUE v_points,
		VALUE v_indices,
		VALUE v_params,
		VALUE v_id);
	static VALUE create_compound_from_cd3(VALUE self, VALUE v_world, VALUE v_points, VALUE v_indices, VALUE v_max_concavity_angle, VALUE v_id);
	static VALUE create_static_mesh(VALUE self, VALUE v_world, VALUE v_polygons, VALUE v_simplify, VALUE v_optimize, VALUE v_id);
	static VALUE get_type(VALUE self, VALUE v_collision);
	static VALUE get_scale(VALUE self, VALUE v_collision);
	static VALUE set_scale(VALUE self, VALUE v_collision, VALUE v_scale);
	static VALUE is_valid(VALUE self, VALUE v_collision);
	static VALUE destroy(VALUE self, VALUE v_collision);
};

void Init_msp_collision(VALUE mNewton);

#endif	/* MSP_COLLISION_H */
