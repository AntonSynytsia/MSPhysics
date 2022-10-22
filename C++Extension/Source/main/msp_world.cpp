/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_world.h"
#include "msp_collision.h"
#include "msp_body.h"
#include "msp_joint.h"
#include "msp_gear.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dVector MSP::World::DEFAULT_GRAVITY(0.0, 0.0, -9.8f);
const int MSP::World::DEFAULT_SOLVER_MODEL(4);
const int MSP::World::DEFAULT_CONVERGENCE_QUALITY(1);
const dFloat MSP::World::DEFAULT_MATERIAL_THICKNESS(0.005f);
const dFloat MSP::World::DEFAULT_CONTACT_MERGE_TOLERANCE(0.005f);
const dFloat MSP::World::MIN_TOUCH_DISTANCE(0.005f);
const dFloat MSP::World::MIN_TIMESTEP(1.0 / 1200.0);
const dFloat MSP::World::MAX_TIMESTEP(1.0 / 30.0);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

std::map<const NewtonWorld*, MSP::World::WorldData*> MSP::World::valid_worlds;


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::World::destructor_callback(const NewtonWorld* const world) {
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    if (valid_worlds.find(world) != valid_worlds.end())
        valid_worlds.erase(world);
    c_clear_touch_events(world);
    // Call world destructor procedure
    if (rb_ary_entry(world_data->m_user_info, 0) != Qnil)
        rb_rescue2(RUBY_METHOD_FUNC(Util::call_proc), rb_ary_entry(world_data->m_user_info, 0), RUBY_METHOD_FUNC(Util::rescue_proc), Qnil, rb_eException, (VALUE)0);
    for (std::set<MSP::Gear::GearData*>::iterator it = MSP::Gear::s_valid_gears.begin(); it != MSP::Gear::s_valid_gears.end();) {
        MSP::Gear::GearData* gear_data = *it;
        ++it;
        if (gear_data->m_world == world)
            MSP::Gear::c_destroy(gear_data);
    }
    for (std::map<MSP::Joint::JointData*, bool>::iterator it = MSP::Joint::s_valid_joints.begin(); it != MSP::Joint::s_valid_joints.end();) {
        MSP::Joint::JointData* joint_data = it->first;
        ++it;
        if (joint_data->m_world == world)
            MSP::Joint::c_destroy(joint_data);
    }
    NewtonDestroyAllBodies(world); // make sure all bodies are destroyed prior to destroying the world!
    delete world_data;
}

int MSP::World::aabb_overlap_callback(const NewtonJoint* const contact, dFloat timestep, int thread_index) {
    const NewtonBody* body0 = NewtonJointGetBody0(contact);
    const NewtonBody* body1 = NewtonJointGetBody1(contact);
    MSP::Body::BodyData* data0 = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body0));
    MSP::Body::BodyData* data1 = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body1));
    NewtonWorld* world = NewtonBodyGetWorld(body0);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    if ((!data0->m_collidable || !data1->m_collidable) ||
        (data0->m_bstatic && data1->m_bstatic) ||
        (NewtonBodyGetFreezeState(body0) == 1 && NewtonBodyGetFreezeState(body1) == 1) ||
        (data0->m_bstatic && NewtonBodyGetFreezeState(body1) == 1) ||
        (data1->m_bstatic && NewtonBodyGetFreezeState(body0) == 1) ||
        (data0->m_non_collidable_bodies.find(body1) != data0->m_non_collidable_bodies.end()) ||
        (data1->m_non_collidable_bodies.find(body0) != data1->m_non_collidable_bodies.end())) {
        if (NewtonBodyGetContinuousCollisionMode(body0) == 1) {
            NewtonBodySetContinuousCollisionMode(body0, 0);
            world_data->m_temp_cccd_bodies_mutex.lock();
            world_data->m_temp_cccd_bodies.push_back(body0);
            world_data->m_temp_cccd_bodies_mutex.unlock();
        }
        if (NewtonBodyGetContinuousCollisionMode(body1) == 1) {
            NewtonBodySetContinuousCollisionMode(body1, 0);
            world_data->m_temp_cccd_bodies_mutex.lock();
            world_data->m_temp_cccd_bodies.push_back(body1);
            world_data->m_temp_cccd_bodies_mutex.unlock();
        }
        return 0;
    }
    else if (NewtonBodyGetFreezeState(body0) == 1 || NewtonBodyGetFreezeState(body1) == 1) {
        if (NewtonBodyGetContinuousCollisionMode(body0) == 1 || NewtonBodyGetContinuousCollisionMode(body1) == 1) {
            NewtonBodySetFreezeState(body0, 0);
            NewtonBodySetFreezeState(body1, 0);
            return 1;
        }
        else {
            const NewtonCollision* colA = NewtonBodyGetCollision(body0);
            const NewtonCollision* colB = NewtonBodyGetCollision(body1);
            dMatrix matrixA, matrixB;
            NewtonBodyGetMatrix(body0, &matrixA[0][0]);
            NewtonBodyGetMatrix(body1, &matrixB[0][0]);
            if (NewtonCollisionIntersectionTest(world, colA, &matrixA[0][0], colB, &matrixB[0][0], 0) == 1) {
                NewtonBodySetFreezeState(body0, 0);
                NewtonBodySetFreezeState(body1, 0);
                return 1;
            }
            else
                return 0;
        }
    }
    else
        return 1;
}

