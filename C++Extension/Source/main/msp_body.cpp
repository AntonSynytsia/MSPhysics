/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "pch.h"
#include "msp_body.h"
#include "msp_collision.h"
#include "msp_world.h"
#include "msp_joint.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Body::DEFAULT_STATIC_FRICTION_COEF(0.90f);
const dFloat MSP::Body::DEFAULT_KINETIC_FRICTION_COEF(0.50f);
const dFloat MSP::Body::DEFAULT_DENSITY(700.0);
const dFloat MSP::Body::DEFAULT_ELASTICITY(0.40f);
const dFloat MSP::Body::DEFAULT_SOFTNESS(0.10f);
const dVector MSP::Body::DEFAULT_LINEAR_DAMPING(0.01f);
const dVector MSP::Body::DEFAULT_ANGULAR_DAMPING(0.01f);
const bool MSP::Body::DEFAULT_LINEAR_DAMPING_ENABLED(true);
const bool MSP::Body::DEFAULT_ANGULAR_DAMPING_ENABLED(true);
const bool MSP::Body::DEFAULT_FRICTION_ENABLED(true);
const bool MSP::Body::DEFAULT_AUTO_SLEEP_ENABLED(true);
const bool MSP::Body::DEFAULT_COLLIDABLE(true);
const bool MSP::Body::DEFAULT_MAGNETIC(false);
const bool MSP::Body::DEFAULT_GRAVITY_ENABLED(true);
const dFloat MSP::Body::MIN_MASS(1.0e-6f);
const dFloat MSP::Body::MAX_MASS(1.0e14f);
const dFloat MSP::Body::MIN_VOLUME(1.0e-6f);
const dFloat MSP::Body::MAX_VOLUME(1.0e14f);
const dFloat MSP::Body::MIN_DENSITY(1.0e-6f);
const dFloat MSP::Body::MAX_DENSITY(1.0e14f);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

std::set<const NewtonBody*> MSP::Body::s_valid_bodies;


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Body::destructor_callback(const NewtonBody* const body) {
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(world));
    c_clear_non_collidable_bodies(body);
    if (s_valid_bodies.find(body) != s_valid_bodies.end())
        s_valid_bodies.erase(body);
    if (body_data->m_group != Qnil && world_data->m_group_to_body_map.find(body_data->m_group) != world_data->m_group_to_body_map.end())
        world_data->m_group_to_body_map.erase(body_data->m_group);
    if (body_data->m_destructor_proc != Qnil)
        rb_rescue2(RUBY_METHOD_FUNC(Util::call_proc), body_data->m_destructor_proc, RUBY_METHOD_FUNC(Util::rescue_proc), Qnil, rb_eException, (VALUE)0);
    VALUE v_body = c_body_to_value(body);
    rb_hash_delete(world_data->m_body_destructors, v_body);
    rb_hash_delete(world_data->m_body_user_datas, v_body);
    rb_hash_delete(world_data->m_body_groups, v_body);
    delete body_data;
}

void MSP::Body::transform_callback(const NewtonBody* const body, const dFloat* const matrix, int thread_index) {
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_matrix_changed = true;
}

void MSP::Body::force_and_torque_callback(const NewtonBody* const body, dFloat timestep, int thread_index) {
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(world));
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));

    dMatrix transformation_matrix;
    dFloat mass;
    dVector inertia;
    NewtonBodyGetMass(body, &mass, &inertia.m_x, &inertia.m_y, &inertia.m_z);
    NewtonBodyGetMatrix(body, &transformation_matrix[0][0]);

    // Gravity
    if (body_data->m_gravity_enabled) {
        dVector force(world_data->m_gravity.Scale(mass));
        NewtonBodyAddForce(body, &force[0]);
    }
    // Linear damping
    if (body_data->m_linear_damping_enabled) {
        dVector velocity;
        NewtonBodyGetVelocity(body, &velocity[0]);
        velocity = transformation_matrix.UnrotateVector(velocity);

        dVector viscous_force(
            -velocity.m_x * body_data->m_linear_damping.m_x * mass,
            -velocity.m_y * body_data->m_linear_damping.m_y * mass,
            -velocity.m_z * body_data->m_linear_damping.m_z * mass);
        viscous_force = transformation_matrix.RotateVector(viscous_force);
        NewtonBodyAddForce(body, &viscous_force[0]);
    }
    // Angular damping
    if (body_data->m_angular_damping_enabled) {
        dVector omega;
        NewtonBodyGetOmega(body, &omega[0]);
        omega = transformation_matrix.UnrotateVector(omega);
        dVector viscous_torque(
            -omega.m_x * body_data->m_angular_damping.m_x * inertia.m_x,
            -omega.m_y * body_data->m_angular_damping.m_y * inertia.m_y,
            -omega.m_z * body_data->m_angular_damping.m_z * inertia.m_z);
        viscous_torque = transformation_matrix.RotateVector(viscous_torque);
        NewtonBodyAddTorque(body, &viscous_torque[0]);
    }
    // Other forces
    if (body_data->m_set_force_state) {
        NewtonBodySetForce(body, &body_data->m_set_force[0]);
        body_data->m_set_force_state = false;
    }
    if (body_data->m_add_force_state) {
        NewtonBodyAddForce(body, &body_data->m_add_force[0]);
        body_data->m_add_force_state = false;
    }
    if (body_data->m_set_torque_state) {
        NewtonBodySetTorque(body, &body_data->m_set_torque[0]);
        body_data->m_set_torque_state = false;
    }
    if (body_data->m_add_torque_state) {
        NewtonBodyAddTorque(body, &body_data->m_add_torque[0]);
        body_data->m_add_torque_state = false;
    }
}

void MSP::Body::collision_iterator(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id) {
    CollisionIteratorData* data = reinterpret_cast<CollisionIteratorData*>(user_data);
    VALUE v_face = rb_ary_new2(vertex_count);
    unsigned int j = 0;
    for (int i = 0; i < vertex_count; ++i) {
        dVector vertex(face_array[j], face_array[j + 1], face_array[j + 2]);
        rb_ary_store(v_face, i, Util::point_to_value(vertex));
        j += 3;
    }
    rb_ary_push(data->m_faces, v_face);
}

void MSP::Body::collision_iterator2(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id) {
    CollisionIteratorData* data = reinterpret_cast<CollisionIteratorData*>(user_data);
    // Store face vertices
    VALUE v_face = rb_ary_new2(vertex_count);
    unsigned int j = 0;
    for (int i = 0; i < vertex_count; ++i) {
        dVector vertex(face_array[j], face_array[j + 1], face_array[j + 2]);
        rb_ary_store(v_face, i, Util::point_to_value(vertex));
        j += 3;
    }
    // Calculate face normal
    dVector p1(face_array[0], face_array[1], face_array[2]);
    dVector p2(face_array[3], face_array[4], face_array[5]);
    dVector p3(face_array[6], face_array[7], face_array[8]);
    dVector u(p2 - p1);
    dVector v(p3 - p1);
    dVector n(u.CrossProduct(v));
    Util::normalize_vector(n);
    VALUE v_face_normal = Util::vector_to_value(n);
    // Transform points with respect to face normal
    dVector* vertices = new dVector[vertex_count];
    dMatrix face_matrix;
    Util::matrix_from_pin_dir(Util::ORIGIN, n, face_matrix);
    j = 0;
    for (int i = 0; i < vertex_count; ++i) {
        dVector vertex(face_array[j], face_array[j + 1], face_array[j + 2]);
        vertices[i] = face_matrix.UntransformVector(vertex);
        j += 3;
    }
    // Calculate face area and centroid
    dVector centroid(0.0, 0.0, vertices[0].m_z);
    dFloat signed_area = 0.0;
    for (int i = 0; i < vertex_count; ++i) {
        const dFloat& x0 = vertices[i].m_x;
        const dFloat& y0 = vertices[i].m_y;
        const dFloat& x1 = vertices[(i+1) % vertex_count].m_x;
        const dFloat& y1 = vertices[(i+1) % vertex_count].m_y;
        dFloat a = x0*y1 - x1*y0;
        signed_area += a;
        centroid.m_x += (x0 + x1)*a;
        centroid.m_y += (y0 + y1)*a;
    }
    signed_area *= 0.5f;
    dFloat scale = 1.0 / (6.0 * signed_area);
    centroid.m_x *= scale;
    centroid.m_y *= scale;
    VALUE v_face_centroid = Util::point_to_value(face_matrix.TransformVector(centroid));
    VALUE v_face_area = Util::to_value(dAbs(signed_area));
    // Store face data
    rb_ary_push(data->m_faces, rb_ary_new3(4, v_face, v_face_centroid, v_face_normal, v_face_area));
    // Dispose of allocated space
    delete[] vertices;
}

void MSP::Body::collision_iterator3(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id) {
    CollisionIteratorData* data = reinterpret_cast<CollisionIteratorData*>(user_data);
    // Calculate face normal
    dVector p1(face_array[0], face_array[1], face_array[2]);
    dVector p2(face_array[3], face_array[4], face_array[5]);
    dVector p3(face_array[6], face_array[7], face_array[8]);
    dVector u(p2 - p1);
    dVector v(p3 - p1);
    dVector n(u.CrossProduct(v));
    Util::normalize_vector(n);
    VALUE v_face_normal = Util::vector_to_value(n);
    // Transform points with respect to face normal
    dVector* vertices = new dVector[vertex_count];
    dMatrix face_matrix;
    Util::matrix_from_pin_dir(Util::ORIGIN, n, face_matrix);
    unsigned int j = 0;
    for (int i = 0; i < vertex_count; ++i) {
        dVector vertex(face_array[j], face_array[j + 1], face_array[j + 2]);
        vertices[i] = face_matrix.UntransformVector(vertex);
        j += 3;
    }
    // Calculate face area and centroid
    dVector centroid(0.0, 0.0, vertices[0].m_z);
    dFloat signed_area = 0.0;
    for (int i = 0; i < vertex_count; ++i) {
        const dFloat& x0 = vertices[i].m_x;
        const dFloat& y0 = vertices[i].m_y;
        const dFloat& x1 = vertices[(i+1) % vertex_count].m_x;
        const dFloat& y1 = vertices[(i+1) % vertex_count].m_y;
        dFloat a = x0*y1 - x1*y0;
        signed_area += a;
        centroid.m_x += (x0 + x1)*a;
        centroid.m_y += (y0 + y1)*a;
    }
    signed_area *= 0.5f;
    dFloat scale = 1.0 / (6.0 * signed_area);
    centroid.m_x *= scale;
    centroid.m_y *= scale;
    VALUE v_face_centroid = Util::point_to_value(face_matrix.TransformVector(centroid));
    VALUE v_face_area = Util::to_value(dAbs(signed_area));
    // Store face data
    rb_ary_push(data->m_faces, rb_ary_new3(3, v_face_centroid, v_face_normal, v_face_area));
    // Dispose of allocated space
    delete[] vertices;
}

