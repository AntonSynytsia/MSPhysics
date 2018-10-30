#ifndef MSP_BODY_H
#define MSP_BODY_H

#include "msp.h"

class MSP::Body {
public:
	// Constant
	static const dFloat DEFAULT_STATIC_FRICTION_COEF;
	static const dFloat DEFAULT_KINETIC_FRICTION_COEF;
	static const dFloat DEFAULT_DENSITY;
	static const dFloat DEFAULT_ELASTICITY;
	static const dFloat DEFAULT_SOFTNESS;
	static const dVector DEFAULT_LINEAR_DAMPING;
	static const dVector DEFAULT_ANGULAR_DAMPING;
	static const bool DEFAULT_LINEAR_DAMPING_ENABLED;
	static const bool DEFAULT_ANGULAR_DAMPING_ENABLED;
	static const bool DEFAULT_FRICTION_ENABLED;
	static const bool DEFAULT_AUTO_SLEEP_ENABLED;
	static const bool DEFAULT_COLLIDABLE;
	static const bool DEFAULT_MAGNETIC;
	static const bool DEFAULT_GRAVITY_ENABLED;
	static const dFloat MIN_MASS;
	static const dFloat MAX_MASS;
	static const dFloat MIN_VOLUME;
	static const dFloat MAX_VOLUME;
	static const dFloat MIN_DENSITY;
	static const dFloat MAX_DENSITY;