void MSP::World::contact_callback(const NewtonJoint* const contact_joint, dFloat timestep, int thread_index) {
    const NewtonBody* body0 = NewtonJointGetBody0(contact_joint);
    const NewtonBody* body1 = NewtonJointGetBody1(contact_joint);
    MSP::Body::BodyData* data0 = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body0));
    MSP::Body::BodyData* data1 = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body1));
    for (void* contact = NewtonContactJointGetFirstContact(contact_joint); contact; contact = NewtonContactJointGetNextContact(contact_joint, contact)) {
        NewtonMaterial* material = NewtonContactGetMaterial(contact);
        if (data0->m_friction_enabled && data1->m_friction_enabled) {
            dFloat sfc = (data0->m_static_friction + data1->m_static_friction) * 0.5f;
            dFloat kfc = (data0->m_kinetic_friction + data1->m_kinetic_friction) * 0.5f;
            NewtonMaterialSetContactFrictionCoef(material, sfc, kfc, 0);
            NewtonMaterialSetContactFrictionCoef(material, sfc, kfc, 1);
        }
        else {
            NewtonMaterialSetContactFrictionState(material, 0, 0);
            NewtonMaterialSetContactFrictionState(material, 0, 1);
        }
        dFloat cor = (data0->m_elasticity + data1->m_elasticity) * 0.5f;
        dFloat sft = (data0->m_softness + data1->m_softness) * 0.5f;
        NewtonMaterialSetContactElasticity(material, cor);
        NewtonMaterialSetContactSoftness(material, sft);
    }
    const NewtonWorld* world = NewtonBodyGetWorld(body0);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    void* contact = NewtonContactJointGetFirstContact(contact_joint);
    const NewtonMaterial* material = NewtonContactGetMaterial(contact);
    if (data0->m_record_touch_data) {
        world_data->m_touch_mutex.lock();
        if (data0->m_touchers.find(body1) == data0->m_touchers.end()) {
            dVector point;
            dVector normal;
            dVector force;
            NewtonMaterialGetContactPositionAndNormal(material, body0, &point[0], &normal[0]);
            NewtonMaterialGetContactForce(material, body0, &force[0]);
            BodyTouchData* touch_data = new BodyTouchData(body0, body1, point, normal, force, NewtonMaterialGetContactNormalSpeed(material));
            world_data->m_touch_data.push_back(touch_data);
            data0->m_touchers[body1] = 0;
        }
        else
            data0->m_touchers[body1] = 2;
        world_data->m_touch_mutex.unlock();
    }
    if (data1->m_record_touch_data) {
        world_data->m_touch_mutex.lock();
        if (data1->m_touchers.find(body0) == data1->m_touchers.end()) {
            dVector point;
            dVector normal;
            dVector force;
            NewtonMaterialGetContactPositionAndNormal(material, body1, &point[0], &normal[0]);
            NewtonMaterialGetContactForce(material, body1, &force[0]);
            BodyTouchData* touch_data = new BodyTouchData(body1, body0, point, normal, force, NewtonMaterialGetContactNormalSpeed(material));
            world_data->m_touch_data.push_back(touch_data);
            data1->m_touchers[body0] = 0;
        }
        else
            data1->m_touchers[body0] = 2;
        world_data->m_touch_mutex.unlock();
    }
}

unsigned MSP::World::ray_prefilter_callback(const NewtonBody* const body, const NewtonCollision* const collision, void* const user_data) {
    return 0;
}

unsigned MSP::World::ray_prefilter_callback_continuous(const NewtonBody* const body, const NewtonCollision* const collision, void* const user_data) {
    return 1;
}

dFloat MSP::World::ray_filter_callback(const NewtonBody* const body, const NewtonCollision* const shape_hit, const dFloat* const hit_contact, const dFloat* const hit_normal, dLong collision_id, void* const user_data, dFloat intersect_param) {
    HitData* hit = reinterpret_cast<HitData*>(user_data);
    hit->m_body = body;
    hit->m_point = dVector(hit_contact);
    hit->m_normal = dVector(hit_normal);
    return intersect_param;
}

dFloat MSP::World::continuous_ray_filter_callback(const NewtonBody* const body, const NewtonCollision* const shape_hit, const dFloat* const hit_contact, const dFloat* const hit_normal, dLong collision_id, void* const user_data, dFloat intersect_param) {
    RayData* ray_data = reinterpret_cast<RayData*>(user_data);
    HitData* hit = new HitData(body, dVector(hit_contact), dVector(hit_normal));
    ray_data->m_hits.push_back(hit);
    return 1.0;
}

int MSP::World::body_iterator(const NewtonBody* const body, void* const user_data) {
    std::vector<const NewtonBody*>* bodies = reinterpret_cast<std::vector<const NewtonBody*>*>(user_data);
    bodies->push_back(body);
    return 1;
}

void MSP::World::collision_copy_constructor_callback(const NewtonWorld* const world, NewtonCollision* const collision, const NewtonCollision* const source_collision) {
    MSP::Collision::CollisionData* data = MSP::Collision::s_valid_collisions[source_collision];
    MSP::Collision::s_valid_collisions[collision] = new MSP::Collision::CollisionData(data->m_scale);
}

void MSP::World::collision_destructor_callback(const NewtonWorld* const world, const NewtonCollision* const collision) {
    std::map<const NewtonCollision*, MSP::Collision::CollisionData*>::iterator it = MSP::Collision::s_valid_collisions.find(collision);
    if (it != MSP::Collision::s_valid_collisions.end()) {
        delete it->second;
        MSP::Collision::s_valid_collisions.erase(it);
    }
}