void MSP::Body::collision_iterator4(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id) {
    CollisionIteratorData2* ci_data = reinterpret_cast<CollisionIteratorData2*>(user_data);
    // Convert face array into points
    std::map<unsigned int, dVector> vertices;
    unsigned int j = 0;
    for (int i = 0; i < vertex_count; ++i) {
        vertices[i] = dVector(face_array[j], face_array[j + 1], face_array[j + 2]);
        j += 3;
    }
    // Calculate face normal
    dVector normal((vertices[1] - vertices[0]).CrossProduct(vertices[2] - vertices[0]));
    dFloat normal_mag = Util::get_vector_magnitude(normal);
    if (normal_mag < 1.0e-6f) return;
    Util::scale_vector(normal, 1.0 / normal_mag);
    // Face matrix from normal
    dMatrix face_matrix;
    Util::matrix_from_pin_dir(vertices[0], normal, face_matrix);
    // Convert face array into points
    std::map<unsigned int, dVector> local_vertices;
    for (std::map<unsigned int, dVector>::const_iterator it = vertices.begin(); it != vertices.end(); ++it) {
        local_vertices[it->first] = face_matrix.UntransformVector(it->second);
    }
    // Calculate relative velocity
    dFloat lv = ci_data->m_velocity.DotProduct3(normal);
    //dFloat lo = data->omega.DotProduct3(normal);
    // For every triangle
    for (int i = 1; i < vertex_count - 1; ++i) {
        // Calculate face area and centroid
        const dVector& p0 = local_vertices[0];
        const dVector& p1 = local_vertices[i];
        const dVector& p2 = local_vertices[i+1];
        dFloat a = p0.m_x*p1.m_y - p1.m_x*p0.m_y;
        dFloat b = p1.m_x*p2.m_y - p2.m_x*p1.m_y;
        dFloat c = p2.m_x*p0.m_y - p0.m_x*p2.m_y;
        dFloat signed_area = (a + b + c) * 0.5f;
        dVector centroid((p0.m_x + p1.m_x)*a + (p1.m_x + p2.m_x)*b + (p2.m_x + p0.m_x)*c, (p0.m_y + p1.m_y)*a + (p1.m_y + p2.m_y)*b + (p2.m_y + p0.m_y)*c, vertices[0].m_z);
        dFloat cscale = 1.0 / (6.0 * signed_area);
        centroid.m_x *= cscale;
        centroid.m_y *= cscale;
        centroid = face_matrix.TransformVector(centroid);
        // Apply force and torque from linear velocity
        if (lv > 0.0) {
            // Calculate the necessary force on face
            dVector point_force(normal.Scale(-dAbs(signed_area) * lv * lv * ci_data->m_drag));
            // Add point force
            ci_data->m_force += point_force;
            dVector r(centroid - ci_data->m_centre);
            ci_data->m_torque += r.CrossProduct(point_force);
        }
        // Apply force and torque from angular velocity
        /*if (dAbs(lo) < 0.9999995f) {
            dFloat qmag = Util::get_vector_magnitude(data->omega);
            if (qmag > 1.0e-6f) {
                dVector dir(centroid - data->centre);
                dFloat dir_mag = Util::get_vector_magnitude(dir);
                dVector xaxis(data->omega);
                Util::scale_vector(xaxis, 1.0 / qmag);
                dVector yaxis;
                dVector zaxis;
                if (dir_mag < 1.0e-6f) {
                    yaxis = normal;
                    zaxis = xaxis.CrossProduct(yaxis);
                    Util::normalize_vector(zaxis);
                }
                else {
                    zaxis = dir.CrossProduct(xaxis);
                    Util::normalize_vector(zaxis);
                    yaxis = xaxis.CrossProduct(zaxis);
                    Util::normalize_vector(yaxis);
                }
                dMatrix omega_matrix(xaxis, yaxis, zaxis, centroid);
                dVector q1(omega_matrix.UntransformVector(vertices[0]));
                dVector q2(omega_matrix.UntransformVector(vertices[i]));
                dVector q3(omega_matrix.UntransformVector(vertices[i+1]));
                dVector qnormal(omega_matrix.UnrotateVector(normal));
                // Calculate the necessary force on face
                dFloat point_force_mag = -qnormal.m_z * qmag * qmag / 6.0 *
                (q1.m_x * (q3.m_y*q3.m_y + q1.m_y*q3.m_y - q1.m_y*q2.m_y - q2.m_y*q2.m_y) +
                 q2.m_x * (q1.m_y*q1.m_y + q2.m_y*q1.m_y - q2.m_y*q3.m_y - q3.m_y*q3.m_y) +
                 q3.m_x * (q2.m_y*q2.m_y + q3.m_y*q2.m_y - q3.m_y*q1.m_y - q1.m_y*q1.m_y));
                dVector point_force(normal.Scale(point_force_mag));
                // Add point force
                data->force += point_force;
                dVector r(centroid - data->centre);
                data->torque += r.CrossProduct(point_force);
            }
        }*/
    }
    // Apply force and torque from linear velocity
    /*if (lv > 0.0) {
        // Transform points with respect to face normal
        std::map<unsigned int, dVector> vertices;
        dMatrix face_matrix;
        Util::matrix_from_pin_dir(Util::ORIGIN, normal, face_matrix);
        for (int i = 0; i < vertex_count * 3; i += 3) {
            dVector vertex(face_matrix.UntransformVector(dVector(face_array[i], face_array[i+1], face_array[i+2])));
            vertices[i] = vertex;
        }
        // Calculate face area and centroid
        dVector centroid(0.0, 0.0, vertices[0].m_z);
        dFloat signed_area = 0.0;
        for (unsigned int i = 0; i < vertex_count; ++i) {
            const dFloat& x0 = vertices[i].m_x;
            const dFloat& y0 = vertices[i].m_y;
            const dFloat& x1 = vertices[(i+1) % vertex_count].m_x;
            const dFloat& y1 = vertices[(i+1) % vertex_count].m_y;
            dFloat a = x0*y1 - x1*y0;
            signed_area += a;
            centroid.m_x += (x0 + x1)*a;
            centroid.m_y += (y0 + y1)*a;
        }
        signed_area *= 0.5f;
        dFloat scale = 1.0 / (6.0 * signed_area);
        centroid.m_x *= scale;
        centroid.m_y *= scale;
        centroid = face_matrix.TransformVector(centroid);
        // Calculate the necessary force on face
        dVector point_force(normal.Scale(-signed_area * lv * lv * data->drag));
        // Add point force
        data->force += point_force;
        dVector r(centroid - data->centre);
        data->torque += r.CrossProduct(point_force);
    }*/
    // Calculate face normal
    /*dVector p1(face_array[0], face_array[1], face_array[2]);
    dVector p2(face_array[3], face_array[4], face_array[5]);
    dVector p3(face_array[6], face_array[7], face_array[8]);
    dVector normal((p2 - p1).CrossProduct(p3 - p1));
    dFloat normal_mag = Util::get_vector_magnitude(normal);
    if (normal_mag < M_EPSILON) return;
    Util::normalize_vector(normal);
    // Transform points with respect to face normal
    std::vector<dVector> vertices;
    dMatrix face_matrix;
    Util::matrix_from_pin_dir(Util::ORIGIN, normal, face_matrix);
    for (int i = 0; i < vertex_count; ++i) {
        dVector vertex(face_matrix.UntransformVector(dVector(face_array[i*3+0], face_array[i*3+1], face_array[i*3+2])));
        vertices.push_back(vertex);
    }
    // Calculate face area and centroid
    dVector centroid(0.0, 0.0, vertices[0].m_z);
    dFloat signed_area = 0.0;
    for (int i = 0; i < vertex_count; ++i) {
        const dFloat& x0 = vertices[i].m_x;
        const dFloat& y0 = vertices[i].m_y;
        const dFloat& x1 = vertices[(i+1) % vertex_count].m_x;
        const dFloat& y1 = vertices[(i+1) % vertex_count].m_y;
        dFloat a = x0*y1 - x1*y0;
        signed_area += a;
        centroid.m_x += (x0 + x1)*a;
        centroid.m_y += (y0 + y1)*a;
    }
    signed_area *= 0.5f;
    dFloat scale = 1.0 / (6.0 * signed_area);
    centroid.m_x *= scale;
    centroid.m_y *= scale;
    centroid = face_matrix.TransformVector(centroid);
    // Calculate point force
    dVector point_veloc;
    NewtonBodyGetPointVelocity(data->body, &centroid[0], &point_veloc[0]);
    if (!Util::is_vector_valid(point_veloc)) return;
    dFloat veloc_mag = Util::get_vector_magnitude(point_veloc);
    if (veloc_mag < M_EPSILON) return;
    dFloat cos_theta = normal.DotProduct3(point_veloc) / veloc_mag;
    if (cos_theta < M_EPSILON) return;
    Util::scale_vector(point_veloc, 1.0 / veloc_mag);
    dMatrix veloc_matrix;
    Util::matrix_from_pin_dir(Util::ORIGIN, point_veloc, veloc_matrix);
    dVector loc_normal(veloc_matrix.UnrotateVector(normal));
    //if (loc_normal.m_z < M_EPSILON) return;
    dFloat quantity = -dAbs(signed_area) * veloc_mag * veloc_mag * data->drag;
    dVector point_force(loc_normal.Scale(quantity));
    point_force = veloc_matrix.RotateVector(point_force);
    //dVector loc_point_force(0.0, 0.0, -loc_normal.m_z * dAbs(signed_area) * data->density * veloc_mag);
    //dVector point_force(veloc_matrix.RotateVector(loc_point_force));
    // Add force and torque
    data->force += point_force;
    dVector r(centroid - data->centre);
    data->torque += r.CrossProduct(point_force);
    // Add additional rotational force.
    //if (1.0 - cos_theta < M_EPSILON) return;
    //dVector side_force(normal.CrossProduct(point_veloc));
    //data->torque += r.CrossProduct(side_force.Scale(dAbs(signed_area) * veloc_mag * data->density));*/
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

bool MSP::Body::c_is_body_valid(const NewtonBody* address) {
    return s_valid_bodies.find(address) != s_valid_bodies.end();
}

VALUE MSP::Body::c_body_to_value(const NewtonBody* body) {
    return rb_ull2inum(reinterpret_cast<unsigned long long>(body));
}

const NewtonBody* MSP::Body::c_value_to_body(VALUE v_body) {
    const NewtonBody* address = reinterpret_cast<NewtonBody*>(rb_num2ull(v_body));
    if (Util::s_validate_objects && s_valid_bodies.find(address) == s_valid_bodies.end())
        rb_raise(rb_eTypeError, "Given address doesn't reference a valid body!");
    return address;
}

bool MSP::Body::c_bodies_collidable(const NewtonBody* body0, const NewtonBody* body1) {
    BodyData* data0 = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body0));
    BodyData* data1 = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body1));
    return (data0->m_collidable && data1->m_collidable && data0->m_non_collidable_bodies.find(body1) == data0->m_non_collidable_bodies.end());
}

bool MSP::Body::c_bodies_aabb_overlap(const NewtonBody* body0, const NewtonBody* body1) {
    dVector minA;
    dVector maxA;
    dVector minB;
    dVector maxB;
    NewtonBodyGetAABB(body0, &minA[0], &maxA[0]);
    NewtonBodyGetAABB(body1, &minB[0], &maxB[0]);
    for (int i = 0; i < 3; ++i) {
        if ((minA[i] >= minB[i] && minA[i] <= maxB[i]) ||
            (maxA[i] >= minB[i] && maxA[i] <= maxB[i]) ||
            (minB[i] >= minA[i] && minB[i] <= maxA[i]) ||
            (maxB[i] >= minA[i] && maxB[i] <= maxA[i])) continue;
        return false;
    }
    return true;
}

void MSP::Body::c_validate_two_bodies(const NewtonBody* body1, const NewtonBody* body2) {
    const NewtonWorld* world1 = NewtonBodyGetWorld(body1);
    const NewtonWorld* world2 = NewtonBodyGetWorld(body2);
    if (body1 == body2)
        rb_raise(rb_eTypeError, "Expected two unique bodies!");
    if (world1 != world2)
        rb_raise(rb_eTypeError, "Expected two bodies from a same world!");
}

void MSP::Body::c_clear_non_collidable_bodies(const NewtonBody* body) {
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    for (std::map<const NewtonBody*, bool>::iterator it = body_data->m_non_collidable_bodies.begin(); it != body_data->m_non_collidable_bodies.end(); ++it) {
        const NewtonBody* other_body = it->first;
        if (s_valid_bodies.find(other_body) != s_valid_bodies.end()) {
            BodyData* other_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(other_body));
            if (other_data->m_non_collidable_bodies.find(body) != other_data->m_non_collidable_bodies.end())
                other_data->m_non_collidable_bodies.erase(body);
        }
    }
    body_data->m_non_collidable_bodies.clear();
}

void MSP::Body::c_body_add_force(BodyData* body_data, const dVector& force) {
    if (body_data->m_add_force_state)
        body_data->m_add_force += force;
    else {
        body_data->m_add_force = force;
        body_data->m_add_force_state = true;
    }
}

void MSP::Body::c_body_set_force(BodyData* body_data, const dVector& force) {
    body_data->m_add_force_state = false;
    if (body_data->m_set_force_state)
        body_data->m_set_force += force;
    else {
        body_data->m_set_force = force;
        body_data->m_set_force_state = true;
    }
}

void MSP::Body::c_body_add_torque(BodyData* body_data, const dVector& torque) {
    if (body_data->m_add_torque_state)
        body_data->m_add_torque += torque;
    else {
        body_data->m_add_torque = torque;
        body_data->m_add_torque_state = true;
    }
}