	// Structures
	struct BodyData {
		dVector m_add_force;
		dVector m_add_torque;
		dVector m_set_force;
		dVector m_set_torque;
		bool m_add_force_state;
		bool m_add_torque_state;
		bool m_set_force_state;
		bool m_set_torque_state;
		bool m_dynamic;
		bool m_bstatic;
		dFloat m_density;
		dFloat m_volume;
		dFloat m_mass;
		dFloat m_elasticity;
		dFloat m_softness;
		dFloat m_static_friction;
		dFloat m_kinetic_friction;
		dVector m_linear_damping;
		dVector m_angular_damping;
		bool m_linear_damping_enabled;
		bool m_angular_damping_enabled;
		bool m_friction_enabled;
		bool m_auto_sleep_enabled;
		bool m_collidable;
		std::map<const NewtonBody*, bool> m_non_collidable_bodies;
		bool m_record_touch_data;
		std::map<const NewtonBody*, char> m_touchers;
		int m_magnet_mode;
		dFloat m_magnet_force;
		dFloat m_magnet_range;
		dFloat m_magnet_strength;
		bool m_magnetic;
		VALUE m_destructor_proc;
		VALUE m_user_data;
		VALUE m_group;
		dVector m_matrix_scale;
		dVector m_default_collision_scale;
		dVector m_default_collision_offset;
		bool m_matrix_changed;
		bool m_gravity_enabled;
		int m_material_id;
		BodyData(const dVector& matrix_scale, const dVector& default_collision_scale, const dVector& default_collision_offset, int material_id, const VALUE& v_group) :
			m_add_force(0.0f),
			m_add_torque(0.0f),
			m_set_force(0.0f),
			m_set_torque(0.0f),
			m_add_force_state(false),
			m_add_torque_state(false),
			m_set_force_state(false),
			m_set_torque_state(false),
			m_dynamic(false),
			m_bstatic(false),
			m_density(DEFAULT_DENSITY),
			m_volume(0.0f),
			m_mass(0.0f),
			m_elasticity(DEFAULT_ELASTICITY),
			m_softness(DEFAULT_SOFTNESS),
			m_static_friction(DEFAULT_STATIC_FRICTION_COEF),
			m_kinetic_friction(DEFAULT_KINETIC_FRICTION_COEF),
			m_linear_damping(DEFAULT_LINEAR_DAMPING),
			m_angular_damping(DEFAULT_ANGULAR_DAMPING),
			m_linear_damping_enabled(DEFAULT_LINEAR_DAMPING_ENABLED),
			m_angular_damping_enabled(DEFAULT_ANGULAR_DAMPING_ENABLED),
			m_friction_enabled(DEFAULT_FRICTION_ENABLED),
			m_auto_sleep_enabled(DEFAULT_AUTO_SLEEP_ENABLED),
			m_collidable(DEFAULT_COLLIDABLE),
			m_record_touch_data(false),
			m_magnet_mode(1),
			m_magnet_force(0.0f),
			m_magnet_range(0.0f),
			m_magnet_strength(0.0f),
			m_magnetic(DEFAULT_MAGNETIC),
			m_destructor_proc(Qnil),
			m_user_data(Qnil),
			m_group(v_group),
			m_matrix_scale(matrix_scale),
			m_default_collision_scale(default_collision_scale),
			m_default_collision_offset(default_collision_offset),
			m_matrix_changed(false),
			m_gravity_enabled(DEFAULT_GRAVITY_ENABLED),
			m_material_id(material_id)
		{
		}
		BodyData(const BodyData* other_body, const VALUE& v_group) :
			m_add_force(0.0f),
			m_add_torque(0.0f),
			m_set_force(0.0f),
			m_set_torque(0.0f),
			m_add_force_state(false),
			m_add_torque_state(false),
			m_set_force_state(false),
			m_set_torque_state(false),
			m_dynamic(other_body->m_dynamic),
			m_bstatic(other_body->m_bstatic),
			m_density(other_body->m_density),
			m_volume(other_body->m_volume),
			m_mass(other_body->m_mass),
			m_elasticity(other_body->m_elasticity),
			m_softness(other_body->m_softness),
			m_static_friction(other_body->m_static_friction),
			m_kinetic_friction(other_body->m_kinetic_friction),
			m_linear_damping(other_body->m_linear_damping),
			m_angular_damping(other_body->m_angular_damping),
			m_linear_damping_enabled(other_body->m_linear_damping_enabled),
			m_angular_damping_enabled(other_body->m_angular_damping_enabled),
			m_friction_enabled(other_body->m_friction_enabled),
			m_auto_sleep_enabled(other_body->m_auto_sleep_enabled),
			m_collidable(other_body->m_collidable),
			m_non_collidable_bodies(other_body->m_non_collidable_bodies),
			m_record_touch_data(false),
			m_magnet_force(other_body->m_magnet_force),
			m_magnet_range(other_body->m_magnet_range),
			m_magnetic(other_body->m_magnetic),
			m_destructor_proc(Qnil),
			m_user_data(Qnil),
			m_group(v_group),
			m_matrix_scale(other_body->m_matrix_scale),
			m_default_collision_scale(other_body->m_default_collision_scale),
			m_default_collision_offset(other_body->m_default_collision_offset),
			m_matrix_changed(false),
			m_gravity_enabled(other_body->m_gravity_enabled),
			m_material_id(other_body->m_material_id)
		{
		}
		~BodyData()
		{
		}
	};

	struct CollisionIteratorData {
		VALUE m_faces;
		CollisionIteratorData(VALUE v_faces) :
			m_faces(v_faces)
		{
		}
	};

	struct CollisionIteratorData2 {
		const NewtonBody* m_body;
		dVector m_centre;
		dVector m_velocity;
		dVector m_omega;
		dFloat m_drag;
		dVector m_wind;
		dVector m_force;
		dVector m_torque;
		CollisionIteratorData2(const NewtonBody* body, const dVector& centre, const dVector& velocity, const dVector& omega, dFloat drag, const dVector& wind, const dVector& force, const dVector& torque) :
			m_body(body),
			m_centre(centre),
			m_velocity(velocity),
			m_omega(omega),
			m_drag(drag),
			m_wind(wind),
			m_force(force),
			m_torque(torque)
		{
		}
	};

	// Variables
	static std::set<const NewtonBody*> s_valid_bodies;

	// Callbacks
	static void destructor_callback(const NewtonBody* const body);
	static void transform_callback(const NewtonBody* const body, const dFloat* const matrix, int thread_index);
	static void force_and_torque_callback(const NewtonBody* const body, dFloat timestep, int thread_index);
	static void collision_iterator(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id);
	static void collision_iterator2(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id);
	static void collision_iterator3(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id);
	static void collision_iterator4(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id);