void MSP::World::draw_collision_iterator(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id) {
    DrawData* draw_data = reinterpret_cast<DrawData*>(user_data);
    VALUE v_face = rb_ary_new2(vertex_count);
    for (int i = 0; i < vertex_count * 3; i += 3) {
        dVector vertex(face_array[i], face_array[i+1], face_array[i+2]);
        rb_ary_store(v_face, i, Util::point_to_value(vertex));
    };
    rb_funcall(draw_data->m_view, Util::INTERN_DRAW, 2, INT2FIX(2), v_face);
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

bool MSP::World::c_is_world_valid(const NewtonWorld* address) {
    return valid_worlds.find(address) != valid_worlds.end();
}

VALUE MSP::World::c_world_to_value(const NewtonWorld* world) {
    return rb_ull2inum(reinterpret_cast<unsigned long long>(world));
}

const NewtonWorld* MSP::World::c_value_to_world(VALUE v_world) {
    const NewtonWorld* address = reinterpret_cast<NewtonWorld*>(rb_num2ull(v_world));
    if (Util::s_validate_objects && valid_worlds.find(address) == valid_worlds.end())
        rb_raise(rb_eTypeError, "Given address doesn't reference a valid world!");
    return address;
}

void MSP::World::c_update_magnets(const NewtonWorld* world, dFloat timestep) {
    dMatrix matrix;
    dVector com;
    dMatrix other_matrix;
    dVector other_com;
    dVector dir;
    for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
        MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body));
        if (body_data->m_magnet_mode == 1) {
            // Magnet force is dissipated linearly over the magnet range.
            /*if (dAbs(body_data->magnet_force) > M_EPSILON && body_data->magnet_range > M_EPSILON) {
                NewtonBodyGetMatrix(body, &matrix[0][0]);
                NewtonBodyGetCentreOfMass(body, &com[0]);
                com = matrix.TransformVector(com);
                for (const NewtonBody* other_body = NewtonWorldGetFirstBody(world); other_body; other_body = NewtonWorldGetNextBody(world, other_body)) {
                    if (other_body != body) {
                        MSP::Body::BodyData* other_body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(other_body));
                        if (other_body_data->magnetic) {
                            NewtonBodyGetMatrix(other_body, &other_matrix[0][0]);
                            NewtonBodyGetCentreOfMass(other_body, &other_com[0]);
                            other_com = other_matrix.TransformVector(other_com);
                            dir = other_com - com;
                            dFloat dist = Util::get_vector_magnitude(dir);
                            if (dist > M_EPSILON && dist < body_data->magnet_range) {
                                Util::scale_vector(dir, (body_data->magnet_range - dist) * body_data->magnet_force / (body_data->magnet_range * dist));
                                MSP::Body::c_body_add_force(other_body_data, dVector(-dir.m_x, -dir.m_y, -dir.m_z));
                                // For every action there is an equal and opposite reaction
                                MSP::Body::c_body_add_force(body_data, dir);
                            }
                        }
                    }
                }
            }*/
            // Magnet force is dissipated quadratically over the magnet range.
            // actual_magnet_force = magnet_force * (distance - magnet_range)^2 / magnet_range^2
            if (dAbs(body_data->m_magnet_force) > M_EPSILON) {
                NewtonBodyGetMatrix(body, &matrix[0][0]);
                NewtonBodyGetCentreOfMass(body, &com[0]);
                com = matrix.TransformVector(com);
                for (const NewtonBody* other_body = NewtonWorldGetFirstBody(world); other_body; other_body = NewtonWorldGetNextBody(world, other_body)) {
                    if (other_body != body) {
                        MSP::Body::BodyData* other_body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(other_body));
                        if (other_body_data->m_magnetic) {
                            NewtonBodyGetMatrix(other_body, &other_matrix[0][0]);
                            NewtonBodyGetCentreOfMass(other_body, &other_com[0]);
                            other_com = other_matrix.TransformVector(other_com);
                            dir = other_com - com;
                            dFloat dist = Util::get_vector_magnitude(dir);
                            if (dist > M_EPSILON && dist < body_data->m_magnet_range) {
                                // f * (x-r)^2 / r^2 -> f * (x^2 - 2xr + r^2) / r^2
                                dFloat diff = dist - body_data->m_magnet_range;
                                dFloat actual_force = body_data->m_magnet_force * diff * diff / (body_data->m_magnet_range * body_data->m_magnet_range);
                                Util::scale_vector(dir, actual_force / dist);
                                MSP::Body::c_body_add_force(other_body_data, dVector(-dir.m_x, -dir.m_y, -dir.m_z));
                                // For every action there is an equal and opposite reaction
                                MSP::Body::c_body_add_force(body_data, dir);
                            }
                        }
                    }
                }
            }
        }
        else {
            // actual_magnet_force = magnet_strength / distance^2
            if (dAbs(body_data->m_magnet_strength) > M_EPSILON) {
                NewtonBodyGetMatrix(body, &matrix[0][0]);
                NewtonBodyGetCentreOfMass(body, &com[0]);
                com = matrix.TransformVector(com);
                for (const NewtonBody* other_body = NewtonWorldGetFirstBody(world); other_body; other_body = NewtonWorldGetNextBody(world, other_body)) {
                    if (other_body != body) {
                        MSP::Body::BodyData* other_body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(other_body));
                        if (other_body_data->m_magnetic) {
                            NewtonBodyGetMatrix(other_body, &other_matrix[0][0]);
                            NewtonBodyGetCentreOfMass(other_body, &other_com[0]);
                            other_com = other_matrix.TransformVector(other_com);
                            dir = other_com - com;
                            dFloat sq_dist = Util::get_vector_magnitude2(dir);
                            dFloat dist = dSqrt(sq_dist);
                            if (dist > M_EPSILON) {
                                // f = m / d^2
                                dFloat actual_force = body_data->m_magnet_strength / sq_dist;
                                Util::scale_vector(dir, actual_force / dist);
                                MSP::Body::c_body_add_force(other_body_data, dVector(-dir.m_x, -dir.m_y, -dir.m_z));
                                // For every action there is an equal and opposite reaction
                                MSP::Body::c_body_add_force(body_data, dir);
                            }
                        }
                    }
                }
            }
        }
    }
}

void MSP::World::c_process_touch_events(const NewtonWorld* world) {
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));

    // Generate onTouch events for non-collidable bodies.
    for (const NewtonBody* body0 = NewtonWorldGetFirstBody(world); body0; body0 = NewtonWorldGetNextBody(world, body0)) {
        MSP::Body::BodyData* body0_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body0));
        if (!body0_data->m_record_touch_data)
            continue;
        const NewtonCollision* colA = NewtonBodyGetCollision(body0);
        const NewtonCollision* colB;
        dMatrix matA;
        dMatrix matB;
        NewtonBodyGetMatrix(body0, &matA[0][0]);
        dFloat points[3];
        dFloat normals[3];
        dFloat penetrations[3];
        long long attrA[1];
        long long attrB[1];
        for (const NewtonBody* body1 = NewtonWorldGetFirstBody(world); body1; body1 = NewtonWorldGetNextBody(world, body1)) {
            if (body0 == body1 ||
                MSP::Body::c_bodies_collidable(body0, body1) ||
                !MSP::Body::c_bodies_aabb_overlap(body0, body1))
                continue;
            if (body0_data->m_touchers.find(body1) == body0_data->m_touchers.end()) {
                colB = NewtonBodyGetCollision(body1);
                NewtonBodyGetMatrix(body1, &matB[0][0]);
                if (NewtonCollisionCollide(world, 1, colA, &matA[0][0], colB, &matB[0][0], points, normals, penetrations, attrA, attrB, 0) != 0) {
                    BodyTouchData* touch_data = new BodyTouchData(body0, body1, dVector(points), dVector(normals), dVector(0.0), 0.0);
                    world_data->m_touch_data.push_back(touch_data);
                    body0_data->m_touchers[body1] = 0;
                }
            }
            else
                body0_data->m_touchers[body1] = 2;
        }
    }

    // Generate onTouching and onUntouch events for all bodies with touchers.
    for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
        MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body));
        if (!body_data->m_record_touch_data || body_data->m_touchers.empty()) continue;
        NewtonCollision* colA = NewtonBodyGetCollision(body);
        dMatrix matrixA;
        NewtonBodyGetMatrix(body, &matrixA[0][0]);
        std::vector<const NewtonBody*> to_erase;
        for (std::map<const NewtonBody*, char>::iterator it = body_data->m_touchers.begin(); it != body_data->m_touchers.end(); ++it) {
            if (it->second == 0) {
                body_data->m_touchers[it->first] = 1;
                continue;
            }
            bool touching = false;
            if (it->second == 1) {
                const NewtonCollision* colB = NewtonBodyGetCollision(it->first);
                dMatrix matrixB;
                NewtonBodyGetMatrix(it->first, &matrixB[0][0]);
                touching = NewtonCollisionIntersectionTest(world, colA, &matrixA[0][0], colB, &matrixB[0][0], 0) == 1;
                if (!touching && NewtonCollisionGetType(colA) < 9 && NewtonCollisionGetType(colB) < 9) {
                    dVector pointA;
                    dVector pointB;
                    dVector normalAB;
                    if (NewtonCollisionClosestPoint(world, colA, &matrixA[0][0], colB, &matrixB[0][0], &pointA[0], &pointB[0], &normalAB[0], 0) != 0) {
                        dVector diff(pointB - pointA);
                        dFloat dist = sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]) - world_data->m_material_thickness;
                        if (dist < MIN_TOUCH_DISTANCE) touching = true;
                    }
                }
            }
            else if (it->second == 2) {
                touching = true;
                body_data->m_touchers[it->first] = 1;
            }
            if (touching) {
                BodyTouchingData* touching_data = new BodyTouchingData(body, it->first);
                world_data->m_touching_data.push_back(touching_data);
            }
            else {
                BodyUntouchData* untouch_data = new BodyUntouchData(body, it->first);
                world_data->m_untouch_data.push_back(untouch_data);
                to_erase.push_back(it->first);
            }
        }
        for (std::vector<const NewtonBody*>::iterator it = to_erase.begin(); it != to_erase.end(); ++it) {
            const NewtonBody* body_to_erase = *it;
            if (body_data->m_touchers.find(body_to_erase) != body_data->m_touchers.end())
                body_data->m_touchers.erase(body_to_erase);
        }
    }
}