void MSP::Body::c_body_set_torque(BodyData* body_data, const dVector& torque) {
    body_data->m_add_torque_state = false;
    if (body_data->m_set_torque_state)
        body_data->m_set_torque += torque;
    else {
        body_data->m_set_torque = torque;
        body_data->m_set_torque_state = true;
    }
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Body::rbf_is_valid(VALUE self, VALUE v_body) {
    return c_is_body_valid(reinterpret_cast<NewtonBody*>(Util::value_to_ull(v_body))) ? Qtrue : Qfalse;
}

VALUE MSP::Body::rbf_create(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_type, VALUE v_id, VALUE v_group) {
    const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
    MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(world));
    const NewtonCollision* collision = MSP::Collision::c_value_to_collision(v_collision);
    int type = Util::value_to_int(v_type);
    int id = Util::value_to_int(v_id);

    dMatrix col_matrix;
    NewtonCollisionGetMatrix(collision, &col_matrix[0][0]);

    dMatrix matrix(Util::value_to_matrix(v_matrix));
    dVector scale(Util::get_matrix_scale(matrix));

    if (Util::is_matrix_flipped(matrix)) {
        matrix.m_front.m_x = -matrix.m_front.m_x;
        matrix.m_front.m_y = -matrix.m_front.m_y;
        matrix.m_front.m_z = -matrix.m_front.m_z;
        scale.m_x = -scale.m_x;
    }
    Util::extract_matrix_scale(matrix);

    NewtonBody* body;
    if (type == 1)
        body = NewtonCreateKinematicBody(world, collision, &matrix[0][0]);
    else
        body = NewtonCreateDynamicBody(world, collision, &matrix[0][0]);

    BodyData* body_data = new BodyData(scale, MSP::Collision::s_valid_collisions[collision]->m_scale, col_matrix.m_posit, id, v_group);

    int collision_type = NewtonCollisionGetType(collision);
    if (collision_type == SERIALIZE_ID_NULL)
        body_data->m_volume = 1.0;
    else if (collision_type < SERIALIZE_ID_TREE)
        body_data->m_volume = NewtonConvexCollisionCalculateVolume(collision);
    else
        body_data->m_volume = 0.0;

    if (body_data->m_volume < MIN_VOLUME) {
        body_data->m_dynamic = false;
        body_data->m_bstatic = true;
        body_data->m_volume = 0.0;
        body_data->m_density = 0.0;
        body_data->m_mass = 0.0;
    }
    else {
        body_data->m_dynamic = true;
        body_data->m_bstatic = false;
        body_data->m_volume = Util::clamp_dFloat(body_data->m_volume, MIN_VOLUME, MAX_VOLUME);
        body_data->m_mass = Util::clamp_dFloat(body_data->m_volume * body_data->m_density, MIN_MASS, MAX_MASS);
    }

    NewtonBodySetMassProperties(body, body_data->m_mass, collision);
    NewtonBodySetForceAndTorqueCallback(body, force_and_torque_callback);
    NewtonBodySetDestructorCallback(body, destructor_callback);
    NewtonBodySetTransformCallback(body, transform_callback);
    NewtonBodySetMaterialGroupID(body, body_data->m_material_id);
    NewtonBodySetCollidable(body, body_data->m_collidable ? 1 : 0);
    NewtonBodySetAutoSleep(body, body_data->m_auto_sleep_enabled ? 1 : 0);
    NewtonBodySetUserData(body, body_data);
    NewtonBodySetLinearDamping(body, 0.0);
    dVector damp(0.0);
    NewtonBodySetAngularDamping(body, &damp[0]);

    s_valid_bodies.insert(body);

    VALUE v_body = c_body_to_value(body);

    if (v_group != Qnil) {
        world_data->m_group_to_body_map[v_group] = body;
        rb_hash_aset(world_data->m_body_groups, v_body, v_group);
    }
    return v_body;
}

VALUE MSP::Body::rbf_destroy(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    NewtonDestroyBody(body);
    return Qnil;
}

VALUE MSP::Body::rbf_get_type(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    return Util::to_value( NewtonBodyGetType(body) );
}

VALUE MSP::Body::rbf_get_world(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    return MSP::World::c_world_to_value( NewtonBodyGetWorld(body) );
}

VALUE MSP::Body::rbf_get_collision(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    return MSP::Collision::c_collision_to_value( NewtonBodyGetCollision(body) );
}

VALUE MSP::Body::rbf_get_simulation_state(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    return NewtonBodyGetSimulationState(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSP::Body::rbf_set_simulation_state(VALUE self, VALUE v_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    NewtonBodySetSimulationState(body, RTEST(v_state) ? 1 : 0);
    return NewtonBodyGetSimulationState(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSP::Body::rbf_get_continuous_collision_state(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    return NewtonBodyGetContinuousCollisionMode(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSP::Body::rbf_set_continuous_collision_state(VALUE self, VALUE v_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    NewtonBodySetContinuousCollisionMode(body, Util::value_to_bool(v_state) ? 1 : 0);
    return Qnil;
}

VALUE MSP::Body::rbf_get_matrix(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    dMatrix matrix;
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    const dVector& dcs = body_data->m_default_collision_scale;
    const dVector& ms = body_data->m_matrix_scale;
    const dVector& cs = MSP::Collision::s_valid_collisions[collision]->m_scale;
    dVector actual_matrix_scale(ms.m_x * cs.m_x / dcs.m_x, ms.m_y * cs.m_y / dcs.m_y, ms.m_z * cs.m_z / dcs.m_z);
    Util::set_matrix_scale(matrix, actual_matrix_scale);
    return Util::matrix_to_value(matrix);
}

VALUE MSP::Body::rbf_get_normal_matrix(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    dMatrix matrix;
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    if (body_data->m_matrix_scale.m_x < 0) {
        matrix.m_front.m_x = -matrix.m_front.m_x;
        matrix.m_front.m_y = -matrix.m_front.m_y;
        matrix.m_front.m_z = -matrix.m_front.m_z;
    }
    return Util::matrix_to_value(matrix);
}

VALUE MSP::Body::rbf_set_matrix(VALUE self, VALUE v_body, VALUE v_matrix) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    dMatrix matrix(Util::value_to_matrix(v_matrix));
    if (Util::is_matrix_flipped(matrix)) {
        matrix.m_front.m_x = -matrix.m_front.m_x;
        matrix.m_front.m_y = -matrix.m_front.m_y;
        matrix.m_front.m_z = -matrix.m_front.m_z;
    }
    Util::extract_matrix_scale(matrix);
    NewtonBodySetMatrix(body, &matrix[0][0]);
    const dVector& dcs = body_data->m_default_collision_scale;
    const dVector& ms = body_data->m_matrix_scale;
    const dVector& cs = MSP::Collision::s_valid_collisions[collision]->m_scale;
    dVector actual_matrix_scale(ms.m_x * cs.m_x / dcs.m_x, ms.m_y * cs.m_y / dcs.m_y, ms.m_z * cs.m_z / dcs.m_z);
    Util::set_matrix_scale(matrix, actual_matrix_scale);
    body_data->m_matrix_changed = true;
    return Qnil;
}

VALUE MSP::Body::rbf_get_position(VALUE self, VALUE v_body, VALUE v_mode) {
    const NewtonBody* body = c_value_to_body(v_body);
    dMatrix matrix;
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    if (Util::value_to_int(v_mode) == 0) {
        return Util::point_to_value(dVector(matrix[3][0], matrix[3][1], matrix[3][2]));
    }
    else {
        dVector com;
        NewtonBodyGetCentreOfMass(body, &com[0]);
        return Util::point_to_value(matrix.TransformVector(com));
    }
}

VALUE MSP::Body::rbf_set_position(VALUE self, VALUE v_body, VALUE v_position, VALUE v_mode) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    dVector position(Util::value_to_point(v_position));
    dMatrix matrix;
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    if (Util::value_to_int(v_mode) == 1) {
        dVector com;
        NewtonBodyGetCentreOfMass(body, &com[0]);
        position = matrix.TransformVector(matrix.UntransformVector(position) - com);
    }
    matrix[3] = position;
    NewtonBodySetMatrix(body, &matrix[0][0]);
    body_data->m_matrix_changed = true;
    return Qnil;
}

VALUE MSP::Body::rbf_get_rotation(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    dFloat rotation[4];
    NewtonBodyGetRotation(body, rotation);
    VALUE v_rotation = rb_ary_new2(4);
    for (unsigned int i = 0; i < 4; ++i)
        rb_ary_store(v_rotation, i, Util::to_value(rotation[i]));
    return v_rotation;
}

VALUE MSP::Body::rbf_get_euler_angles(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    dMatrix matrix;
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    dVector angles0;
    dVector angles1;
    NewtonGetEulerAngle(&matrix[0][0], &angles0[0], &angles1[0]);
    return Util::vector_to_value(angles0);
}

VALUE MSP::Body::rbf_set_euler_angles(VALUE self, VALUE v_body, VALUE v_angles) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    dVector angles(Util::value_to_vector(v_angles));
    dMatrix matrix;
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    NewtonSetEulerAngle(&angles[0], &matrix[0][0]);
    NewtonBodySetMatrix(body, &matrix[0][0]);
    body_data->m_matrix_changed = true;
    //dVector angles0;
    //dVector angles1;
    //NewtonGetEulerAngle(&matrix[0][0], &angles0[0], &angles1[0]);
    //return Util::vector_to_value(angles0);
    return Qnil;
}

VALUE MSP::Body::rbf_get_velocity(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    dVector velocity;
    NewtonBodyGetVelocity(body, &velocity[0]);
    return Util::vector_to_value(velocity.Scale(M_INCH_TO_METER));
}

VALUE MSP::Body::rbf_set_velocity(VALUE self, VALUE v_body, VALUE v_velocity) {
    const NewtonBody* body = c_value_to_body(v_body);
    //BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    //if (!body_data->bstatic) {
        dVector velocity(Util::value_to_vector(v_velocity).Scale(M_METER_TO_INCH));
        NewtonBodySetVelocity(body, &velocity[0]);
    //}
    return Qnil;
}

VALUE MSP::Body::rbf_integrate_velocity(VALUE self, VALUE v_body, VALUE v_timestep) {
    const NewtonBody* body = c_value_to_body(v_body);
    dFloat timestep = Util::value_to_dFloat(v_timestep);
    NewtonBodyIntegrateVelocity(body, timestep);
    return Qnil;
}

VALUE MSP::Body::rbf_get_omega(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    dVector omega;
    NewtonBodyGetOmega(body, &omega[0]);
    return Util::vector_to_value(omega);
}

VALUE MSP::Body::rbf_set_omega(VALUE self, VALUE v_body, VALUE v_omega) {
    const NewtonBody* body = c_value_to_body(v_body);
    //BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    //if (!body_data->bstatic) {
        dVector omega(Util::value_to_vector(v_omega));
        NewtonBodySetOmega(body, &omega[0]);
    //}
    return Qnil;
}

VALUE MSP::Body::rbf_get_centre_of_mass(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    dVector com;
    NewtonBodyGetCentreOfMass(body, &com[0]);
    com.m_x /= body_data->m_matrix_scale.m_x;
    com.m_y /= body_data->m_matrix_scale.m_y;
    com.m_z /= body_data->m_matrix_scale.m_z;
    return Util::point_to_value(com);
}

VALUE MSP::Body::rbf_set_centre_of_mass(VALUE self, VALUE v_body, VALUE v_com) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    dVector com(Util::value_to_point(v_com));
    com.m_x *= body_data->m_matrix_scale.m_x;
    com.m_y *= body_data->m_matrix_scale.m_y;
    com.m_z *= body_data->m_matrix_scale.m_z;
    NewtonBodySetCentreOfMass(body, &com[0]);
    return Qnil;
}

VALUE MSP::Body::rbf_get_inertia(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    dFloat mass;
    dVector inertia;
    NewtonBodyGetMass(body, &mass, &inertia.m_x, &inertia.m_y, &inertia.m_z);
    return Util::vector_to_value(inertia);
}

VALUE MSP::Body::rbf_get_mass(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_mass);
}

VALUE MSP::Body::rbf_set_mass(VALUE self, VALUE v_body, VALUE v_mass) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    if (!body_data->m_dynamic)
        return Qnil;
    body_data->m_mass = Util::clamp_dFloat(Util::value_to_dFloat(v_mass), MIN_MASS, MAX_MASS);
    body_data->m_density = Util::clamp_dFloat(body_data->m_mass / body_data->m_volume, MIN_DENSITY, MAX_DENSITY);
    dVector com;
    NewtonBodyGetCentreOfMass(body, &com[0]);
    NewtonBodySetMassProperties(body, body_data->m_bstatic ? 0.0 : body_data->m_mass, NewtonBodyGetCollision(body));
    NewtonBodySetCentreOfMass(body, &com[0]);
    return Qnil;
}

VALUE MSP::Body::rbf_get_density(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_density);
}

