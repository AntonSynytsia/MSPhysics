#ifndef MSP_WORLD_H
#define MSP_WORLD_H

#include "msp_util.h"
#include "msp_joint.h"

namespace MSNewton {
	class World;
}

class MSNewton::World {
private:
	// Structures
	typedef struct DrawData {
		VALUE v_view;
		VALUE v_bb;
		dFloat scale;
	};

	// Constants
	static const VALUE V_GL_LINE_LOOP;

	// Callback Functions
	static void destructor_callback(const NewtonWorld* const world);
	static int aabb_overlap_callback(const NewtonMaterial* const material, const NewtonBody* const body0, const NewtonBody* const body1, int thread_index);
	static void contact_callback(const NewtonJoint* const contact_joint, dFloat timestep, int thread_index);
	static unsigned ray_prefilter_callback(const NewtonBody* const body, const NewtonCollision* const collision, void* const user_data);
	static dFloat ray_filter_callback(const NewtonBody* const body, const NewtonCollision* const shape_hit, const dFloat* const hit_contact, const dFloat* const hit_normal, dLong collision_id, void* const user_data, dFloat intersect_param);
	static dFloat continuous_ray_filter_callback(const NewtonBody* const body, const NewtonCollision* const shape_hit, const dFloat* const hit_contact, const dFloat* const hit_normal, dLong collision_id, void* const user_data, dFloat intersect_param);
	static int body_iterator(const NewtonBody* const body, void* const user_data);
	static void collision_copy_constructor_callback(const NewtonWorld* const world, NewtonCollision* const collision, const NewtonCollision* const source_collision);
	static void collision_destructor_callback(const NewtonWorld* const world, const NewtonCollision* const collision);
	static void draw_collision_iterator(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id);

	// Helper Functions
	static void c_update_magnets(const NewtonWorld* world);
	static void c_process_touch_events(const NewtonWorld* world);
	static void c_clear_touch_events(const NewtonWorld* world);
	static void c_clear_matrix_change_record(const NewtonWorld* world);
	static void c_disconnect_flagged_joints(const NewtonWorld* world);
	static void c_enable_cccd_bodies(const NewtonWorld* world);

public:
	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_world);
	static VALUE create(VALUE self, VALUE v_world_scale);
	static VALUE destroy(VALUE self, VALUE v_world);
	static VALUE get_max_threads_count(VALUE self, VALUE v_world);
	static VALUE set_max_threads_count(VALUE self, VALUE v_world, VALUE v_count);
	static VALUE get_cur_threads_count(VALUE self, VALUE v_world);
	static VALUE destroy_all_bodies(VALUE self, VALUE v_world);
	static VALUE get_body_count(VALUE self, VALUE v_world);
	static VALUE get_constraint_count(VALUE self, VALUE v_world);
	static VALUE update(int argc, VALUE* argv, VALUE self);
	static VALUE update_async(int argc, VALUE* argv, VALUE self);
	static VALUE get_gravity(VALUE self, VALUE v_world);
	static VALUE set_gravity(VALUE self, VALUE v_world, VALUE v_gravity);
	static VALUE get_bodies(VALUE self, VALUE v_world);
	static VALUE get_body_datas(VALUE self, VALUE v_world);
	static VALUE get_bodies_in_aabb(VALUE self, VALUE v_world, VALUE v_min_pt, VALUE v_max_pt);
	static VALUE get_first_body(VALUE self, VALUE v_world);
	static VALUE get_next_body(VALUE self, VALUE v_world, VALUE v_body);
	static VALUE get_solver_model(VALUE self, VALUE v_world);
	static VALUE set_solver_model(VALUE self, VALUE v_world, VALUE v_solver_model);
	static VALUE get_friction_model(VALUE self, VALUE v_world);
	static VALUE set_friction_model(VALUE self, VALUE v_world, VALUE v_friction_model);
	static VALUE get_material_thickness(VALUE self, VALUE v_world);
	static VALUE set_material_thickness(VALUE self, VALUE v_world, VALUE v_material_thinkness);
	static VALUE ray_cast(VALUE self, VALUE v_world, VALUE v_point1, VALUE v_point2);
	static VALUE continuous_ray_cast(VALUE self, VALUE v_world, VALUE v_point1, VALUE v_point2);
	static VALUE convex_ray_cast(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_target);
	static VALUE continuous_convex_ray_cast(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_target, VALUE v_max_hits);
	static VALUE add_explosion(VALUE self, VALUE v_world, VALUE v_center, VALUE v_blast_radius, VALUE v_blast_force);
	static VALUE get_aabb(VALUE self, VALUE v_world);
	static VALUE get_destructor_proc(VALUE self, VALUE v_world);
	static VALUE set_destructor_proc(VALUE self, VALUE v_world, VALUE v_proc);
	static VALUE get_user_data(VALUE self, VALUE v_world);
	static VALUE set_user_data(VALUE self, VALUE v_world, VALUE v_user_data);
	static VALUE get_touch_data_at(VALUE self, VALUE v_world, VALUE v_index);
	static VALUE get_touch_data_count(VALUE self, VALUE v_world);
	static VALUE get_touching_data_at(VALUE self, VALUE v_world, VALUE v_index);
	static VALUE get_touching_data_count(VALUE self, VALUE v_world);
	static VALUE get_untouch_data_at(VALUE self, VALUE v_world, VALUE v_index);
	static VALUE get_untouch_data_count(VALUE self, VALUE v_world);
	static VALUE get_time(VALUE self, VALUE v_world);
	static VALUE serialize_to_file(VALUE self, VALUE v_world, VALUE v_full_path);
	static VALUE get_contact_merge_tolerance(VALUE self, VALUE v_world);
	static VALUE set_contact_merge_tolerance(VALUE self, VALUE v_world, VALUE v_tolerance);
	static VALUE get_scale(VALUE self, VALUE v_world);
	static VALUE get_joints(VALUE self, VALUE v_world);
	static VALUE get_joint_datas(VALUE self, VALUE v_world);
	static VALUE get_default_material_id(VALUE self, VALUE v_world);
	static VALUE draw_collision_wireframe(VALUE self, VALUE v_world, VALUE v_view, VALUE v_bb, VALUE v_sleep_color, VALUE v_active_color, VALUE v_line_width, VALUE v_line_stipple);
};

void Init_msp_world(VALUE mNewton);

#endif	/* MSP_WORLD_H */