void MSP::World::c_clear_touch_events(const NewtonWorld* world) {
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    for (std::vector<BodyTouchData*>::const_iterator it = world_data->m_touch_data.begin(); it != world_data->m_touch_data.end(); ++it) {
        BodyTouchData* data = *it;
        delete data;
    }
    for (std::vector<BodyTouchingData*>::const_iterator it = world_data->m_touching_data.begin(); it != world_data->m_touching_data.end(); ++it) {
        BodyTouchingData* data = *it;
        delete data;
    }
    for (std::vector<BodyUntouchData*>::const_iterator it = world_data->m_untouch_data.begin(); it != world_data->m_untouch_data.end(); ++it) {
        BodyUntouchData* data = *it;
        delete data;
    }
    world_data->m_touch_data.clear();
    world_data->m_touching_data.clear();
    world_data->m_untouch_data.clear();
}

void MSP::World::c_clear_matrix_change_record(const NewtonWorld* world) {
    for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
        MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body));
        body_data->m_matrix_changed = false;
    }
}

void MSP::World::c_disconnect_flagged_joints(const NewtonWorld* world) {
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    for (std::vector<const NewtonJoint*>::iterator it = world_data->m_joints_to_disconnect.begin(); it != world_data->m_joints_to_disconnect.end(); ++it)
        NewtonDestroyJoint(world, *it);
    world_data->m_joints_to_disconnect.clear();
}

void MSP::World::c_enable_cccd_bodies(const NewtonWorld* world) {
    WorldData* world_data(reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world)));
    for (std::vector<const NewtonBody*>::iterator it = world_data->m_temp_cccd_bodies.begin(); it != world_data->m_temp_cccd_bodies.end(); ++it)
        NewtonBodySetContinuousCollisionMode(*it, 1);
    world_data->m_temp_cccd_bodies.clear();
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::World::rbf_is_valid(VALUE self, VALUE v_world) {
    return c_is_world_valid(reinterpret_cast<NewtonWorld*>(Util::value_to_ull(v_world))) ? Qtrue : Qfalse;
}

VALUE MSP::World::rbf_create(VALUE self) {
    NewtonWorld* world = NewtonCreate();
    int id = NewtonMaterialCreateGroupID(world);
    WorldData* world_data = new WorldData(id);
    world_data->m_max_threads = NewtonGetThreadsCount(world);
    valid_worlds[world] = world_data;
    NewtonWorldSetUserData(world, world_data);
    NewtonInvalidateCache(world);

    NewtonSetNumberOfSubsteps(world, 2);
	NewtonSetSolverIterations(world, DEFAULT_SOLVER_MODEL);
	NewtonSetThreadsCount(world, 1);

	NewtonSelectBroadphaseAlgorithm(world, 0);
	NewtonSetParallelSolverOnLargeIsland(world, 1);	

    NewtonSetContactMergeTolerance(world, DEFAULT_CONTACT_MERGE_TOLERANCE);
    NewtonMaterialSetSurfaceThickness(world, id, id, DEFAULT_MATERIAL_THICKNESS);
    NewtonMaterialSetDefaultFriction(world, id, id, MSP::Body::DEFAULT_STATIC_FRICTION_COEF, MSP::Body::DEFAULT_KINETIC_FRICTION_COEF);
    NewtonMaterialSetDefaultElasticity(world, id, id, MSP::Body::DEFAULT_ELASTICITY);
    NewtonMaterialSetDefaultSoftness(world, id, id, MSP::Body::DEFAULT_SOFTNESS);
    //NewtonSetSolverConvergenceQuality(world, DEFAULT_CONVERGENCE_QUALITY);
    //NewtonSelectBroadphaseAlgorithm(world, 0);
    NewtonMaterialSetCollisionCallback(world, id, id, aabb_overlap_callback, contact_callback);
    NewtonWorldSetDestructorCallback(world, destructor_callback);
    NewtonWorldSetCollisionConstructorDestructorCallback(world, collision_copy_constructor_callback, collision_destructor_callback);
    return c_world_to_value(world);
}

VALUE MSP::World::rbf_destroy(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    NewtonDestroy(world);
    return Qnil;
}

VALUE MSP::World::rbf_get_max_possible_threads_count(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    return Util::to_value(NewtonGetMaxThreadsCount(world));
}

VALUE MSP::World::rbf_get_max_threads_count(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return Util::to_value(world_data->m_max_threads);
}

VALUE MSP::World::rbf_set_max_threads_count(VALUE self, VALUE v_world, VALUE v_count) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    world_data->m_max_threads = Util::clamp_int(Util::value_to_int(v_count), 1, NewtonGetMaxThreadsCount(world));
    NewtonSetThreadsCount(world, world_data->m_max_threads);
    return Qnil;
}

VALUE MSP::World::rbf_get_cur_threads_count(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    return Util::to_value(NewtonGetThreadsCount(world));
}

VALUE MSP::World::rbf_destroy_all_bodies(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    int count = NewtonWorldGetBodyCount(world);
    NewtonDestroyAllBodies(world);
    return Util::to_value(count);
}

VALUE MSP::World::rbf_get_body_count(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    return Util::to_value( NewtonWorldGetBodyCount(world) );
}

VALUE MSP::World::rbf_get_constraint_count(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    return Util::to_value( NewtonWorldGetConstraintCount(world) );
}