VALUE MSP::Body::rbf_set_density(VALUE self, VALUE v_body, VALUE v_density) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    if (!body_data->m_dynamic)
        return Qnil;
    body_data->m_density = Util::clamp_dFloat(Util::value_to_dFloat(v_density), MIN_DENSITY, MAX_DENSITY);
    body_data->m_mass = Util::clamp_dFloat(body_data->m_density * body_data->m_volume, MIN_MASS, MAX_MASS);
    dVector com;
    NewtonBodyGetCentreOfMass(body, &com[0]);
    NewtonBodySetMassProperties(body, body_data->m_bstatic ? 0.0 : body_data->m_mass, NewtonBodyGetCollision(body));
    NewtonBodySetCentreOfMass(body, &com[0]);
    return Qnil;
}

VALUE MSP::Body::rbf_get_volume(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_volume);
}

VALUE MSP::Body::rbf_set_volume(VALUE self, VALUE v_body, VALUE v_volume) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    if (!body_data->m_dynamic) return Qnil;
    body_data->m_volume = Util::clamp_dFloat(Util::value_to_dFloat(v_volume), MIN_VOLUME, MAX_VOLUME);
    body_data->m_mass = Util::clamp_dFloat(body_data->m_density * body_data->m_volume, MIN_MASS, MAX_MASS);
    dVector com;
    NewtonBodyGetCentreOfMass(body, &com[0]);
    NewtonBodySetMassProperties(body, body_data->m_bstatic ? 0.0 : body_data->m_mass, NewtonBodyGetCollision(body));
    NewtonBodySetCentreOfMass(body, &com[0]);
    return Qnil;
}

VALUE MSP::Body::rbf_reset_mass_properties(VALUE self, VALUE v_body, VALUE v_density) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    dFloat density = Util::clamp_dFloat(Util::value_to_dFloat(v_density), MIN_DENSITY, MAX_DENSITY);
    if (!body_data->m_dynamic) return Qfalse;
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    body_data->m_density = density;
    body_data->m_volume = Util::clamp_dFloat(NewtonConvexCollisionCalculateVolume(collision), MIN_VOLUME, MAX_VOLUME);
    body_data->m_mass = Util::clamp_dFloat(body_data->m_density * body_data->m_volume, MIN_MASS, MAX_MASS);
    NewtonBodySetMassProperties(body, body_data->m_bstatic ? 0.0 : body_data->m_mass, collision);
    return Qtrue;
}

VALUE MSP::Body::rbf_is_static(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_bstatic);
}

VALUE MSP::Body::rbf_set_static(VALUE self, VALUE v_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    if (!body_data->m_dynamic) return Qnil;
    bool state = Util::value_to_bool(v_state);
    if (state == body_data->m_bstatic) return Qnil;
    body_data->m_bstatic = state;
    dVector com;
    NewtonBodyGetCentreOfMass(body, &com[0]);
    NewtonBodySetMassProperties(body, body_data->m_bstatic ? 0.0 : body_data->m_mass, NewtonBodyGetCollision(body));
    NewtonBodySetCentreOfMass(body, &com[0]);
    NewtonBodySetSleepState(body, 0);
    if (body_data->m_bstatic) {
        dVector zero_vector(0.0);
        NewtonBodySetVelocity(body, &zero_vector[0]);
        NewtonBodySetOmega(body, &zero_vector[0]);
        body_data->m_matrix_changed = true;
    }
    else {
        NewtonBodySetAutoSleep(body, body_data->m_auto_sleep_enabled ? 1 : 0);
    }
    return Qnil;
}

VALUE MSP::Body::rbf_is_collidable(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_collidable);
}

VALUE MSP::Body::rbf_set_collidable(VALUE self, VALUE v_body, VALUE v_collidable) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_collidable = Util::value_to_bool(v_collidable);
    NewtonBodySetCollidable(body, body_data->m_collidable ? 1 : 0);
    return Qnil;
}

VALUE MSP::Body::rbf_is_frozen(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    return NewtonBodyGetFreezeState(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSP::Body::rbf_set_frozen(VALUE self, VALUE v_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    NewtonBodySetFreezeState(body, Util::value_to_bool(v_state) ? 1 : 0);
    return Qnil;
}

VALUE MSP::Body::rbf_is_sleeping(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    return NewtonBodyGetSleepState(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSP::Body::rbf_set_sleeping(VALUE self, VALUE v_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    NewtonBodySetSleepState(body, Util::value_to_bool(v_state) ? 1 : 0);
    return Qnil;
}

VALUE MSP::Body::rbf_get_auto_sleep_state(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    //return NewtonBodyGetAutoSleep(body) == 1 ? Qtrue : Qfalse;
    return Util::to_value(body_data->m_auto_sleep_enabled);
}

VALUE MSP::Body::rbf_set_auto_sleep_state(VALUE self, VALUE v_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_auto_sleep_enabled = Util::value_to_bool(v_state);
    NewtonBodySetAutoSleep(body, body_data->m_auto_sleep_enabled ? 1 : 0);
    return Qnil;
}

VALUE MSP::Body::rbf_is_non_collidable_with(VALUE self, VALUE v_body, VALUE v_other_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonBody* other_body = c_value_to_body(v_other_body);
    c_validate_two_bodies(body, other_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return body_data->m_non_collidable_bodies.find(other_body) != body_data->m_non_collidable_bodies.end() ? Qtrue : Qfalse;
}

VALUE MSP::Body::rbf_set_non_collidable_with(VALUE self, VALUE v_body, VALUE v_other_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonBody* other_body = c_value_to_body(v_other_body);
    bool state = Util::value_to_bool(v_state);
    c_validate_two_bodies(body, other_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    BodyData* other_body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(other_body));
    if (state) {
        body_data->m_non_collidable_bodies[other_body] = true;
        other_body_data->m_non_collidable_bodies[body] = true;
    }
    else {
        if (body_data->m_non_collidable_bodies.find(other_body) != body_data->m_non_collidable_bodies.end())
            body_data->m_non_collidable_bodies.erase(other_body);
        if (other_body_data->m_non_collidable_bodies.find(body) != other_body_data->m_non_collidable_bodies.end())
            other_body_data->m_non_collidable_bodies.erase(body);
    }
    return Qnil;
}

VALUE MSP::Body::rbf_get_non_collidable_bodies(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_non_collidable_bodies = rb_ary_new();
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    for (std::map<const NewtonBody*, bool>::iterator it = body_data->m_non_collidable_bodies.begin(); it != body_data->m_non_collidable_bodies.end(); ++it) {
        VALUE v_address = c_body_to_value(it->first);
        if (proc_given) {
            BodyData* other_body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(it->first));
            VALUE v_result = rb_yield_values(2, v_address, other_body_data->m_user_data);
            if (v_result != Qnil)
                rb_ary_push(v_non_collidable_bodies, v_result);
        }
        else
            rb_ary_push(v_non_collidable_bodies, v_address);
    }
    return v_non_collidable_bodies;
}

VALUE MSP::Body::rbf_clear_non_collidable_bodies(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    unsigned int count = (unsigned int)body_data->m_non_collidable_bodies.size();
    c_clear_non_collidable_bodies(body);
    return Util::to_value(count);
}

VALUE MSP::Body::rbf_get_elasticity(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_elasticity);
}

VALUE MSP::Body::rbf_set_elasticity(VALUE self, VALUE v_body, VALUE v_elasticity) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_elasticity = Util::clamp_dFloat(Util::value_to_dFloat(v_elasticity), 0.01f, 2.00f);
    return Qnil;
}

VALUE MSP::Body::rbf_get_softness(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_softness);
}

VALUE MSP::Body::rbf_set_softness(VALUE self, VALUE v_body, VALUE v_softness) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_softness = Util::clamp_dFloat(Util::value_to_dFloat(v_softness), 0.01f, 1.00f);
    return Qnil;
}

VALUE MSP::Body::rbf_get_static_friction(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_static_friction);
}

VALUE MSP::Body::rbf_set_static_friction(VALUE self, VALUE v_body, VALUE v_friction) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_static_friction = Util::clamp_dFloat(Util::value_to_dFloat(v_friction), 0.01f, 2.00f);
    return Qnil;
}

VALUE MSP::Body::rbf_get_kinetic_friction(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_kinetic_friction);
}

VALUE MSP::Body::rbf_set_kinetic_friction(VALUE self, VALUE v_body, VALUE v_friction) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_kinetic_friction = Util::clamp_dFloat(Util::value_to_dFloat(v_friction), 0.01f, 2.00f);
    return Qnil;
}

VALUE MSP::Body::rbf_get_friction_state(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_friction_enabled);
}

VALUE MSP::Body::rbf_set_friction_state(VALUE self, VALUE v_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_friction_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Body::rbf_get_magnet_mode(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_magnet_mode);
}

VALUE MSP::Body::rbf_set_magnet_mode(VALUE self, VALUE v_body, VALUE v_mode) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_magnet_mode = Util::value_to_int(v_mode);
    if (body_data->m_magnet_mode != 1 && body_data->m_magnet_mode != 2)
        body_data->m_magnet_mode = 1;
    return Qnil;
}

VALUE MSP::Body::rbf_get_magnet_force(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_magnet_force * M_INCH_TO_METER);
}

VALUE MSP::Body::rbf_set_magnet_force(VALUE self, VALUE v_body, VALUE v_force) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_magnet_force = Util::value_to_dFloat(v_force) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Body::rbf_get_magnet_range(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_magnet_range * M_INCH_TO_METER);
}

VALUE MSP::Body::rbf_set_magnet_range(VALUE self, VALUE v_body, VALUE v_range) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_magnet_range = Util::value_to_dFloat(v_range) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Body::rbf_get_magnet_strength(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_magnet_strength * M_INCH_TO_METER);
}

VALUE MSP::Body::rbf_set_magnet_strength(VALUE self, VALUE v_body, VALUE v_strength) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_magnet_strength = Util::value_to_dFloat(v_strength) * M_METER_TO_INCH;
    return Qnil;
}

VALUE MSP::Body::rbf_is_magnetic(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_magnetic);
}

VALUE MSP::Body::rbf_set_magnetic(VALUE self, VALUE v_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_magnetic = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Body::rbf_get_aabb(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    dVector min;
    dVector max;
    NewtonBodyGetAABB(body, &min[0], &max[0]);
    return rb_ary_new3(2, Util::point_to_value(min), Util::point_to_value(max));
}

VALUE MSP::Body::rbf_get_linear_damping(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::vector_to_value(body_data->m_linear_damping);
}

VALUE MSP::Body::rbf_set_linear_damping(VALUE self, VALUE v_body, VALUE v_damp_vector) {
    const NewtonBody* body = c_value_to_body(v_body);
    dVector damp_vector(Util::value_to_vector(v_damp_vector));
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_linear_damping.m_x = Util::clamp_dFloat(damp_vector.m_x, 0.0, 1.0);
    body_data->m_linear_damping.m_y = Util::clamp_dFloat(damp_vector.m_y, 0.0, 1.0);
    body_data->m_linear_damping.m_z = Util::clamp_dFloat(damp_vector.m_z, 0.0, 1.0);
    body_data->m_linear_damping_enabled = (damp_vector.m_x > M_EPSILON || damp_vector.m_y > M_EPSILON || damp_vector.m_z > M_EPSILON);
    return Qnil;
}

VALUE MSP::Body::rbf_get_angular_damping(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::vector_to_value(body_data->m_angular_damping);
}

VALUE MSP::Body::rbf_set_angular_damping(VALUE self, VALUE v_body, VALUE v_damp_vector) {
    const NewtonBody* body = c_value_to_body(v_body);
    dVector damp_vector(Util::value_to_vector(v_damp_vector));
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_angular_damping.m_x = Util::clamp_dFloat(damp_vector.m_x, 0.0, 1.0);
    body_data->m_angular_damping.m_y = Util::clamp_dFloat(damp_vector.m_y, 0.0, 1.0);
    body_data->m_angular_damping.m_z = Util::clamp_dFloat(damp_vector.m_z, 0.0, 1.0);
    body_data->m_angular_damping_enabled = (damp_vector.m_x > M_EPSILON || damp_vector.m_y > M_EPSILON || damp_vector.m_z > M_EPSILON);
    return Qnil;
}

VALUE MSP::Body::rbf_get_point_velocity(VALUE self, VALUE v_body, VALUE v_point) {
    const NewtonBody* body = c_value_to_body(v_body);
    dVector point(Util::value_to_point(v_point));
    dVector velocity;
    NewtonBodyGetPointVelocity(body, &point[0], &velocity[0]);
    return Util::vector_to_value(velocity.Scale(M_INCH_TO_METER));
}

VALUE MSP::Body::rbf_add_point_force(VALUE self, VALUE v_body, VALUE v_point, VALUE v_force) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    if (body_data->m_bstatic) return Qfalse;
    dMatrix matrix;
    dVector centre;
    dVector point(Util::value_to_point(v_point));
    dVector force(Util::value_to_vector(v_force).Scale(M_METER_TO_INCH));
    NewtonBodyGetCentreOfMass(body, &centre[0]);
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    centre = matrix.TransformVector(centre);
    dVector torque((point - centre).CrossProduct(force));
    c_body_add_force(body_data, force);
    c_body_add_torque(body_data, torque);
    return Qtrue;
}