	// Helper Functions
	static bool c_is_body_valid(const NewtonBody* address);
	static VALUE c_body_to_value(const NewtonBody* body);
	static const NewtonBody* c_value_to_body(VALUE v_body);
	static bool c_bodies_collidable(const NewtonBody* body0, const NewtonBody* body1);
	static bool c_bodies_aabb_overlap(const NewtonBody* body0, const NewtonBody* body1);
	static void c_validate_two_bodies(const NewtonBody* body1, const NewtonBody* body2);
	static void c_clear_non_collidable_bodies(const NewtonBody* body);
	static void c_body_add_force(BodyData* body_data, const dVector& force);
	static void c_body_set_force(BodyData* body_data, const dVector& force);
	static void c_body_add_torque(BodyData* body_data, const dVector& torque);
	static void c_body_set_torque(BodyData* body_data, const dVector& torque);

	// Ruby Functions
	static VALUE rbf_is_valid(VALUE self, VALUE v_body);
	static VALUE rbf_create(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_type, VALUE v_id, VALUE v_group);
	static VALUE rbf_destroy(VALUE self, VALUE v_body);
	static VALUE rbf_get_type(VALUE self, VALUE v_body);
	static VALUE rbf_get_world(VALUE self, VALUE v_body);
	static VALUE rbf_get_collision(VALUE self, VALUE v_body);
	static VALUE rbf_get_simulation_state(VALUE self, VALUE v_body);
	static VALUE rbf_set_simulation_state(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE rbf_get_continuous_collision_state(VALUE self, VALUE v_body);
	static VALUE rbf_set_continuous_collision_state(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE rbf_get_matrix(VALUE self, VALUE v_body);
	static VALUE rbf_get_normal_matrix(VALUE self, VALUE v_body);
	static VALUE rbf_set_matrix(VALUE self, VALUE v_body, VALUE v_matrix);
	static VALUE rbf_get_position(VALUE self, VALUE v_body, VALUE v_mode);
	static VALUE rbf_set_position(VALUE self, VALUE v_body, VALUE v_position, VALUE v_mode);
	static VALUE rbf_get_rotation(VALUE self, VALUE v_body);
	static VALUE rbf_get_euler_angles(VALUE self, VALUE v_body);
	static VALUE rbf_set_euler_angles(VALUE self, VALUE v_body, VALUE v_angles);
	static VALUE rbf_get_velocity(VALUE self, VALUE v_body);
	static VALUE rbf_set_velocity(VALUE self, VALUE v_body, VALUE v_velocity);
	static VALUE rbf_integrate_velocity(VALUE self, VALUE v_body, VALUE v_timestep);
	static VALUE rbf_get_omega(VALUE self, VALUE v_body);
	static VALUE rbf_set_omega(VALUE self, VALUE v_body, VALUE v_omega);
	static VALUE rbf_get_centre_of_mass(VALUE self, VALUE v_body);
	static VALUE rbf_set_centre_of_mass(VALUE self, VALUE v_body, VALUE v_com);
	static VALUE rbf_get_inertia(VALUE self, VALUE v_body);
	static VALUE rbf_get_mass(VALUE self, VALUE v_body);
	static VALUE rbf_set_mass(VALUE self, VALUE v_body, VALUE v_mass);
	static VALUE rbf_get_density(VALUE self, VALUE v_body);
	static VALUE rbf_set_density(VALUE self, VALUE v_body, VALUE v_density);
	static VALUE rbf_get_volume(VALUE self, VALUE v_body);
	static VALUE rbf_set_volume(VALUE self, VALUE v_body, VALUE v_volume);
	static VALUE rbf_reset_mass_properties(VALUE self, VALUE v_body, VALUE v_density);
	static VALUE rbf_is_static(VALUE self, VALUE v_body);
	static VALUE rbf_set_static(VALUE self, VALUE v_body, VALUE v_static);
	static VALUE rbf_is_collidable(VALUE self, VALUE v_body);
	static VALUE rbf_set_collidable(VALUE self, VALUE v_body, VALUE v_collidable);
	static VALUE rbf_is_frozen(VALUE self, VALUE v_body);
	static VALUE rbf_set_frozen(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE rbf_is_sleeping(VALUE self, VALUE v_body);
	static VALUE rbf_set_sleeping(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE rbf_get_auto_sleep_state(VALUE self, VALUE v_body);
	static VALUE rbf_set_auto_sleep_state(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE rbf_is_non_collidable_with(VALUE self, VALUE v_body, VALUE v_other_body);
	static VALUE rbf_set_non_collidable_with(VALUE self, VALUE v_body, VALUE v_other_body, VALUE v_state);
	static VALUE rbf_get_non_collidable_bodies(VALUE self, VALUE v_body);
	static VALUE rbf_clear_non_collidable_bodies(VALUE self, VALUE v_body);
	static VALUE rbf_get_elasticity(VALUE self, VALUE v_body);
	static VALUE rbf_set_elasticity(VALUE self, VALUE v_body, VALUE v_elasticity);
	static VALUE rbf_get_softness(VALUE self, VALUE v_body);
	static VALUE rbf_set_softness(VALUE self, VALUE v_body, VALUE v_softness);
	static VALUE rbf_get_static_friction(VALUE self, VALUE v_body);
	static VALUE rbf_set_static_friction(VALUE self, VALUE v_body, VALUE v_friction);
	static VALUE rbf_get_kinetic_friction(VALUE self, VALUE v_body);
	static VALUE rbf_set_kinetic_friction(VALUE self, VALUE v_body, VALUE v_friction);
	static VALUE rbf_get_friction_state(VALUE self, VALUE v_body);
	static VALUE rbf_set_friction_state(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE rbf_get_magnet_mode(VALUE self, VALUE v_body);
	static VALUE rbf_set_magnet_mode(VALUE self, VALUE v_body, VALUE v_mode);
	static VALUE rbf_get_magnet_force(VALUE self, VALUE v_body);
	static VALUE rbf_set_magnet_force(VALUE self, VALUE v_body, VALUE v_force);
	static VALUE rbf_get_magnet_range(VALUE self, VALUE v_body);
	static VALUE rbf_set_magnet_range(VALUE self, VALUE v_body, VALUE v_range);
	static VALUE rbf_get_magnet_strength(VALUE self, VALUE v_body);
	static VALUE rbf_set_magnet_strength(VALUE self, VALUE v_body, VALUE v_strength);
	static VALUE rbf_is_magnetic(VALUE self, VALUE v_body);
	static VALUE rbf_set_magnetic(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE rbf_get_aabb(VALUE self, VALUE v_body);
	static VALUE rbf_get_linear_damping(VALUE self, VALUE v_body);
	static VALUE rbf_set_linear_damping(VALUE self, VALUE v_body, VALUE v_damp_vector);
	static VALUE rbf_get_angular_damping(VALUE self, VALUE v_body);
	static VALUE rbf_set_angular_damping(VALUE self, VALUE v_body, VALUE v_damp_vector);
	static VALUE rbf_get_point_velocity(VALUE self, VALUE v_body, VALUE v_point);
	static VALUE rbf_add_point_force(VALUE self, VALUE v_body, VALUE v_point, VALUE v_force);
	static VALUE rbf_add_impulse(VALUE self, VALUE v_body, VALUE v_center, VALUE v_delta_vel, VALUE v_timestep);
	static VALUE rbf_get_force(VALUE self, VALUE v_body);
	static VALUE rbf_add_force(VALUE self, VALUE v_body, VALUE v_force);
	static VALUE rbf_set_force(VALUE self, VALUE v_body, VALUE v_force);
	static VALUE rbf_get_torque(VALUE self, VALUE v_body);
	static VALUE rbf_add_torque(VALUE self, VALUE v_body, VALUE v_torque);
	static VALUE rbf_set_torque(VALUE self, VALUE v_body, VALUE v_torque);
	static VALUE rbf_get_net_contact_force(VALUE self, VALUE v_body);
	static VALUE rbf_get_net_joint_tension1(VALUE self, VALUE v_body);
	static VALUE rbf_get_net_joint_tension2(VALUE self, VALUE v_body);
	static VALUE rbf_get_contacts(VALUE self, VALUE v_body, VALUE v_inc_non_collidable);
	static VALUE rbf_get_touching_bodies(VALUE self, VALUE v_body, VALUE v_inc_non_collidable);
	static VALUE rbf_get_contact_points(VALUE self, VALUE v_body, VALUE v_inc_non_collidable);
	static VALUE rbf_get_collision_faces(VALUE self, VALUE v_body);
	static VALUE rbf_get_collision_faces2(VALUE self, VALUE v_body);
	static VALUE rbf_get_collision_faces3(VALUE self, VALUE v_body);
	static VALUE rbf_apply_pick_and_drag(VALUE self, VALUE v_body, VALUE v_pick_pt, VALUE v_dest_pt, VALUE v_stiffness, VALUE v_damp, VALUE v_timestep);
	static VALUE rbf_apply_buoyancy(VALUE self, VALUE v_body, VALUE v_plane_origin, VALUE v_plane_normal, VALUE v_density, VALUE v_linear_viscosity, VALUE v_angular_viscosity, VALUE v_linear_current, VALUE v_angular_current, VALUE v_timestep);
	static VALUE rbf_apply_aerodynamics(VALUE self, VALUE v_body, VALUE v_drag, VALUE v_wind);
	static VALUE rbf_copy(VALUE self, VALUE v_body, VALUE v_matrix, VALUE v_reapply_forces, VALUE v_type, VALUE v_group);
	static VALUE rbf_get_destructor_proc(VALUE self, VALUE v_body);
	static VALUE rbf_set_destructor_proc(VALUE self, VALUE v_body, VALUE v_proc);
	static VALUE rbf_get_user_data(VALUE self, VALUE v_body);
	static VALUE rbf_set_user_data(VALUE self, VALUE v_body, VALUE v_user_data);
	static VALUE rbf_get_record_touch_data_state(VALUE self, VALUE v_body);
	static VALUE rbf_set_record_touch_data_state(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE rbf_get_matrix_scale(VALUE self, VALUE v_body);
	static VALUE rbf_set_matrix_scale(VALUE self, VALUE v_body, VALUE v_scale);
	static VALUE rbf_matrix_changed(VALUE self, VALUE v_body);
	static VALUE rbf_enable_gravity(VALUE self, VALUE v_body, VALUE v_state);
	static VALUE rbf_is_gravity_enabled(VALUE self, VALUE v_body);
	static VALUE rbf_get_contained_joints(VALUE self, VALUE v_body);
	static VALUE rbf_get_connected_joints(VALUE self, VALUE v_body);
	static VALUE rbf_get_connected_bodies(VALUE self, VALUE v_body);
	static VALUE rbf_get_material_id(VALUE self, VALUE v_body);
	static VALUE rbf_set_material_id(VALUE self, VALUE v_body, VALUE v_id);
	static VALUE rbf_get_collision_scale(VALUE self, VALUE v_body);
	static VALUE rbf_set_collision_scale(VALUE self, VALUE v_body, VALUE v_scale);
	static VALUE rbf_get_default_collision_scale(VALUE self, VALUE v_body);
	static VALUE rbf_get_actual_matrix_scale(VALUE self, VALUE v_body);
	static VALUE rbf_get_group(VALUE self, VALUE v_body);
	static VALUE rbf_get_body_by_group(VALUE self, VALUE v_world, VALUE v_body);
	static VALUE rbf_get_body_data_by_group(VALUE self, VALUE v_world, VALUE v_body);

	// Main
	static void init_ruby(VALUE mNewton);
};

#endif	/* MSP_BODY_H */