VALUE MSP::World::rbf_update(VALUE self, VALUE v_world, VALUE v_timestep) {
    const NewtonWorld* world = c_value_to_world(v_world);
    dFloat timestep = Util::clamp_dFloat(Util::value_to_dFloat(v_timestep), MIN_TIMESTEP, MAX_TIMESTEP);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    c_clear_touch_events(world);
    c_update_magnets(world, timestep);
    NewtonUpdate(world, timestep);
    c_enable_cccd_bodies(world);
    c_disconnect_flagged_joints(world);
    c_process_touch_events(world);
    world_data->m_time += timestep;
    return Util::to_value(timestep);
}

VALUE MSP::World::rbf_update_async(VALUE self, VALUE v_world, VALUE v_timestep) {
    const NewtonWorld* world = c_value_to_world(v_world);
    dFloat timestep = Util::clamp_dFloat(Util::value_to_dFloat(v_timestep), MIN_TIMESTEP, MAX_TIMESTEP);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    c_clear_touch_events(world);
    c_update_magnets(world, timestep);
    NewtonUpdate(world, timestep);
    c_enable_cccd_bodies(world);
    c_disconnect_flagged_joints(world);
    c_process_touch_events(world);
    world_data->m_time += timestep;
    return Util::to_value(timestep);
}

VALUE MSP::World::rbf_get_gravity(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return Util::vector_to_value(world_data->m_gravity.Scale(M_INCH_TO_METER));
}

VALUE MSP::World::rbf_set_gravity(VALUE self, VALUE v_world, VALUE v_gravity) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    world_data->m_gravity = Util::value_to_vector(v_gravity).Scale(M_METER_TO_INCH);
    return Qnil;
}

VALUE MSP::World::rbf_get_bodies(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_bodies = rb_ary_new();
    for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
        VALUE v_address = MSP::Body::c_body_to_value(body);
        if (proc_given) {
            MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body));
            VALUE v_result = rb_yield_values(2, v_address, body_data->m_user_data);
            if (v_result != Qnil) rb_ary_push(v_bodies, v_result);
        }
        else
            rb_ary_push(v_bodies, v_address);
    }
    return v_bodies;
}

VALUE MSP::World::rbf_get_joints(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_joints = rb_ary_new();
    for (std::map<MSP::Joint::JointData*, bool>::iterator it = MSP::Joint::s_valid_joints.begin(); it != MSP::Joint::s_valid_joints.end(); ++it) {
        if (it->first->m_world == world) {
            VALUE v_address = MSP::Joint::c_joint_to_value(it->first);
            if (proc_given) {
                VALUE v_result = rb_yield_values(2, v_address, it->first->m_user_data);
                if (v_result != Qnil) rb_ary_push(v_joints, v_result);
            }
            else
                rb_ary_push(v_joints, v_address);
        }
    }
    return v_joints;
}

VALUE MSP::World::rbf_get_gears(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_gears = rb_ary_new();
    for (std::set<MSP::Gear::GearData*>::iterator it = MSP::Gear::s_valid_gears.begin(); it != MSP::Gear::s_valid_gears.end(); ++it) {
        MSP::Gear::GearData* gear_data = *it;
        if (gear_data->m_world == world) {
            VALUE v_address = MSP::Gear::c_gear_to_value(gear_data);
            if (proc_given) {
                VALUE v_result = rb_yield_values(2, v_address, gear_data->m_user_data);
                if (v_result != Qnil) rb_ary_push(v_gears, v_result);
            }
            else
                rb_ary_push(v_gears, v_address);
        }
    }
    return v_gears;
}

VALUE MSP::World::rbf_get_bodies_in_aabb(VALUE self, VALUE v_world, VALUE v_min_pt, VALUE v_max_pt) {
    const NewtonWorld* world = c_value_to_world(v_world);
    std::vector<const NewtonBody*> bodies;
    NewtonWorldForEachBodyInAABBDo(world, &(Util::value_to_point(v_min_pt))[0], &(Util::value_to_point(v_max_pt))[0], body_iterator, &bodies);
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_bodies = rb_ary_new();
    for (std::vector<const NewtonBody*>::iterator it = bodies.begin(); it != bodies.end(); ++it) {
        VALUE v_address = MSP::Body::c_body_to_value(*it);
        if (proc_given) {
            MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(*it));
            VALUE v_result = rb_yield_values(2, v_address, body_data->m_user_data);
            if (v_result != Qnil) rb_ary_push(v_bodies, v_result);
        }
        else
            rb_ary_push(v_bodies, v_address);
    }
    return v_bodies;
}

VALUE MSP::World::rbf_get_first_body(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    NewtonBody* body = NewtonWorldGetFirstBody(world);
    return body ? MSP::Body::c_body_to_value(body) : Qnil;
}

VALUE MSP::World::rbf_get_next_body(VALUE self, VALUE v_world, VALUE v_body) {
    const NewtonWorld* world = c_value_to_world(v_world);
    const NewtonBody* body = MSP::Body::c_value_to_body(v_body);
    NewtonBody* next_body = NewtonWorldGetNextBody(world, body);
    return next_body ? MSP::Body::c_body_to_value(next_body) : Qnil;
}

VALUE MSP::World::rbf_get_solver_model(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return Util::to_value( world_data->m_solver_model );
}

VALUE MSP::World::rbf_set_solver_model(VALUE self, VALUE v_world, VALUE v_solver_model) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    world_data->m_solver_model = Util::clamp_int(Util::value_to_int(v_solver_model), 1, 256);
    NewtonSetSolverIterations(world, world_data->m_solver_model);
    return Qnil;
}

VALUE MSP::World::rbf_get_material_thickness(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return Util::to_value( world_data->m_material_thickness );
}

VALUE MSP::World::rbf_set_material_thickness(VALUE self, VALUE v_world, VALUE v_material_thinkness) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    world_data->m_material_thickness = Util::clamp_dFloat(Util::value_to_dFloat(v_material_thinkness), 0.0, 1.0/32.0);
    NewtonMaterialSetSurfaceThickness(world, world_data->m_material_id, world_data->m_material_id, world_data->m_material_thickness);
    return Qnil;
}

VALUE MSP::World::rbf_ray_cast(VALUE self, VALUE v_world, VALUE v_point1, VALUE v_point2) {
    const NewtonWorld* world = c_value_to_world(v_world);
    dVector point1(Util::value_to_point(v_point1));
    dVector point2(Util::value_to_point(v_point2));
    HitData* hit = new HitData(nullptr, dVector(0.0), dVector(0.0));
    NewtonWorldRayCast(world, &point1[0], &point2[0], ray_filter_callback, reinterpret_cast<void*>(hit), NULL, 0);
    VALUE v_res;
    if (hit->m_body != nullptr) {
        MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(hit->m_body));
        v_res = rb_ary_new3(4, MSP::Body::c_body_to_value(hit->m_body), body_data->m_user_data, Util::point_to_value(hit->m_point), Util::vector_to_value(hit->m_normal));
    }
    else
        v_res = Qnil;
    delete hit;
    return v_res;
}