VALUE MSP::Body::rbf_add_impulse(VALUE self, VALUE v_body, VALUE v_center, VALUE v_delta_vel, VALUE v_timestep) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    if (body_data->m_bstatic) return Qfalse;
    dVector center(Util::value_to_point(v_center));
    dVector delta_vel(Util::value_to_vector(v_delta_vel).Scale(M_METER_TO_INCH));
    dFloat timestep = Util::value_to_dFloat(v_timestep);
    NewtonBodyAddImpulse(body, &center[0], &delta_vel[0], timestep);
    return Qtrue;
}

VALUE MSP::Body::rbf_get_force(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    dVector force;
    NewtonBodyGetForce(body, &force[0]);
    return Util::vector_to_value(force.Scale(M_INCH_TO_METER));
}

VALUE MSP::Body::rbf_add_force(VALUE self, VALUE v_body, VALUE v_force) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    if (body_data->m_bstatic)
        return Qfalse;
    else {
        c_body_add_force(body_data, Util::value_to_vector(v_force).Scale(M_METER_TO_INCH));
        return Qtrue;
    }
}

VALUE MSP::Body::rbf_set_force(VALUE self, VALUE v_body, VALUE v_force) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    if (body_data->m_bstatic)
        return Qfalse;
    else {
        c_body_set_force(body_data, Util::value_to_vector(v_force).Scale(M_METER_TO_INCH));
        return Qtrue;
    }
}

VALUE MSP::Body::rbf_get_torque(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    dVector torque;
    NewtonBodyGetTorque(body, &torque[0]);
    return Util::vector_to_value(torque.Scale(M_INCH2_TO_METER2));
}

VALUE MSP::Body::rbf_add_torque(VALUE self, VALUE v_body, VALUE v_torque) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    if (body_data->m_bstatic)
        return Qfalse;
    else {
        c_body_add_torque(body_data, Util::value_to_vector(v_torque).Scale(M_METER2_TO_INCH2));
        return Qtrue;
    }
}

VALUE MSP::Body::rbf_set_torque(VALUE self, VALUE v_body, VALUE v_torque) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    if (body_data->m_bstatic)
        return Qfalse;
    else {
        c_body_set_torque(body_data, Util::value_to_vector(v_torque).Scale(M_METER2_TO_INCH2));
        return Qtrue;
    }
}

VALUE MSP::Body::rbf_get_net_contact_force(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    dVector net_force(0.0);
    for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint)) {
        for (void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact)) {
            NewtonMaterial* material = NewtonContactGetMaterial(contact);
            dVector force;
            NewtonMaterialGetContactForce(material, body, &force[0]);
            net_force += force;
        }
    }
    return Util::vector_to_value(net_force.Scale(M_INCH_TO_METER));
}

VALUE MSP::Body::rbf_get_net_joint_tension1(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    dVector net_force(0.0);
    for (std::map<MSP::Joint::JointData*, bool>::const_iterator it = MSP::Joint::s_valid_joints.begin(); it != MSP::Joint::s_valid_joints.end(); ++it) {
        if (it->first->m_world == world) {
            dMatrix parent_matrix;
            MSP::Joint::c_calculate_global_parent_matrix(it->first, parent_matrix);
            dVector tension(parent_matrix.RotateVector(it->first->m_tension1));
            if (it->first->m_parent == body)
                net_force -= tension;
            else if (it->first->m_child == body)
                net_force += tension;
        }
    }
    return Util::vector_to_value(net_force.Scale(M_INCH_TO_METER));
}

VALUE MSP::Body::rbf_get_net_joint_tension2(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    dVector net_force(0.0);
    for (std::map<MSP::Joint::JointData*, bool>::const_iterator it = MSP::Joint::s_valid_joints.begin(); it != MSP::Joint::s_valid_joints.end(); ++it) {
        if (it->first->m_world == world) {
            dMatrix parent_matrix;
            MSP::Joint::c_calculate_global_parent_matrix(it->first, parent_matrix);
            dVector tension(parent_matrix.RotateVector(it->first->m_tension2));
            if (it->first->m_parent == body)
                net_force -= tension;
            else if (it->first->m_child == body)
                net_force += tension;
        }
    }
    return Util::vector_to_value(net_force.Scale(M_INCH_TO_METER));
}

VALUE MSP::Body::rbf_get_contacts(VALUE self, VALUE v_body, VALUE v_inc_non_collidable) {
    const NewtonBody* body = c_value_to_body(v_body);
    bool inc_non_collidable = Util::value_to_bool(v_inc_non_collidable);
    bool proc_given = (rb_block_given_p() != 0);
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    VALUE v_contacts = rb_ary_new();
    for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint)) {
        const NewtonBody* touching_body = NewtonJointGetBody0(joint);
        if (touching_body == body)
            touching_body = NewtonJointGetBody1(joint);
        BodyData* touching_body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(touching_body));
        VALUE v_touching_body = c_body_to_value(touching_body);
        for (void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact)) {
            NewtonMaterial* material = NewtonContactGetMaterial(contact);
            dVector point, normal, force;
            NewtonMaterialGetContactPositionAndNormal(material, body, &point[0], &normal[0]);
            NewtonMaterialGetContactForce(material, body, &force[0]);
            double speed = (double)NewtonMaterialGetContactNormalSpeed(material) * M_INCH_TO_METER;
            VALUE v_point = Util::point_to_value(point);
            VALUE v_normal = Util::vector_to_value(normal);
            VALUE v_force = Util::vector_to_value(force.Scale(M_INCH_TO_METER));
            VALUE v_speed = Util::to_value(speed);
            if (proc_given) {
                VALUE v_result = rb_yield_values(6, v_touching_body, touching_body_data->m_user_data, v_point, v_normal, v_force, v_speed);
                if (v_result != Qnil) rb_ary_push(v_contacts, v_result);
            }
            else
                rb_ary_push(v_contacts, rb_ary_new3(6, v_touching_body, touching_body_data->m_user_data, v_point, v_normal, v_force, v_speed));
        }
    }
    if (inc_non_collidable) {
        const NewtonCollision* colA = NewtonBodyGetCollision(body);
        const NewtonCollision* colB;
        dMatrix matA;
        dMatrix matB;
        NewtonBodyGetMatrix(body, &matA[0][0]);
        dFloat points[3*MSP_NON_COL_CONTACTS_CAPACITY];
        dFloat normals[3*MSP_NON_COL_CONTACTS_CAPACITY];
        dFloat penetrations[3*MSP_NON_COL_CONTACTS_CAPACITY];
        long long attrA[MSP_NON_COL_CONTACTS_CAPACITY];
        long long attrB[MSP_NON_COL_CONTACTS_CAPACITY];
        for (const NewtonBody* tbody = NewtonWorldGetFirstBody(world); tbody; tbody = NewtonWorldGetNextBody(world, tbody)) {
            if (tbody == body || c_bodies_collidable(tbody, body) || !c_bodies_aabb_overlap(tbody, body)) continue;
            colB = NewtonBodyGetCollision(tbody);
            NewtonBodyGetMatrix(tbody, &matB[0][0]);
            int count = NewtonCollisionCollide(world, MSP_NON_COL_CONTACTS_CAPACITY, colA, &matA[0][0], colB, &matB[0][0], points, normals, penetrations, attrA, attrB, 0);
            if (count == 0) continue;
            BodyData* touching_body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(tbody));
            VALUE v_touching_body = c_body_to_value(tbody);
            dVector force(0.0);
            dFloat speed = 0.0;
            for (int i = 0; i < count*3; i += 3) {
                dVector point(points[i+0], points[i+1], points[i+2]);
                dVector normal(normals[i+0], normals[i+1], normals[i+2]);
                VALUE v_point = Util::point_to_value(point);
                VALUE v_normal = Util::vector_to_value(normal);
                VALUE v_force = Util::vector_to_value(force.Scale(M_INCH_TO_METER));
                VALUE v_speed = Util::to_value(speed);
                if (proc_given) {
                    VALUE v_result = rb_yield_values(6, v_touching_body, touching_body_data->m_user_data, v_point, v_normal, v_force, v_speed);
                    if (v_result != Qnil) rb_ary_push(v_contacts, v_result);
                }
                else
                    rb_ary_push(v_contacts, rb_ary_new3(6, v_touching_body, touching_body_data->m_user_data, v_point, v_normal, v_force, v_speed));
            }
        }
    }
    return v_contacts;
}

VALUE MSP::Body::rbf_get_touching_bodies(VALUE self, VALUE v_body, VALUE v_inc_non_collidable) {
    const NewtonBody* body = c_value_to_body(v_body);
    bool inc_non_collidable = Util::value_to_bool(v_inc_non_collidable);
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_touching_bodies = rb_ary_new();
    for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint)) {
        NewtonBody* touching_body = NewtonJointGetBody0(joint);
        if (!inc_non_collidable && !c_bodies_collidable(touching_body, body)) continue;
        if (touching_body == body)
            touching_body = NewtonJointGetBody1(joint);
        VALUE v_touching_body = c_body_to_value(touching_body);
        if (proc_given) {
            BodyData* touching_body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(touching_body));
            VALUE v_result = rb_yield_values(2, v_touching_body, touching_body_data->m_user_data);
            if (v_result != Qnil) rb_ary_push(v_touching_bodies, v_result);
        }
        else
            rb_ary_push(v_touching_bodies, v_touching_body);
    }
    /*if (inc_non_collidable) {
        const NewtonWorld* world = NewtonBodyGetWorld(body);
        const NewtonCollision* colA = NewtonBodyGetCollision(body);
        const NewtonCollision* colB;
        dMatrix matA;
        dMatrix matB;
        NewtonBodyGetMatrix(body, &matA[0][0]);
        for (const NewtonBody* tbody = NewtonWorldGetFirstBody(world); tbody; tbody = NewtonWorldGetNextBody(world, tbody)) {
            if (tbody == body || c_bodies_collidable(tbody, body) || !c_bodies_aabb_overlap(tbody, body)) continue;
            colB = NewtonBodyGetCollision(tbody);
            NewtonBodyGetMatrix(tbody, &matB[0][0]);
            if (NewtonCollisionIntersectionTest(world, colA, &matA[0][0], colB, &matB[0][0], 0) == 1)
                rb_ary_push(v_touching_bodies, c_body_to_value(tbody));
        }
    }*/
    return v_touching_bodies;
}

VALUE MSP::Body::rbf_get_contact_points(VALUE self, VALUE v_body, VALUE v_inc_non_collidable) {
    const NewtonBody* body = c_value_to_body(v_body);
    bool inc_non_collidable = Util::value_to_bool(v_inc_non_collidable);
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    VALUE v_contact_points = rb_ary_new();
    for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint)) {
        for (void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact)) {
            NewtonMaterial* material = NewtonContactGetMaterial(contact);
            dVector point;
            dVector normal;
            NewtonMaterialGetContactPositionAndNormal(material, body, &point[0], &normal[0]);
            rb_ary_push(v_contact_points, Util::point_to_value(point));
        }
    }
    if (inc_non_collidable) {
        const NewtonCollision* colA = NewtonBodyGetCollision(body);
        const NewtonCollision* colB;
        dMatrix matA;
        dMatrix matB;
        dFloat points[3*MSP_NON_COL_CONTACTS_CAPACITY];
        dFloat normals[3*MSP_NON_COL_CONTACTS_CAPACITY];
        dFloat penetrations[3*MSP_NON_COL_CONTACTS_CAPACITY];
        long long attrA[MSP_NON_COL_CONTACTS_CAPACITY];
        long long attrB[MSP_NON_COL_CONTACTS_CAPACITY];
        NewtonBodyGetMatrix(body, &matA[0][0]);
        for (const NewtonBody* tbody = NewtonWorldGetFirstBody(world); tbody; tbody = NewtonWorldGetNextBody(world, tbody)) {
            if (tbody == body || c_bodies_collidable(tbody, body) || !c_bodies_aabb_overlap(tbody, body)) continue;
            colB = NewtonBodyGetCollision(tbody);
            NewtonBodyGetMatrix(tbody, &matB[0][0]);
            int count = NewtonCollisionCollide(world, MSP_NON_COL_CONTACTS_CAPACITY, colA, &matA[0][0], colB, &matB[0][0], points, normals, penetrations, attrA, attrB, 0);
            if (count == 0) continue;
            for (int i = 0; i < count*3; i += 3) {
                dVector point(points[i+0], points[i+1], points[i+2]);
                rb_ary_push(v_contact_points, Util::point_to_value(point));
            }
        }
    }
    return v_contact_points;
}

