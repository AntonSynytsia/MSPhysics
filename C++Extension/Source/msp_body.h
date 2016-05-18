#ifndef MSP_BODY_H
#define MSP_BODY_H

#include "msp_util.h"

namespace MSNewton {
	class Body;
}

class MSNewton::Body {
private:
	// Variables
	static std::map<VALUE, const NewtonBody*> map_group_to_body;

	// Callbacks
	static void destructor_callback(const NewtonBody* const body);
	static void transform_callback(const NewtonBody* const body, const dFloat* const matrix, int thread_index);
	static void force_and_torque_callback(const NewtonBody* const body, dFloat timestep, int thread_index);
	static void collision_iterator(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id);
	static void collision_iterator2(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id);
	static void collision_iterator3(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id);
	static void collision_iterator4(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id);

	// Helper Functions
	static void c_clear_non_collidable_bodies(const NewtonBody* body);

public:
	// Ruby Functions
	static VALUE is_valid(VALUE self, VALUE v_body);
	static VALUE create_dynamic(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_id, VALUE v_group);
	static VALUE destroy(VALUE self, VALUE v_body);
	static VALUE get_world(VALUE self, VALUE v_body);
	static VALUE get_collision(VALUE self, VALUE v_body);
	static VALUE get_simulation_state(VALUE self, VALUE v_body);
	static VALUE set_simulation_state(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE get_continuous_collision_state(VALUE self, VALUE v_body);
	static VALUE set_continuous_collision_state(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE get_matrix(VALUE self, VALUE v_body);
	static VALUE get_normal_matrix(VALUE self, VALUE v_body);
	static VALUE set_matrix(VALUE self, VALUE v_body, VALUE v_matrix);
	static VALUE get_position(VALUE self, VALUE v_body, VALUE v_mode);
	static VALUE set_position(VALUE self, VALUE v_body, VALUE v_position, VALUE v_mode);
	static VALUE get_rotation(VALUE self, VALUE v_body);
	static VALUE get_euler_angles(VALUE self, VALUE v_body);
	static VALUE set_euler_angles(VALUE self, VALUE v_body, VALUE v_angles);
	static VALUE get_velocity(VALUE self, VALUE v_body);
	static VALUE set_velocity(VALUE self, VALUE v_body, VALUE v_velocity);
	static VALUE get_omega(VALUE self, VALUE v_body);
	static VALUE set_omega(VALUE self, VALUE v_body, VALUE v_omega);
	static VALUE get_centre_of_mass(VALUE self, VALUE v_body);
	static VALUE set_centre_of_mass(VALUE self, VALUE v_body, VALUE v_com);
	static VALUE get_mass(VALUE self, VALUE v_body);
	static VALUE set_mass(VALUE self, VALUE v_body, VALUE v_mass);
	static VALUE get_density(VALUE self, VALUE v_body);
	static VALUE set_density(VALUE self, VALUE v_body, VALUE v_density);
	static VALUE get_volume(VALUE self, VALUE v_body);
	static VALUE set_volume(VALUE self, VALUE v_body, VALUE v_volume);
	static VALUE reset_mass_properties(VALUE self, VALUE v_body, VALUE v_density);
	static VALUE is_static(VALUE self, VALUE v_body);
	static VALUE set_static(VALUE self, VALUE v_body, VALUE v_static);
	static VALUE is_collidable(VALUE self, VALUE v_body);
	static VALUE set_collidable(VALUE self, VALUE v_body, VALUE v_collidable);
	static VALUE is_frozen(VALUE self, VALUE v_body);
	static VALUE set_frozen(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE is_sleeping(VALUE self, VALUE v_body);
	static VALUE set_sleeping(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE get_auto_sleep_state(VALUE self, VALUE v_body);
	static VALUE set_auto_sleep_state(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE is_non_collidable_with(VALUE self, VALUE v_body, VALUE v_other_body);
	static VALUE set_non_collidable_with(VALUE self, VALUE v_body, VALUE v_other_body, VALUE v_state);
	static VALUE get_non_collidable_bodies(VALUE self, VALUE v_body);
	static VALUE clear_non_collidable_bodies(VALUE self, VALUE v_body);
	static VALUE get_elasticity(VALUE self, VALUE v_body);
	static VALUE set_elasticity(VALUE self, VALUE v_body, VALUE v_elasticity);
	static VALUE get_softness(VALUE self, VALUE v_body);
	static VALUE set_softness(VALUE self, VALUE v_body, VALUE v_softness);
	static VALUE get_static_friction(VALUE self, VALUE v_body);
	static VALUE set_static_friction(VALUE self, VALUE v_body, VALUE v_friction);
	static VALUE get_dynamic_friction(VALUE self, VALUE v_body);
	static VALUE set_dynamic_friction(VALUE self, VALUE v_body, VALUE v_friction);
	static VALUE get_friction_state(VALUE self, VALUE v_body);
	static VALUE set_friction_state(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE get_magnet_force(VALUE self, VALUE v_body);
	static VALUE set_magnet_force(VALUE self, VALUE v_body, VALUE v_force);
	static VALUE get_magnet_range(VALUE self, VALUE v_body);
	static VALUE set_magnet_range(VALUE self, VALUE v_body, VALUE v_range);
	static VALUE is_magnetic(VALUE self, VALUE v_body);
	static VALUE set_magnetic(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE get_aabb(VALUE self, VALUE v_body);
	static VALUE get_linear_damping(VALUE self, VALUE v_body);
	static VALUE set_linear_damping(VALUE self, VALUE v_body, VALUE v_damp);
	static VALUE get_angular_damping(VALUE self, VALUE v_body);
	static VALUE set_angular_damping(VALUE self, VALUE v_body, VALUE v_damp);
	static VALUE get_point_velocity(VALUE self, VALUE v_body, VALUE v_point);
	static VALUE add_point_force(VALUE self, VALUE v_body, VALUE v_point, VALUE v_force);
	static VALUE add_impulse(VALUE self, VALUE v_body, VALUE v_center, VALUE v_delta_vel);
	static VALUE get_force(VALUE self, VALUE v_body);
	static VALUE get_force_acc(VALUE self, VALUE v_body);
	static VALUE add_force(VALUE self, VALUE v_body, VALUE v_force);
	static VALUE add_force2(VALUE self, VALUE v_body, VALUE v_force);
	static VALUE set_force(VALUE self, VALUE v_body, VALUE v_force);
	static VALUE set_force2(VALUE self, VALUE v_body, VALUE v_force);
	static VALUE get_torque(VALUE self, VALUE v_body);
	static VALUE get_torque_acc(VALUE self, VALUE v_body);
	static VALUE add_torque(VALUE self, VALUE v_body, VALUE v_torque);
	static VALUE add_torque2(VALUE self, VALUE v_body, VALUE v_torque);
	static VALUE set_torque(VALUE self, VALUE v_body, VALUE v_torque);
	static VALUE set_torque2(VALUE self, VALUE v_body, VALUE v_torque);
	static VALUE get_net_contact_force(VALUE self, VALUE v_body);
	static VALUE get_contacts(VALUE self, VALUE v_body, VALUE v_inc_non_collidable);
	static VALUE get_touching_bodies(VALUE self, VALUE v_body, VALUE v_inc_non_collidable);
	static VALUE get_contact_points(VALUE self, VALUE v_body, VALUE v_inc_non_collidable);
	static VALUE get_collision_faces(VALUE self, VALUE v_body);
	static VALUE get_collision_faces2(VALUE self, VALUE v_body);
	static VALUE get_collision_faces3(VALUE self, VALUE v_body);
	static VALUE apply_pick_and_drag(VALUE self, VALUE v_body, VALUE v_pick_pt, VALUE v_dest_pt, VALUE v_stiffness, VALUE v_damp);
	static VALUE apply_pick_and_drag2(VALUE self, VALUE v_body, VALUE v_pick_pt, VALUE v_dest_pt, VALUE v_stiffness, VALUE v_angular_damp, VALUE v_timestep);
	static VALUE apply_buoyancy(VALUE self, VALUE v_body, VALUE v_plane_origin, VALUE v_plane_normal, VALUE v_plane_current, VALUE v_density, VALUE v_linear_viscosity, VALUE v_angular_viscosity);
	static VALUE apply_fluid_resistance(VALUE self, VALUE v_body, VALUE v_density);
	static VALUE copy(VALUE self, VALUE v_body, VALUE v_matrix, VALUE v_reapply_forces, VALUE v_group);
	static VALUE get_destructor_proc(VALUE self, VALUE v_body);
	static VALUE set_destructor_proc(VALUE self, VALUE v_body, VALUE v_proc);
	static VALUE get_user_data(VALUE self, VALUE v_body);
	static VALUE set_user_data(VALUE self, VALUE v_body, VALUE v_user_data);
	static VALUE get_record_touch_data_state(VALUE self, VALUE v_body);
	static VALUE set_record_touch_data_state(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE get_matrix_scale(VALUE self, VALUE v_body);
	static VALUE set_matrix_scale(VALUE self, VALUE v_body, VALUE v_scale);
	static VALUE matrix_changed(VALUE self, VALUE v_body);
	static VALUE enable_gravity(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE is_gravity_enabled(VALUE self, VALUE v_body);
	static VALUE get_contained_joints(VALUE self, VALUE v_body);
	static VALUE get_connected_joints(VALUE self, VALUE v_body);
	static VALUE get_connected_bodies(VALUE self, VALUE v_body);
	static VALUE get_material_id(VALUE self, VALUE v_body);
	static VALUE set_material_id(VALUE self, VALUE v_body, VALUE v_id);
	static VALUE get_collision_scale(VALUE self, VALUE v_body);
	static VALUE set_collision_scale(VALUE self, VALUE v_body, VALUE v_scale);
	static VALUE get_default_collision_scale(VALUE self, VALUE v_body);
	static VALUE get_actual_matrix_scale(VALUE self, VALUE v_body);
	static VALUE get_group(VALUE self, VALUE v_body);
	static VALUE get_body_by_group(VALUE self, VALUE v_body);
	static VALUE get_body_data_by_group(VALUE self, VALUE v_body);
};

void Init_msp_body(VALUE mNewton);

#endif	/* MSP_BODY_H */