VALUE MSP::World::rbf_continuous_ray_cast(VALUE self, VALUE v_world, VALUE v_point1, VALUE v_point2) {
    const NewtonWorld* world = c_value_to_world(v_world);
    dVector point1(Util::value_to_point(v_point1));
    dVector point2(Util::value_to_point(v_point2));
    RayData* ray_data = new RayData();
    bool proc_given = (rb_block_given_p() != 0);
    NewtonWorldRayCast(world, &point1[0], &point2[0], continuous_ray_filter_callback, reinterpret_cast<void*>(ray_data), NULL, 0);
    VALUE v_hits = rb_ary_new();
    for (std::vector<HitData*>::iterator it = ray_data->m_hits.begin(); it != ray_data->m_hits.end(); ++it) {
        HitData* hit = *it;
        MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(hit->m_body));
        VALUE v_address = MSP::Body::c_body_to_value(hit->m_body);
        VALUE v_point = Util::point_to_value(hit->m_point);
        VALUE v_normal = Util::vector_to_value(hit->m_normal);
        if (proc_given) {
            VALUE v_result = rb_yield_values(4, v_address, body_data->m_user_data, v_point, v_normal);
            if (v_result != Qnil) rb_ary_push(v_hits, v_result);
        }
        else
            rb_ary_push(v_hits, rb_ary_new3(4, v_address, body_data->m_user_data, v_point, v_normal));
        delete hit;
    }
    delete ray_data;
    return v_hits;
}

VALUE MSP::World::rbf_convex_ray_cast(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_target) {
    const NewtonWorld* world = c_value_to_world(v_world);
    const NewtonCollision* collision = MSP::Collision::c_value_to_collision(v_collision);
    dMatrix matrix(Util::value_to_matrix(v_matrix));
    dVector target(Util::value_to_point(v_target));
    dFloat hit_param;
    NewtonWorldConvexCastReturnInfo info[1];
    int hit_count = NewtonWorldConvexCast(world, &matrix[0][0], &target[0], collision, &hit_param, nullptr, ray_prefilter_callback, &info[0], 1, 0);
    if (hit_count != 0) {
        MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(info[0].m_hitBody));
        return rb_ary_new3(
            5,
            MSP::Body::c_body_to_value(info[0].m_hitBody),
            body_data->m_user_data,
            Util::point_to_value(dVector(info[0].m_point)),
            Util::vector_to_value(dVector(info[0].m_normal)),
            Util::to_value(info[0].m_penetration));
    }
    else
        return Qnil;
}

VALUE MSP::World::rbf_continuous_convex_ray_cast(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_target, VALUE v_max_hits) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    const NewtonCollision* collision = MSP::Collision::c_value_to_collision(v_collision);
    dMatrix matrix(Util::value_to_matrix(v_matrix));
    dVector target(Util::value_to_point(v_target));
    bool proc_given = (rb_block_given_p() != 0);
    int max_hits = Util::clamp_int(Util::value_to_int(v_max_hits), 1, MSP_MAX_RAY_HITS);
    dFloat hit_param;
    int num_hits = NewtonWorldConvexCast(world, &matrix[0][0], &target[0], collision, &hit_param, nullptr, ray_prefilter_callback_continuous, &world_data->m_hit_buffer[0], max_hits, 0);
    VALUE v_hits = rb_ary_new();
    for (int i = 0; i < num_hits; ++i) {
        NewtonWorldConvexCastReturnInfo& hit = world_data->m_hit_buffer[i];
        MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(hit.m_hitBody));
        VALUE v_address = MSP::Body::c_body_to_value(hit.m_hitBody);
        VALUE v_point = Util::point_to_value(hit.m_point);
        VALUE v_normal = Util::vector_to_value(hit.m_normal);
        VALUE v_penetration = Util::to_value(hit.m_penetration);
        if (proc_given) {
            VALUE v_result = rb_yield_values(5, v_address, body_data->m_user_data, v_point, v_normal, v_penetration);
            if (v_result != Qnil)
                rb_ary_push(v_hits, v_result);
        }
        else
            rb_ary_push(v_hits, rb_ary_new3(5, v_address, body_data->m_user_data, v_point, v_normal, v_penetration));
    }
    return v_hits;
}

VALUE MSP::World::rbf_add_explosion(VALUE self, VALUE v_world, VALUE v_center, VALUE v_blast_radius, VALUE v_blast_force) {
    const NewtonWorld* world = c_value_to_world(v_world);
    dVector blast_point(Util::value_to_point(v_center));
    dFloat blast_radius = Util::value_to_dFloat(v_blast_radius) * M_METER_TO_INCH;
    dFloat blast_force = Util::value_to_dFloat(v_blast_force) * M_METER_TO_INCH;
    if (blast_radius > M_EPSILON && dAbs(blast_force) > M_EPSILON) {
        HitData* hit = new HitData(nullptr, dVector(0.0), dVector(0.0));
        dFloat inv_blast_radius = 1.0 / blast_radius;
        for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
            MSP::Body::BodyData* body_data = reinterpret_cast<MSP::Body::BodyData*>(NewtonBodyGetUserData(body));
            if (body_data->m_dynamic && !body_data->m_bstatic && body_data->m_mass > MSP::Body::MIN_MASS) {
                dMatrix matrix;
                dVector centre;
                NewtonBodyGetMatrix(body, &matrix[0][0]);
                NewtonBodyGetCentreOfMass(body, &centre[0]);
                centre = matrix.TransformVector(centre);
                if (Util::get_vector_magnitude(centre - blast_point) <= blast_radius) {
                    hit->m_body = nullptr;
                    NewtonWorldRayCast(world, &blast_point[0], &centre[0], ray_filter_callback, reinterpret_cast<void*>(hit), NULL, 0);
                    if (hit->m_body == body) {
                        dVector force(hit->m_point - blast_point);
                        dFloat dist = Util::get_vector_magnitude(force);
                        if (dist > M_EPSILON) {
                            Util::scale_vector(force, (blast_radius - dist) * blast_force * inv_blast_radius / dist);
                            MSP::Body::c_body_add_force(body_data, force);
                            dFloat moment = force.DotProduct3(hit->m_normal);
                            dVector torque((hit->m_point - centre).CrossProduct(hit->m_normal.Scale(-moment)));
                            MSP::Body::c_body_add_torque(body_data, torque);
                        }
                    }
                }
            }
        }
        delete hit;
        return Qtrue;
    }
    else
        return Qfalse;
}

