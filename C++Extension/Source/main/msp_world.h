/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#ifndef MSP_WORLD_H
#define MSP_WORLD_H

#include "msp.h"

class MSP::World {
public:
    // Constants
    static const dVector DEFAULT_GRAVITY;
    static const int DEFAULT_SOLVER_MODEL;
    static const int DEFAULT_CONVERGENCE_QUALITY;
    static const dFloat DEFAULT_MATERIAL_THICKNESS;
    static const dFloat DEFAULT_CONTACT_MERGE_TOLERANCE;
    static const dFloat MIN_TOUCH_DISTANCE;
    static const dFloat MIN_TIMESTEP;
    static const dFloat MAX_TIMESTEP;

    // Structures
    struct BodyTouchData {
        const NewtonBody* m_body0;
        const NewtonBody* m_body1;
        dVector m_point;
        dVector m_normal;
        dVector m_force;
        dFloat m_speed;
        BodyTouchData(const NewtonBody* body0, const NewtonBody* body1, const dVector& point, const dVector& normal, const dVector& force, dFloat speed) :
            m_body0(body0),
            m_body1(body1),
            m_point(point),
            m_normal(normal),
            m_force(force),
            m_speed(speed)
        {
        }
    };

    struct BodyTouchingData {
        const NewtonBody* m_body0;
        const NewtonBody* m_body1;
        BodyTouchingData(const NewtonBody* body0, const NewtonBody* body1) :
            m_body0(body0),
            m_body1(body1)
        {
        }
    };

    struct BodyUntouchData {
        const NewtonBody* m_body0;
        const NewtonBody* m_body1;
        BodyUntouchData(const NewtonBody* body0, const NewtonBody* body1) :
            m_body0(body0),
            m_body1(body1)
        {
        }
    };

    struct DrawData {
        VALUE m_view;
        VALUE m_bb;
        DrawData(VALUE view, VALUE bb) :
            m_view(view),
            m_bb(bb)
        {
        }
    };

    struct HitData {
        const NewtonBody* m_body;
        dVector m_point;
        dVector m_normal;
        HitData(const NewtonBody* body, const dVector& point, const dVector& normal) :
            m_body(body),
            m_point(point),
            m_normal(normal)
        {
        }
    };

    struct RayData {
        std::vector<HitData*> m_hits;
    };

    struct WorldData {
        unsigned int m_max_threads;
        int m_solver_model;
        dFloat m_material_thickness;
        dVector m_gravity;
        VALUE m_user_info;
        VALUE m_body_destructors;
        VALUE m_body_user_datas;
        VALUE m_body_groups;
        VALUE m_joint_user_datas;
        VALUE m_gear_user_datas;
        std::map<VALUE, const NewtonBody*> m_group_to_body_map;
        std::vector<BodyTouchData*> m_touch_data;
        std::vector<BodyTouchingData*> m_touching_data;
        std::vector<BodyUntouchData*> m_untouch_data;
        std::vector<const NewtonJoint*> m_joints_to_disconnect;
        double m_time;
        int m_material_id;
        std::vector<const NewtonBody*> m_temp_cccd_bodies;
        NewtonWorldConvexCastReturnInfo m_hit_buffer[MSP_MAX_RAY_HITS];
        WorldData(int material_id) :
            m_max_threads(1),
            m_solver_model(DEFAULT_SOLVER_MODEL),
            m_material_thickness(DEFAULT_MATERIAL_THICKNESS),
            m_gravity(DEFAULT_GRAVITY),
            m_user_info(rb_ary_new2(7)),
            m_body_destructors(rb_hash_new()),
            m_body_user_datas(rb_hash_new()),
            m_body_groups(rb_hash_new()),
            m_joint_user_datas(rb_hash_new()),
            m_gear_user_datas(rb_hash_new()),
            m_time(0.0),
            m_material_id(material_id)
        {
            rb_gc_register_address(&m_user_info);
            rb_ary_store(m_user_info, 0, Qnil); // world destructor proc
            rb_ary_store(m_user_info, 1, Qnil); // world user data
            rb_ary_store(m_user_info, 2, m_body_destructors);
            rb_ary_store(m_user_info, 3, m_body_user_datas);
            rb_ary_store(m_user_info, 4, m_body_groups);
            rb_ary_store(m_user_info, 5, m_joint_user_datas);
            rb_ary_store(m_user_info, 6, m_gear_user_datas);
        }
        ~WorldData()
        {
#ifndef RUBY_VERSION18
            rb_hash_clear(m_body_destructors);
            rb_hash_clear(m_body_user_datas);
            rb_hash_clear(m_body_groups);
            rb_hash_clear(m_joint_user_datas);
            rb_hash_clear(m_gear_user_datas);
            rb_ary_clear(m_user_info);
#endif
            rb_gc_unregister_address(&m_user_info);
        }
    };