VALUE MSP::Body::rbf_get_collision_faces(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    dMatrix matrix;
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    CollisionIteratorData iterator_data(rb_ary_new());
    NewtonCollisionForEachPolygonDo(collision, &matrix[0][0], collision_iterator, reinterpret_cast<void*>(&iterator_data));
    return iterator_data.m_faces;
}

VALUE MSP::Body::rbf_get_collision_faces2(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    dMatrix matrix;
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    CollisionIteratorData iterator_data(rb_ary_new());
    NewtonCollisionForEachPolygonDo(collision, &matrix[0][0], collision_iterator2, reinterpret_cast<void*>(&iterator_data));
    return iterator_data.m_faces;
}

VALUE MSP::Body::rbf_get_collision_faces3(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    dMatrix matrix;
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    CollisionIteratorData iterator_data(rb_ary_new());
    NewtonCollisionForEachPolygonDo(collision, &matrix[0][0], collision_iterator3, reinterpret_cast<void*>(&iterator_data));
    return iterator_data.m_faces;
}

VALUE MSP::Body::rbf_apply_pick_and_drag(VALUE self, VALUE v_body, VALUE v_pick_pt, VALUE v_dest_pt, VALUE v_stiffness, VALUE v_damp, VALUE v_timestep) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    dVector pick_pt(Util::value_to_point(v_pick_pt));
    dVector dest_pt(Util::value_to_point(v_dest_pt));
    dFloat stiffness = Util::clamp_dFloat(Util::value_to_dFloat(v_stiffness), 0.0, 1.0);
    dFloat damp = Util::clamp_dFloat(Util::value_to_dFloat(v_damp), 0.0, 1.0);
    dFloat timestep = Util::value_to_dFloat(v_timestep);
    dFloat inv_timestep = timestep > M_EPSILON ? 1.0 / timestep : 0.0;
    // Verify
    if (body_data->m_bstatic)
        return Qfalse;
    // Get data
    dMatrix matrix;
    dVector com;
    dVector inertia;
    dVector velocity;
    dVector omega;
    dFloat mass;
    NewtonBodyGetMass(body, &mass, &inertia.m_x, &inertia.m_y, &inertia.m_z);
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    NewtonBodyGetCentreOfMass(body, &com[0]);
    NewtonBodyGetVelocity(body, &velocity[0]);
    NewtonBodyGetOmega(body, &omega[0]);
    com = matrix.TransformVector(com);
    //inertia = matrix.RotateVector(inertia);
    //dFloat inv_mass = 1.0 / mass;
    // Calculate force
    dVector des_vel((dest_pt - pick_pt).Scale((1.0 - damp) * inv_timestep));
    dVector accel((des_vel - velocity).Scale(inv_timestep));
    dVector force(accel.Scale(mass * stiffness));
    // Calculate torque
    dVector torque((pick_pt - com).CrossProduct(force));
    // Add force and torque
    c_body_add_force(body_data, force);
    c_body_add_torque(body_data, torque);
    // Unfreeze the body
    NewtonBodySetFreezeState(body, 0);
    // Set it active
    NewtonBodySetSleepState(body, 0);
    // Return success
    return Qtrue;
}

VALUE MSP::Body::rbf_apply_buoyancy(VALUE self, VALUE v_body, VALUE v_plane_origin, VALUE v_plane_normal, VALUE v_density, VALUE v_linear_viscosity, VALUE v_angular_viscosity, VALUE v_linear_current, VALUE v_angular_current, VALUE v_timestep) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(world));
    dVector origin(Util::value_to_point(v_plane_origin));
    dVector normal(Util::value_to_vector(v_plane_normal));
    dVector linear_current(Util::value_to_vector(v_linear_current));
    dVector angular_current(Util::value_to_vector(v_angular_current));
    dFloat fluid_density = Util::max_dFloat(Util::value_to_dFloat(v_density), MIN_DENSITY);
    dFloat linear_viscosity = Util::clamp_dFloat(Util::value_to_dFloat(v_linear_viscosity), 0.0, 1.0);
    dFloat angular_viscosity = Util::clamp_dFloat(Util::value_to_dFloat(v_angular_viscosity), 0.0, 1.0);
    dFloat timestep = Util::value_to_dFloat(v_timestep);

    if (body_data->m_bstatic) return Qfalse;

    // dMatrix plane_matrix;
    // Util::matrix_from_pin_dir(origin, normal, plane_matrix);
    // normal.m_w = plane_matrix.UntransformVector(Util::ORIGIN).m_z;


    normal.m_w = -normal.DotProduct3(origin);

    dMatrix transformation_matrix;
    //dVector com;
    NewtonBodyGetMatrix(body, &transformation_matrix[0][0]);
    //NewtonBodyGetCentreOfMass(body, &com[0]);
    //com = transformation_matrix.TransformVector(com);

    dVector centerOfPreasure(0.0f);

    dFloat volume = NewtonConvexCollisionCalculateBuoyancyVolume(collision, &transformation_matrix[0][0], &normal[0], &centerOfPreasure[0]);
    if (volume > 0.0) {

        dVector inertia;
        dFloat mass;
        NewtonBodyGetMass(body, &mass, &inertia.m_x, &inertia.m_y, &inertia.m_z);

        // if some part of the shape si under water, calculate the buoyancy force base on 
		// Archimedes's buoyancy principle, which is the buoyancy force is equal to the 
		// weight of the fluid displaced by the volume under water. 
		dVector cog(0.0f);
		//const dFloat solidDentityFactor = 1.35f;

		// calculate the ratio of volumes an use it calculate a density equivalent
		dFloat shapeVolume = NewtonConvexCollisionCalculateVolume(collision);
		dFloat density = mass / shapeVolume;

		dFloat displacedMass = density * volume;
		NewtonBodyGetCentreOfMass(body, &cog[0]);
		centerOfPreasure -= transformation_matrix.TransformVector(cog);

		// now with the mass and center of mass of the volume under water, calculate buoyancy force and torque
		dVector force(world_data->m_gravity.Scale(displacedMass));
		dVector torque(centerOfPreasure.CrossProduct(force));

        c_body_add_force(body_data, force);
        c_body_add_torque(body_data, torque);

		// apply a fake viscous drag to damp the under water motion 
		dVector omega(0.0f);
		dVector veloc(0.0f);
		NewtonBodyGetOmega(body, &omega[0]);
		NewtonBodyGetVelocity(body, &veloc[0]);
		omega = omega.Scale(angular_viscosity);
		veloc = veloc.Scale(linear_viscosity);
		NewtonBodySetOmega(body, &omega[0]);
		NewtonBodySetVelocity(body, &veloc[0]);

        return Qtrue;
    } else {
        return Qfalse;
    }
}

VALUE MSP::Body::rbf_apply_aerodynamics(VALUE self, VALUE v_body, VALUE v_drag, VALUE v_wind) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    dFloat drag = Util::value_to_dFloat(v_drag);
    dVector wind(Util::value_to_vector(v_wind).Scale(M_METER_TO_INCH));
    if (body_data->m_bstatic || !body_data->m_dynamic)
        return Qnil;
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    dMatrix matrix;
    dVector centre;
    NewtonBodyGetMatrix(body, &matrix[0][0]);
    NewtonBodyGetCentreOfMass(body, &centre[0]);
    centre = matrix.TransformVector(centre);
    dVector velocity;
    dVector omega;
    NewtonBodyGetVelocity(body, &velocity[0]);
    NewtonBodyGetOmega(body, &omega[0]);
    CollisionIteratorData2* iterator_data = new CollisionIteratorData2(body, centre, velocity, omega, drag, wind, dVector(0.0), dVector(0.0));
    NewtonCollisionForEachPolygonDo(collision, &matrix[0][0], collision_iterator4, reinterpret_cast<void*>(iterator_data));

    c_body_add_force(body_data, iterator_data->m_force);
    c_body_add_torque(body_data, iterator_data->m_torque);

    VALUE v_result = rb_ary_new3(2, Util::vector_to_value(iterator_data->m_force.Scale(M_INCH_TO_METER)), Util::vector_to_value(iterator_data->m_torque.Scale(M_INCH2_TO_METER2)));
    delete iterator_data;
    return v_result;
}

VALUE MSP::Body::rbf_copy(VALUE self, VALUE v_body, VALUE v_matrix, VALUE v_reapply_forces, VALUE v_type, VALUE v_group) {
    const NewtonBody* body = c_value_to_body(v_body);
    bool reapply_forces = Util::value_to_bool(v_reapply_forces);
    int type = Util::value_to_int(v_type);

    const NewtonWorld* world = NewtonBodyGetWorld(body);
    MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(world));
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));

    dMatrix matrix;
    if (v_matrix == Qnil)
        NewtonBodyGetMatrix(body, &matrix[0][0]);
    else
        matrix = Util::value_to_matrix(v_matrix);
    if (Util::is_matrix_flipped(matrix)) {
        matrix.m_front.m_x = -matrix.m_front.m_x;
        matrix.m_front.m_y = -matrix.m_front.m_y;
        matrix.m_front.m_z = -matrix.m_front.m_z;
    }
    Util::extract_matrix_scale(matrix);

    const NewtonCollision* new_col = NewtonCollisionCreateInstance(NewtonBodyGetCollision(body));

    NewtonBody* new_body;
    if (type == 1)
        new_body = NewtonCreateKinematicBody(world, new_col, &matrix[0][0]);
    else
        new_body = NewtonCreateDynamicBody(world, new_col, &matrix[0][0]);

    NewtonBodySetMassProperties(new_body, body_data->m_bstatic ? 0.0 : body_data->m_mass, new_col);
    NewtonDestroyCollision(new_col);

    dVector com;
    NewtonBodyGetCentreOfMass(body, &com[0]);
    NewtonBodySetCentreOfMass(new_body, &com[0]);

    BodyData* new_data = new BodyData(body_data, v_group);

    s_valid_bodies.insert(new_body);
    NewtonBodySetUserData(new_body, new_data);

    VALUE v_new_body = c_body_to_value(new_body);

    if (v_group != Qnil) {
        world_data->m_group_to_body_map[v_group] = new_body;
        rb_hash_aset(world_data->m_body_groups, v_new_body, v_group);
    }

    NewtonBodySetMaterialGroupID(new_body, body_data->m_material_id);

    NewtonBodySetForceAndTorqueCallback(new_body, force_and_torque_callback);
    NewtonBodySetDestructorCallback(new_body, destructor_callback);
    NewtonBodySetTransformCallback(new_body, transform_callback);

    /*NewtonBodySetLinearDamping(new_body, NewtonBodyGetLinearDamping(body));
    dVector angular_damp;
    NewtonBodyGetAngularDamping(body, &angular_damp[0]);
    NewtonBodySetAngularDamping(new_body, &angular_damp[0]);*/
    NewtonBodySetLinearDamping(body, 0.0);
    dVector damp(0.0);
    NewtonBodySetAngularDamping(body, &damp[0]);

    NewtonBodySetSimulationState(new_body, NewtonBodyGetSimulationState(body));
    NewtonBodySetContinuousCollisionMode(new_body, NewtonBodyGetContinuousCollisionMode(body));

    NewtonBodySetFreezeState(new_body, NewtonBodyGetFreezeState(body));
    NewtonBodySetAutoSleep(new_body, body_data->m_auto_sleep_enabled ? 1 : 0);
    NewtonBodySetCollidable(new_body, body_data->m_collidable ? 1 : 0);

    if (reapply_forces) {
        dVector omega;
        dVector velocity;
        NewtonBodyGetOmega(body, &omega[0]);
        NewtonBodyGetVelocity(body, &velocity[0]);
        NewtonBodySetOmega(new_body, &omega[0]);
        NewtonBodySetVelocity(new_body, &velocity[0]);
    }

    return v_new_body;
}