VALUE MSP::World::rbf_get_aabb(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    if (NewtonWorldGetBodyCount(world) == 0)
        return Qnil;
    dVector world_min;
    dVector world_max;
    bool first_time = true;
    for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
        if(first_time) {
            NewtonBodyGetAABB(body, &world_min[0], &world_max[0]);
            first_time = false;
            continue;
        }
        dVector min;
        dVector max;
        NewtonBodyGetAABB(body, &min[0], &max[0]);
        for (int i = 0; i < 3; ++i) {
            if (min[i] < world_min[i]) world_min[i] = min[i];
            if (max[i] > world_max[i]) world_max[i] = max[i];
        }
    }
    return rb_ary_new3(2, Util::point_to_value(world_min), Util::point_to_value(world_max));
}

VALUE MSP::World::rbf_get_destructor_proc(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return rb_ary_entry(world_data->m_user_info, 0);
}

VALUE MSP::World::rbf_set_destructor_proc(VALUE self, VALUE v_world, VALUE v_proc) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    if (v_proc == Qnil || rb_class_of(v_proc) == rb_cProc)
        rb_ary_store(world_data->m_user_info, 0, v_proc);
    else
        rb_raise(rb_eTypeError, "Expected nil or a Proc object!");
    return Qnil;
}

VALUE MSP::World::rbf_get_user_data(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return rb_ary_entry(world_data->m_user_info, 1);
}

VALUE MSP::World::rbf_set_user_data(VALUE self, VALUE v_world, VALUE v_user_data) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    rb_ary_store(world_data->m_user_info, 1, v_user_data);
    return Qnil;
}

VALUE MSP::World::rbf_get_touch_data_at(VALUE self, VALUE v_world, VALUE v_index) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    unsigned int index = Util::value_to_uint(v_index);
    if (index >= world_data->m_touch_data.size())
        return Qnil;
    const BodyTouchData* touch_data = world_data->m_touch_data[index];
    VALUE v_touch_data = rb_ary_new2(6);
    rb_ary_store(v_touch_data, 0, MSP::Body::c_body_to_value(touch_data->m_body0));
    rb_ary_store(v_touch_data, 1, MSP::Body::c_body_to_value(touch_data->m_body1));
    rb_ary_store(v_touch_data, 2, Util::point_to_value(touch_data->m_point));
    rb_ary_store(v_touch_data, 3, Util::vector_to_value(touch_data->m_normal));
    rb_ary_store(v_touch_data, 4, Util::vector_to_value(touch_data->m_force.Scale(M_INCH_TO_METER)));
    rb_ary_store(v_touch_data, 5, Util::to_value(touch_data->m_speed * M_INCH_TO_METER));
    return v_touch_data;
}

VALUE MSP::World::rbf_get_touch_data_count(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return Util::to_value( (unsigned int)world_data->m_touch_data.size() );
}

VALUE MSP::World::rbf_get_touching_data_at(VALUE self, VALUE v_world, VALUE v_index) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    unsigned int index = Util::value_to_uint(v_index);
    if (index >= world_data->m_touching_data.size())
        return Qnil;
    const BodyTouchingData* touching_data = world_data->m_touching_data[index];
    VALUE v_touching_data = rb_ary_new2(2);
    rb_ary_store(v_touching_data, 0, MSP::Body::c_body_to_value(touching_data->m_body0));
    rb_ary_store(v_touching_data, 1, MSP::Body::c_body_to_value(touching_data->m_body1));
    return v_touching_data;
}

VALUE MSP::World::rbf_get_touching_data_count(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return Util::to_value( (unsigned int)world_data->m_touching_data.size() );
}

VALUE MSP::World::rbf_get_untouch_data_at(VALUE self, VALUE v_world, VALUE v_index) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    unsigned int index = Util::value_to_uint(v_index);
    if (index >= world_data->m_untouch_data.size()) return Qnil;
    const BodyUntouchData* untouch_data = world_data->m_untouch_data[index];
    VALUE v_untouch_data = rb_ary_new2(2);
    rb_ary_store(v_untouch_data, 0, MSP::Body::c_body_to_value(untouch_data->m_body0));
    rb_ary_store(v_untouch_data, 1, MSP::Body::c_body_to_value(untouch_data->m_body1));
    return v_untouch_data;
}

VALUE MSP::World::rbf_get_untouch_data_count(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return Util::to_value( (unsigned int)world_data->m_untouch_data.size() );
}

VALUE MSP::World::rbf_get_time(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return Util::to_value( world_data->m_time );
}

VALUE MSP::World::rbf_serialize_to_file(VALUE self, VALUE v_world, VALUE v_full_path) {
    const NewtonWorld* world = c_value_to_world(v_world);
    NewtonSerializeToFile(world, Util::value_to_c_str(v_full_path), NULL, NULL);
    return Qnil;
}

VALUE MSP::World::rbf_get_contact_merge_tolerance(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    return Util::to_value( NewtonGetContactMergeTolerance(world) );
}

VALUE MSP::World::rbf_set_contact_merge_tolerance(VALUE self, VALUE v_world, VALUE v_tolerance) {
    const NewtonWorld* world = c_value_to_world(v_world);
    dFloat tolerance = Util::value_to_dFloat(v_tolerance);
    NewtonSetContactMergeTolerance(world, tolerance);
    return Qnil;
}

VALUE MSP::World::rbf_get_default_material_id(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    WorldData* world_data = reinterpret_cast<WorldData*>(NewtonWorldGetUserData(world));
    return Util::to_value(world_data->m_material_id);
}

VALUE MSP::World::rbf_draw_collision_wireframe(VALUE self, VALUE v_world, VALUE v_view, VALUE v_bb, VALUE v_sleep_color, VALUE v_active_color, VALUE v_line_width, VALUE v_line_stipple) {
    const NewtonWorld* world = c_value_to_world(v_world);
    DrawData draw_data(v_view, v_bb);
    rb_funcall(v_view, Util::INTERN_SLINE_WIDTH, 1, v_line_width);
    rb_funcall(v_view, Util::INTERN_SLINE_STIPPLE, 1, v_line_stipple);
    unsigned int count = 0;
    dMatrix matrix;
    for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
        const NewtonCollision* collision = NewtonBodyGetCollision(body);
        NewtonBodyGetMatrix(body, &matrix[0][0]);
        //rb_funcall(v_bb, Util::INTERN_ADD, 1, Util::point_to_value(matrix.m_posit, world_data->inverse_scale));
        if (NewtonBodyGetSleepState(body) == 1)
            rb_funcall(v_view, Util::INTERN_SDRAWING_COLOR, 1, v_sleep_color);
        else
            rb_funcall(v_view, Util::INTERN_SDRAWING_COLOR, 1, v_active_color);
        NewtonCollisionForEachPolygonDo(collision, &matrix[0][0], draw_collision_iterator, reinterpret_cast<void*>(&draw_data));
        ++count;
    }
    return Util::to_value(count);
}