    // Variables
    static std::map<const NewtonWorld*, WorldData*> valid_worlds;

    // Callback Functions
    static void destructor_callback(const NewtonWorld* const world);
    static int aabb_overlap_callback(const NewtonJoint* const contact, dFloat timestep, int thread_index);
    static void contact_callback(const NewtonJoint* const contact_joint, dFloat timestep, int thread_index);
    static unsigned ray_prefilter_callback(const NewtonBody* const body, const NewtonCollision* const collision, void* const user_data);
    static unsigned ray_prefilter_callback_continuous(const NewtonBody* const body, const NewtonCollision* const collision, void* const user_data);
    static dFloat ray_filter_callback(const NewtonBody* const body, const NewtonCollision* const shape_hit, const dFloat* const hit_contact, const dFloat* const hit_normal, dLong collision_id, void* const user_data, dFloat intersect_param);
    static dFloat continuous_ray_filter_callback(const NewtonBody* const body, const NewtonCollision* const shape_hit, const dFloat* const hit_contact, const dFloat* const hit_normal, dLong collision_id, void* const user_data, dFloat intersect_param);
    static int body_iterator(const NewtonBody* const body, void* const user_data);
    static void collision_copy_constructor_callback(const NewtonWorld* const world, NewtonCollision* const collision, const NewtonCollision* const source_collision);
    static void collision_destructor_callback(const NewtonWorld* const world, const NewtonCollision* const collision);
    static void draw_collision_iterator(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id);

    // Helper Functions
    static bool c_is_world_valid(const NewtonWorld* address);
    static VALUE c_world_to_value(const NewtonWorld* world);
    static const NewtonWorld* c_value_to_world(VALUE v_world);
    static void c_update_magnets(const NewtonWorld* world, dFloat timestep);
    static void c_process_touch_events(const NewtonWorld* world);
    static void c_clear_touch_events(const NewtonWorld* world);
    static void c_clear_matrix_change_record(const NewtonWorld* world);
    static void c_disconnect_flagged_joints(const NewtonWorld* world);
    static void c_enable_cccd_bodies(const NewtonWorld* world);