VALUE MSP::Body::rbf_get_destructor_proc(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return body_data->m_destructor_proc;
}

VALUE MSP::Body::rbf_set_destructor_proc(VALUE self, VALUE v_body, VALUE v_proc) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(world));
    if (v_proc == Qnil || rb_class_of(v_proc) == rb_cProc) {
        body_data->m_destructor_proc = v_proc;
        rb_hash_aset(world_data->m_body_destructors, c_body_to_value(body), v_proc);
    }
    else
        rb_raise(rb_eTypeError, "Expected nil or a Proc object!");
    return Qnil;
}

VALUE MSP::Body::rbf_get_user_data(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return body_data->m_user_data;
}

VALUE MSP::Body::rbf_set_user_data(VALUE self, VALUE v_body, VALUE v_user_data) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(world));
    body_data->m_user_data = v_user_data;
    rb_hash_aset(world_data->m_body_user_datas, c_body_to_value(body), v_user_data);
    return Qnil;
}

VALUE MSP::Body::rbf_get_record_touch_data_state(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_record_touch_data);
}

VALUE MSP::Body::rbf_set_record_touch_data_state(VALUE self, VALUE v_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_record_touch_data = Util::value_to_bool(v_state);
    if (!body_data->m_record_touch_data) body_data->m_touchers.clear();
    return Qnil;
}

VALUE MSP::Body::rbf_get_matrix_scale(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::vector_to_value(body_data->m_matrix_scale);
}

VALUE MSP::Body::rbf_set_matrix_scale(VALUE self, VALUE v_body, VALUE v_scale) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_matrix_scale = Util::value_to_vector(v_scale);
    return Qnil;
}

VALUE MSP::Body::rbf_matrix_changed(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_matrix_changed);
}

VALUE MSP::Body::rbf_enable_gravity(VALUE self, VALUE v_body, VALUE v_state) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    body_data->m_gravity_enabled = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::Body::rbf_is_gravity_enabled(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_gravity_enabled);
}

VALUE MSP::Body::rbf_get_contained_joints(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_contained_joints = rb_ary_new();
    for (std::map<MSP::Joint::JointData*, bool>::const_iterator it = MSP::Joint::s_valid_joints.begin(); it != MSP::Joint::s_valid_joints.end(); ++it) {
        if (it->first->m_parent == body && it->first->m_world == world) {
            VALUE v_address = MSP::Joint::c_joint_to_value(it->first);
            if (proc_given) {
                VALUE v_result = rb_yield_values(2, v_address, it->first->m_user_data);
                if (v_result != Qnil) rb_ary_push(v_contained_joints, v_result);
            }
            else
                rb_ary_push(v_contained_joints, v_address);
        }
    }
    return v_contained_joints;
}

VALUE MSP::Body::rbf_get_connected_joints(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_connected_joints = rb_ary_new();
    for (std::map<MSP::Joint::JointData*, bool>::const_iterator it = MSP::Joint::s_valid_joints.begin(); it != MSP::Joint::s_valid_joints.end(); ++it) {
        if (it->first->m_child == body && it->first->m_world == world) {
            VALUE v_address = MSP::Joint::c_joint_to_value(it->first);
            if (proc_given) {
                VALUE v_result = rb_yield_values(2, v_address, it->first->m_user_data);
                if (v_result != Qnil) rb_ary_push(v_connected_joints, v_result);
            }
            else
                rb_ary_push(v_connected_joints, v_address);
        }
    }
    return v_connected_joints;
}

VALUE MSP::Body::rbf_get_connected_bodies(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonWorld* world = NewtonBodyGetWorld(body);
    bool proc_given = (rb_block_given_p() != 0);
    VALUE v_connected_bodies = rb_ary_new();
    for (std::map<MSP::Joint::JointData*, bool>::const_iterator it = MSP::Joint::s_valid_joints.begin(); it != MSP::Joint::s_valid_joints.end(); ++it) {
        const MSP::Joint::JointData* joint_data = it->first;
        if (joint_data->m_world == world && joint_data->m_connected) {
            if (joint_data->m_child == body && joint_data->m_parent != nullptr) {
                VALUE v_address = c_body_to_value(joint_data->m_parent);
                if (proc_given) {
                    BodyData* other_body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(joint_data->m_parent));
                    VALUE v_result = rb_yield_values(2, v_address, other_body_data->m_user_data);
                    if (v_result != Qnil)
                        rb_ary_push(v_connected_bodies, v_result);
                }
                else
                    rb_ary_push(v_connected_bodies, v_address);
            }
            else if (joint_data->m_parent == body && joint_data->m_child != nullptr) {
                VALUE v_address = c_body_to_value(joint_data->m_child);
                if (proc_given) {
                    BodyData* other_body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(joint_data->m_child));
                    VALUE v_result = rb_yield_values(2, v_address, other_body_data->m_user_data);
                    if (v_result != Qnil)
                        rb_ary_push(v_connected_bodies, v_result);
                }
                else
                    rb_ary_push(v_connected_bodies, v_address);
            }
        }
    }
    return v_connected_bodies;
}

VALUE MSP::Body::rbf_get_material_id(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::to_value(body_data->m_material_id);
}

VALUE MSP::Body::rbf_set_material_id(VALUE self, VALUE v_body, VALUE v_id) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    int id = Util::value_to_int(v_id);
    body_data->m_material_id = id;
    NewtonBodySetMaterialGroupID(body, id);
    return Qnil;
}

VALUE MSP::Body::rbf_get_collision_scale(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    return Util::vector_to_value(MSP::Collision::s_valid_collisions[collision]->m_scale);
}

VALUE MSP::Body::rbf_set_collision_scale(VALUE self, VALUE v_body, VALUE v_scale) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    if (NewtonCollisionGetType(collision) > 6)
        rb_raise(rb_eTypeError, "Only convex collisions can be scaled!");
    dVector scale(Util::value_to_vector(v_scale));
    dVector& cscale = MSP::Collision::s_valid_collisions[collision]->m_scale;
    cscale.m_x = Util::clamp_dFloat(scale.m_x, 0.01f, 100.0);
    cscale.m_y = Util::clamp_dFloat(scale.m_y, 0.01f, 100.0);
    cscale.m_z = Util::clamp_dFloat(scale.m_z, 0.01f, 100.0);
    const dVector& dco = body_data->m_default_collision_offset;
    const dVector& dcs = body_data->m_default_collision_scale;
    dMatrix col_matrix;
    NewtonCollisionGetMatrix(collision, &col_matrix[0][0]);
    col_matrix.m_posit.m_x = dco.m_x * scale.m_x / dcs.m_x;
    col_matrix.m_posit.m_y = dco.m_y * scale.m_y / dcs.m_y;
    col_matrix.m_posit.m_z = dco.m_z * scale.m_z / dcs.m_z;
    NewtonCollisionSetMatrix(collision, &col_matrix[0][0]);
    NewtonBodySetCollisionScale(body, cscale.m_x, cscale.m_y, cscale.m_z);
    body_data->m_volume = Util::clamp_dFloat(NewtonConvexCollisionCalculateVolume(collision), MIN_VOLUME, MAX_VOLUME) * M_INCH3_TO_METER3;
    body_data->m_mass = Util::clamp_dFloat(body_data->m_density * body_data->m_volume, MIN_MASS, MAX_MASS);
    dVector com;
    NewtonBodyGetCentreOfMass(body, &com[0]);
    NewtonBodySetMassProperties(body, body_data->m_bstatic ? 0.0 : body_data->m_mass, collision);
    NewtonBodySetCentreOfMass(body, &com[0]);
    NewtonBodySetSleepState(body, 0);
    body_data->m_matrix_changed = true;
    return Qnil;
}

VALUE MSP::Body::rbf_get_default_collision_scale(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return Util::vector_to_value(body_data->m_default_collision_scale);
}

VALUE MSP::Body::rbf_get_actual_matrix_scale(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    const NewtonCollision* collision = NewtonBodyGetCollision(body);
    const dVector& dcs = body_data->m_default_collision_scale;
    const dVector& ms = body_data->m_matrix_scale;
    const dVector& cs = MSP::Collision::s_valid_collisions[collision]->m_scale;
    dVector actual_matrix_scale(ms.m_x * cs.m_x / dcs.m_x, ms.m_y * cs.m_y / dcs.m_y, ms.m_z * cs.m_z / dcs.m_z);
    return Util::vector_to_value(actual_matrix_scale);
}

VALUE MSP::Body::rbf_get_group(VALUE self, VALUE v_body) {
    const NewtonBody* body = c_value_to_body(v_body);
    BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(body));
    return body_data->m_group;
}

VALUE MSP::Body::rbf_get_body_by_group(VALUE self, VALUE v_world, VALUE v_group) {
    const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
    MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(world));
    std::map<VALUE, const NewtonBody*>::iterator it = world_data->m_group_to_body_map.find(v_group);
    if (it == world_data->m_group_to_body_map.end())
        return Qnil;
    else
        return c_body_to_value(it->second);
}