VALUE MSP::World::rbf_clear_matrix_change_record(VALUE self, VALUE v_world) {
    const NewtonWorld* world = c_value_to_world(v_world);
    c_clear_matrix_change_record(world);
    return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::World::init_ruby(VALUE mNewton) {
    VALUE mWorld = rb_define_module_under(mNewton, "World");

    rb_define_module_function(mWorld, "is_valid?", VALUEFUNC(MSP::World::rbf_is_valid), 1);
    rb_define_module_function(mWorld, "create", VALUEFUNC(MSP::World::rbf_create), 0);
    rb_define_module_function(mWorld, "destroy", VALUEFUNC(MSP::World::rbf_destroy), 1);
    rb_define_module_function(mWorld, "get_max_possible_threads_count", VALUEFUNC(MSP::World::rbf_get_max_possible_threads_count), 1);
    rb_define_module_function(mWorld, "get_max_threads_count", VALUEFUNC(MSP::World::rbf_get_max_threads_count), 1);
    rb_define_module_function(mWorld, "set_max_threads_count", VALUEFUNC(MSP::World::rbf_set_max_threads_count), 2);
    rb_define_module_function(mWorld, "get_cur_threads_count", VALUEFUNC(MSP::World::rbf_get_cur_threads_count), 1);
    rb_define_module_function(mWorld, "destroy_all_bodies", VALUEFUNC(MSP::World::rbf_destroy_all_bodies), 1);
    rb_define_module_function(mWorld, "get_body_count", VALUEFUNC(MSP::World::rbf_get_body_count), 1);
    rb_define_module_function(mWorld, "get_constraint_count", VALUEFUNC(MSP::World::rbf_get_constraint_count), 1);
    rb_define_module_function(mWorld, "update", VALUEFUNC(MSP::World::rbf_update), 2);
    rb_define_module_function(mWorld, "update_async", VALUEFUNC(MSP::World::rbf_update_async), 2);
    rb_define_module_function(mWorld, "get_gravity", VALUEFUNC(MSP::World::rbf_get_gravity), 1);
    rb_define_module_function(mWorld, "set_gravity", VALUEFUNC(MSP::World::rbf_set_gravity), 2);
    rb_define_module_function(mWorld, "get_bodies", VALUEFUNC(MSP::World::rbf_get_bodies), 1);
    rb_define_module_function(mWorld, "get_joints", VALUEFUNC(MSP::World::rbf_get_joints), 1);
    rb_define_module_function(mWorld, "get_gears", VALUEFUNC(MSP::World::rbf_get_gears), 1);
    rb_define_module_function(mWorld, "get_bodies_in_aabb", VALUEFUNC(MSP::World::rbf_get_bodies_in_aabb), 3);
    rb_define_module_function(mWorld, "get_first_body", VALUEFUNC(MSP::World::rbf_get_first_body), 1);
    rb_define_module_function(mWorld, "get_next_body", VALUEFUNC(MSP::World::rbf_get_next_body), 2);
    rb_define_module_function(mWorld, "get_solver_model", VALUEFUNC(MSP::World::rbf_get_solver_model), 1);
    rb_define_module_function(mWorld, "set_solver_model", VALUEFUNC(MSP::World::rbf_set_solver_model), 2);
    rb_define_module_function(mWorld, "get_material_thickness", VALUEFUNC(MSP::World::rbf_get_material_thickness), 1);
    rb_define_module_function(mWorld, "set_material_thickness", VALUEFUNC(MSP::World::rbf_set_material_thickness), 2);
    rb_define_module_function(mWorld, "ray_cast", VALUEFUNC(MSP::World::rbf_ray_cast), 3);
    rb_define_module_function(mWorld, "continuous_ray_cast", VALUEFUNC(MSP::World::rbf_continuous_ray_cast), 3);
    rb_define_module_function(mWorld, "convex_ray_cast", VALUEFUNC(MSP::World::rbf_convex_ray_cast), 4);
    rb_define_module_function(mWorld, "continuous_convex_ray_cast", VALUEFUNC(MSP::World::rbf_continuous_convex_ray_cast), 5);
    rb_define_module_function(mWorld, "add_explosion", VALUEFUNC(MSP::World::rbf_add_explosion), 4);
    rb_define_module_function(mWorld, "get_aabb", VALUEFUNC(MSP::World::rbf_get_aabb), 1);
    rb_define_module_function(mWorld, "get_destructor_proc", VALUEFUNC(MSP::World::rbf_get_destructor_proc), 1);
    rb_define_module_function(mWorld, "set_destructor_proc", VALUEFUNC(MSP::World::rbf_set_destructor_proc), 2);
    rb_define_module_function(mWorld, "get_user_data", VALUEFUNC(MSP::World::rbf_get_user_data), 1);
    rb_define_module_function(mWorld, "set_user_data", VALUEFUNC(MSP::World::rbf_set_user_data), 2);
    rb_define_module_function(mWorld, "get_touch_data_at", VALUEFUNC(MSP::World::rbf_get_touch_data_at), 2);
    rb_define_module_function(mWorld, "get_touch_data_count", VALUEFUNC(MSP::World::rbf_get_touch_data_count), 1);
    rb_define_module_function(mWorld, "get_touching_data_at", VALUEFUNC(MSP::World::rbf_get_touching_data_at), 2);
    rb_define_module_function(mWorld, "get_touching_data_count", VALUEFUNC(MSP::World::rbf_get_touching_data_count), 1);
    rb_define_module_function(mWorld, "get_untouch_data_at", VALUEFUNC(MSP::World::rbf_get_untouch_data_at), 2);
    rb_define_module_function(mWorld, "get_untouch_data_count", VALUEFUNC(MSP::World::rbf_get_untouch_data_count), 1);
    rb_define_module_function(mWorld, "get_time", VALUEFUNC(MSP::World::rbf_get_time), 1);
    rb_define_module_function(mWorld, "serialize_to_file", VALUEFUNC(MSP::World::rbf_serialize_to_file), 2);
    rb_define_module_function(mWorld, "get_contact_merge_tolerance", VALUEFUNC(MSP::World::rbf_get_contact_merge_tolerance), 1);
    rb_define_module_function(mWorld, "set_contact_merge_tolerance", VALUEFUNC(MSP::World::rbf_set_contact_merge_tolerance), 2);
    rb_define_module_function(mWorld, "get_default_material_id", VALUEFUNC(MSP::World::rbf_get_default_material_id), 1);
    rb_define_module_function(mWorld, "draw_collision_wireframe", VALUEFUNC(MSP::World::rbf_draw_collision_wireframe), 7);
    rb_define_module_function(mWorld, "clear_matrix_change_record", VALUEFUNC(MSP::World::rbf_clear_matrix_change_record), 1);
}