    // Ruby Functions
    static VALUE rbf_is_valid(VALUE self, VALUE v_world);
    static VALUE rbf_create(VALUE self);
    static VALUE rbf_destroy(VALUE self, VALUE v_world);
    static VALUE rbf_get_max_possible_threads_count(VALUE self, VALUE v_world);
    static VALUE rbf_get_max_threads_count(VALUE self, VALUE v_world);
    static VALUE rbf_set_max_threads_count(VALUE self, VALUE v_world, VALUE v_count);
    static VALUE rbf_get_cur_threads_count(VALUE self, VALUE v_world);
    static VALUE rbf_destroy_all_bodies(VALUE self, VALUE v_world);
    static VALUE rbf_get_body_count(VALUE self, VALUE v_world);
    static VALUE rbf_get_constraint_count(VALUE self, VALUE v_world);
    static VALUE rbf_update(VALUE self, VALUE v_world, VALUE v_timestep);
    static VALUE rbf_update_async(VALUE self, VALUE v_world, VALUE v_timestep);
    static VALUE rbf_get_gravity(VALUE self, VALUE v_world);
    static VALUE rbf_set_gravity(VALUE self, VALUE v_world, VALUE v_gravity);
    static VALUE rbf_get_bodies(VALUE self, VALUE v_world);
    static VALUE rbf_get_joints(VALUE self, VALUE v_world);
    static VALUE rbf_get_gears(VALUE self, VALUE v_world);
    static VALUE rbf_get_bodies_in_aabb(VALUE self, VALUE v_world, VALUE v_min_pt, VALUE v_max_pt);
    static VALUE rbf_get_first_body(VALUE self, VALUE v_world);
    static VALUE rbf_get_next_body(VALUE self, VALUE v_world, VALUE v_body);
    static VALUE rbf_get_solver_model(VALUE self, VALUE v_world);
    static VALUE rbf_set_solver_model(VALUE self, VALUE v_world, VALUE v_solver_model);
    static VALUE rbf_get_material_thickness(VALUE self, VALUE v_world);
    static VALUE rbf_set_material_thickness(VALUE self, VALUE v_world, VALUE v_material_thinkness);
    static VALUE rbf_ray_cast(VALUE self, VALUE v_world, VALUE v_point1, VALUE v_point2);
    static VALUE rbf_continuous_ray_cast(VALUE self, VALUE v_world, VALUE v_point1, VALUE v_point2);
    static VALUE rbf_convex_ray_cast(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_target);
    static VALUE rbf_continuous_convex_ray_cast(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_target, VALUE v_max_hits);
    static VALUE rbf_add_explosion(VALUE self, VALUE v_world, VALUE v_center, VALUE v_blast_radius, VALUE v_blast_force);
    static VALUE rbf_get_aabb(VALUE self, VALUE v_world);
    static VALUE rbf_get_destructor_proc(VALUE self, VALUE v_world);
    static VALUE rbf_set_destructor_proc(VALUE self, VALUE v_world, VALUE v_proc);
    static VALUE rbf_get_user_data(VALUE self, VALUE v_world);
    static VALUE rbf_set_user_data(VALUE self, VALUE v_world, VALUE v_user_data);
    static VALUE rbf_get_touch_data_at(VALUE self, VALUE v_world, VALUE v_index);
    static VALUE rbf_get_touch_data_count(VALUE self, VALUE v_world);
    static VALUE rbf_get_touching_data_at(VALUE self, VALUE v_world, VALUE v_index);
    static VALUE rbf_get_touching_data_count(VALUE self, VALUE v_world);
    static VALUE rbf_get_untouch_data_at(VALUE self, VALUE v_world, VALUE v_index);
    static VALUE rbf_get_untouch_data_count(VALUE self, VALUE v_world);
    static VALUE rbf_get_time(VALUE self, VALUE v_world);
    static VALUE rbf_serialize_to_file(VALUE self, VALUE v_world, VALUE v_full_path);
    static VALUE rbf_get_contact_merge_tolerance(VALUE self, VALUE v_world);
    static VALUE rbf_set_contact_merge_tolerance(VALUE self, VALUE v_world, VALUE v_tolerance);
    static VALUE rbf_get_default_material_id(VALUE self, VALUE v_world);
    static VALUE rbf_draw_collision_wireframe(VALUE self, VALUE v_world, VALUE v_view, VALUE v_bb, VALUE v_sleep_color, VALUE v_active_color, VALUE v_line_width, VALUE v_line_stipple);
    static VALUE rbf_clear_matrix_change_record(VALUE self, VALUE v_world);

    // Main
    static void init_ruby(VALUE mNewton);
};

#endif  /* MSP_WORLD_H */