VALUE MSP::Body::rbf_get_body_data_by_group(VALUE self, VALUE v_world, VALUE v_group) {
    const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
    MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(world));
    std::map<VALUE, const NewtonBody*>::iterator it = world_data->m_group_to_body_map.find(v_group);
    if (it == world_data->m_group_to_body_map.end())
        return Qnil;
    else {
        BodyData* body_data = reinterpret_cast<BodyData*>(NewtonBodyGetUserData(it->second));
        return body_data->m_user_data;
    }
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::Body::init_ruby(VALUE mNewton) {
    VALUE mBody = rb_define_module_under(mNewton, "Body");

    rb_define_module_function(mBody, "is_valid?", VALUEFUNC(MSP::Body::rbf_is_valid), 1);
    rb_define_module_function(mBody, "create", VALUEFUNC(MSP::Body::rbf_create), 6);
    rb_define_module_function(mBody, "destroy", VALUEFUNC(MSP::Body::rbf_destroy), 1);
    rb_define_module_function(mBody, "get_type", VALUEFUNC(MSP::Body::rbf_get_type), 1);
    rb_define_module_function(mBody, "get_world", VALUEFUNC(MSP::Body::rbf_get_world), 1);
    rb_define_module_function(mBody, "get_collision", VALUEFUNC(MSP::Body::rbf_get_collision), 1);
    rb_define_module_function(mBody, "get_simulation_state", VALUEFUNC(MSP::Body::rbf_get_simulation_state), 1);
    rb_define_module_function(mBody, "set_simulation_state", VALUEFUNC(MSP::Body::rbf_set_simulation_state), 2);
    rb_define_module_function(mBody, "get_continuous_collision_state", VALUEFUNC(MSP::Body::rbf_get_continuous_collision_state), 1);
    rb_define_module_function(mBody, "set_continuous_collision_state", VALUEFUNC(MSP::Body::rbf_set_continuous_collision_state), 2);
    rb_define_module_function(mBody, "get_matrix", VALUEFUNC(MSP::Body::rbf_get_matrix), 1);
    rb_define_module_function(mBody, "get_normal_matrix", VALUEFUNC(MSP::Body::rbf_get_normal_matrix), 1);
    rb_define_module_function(mBody, "set_matrix", VALUEFUNC(MSP::Body::rbf_set_matrix), 2);
    rb_define_module_function(mBody, "get_position", VALUEFUNC(MSP::Body::rbf_get_position), 2);
    rb_define_module_function(mBody, "set_position", VALUEFUNC(MSP::Body::rbf_set_position), 3);
    rb_define_module_function(mBody, "get_rotation", VALUEFUNC(MSP::Body::rbf_get_rotation), 1);
    rb_define_module_function(mBody, "get_euler_angles", VALUEFUNC(MSP::Body::rbf_get_euler_angles), 1);
    rb_define_module_function(mBody, "set_euler_angles", VALUEFUNC(MSP::Body::rbf_set_euler_angles), 2);
    rb_define_module_function(mBody, "get_velocity", VALUEFUNC(MSP::Body::rbf_get_velocity), 1);
    rb_define_module_function(mBody, "set_velocity", VALUEFUNC(MSP::Body::rbf_set_velocity), 2);
    rb_define_module_function(mBody, "integrate_velocity", VALUEFUNC(MSP::Body::rbf_integrate_velocity), 2);
    rb_define_module_function(mBody, "get_omega", VALUEFUNC(MSP::Body::rbf_get_omega), 1);
    rb_define_module_function(mBody, "set_omega", VALUEFUNC(MSP::Body::rbf_set_omega), 2);
    rb_define_module_function(mBody, "get_centre_of_mass", VALUEFUNC(MSP::Body::rbf_get_centre_of_mass), 1);
    rb_define_module_function(mBody, "set_centre_of_mass", VALUEFUNC(MSP::Body::rbf_set_centre_of_mass), 2);
    rb_define_module_function(mBody, "get_inertia", VALUEFUNC(MSP::Body::rbf_get_inertia), 1);
    rb_define_module_function(mBody, "get_mass", VALUEFUNC(MSP::Body::rbf_get_mass), 1);
    rb_define_module_function(mBody, "set_mass", VALUEFUNC(MSP::Body::rbf_set_mass), 2);
    rb_define_module_function(mBody, "get_density", VALUEFUNC(MSP::Body::rbf_get_density), 1);
    rb_define_module_function(mBody, "set_density", VALUEFUNC(MSP::Body::rbf_set_density), 2);
    rb_define_module_function(mBody, "get_volume", VALUEFUNC(MSP::Body::rbf_get_volume), 1);
    rb_define_module_function(mBody, "set_volume", VALUEFUNC(MSP::Body::rbf_set_volume), 2);
    rb_define_module_function(mBody, "reset_mass_properties", VALUEFUNC(MSP::Body::rbf_reset_mass_properties), 2);
    rb_define_module_function(mBody, "is_static?", VALUEFUNC(MSP::Body::rbf_is_static), 1);
    rb_define_module_function(mBody, "set_static", VALUEFUNC(MSP::Body::rbf_set_static), 2);
    rb_define_module_function(mBody, "is_collidable?", VALUEFUNC(MSP::Body::rbf_is_collidable), 1);
    rb_define_module_function(mBody, "set_collidable", VALUEFUNC(MSP::Body::rbf_set_collidable), 2);
    rb_define_module_function(mBody, "is_frozen?", VALUEFUNC(MSP::Body::rbf_is_frozen), 1);
    rb_define_module_function(mBody, "set_frozen", VALUEFUNC(MSP::Body::rbf_set_frozen), 2);
    rb_define_module_function(mBody, "is_sleeping?", VALUEFUNC(MSP::Body::rbf_is_sleeping), 1);
    rb_define_module_function(mBody, "set_sleeping", VALUEFUNC(MSP::Body::rbf_set_sleeping), 2);
    rb_define_module_function(mBody, "get_auto_sleep_state", VALUEFUNC(MSP::Body::rbf_get_auto_sleep_state), 1);
    rb_define_module_function(mBody, "set_auto_sleep_state", VALUEFUNC(MSP::Body::rbf_set_auto_sleep_state), 2);
    rb_define_module_function(mBody, "is_non_collidable_with?", VALUEFUNC(MSP::Body::rbf_is_non_collidable_with), 2);
    rb_define_module_function(mBody, "set_non_collidable_with", VALUEFUNC(MSP::Body::rbf_set_non_collidable_with), 3);
    rb_define_module_function(mBody, "get_non_collidable_bodies", VALUEFUNC(MSP::Body::rbf_get_non_collidable_bodies), 1);
    rb_define_module_function(mBody, "clear_non_collidable_bodies", VALUEFUNC(MSP::Body::rbf_clear_non_collidable_bodies), 1);
    rb_define_module_function(mBody, "get_elasticity", VALUEFUNC(MSP::Body::rbf_get_elasticity), 1);
    rb_define_module_function(mBody, "set_elasticity", VALUEFUNC(MSP::Body::rbf_set_elasticity), 2);
    rb_define_module_function(mBody, "get_softness", VALUEFUNC(MSP::Body::rbf_get_softness), 1);
    rb_define_module_function(mBody, "set_softness", VALUEFUNC(MSP::Body::rbf_set_softness), 2);
    rb_define_module_function(mBody, "get_static_friction", VALUEFUNC(MSP::Body::rbf_get_static_friction), 1);
    rb_define_module_function(mBody, "set_static_friction", VALUEFUNC(MSP::Body::rbf_set_static_friction), 2);
    rb_define_module_function(mBody, "get_kinetic_friction", VALUEFUNC(MSP::Body::rbf_get_kinetic_friction), 1);
    rb_define_module_function(mBody, "set_kinetic_friction", VALUEFUNC(MSP::Body::rbf_set_kinetic_friction), 2);
    rb_define_module_function(mBody, "get_friction_state", VALUEFUNC(MSP::Body::rbf_get_friction_state), 1);
    rb_define_module_function(mBody, "set_friction_state", VALUEFUNC(MSP::Body::rbf_set_friction_state), 2);
    rb_define_module_function(mBody, "get_magnet_mode", VALUEFUNC(MSP::Body::rbf_get_magnet_mode), 1);
    rb_define_module_function(mBody, "set_magnet_mode", VALUEFUNC(MSP::Body::rbf_set_magnet_mode), 2);
    rb_define_module_function(mBody, "get_magnet_force", VALUEFUNC(MSP::Body::rbf_get_magnet_force), 1);
    rb_define_module_function(mBody, "set_magnet_force", VALUEFUNC(MSP::Body::rbf_set_magnet_force), 2);
    rb_define_module_function(mBody, "get_magnet_range", VALUEFUNC(MSP::Body::rbf_get_magnet_range), 1);
    rb_define_module_function(mBody, "set_magnet_range", VALUEFUNC(MSP::Body::rbf_set_magnet_range), 2);
    rb_define_module_function(mBody, "get_magnet_strength", VALUEFUNC(MSP::Body::rbf_get_magnet_strength), 1);
    rb_define_module_function(mBody, "set_magnet_strength", VALUEFUNC(MSP::Body::rbf_set_magnet_strength), 2);
    rb_define_module_function(mBody, "is_magnetic?", VALUEFUNC(MSP::Body::rbf_is_magnetic), 1);
    rb_define_module_function(mBody, "set_magnetic", VALUEFUNC(MSP::Body::rbf_set_magnetic), 2);
    rb_define_module_function(mBody, "get_aabb", VALUEFUNC(MSP::Body::rbf_get_aabb), 1);
    rb_define_module_function(mBody, "get_linear_damping", VALUEFUNC(MSP::Body::rbf_get_linear_damping), 1);
    rb_define_module_function(mBody, "set_linear_damping", VALUEFUNC(MSP::Body::rbf_set_linear_damping), 2);
    rb_define_module_function(mBody, "get_angular_damping", VALUEFUNC(MSP::Body::rbf_get_angular_damping), 1);
    rb_define_module_function(mBody, "set_angular_damping", VALUEFUNC(MSP::Body::rbf_set_angular_damping), 2);
    rb_define_module_function(mBody, "get_point_velocity", VALUEFUNC(MSP::Body::rbf_get_point_velocity), 2);
    rb_define_module_function(mBody, "add_point_force", VALUEFUNC(MSP::Body::rbf_add_point_force), 3);
    rb_define_module_function(mBody, "add_impulse", VALUEFUNC(MSP::Body::rbf_add_impulse), 4);
    rb_define_module_function(mBody, "get_force", VALUEFUNC(MSP::Body::rbf_get_force), 1);
    rb_define_module_function(mBody, "add_force", VALUEFUNC(MSP::Body::rbf_add_force), 2);
    rb_define_module_function(mBody, "set_force", VALUEFUNC(MSP::Body::rbf_set_force), 2);
    rb_define_module_function(mBody, "get_torque", VALUEFUNC(MSP::Body::rbf_get_torque), 1);
    rb_define_module_function(mBody, "add_torque", VALUEFUNC(MSP::Body::rbf_add_torque), 2);
    rb_define_module_function(mBody, "set_torque", VALUEFUNC(MSP::Body::rbf_set_torque), 2);
    rb_define_module_function(mBody, "get_net_contact_force", VALUEFUNC(MSP::Body::rbf_get_net_contact_force), 1);
    rb_define_module_function(mBody, "get_net_joint_tension1", VALUEFUNC(MSP::Body::rbf_get_net_joint_tension1), 1);
    rb_define_module_function(mBody, "get_net_joint_tension2", VALUEFUNC(MSP::Body::rbf_get_net_joint_tension2), 1);
    rb_define_module_function(mBody, "get_contacts", VALUEFUNC(MSP::Body::rbf_get_contacts), 2);
    rb_define_module_function(mBody, "get_touching_bodies", VALUEFUNC(MSP::Body::rbf_get_touching_bodies), 2);
    rb_define_module_function(mBody, "get_contact_points", VALUEFUNC(MSP::Body::rbf_get_contact_points), 2);
    rb_define_module_function(mBody, "get_collision_faces", VALUEFUNC(MSP::Body::rbf_get_collision_faces), 1);
    rb_define_module_function(mBody, "get_collision_faces2", VALUEFUNC(MSP::Body::rbf_get_collision_faces2), 1);
    rb_define_module_function(mBody, "get_collision_faces3", VALUEFUNC(MSP::Body::rbf_get_collision_faces3), 1);
    rb_define_module_function(mBody, "apply_pick_and_drag", VALUEFUNC(MSP::Body::rbf_apply_pick_and_drag), 6);
    rb_define_module_function(mBody, "apply_buoyancy", VALUEFUNC(MSP::Body::rbf_apply_buoyancy), 9);
    rb_define_module_function(mBody, "apply_aerodynamics", VALUEFUNC(MSP::Body::rbf_apply_aerodynamics), 3);
    rb_define_module_function(mBody, "copy", VALUEFUNC(MSP::Body::rbf_copy), 5);
    rb_define_module_function(mBody, "get_destructor_proc", VALUEFUNC(MSP::Body::rbf_get_destructor_proc), 1);
    rb_define_module_function(mBody, "set_destructor_proc", VALUEFUNC(MSP::Body::rbf_set_destructor_proc), 2);
    rb_define_module_function(mBody, "get_user_data", VALUEFUNC(MSP::Body::rbf_get_user_data), 1);
    rb_define_module_function(mBody, "set_user_data", VALUEFUNC(MSP::Body::rbf_set_user_data), 2);
    rb_define_module_function(mBody, "get_record_touch_data_state", VALUEFUNC(MSP::Body::rbf_get_record_touch_data_state), 1);
    rb_define_module_function(mBody, "set_record_touch_data_state", VALUEFUNC(MSP::Body::rbf_set_record_touch_data_state), 2);
    rb_define_module_function(mBody, "get_matrix_scale", VALUEFUNC(MSP::Body::rbf_get_matrix_scale), 1);
    rb_define_module_function(mBody, "set_matrix_scale", VALUEFUNC(MSP::Body::rbf_set_matrix_scale), 2);
    rb_define_module_function(mBody, "matrix_changed?", VALUEFUNC(MSP::Body::rbf_matrix_changed), 1);
    rb_define_module_function(mBody, "enable_gravity", VALUEFUNC(MSP::Body::rbf_enable_gravity), 2);
    rb_define_module_function(mBody, "is_gravity_enabled?", VALUEFUNC(MSP::Body::rbf_is_gravity_enabled), 1);
    rb_define_module_function(mBody, "get_contained_joints", VALUEFUNC(MSP::Body::rbf_get_contained_joints), 1);
    rb_define_module_function(mBody, "get_connected_joints", VALUEFUNC(MSP::Body::rbf_get_connected_joints), 1);
    rb_define_module_function(mBody, "get_connected_bodies", VALUEFUNC(MSP::Body::rbf_get_connected_bodies), 1);
    rb_define_module_function(mBody, "get_material_id", VALUEFUNC(MSP::Body::rbf_get_material_id), 1);
    rb_define_module_function(mBody, "set_material_id", VALUEFUNC(MSP::Body::rbf_set_material_id), 2);
    rb_define_module_function(mBody, "get_collision_scale", VALUEFUNC(MSP::Body::rbf_get_collision_scale), 1);
    rb_define_module_function(mBody, "set_collision_scale", VALUEFUNC(MSP::Body::rbf_set_collision_scale), 2);
    rb_define_module_function(mBody, "get_default_collision_scale", VALUEFUNC(MSP::Body::rbf_get_default_collision_scale), 1);
    rb_define_module_function(mBody, "get_actual_matrix_scale", VALUEFUNC(MSP::Body::rbf_get_actual_matrix_scale), 1);
    rb_define_module_function(mBody, "get_group", VALUEFUNC(MSP::Body::rbf_get_group), 1);
    rb_define_module_function(mBody, "get_body_by_group", VALUEFUNC(MSP::Body::rbf_get_body_by_group), 2);
    rb_define_module_function(mBody, "get_body_data_by_group", VALUEFUNC(MSP::Body::rbf_get_body_data_by_group), 2);
}
