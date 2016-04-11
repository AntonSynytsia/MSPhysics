#include "msp_body.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Body::destructor_callback(const NewtonBody* const body) {
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	c_clear_non_collidable_bodies(body);
	valid_bodies.erase(body);
	body_data->non_collidable_bodies.clear();
	body_data->touchers.clear();
	if (RARRAY_LEN(body_data->destructor_proc) == 1 && rb_ary_entry(body_data->destructor_proc, 0) != Qnil)
		rb_rescue2(RUBY_METHOD_FUNC(Util::call_proc), rb_ary_entry(body_data->destructor_proc, 0), RUBY_METHOD_FUNC(Util::rescue_proc), Qnil, rb_eException, (VALUE)0);
	rb_ary_clear(body_data->destructor_proc);
	rb_ary_clear(body_data->user_data);
	rb_gc_unregister_address(&body_data->destructor_proc);
	rb_gc_unregister_address(&body_data->user_data);
	#ifndef RUBY_VERSION18
		rb_ary_free(body_data->destructor_proc);
		rb_ary_free(body_data->user_data);
	#endif
	delete body_data;
}

void MSNewton::Body::transform_callback(const NewtonBody* const body, const dFloat* const matrix, int thread_index) {
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	body_data->matrix_changed = true;
}

void MSNewton::Body::force_and_torque_callback(const NewtonBody* const body, dFloat timestep, int thread_index) {
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	const NewtonCollision* collision = NewtonBodyGetCollision(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);

	if (body_data->gravity_enabled) {
		dFloat mass, ixx, iyy, izz;
		NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);
		dVector force = world_data->gravity.Scale(mass);
		NewtonBodySetForce(body, &force[0]);
	}

	if (!body_data->pick_and_drag.empty()) {
		for (std::vector<PickAndDragData>::iterator it = body_data->pick_and_drag.begin(); it != body_data->pick_and_drag.end(); ++it) {
			dMatrix matrix;
			NewtonBodyGetMatrix(body, &matrix[0][0]);
			dVector pick_pt = matrix.TransformVector((*it).loc_pick_pt);
			dVector velocity;
			NewtonBodyGetVelocity(body, &velocity[0]);
			dVector com;
			NewtonBodyGetCentreOfMass(body, &com[0]);
			// Calculate force
			dVector force = ((*it).dest_pt - pick_pt).Scale(body_data->mass * (*it).stiffness);
			force -= velocity.Scale(body_data->mass * (*it).damper);
			// Calculate torque
			dVector point = matrix.RotateVector((*it).loc_pick_pt - com);
			dVector torque = point * force;
			// Add force and torque
			NewtonBodyAddForce(body, &force[0]);
			NewtonBodyAddTorque(body, &torque[0]);
		}
		if (world_data->process_info) body_data->pick_and_drag.clear();
	}
	if (!body_data->buoyancy.empty()) {
		for (std::vector<BuoyancyData>::iterator it = body_data->buoyancy.begin(); it != body_data->buoyancy.end(); ++it) {
			dMatrix matrix;
			NewtonBodyGetMatrix(body, &matrix[0][0]);
			dVector com;
			NewtonBodyGetCentreOfMass(body, &com[0]);
			com = matrix.TransformVector(com);
			dVector force, torque;
			NewtonConvexCollisionCalculateBuoyancyAcceleration(collision, &matrix[0][0], &com[0], &world_data->gravity[0], &(*it).plane[0], (*it).density, 0.0f, &force[0], &torque[0]);
			if (Util::get_vector_magnitude(force) > EPSILON) {
				NewtonBodyAddForce(body, &(force + (*it).current.Scale(body_data->mass))[0]);
				NewtonBodyAddTorque(body, &torque[0]);
				if ((*it).angular_ratio > EPSILON) {
					dVector omega;
					NewtonBodyGetOmega(body, &omega[0]);
					omega = omega.Scale((*it).angular_ratio);
					NewtonBodySetOmega(body, &omega[0]);
				}
				if ((*it).linear_ratio > EPSILON) {
					dVector velocity;
					NewtonBodyGetVelocity(body, &velocity[0]);
					velocity = velocity.Scale((*it).linear_ratio);
					NewtonBodySetVelocity(body, &velocity[0]);
				}
			}
		}
		if (world_data->process_info) body_data->buoyancy.clear();
	}

	if (body_data->add_force_state) {
		NewtonBodyAddForce(body, &body_data->add_force[0]);
		body_data->add_force[0] = 0.0f;
		body_data->add_force[1] = 0.0f;
		body_data->add_force[2] = 0.0f;
		body_data->add_force_state = false;
	}
	if (body_data->add_torque_state) {
		NewtonBodyAddTorque(body, &body_data->add_torque[0]);
		body_data->add_torque[0] = 0.0f;
		body_data->add_torque[1] = 0.0f;
		body_data->add_torque[2] = 0.0f;
		body_data->add_torque_state = false;
	}
	if (body_data->add_force2_state) {
		NewtonBodyAddForce(body, &body_data->add_force2[0]);
		if (world_data->process_info) {
			body_data->add_force2[0] = 0.0f;
			body_data->add_force2[1] = 0.0f;
			body_data->add_force2[2] = 0.0f;
			body_data->add_force2_state = false;
		}
	}
	if (body_data->add_torque2_state) {
		NewtonBodyAddTorque(body, &body_data->add_torque2[0]);
		if (world_data->process_info) {
			body_data->add_torque2[0] = 0.0f;
			body_data->add_torque2[1] = 0.0f;
			body_data->add_torque2[2] = 0.0f;
			body_data->add_torque2_state = false;
		}
	}
	if (body_data->set_force_state) {
		NewtonBodySetForce(body, &body_data->set_force[0]);
		body_data->set_force[0] = 0.0f;
		body_data->set_force[1] = 0.0f;
		body_data->set_force[2] = 0.0f;
		body_data->set_force_state = false;
	}
	if (body_data->set_torque_state) {
		NewtonBodySetTorque(body, &body_data->set_torque[0]);
		body_data->set_torque[0] = 0.0f;
		body_data->set_torque[1] = 0.0f;
		body_data->set_torque[2] = 0.0f;
		body_data->set_torque_state = false;
	}
	if (body_data->set_force2_state) {
		NewtonBodySetForce(body, &body_data->set_force2[0]);
		if (world_data->process_info) {
			body_data->set_force2[0] = 0.0f;
			body_data->set_force2[1] = 0.0f;
			body_data->set_force2[2] = 0.0f;
			body_data->set_force2_state = false;
		}
	}
	if (body_data->set_torque2_state) {
		NewtonBodySetTorque(body, &body_data->set_torque2[0]);
		if (world_data->process_info) {
			body_data->set_torque2[0] = 0.0f;
			body_data->set_torque2[1] = 0.0f;
			body_data->set_torque2[2] = 0.0f;
			body_data->set_torque2_state = false;
		}
	}
}

void MSNewton::Body::collision_iterator(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id) {
	CollisionIteratorData* data = (CollisionIteratorData*)(user_data);
	VALUE v_face = rb_ary_new2(vertex_count);
	for (int i = 0; i < vertex_count; ++i) {
		dVector vertex(face_array[i*3+0], face_array[i*3+1], face_array[i*3+2]);
		rb_ary_store(v_face, i, Util::point_to_value(vertex, data->scale));
	}
	rb_ary_push(data->faces, v_face);
}

void MSNewton::Body::collision_iterator2(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id) {
	CollisionIteratorData* data = (CollisionIteratorData*)(user_data);
	// Store face vertices
	VALUE v_face = rb_ary_new2(vertex_count);
	for (int i = 0; i < vertex_count; ++i) {
		dVector vertex(face_array[i*3+0], face_array[i*3+1], face_array[i*3+2]);
		rb_ary_store(v_face, i, Util::point_to_value(vertex, data->scale));
	}
	// Calculate face normal
	dVector p1(face_array[0], face_array[1], face_array[2]);
	dVector p2(face_array[3], face_array[4], face_array[5]);
	dVector p3(face_array[6], face_array[7], face_array[8]);
	dVector u = p2 - p1;
	dVector v = p3 - p1;
	dVector n = u * v;
	Util::normalize_vector(n);
	VALUE v_face_normal = Util::vector_to_value(n);
	// Transform points with respect to face normal
	std::vector<dVector> vertices;
	dMatrix face_matrix = Util::matrix_from_pin_dir(ORIGIN, n);
	for (int i = 0; i < vertex_count; ++i) {
		dVector vertex = face_matrix.UntransformVector(dVector(face_array[i*3+0], face_array[i*3+1], face_array[i*3+2]));
		vertices.push_back(vertex);
	}
	// Calculate face area and centroid
	dVector centroid(0.0f, 0.0f, vertices[0].m_z);
	dFloat signed_area = 0.0f;
	for (int i = 0; i < vertex_count; ++i) {
		dFloat x0 = vertices[i].m_x;
		dFloat y0 = vertices[i].m_y;
		dFloat x1 = vertices[(i+1) % vertex_count].m_x;
		dFloat y1 = vertices[(i+1) % vertex_count].m_y;
		dFloat a = x0*y1 - x1*y0;
		signed_area += a;
		centroid.m_x += (x0 + x1)*a;
		centroid.m_y += (y0 + y1)*a;
	}
	signed_area *= 0.5f;
	centroid.m_x /= 6.0f * signed_area;
	centroid.m_y /= 6.0f * signed_area;
	VALUE v_face_centroid = Util::point_to_value(face_matrix.TransformVector(centroid), data->scale);
	VALUE v_face_area = Util::to_value(dAbs(signed_area) * data->scale * data->scale * METER_TO_INCH * METER_TO_INCH);
	// Store face data
	rb_ary_push(data->faces, rb_ary_new3(4, v_face, v_face_centroid, v_face_normal, v_face_area));
}

void MSNewton::Body::collision_iterator3(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id) {
	CollisionIteratorData* data = (CollisionIteratorData*)(user_data);
	// Calculate face normal
	dVector p1(face_array[0], face_array[1], face_array[2]);
	dVector p2(face_array[3], face_array[4], face_array[5]);
	dVector p3(face_array[6], face_array[7], face_array[8]);
	dVector u = p2 - p1;
	dVector v = p3 - p1;
	dVector n = u * v;
	Util::normalize_vector(n);
	VALUE v_face_normal = Util::vector_to_value(n);
	// Transform points with respect to face normal
	std::vector<dVector> vertices;
	dMatrix face_matrix = Util::matrix_from_pin_dir(ORIGIN, n);
	for (int i = 0; i < vertex_count; ++i) {
		dVector vertex = face_matrix.UntransformVector(dVector(face_array[i*3+0], face_array[i*3+1], face_array[i*3+2]));
		vertices.push_back(vertex);
	}
	// Calculate face area and centroid
	dVector centroid(0.0f, 0.0f, vertices[0].m_z);
	dFloat signed_area = 0.0f;
	for (int i = 0; i < vertex_count; ++i) {
		dFloat x0 = vertices[i].m_x;
		dFloat y0 = vertices[i].m_y;
		dFloat x1 = vertices[(i+1) % vertex_count].m_x;
		dFloat y1 = vertices[(i+1) % vertex_count].m_y;
		dFloat a = x0*y1 - x1*y0;
		signed_area += a;
		centroid.m_x += (x0 + x1)*a;
		centroid.m_y += (y0 + y1)*a;
	}
	signed_area *= 0.5f;
	centroid.m_x /= 6.0f * signed_area;
	centroid.m_y /= 6.0f * signed_area;
	VALUE v_face_centroid = Util::point_to_value(face_matrix.TransformVector(centroid), data->scale);
	VALUE v_face_area = Util::to_value(dAbs(signed_area) * data->scale * data->scale * METER_TO_INCH * METER_TO_INCH);
	// Store face data
	rb_ary_push(data->faces, rb_ary_new3(3, v_face_centroid, v_face_normal, v_face_area));
}

void MSNewton::Body::collision_iterator4(void* const user_data, int vertex_count, const dFloat* const face_array, int face_id) {
	CollisionIteratorData2* data = (CollisionIteratorData2*)(user_data);
	// Calculate face normal
	dVector p1(face_array[0], face_array[1], face_array[2]);
	dVector p2(face_array[3], face_array[4], face_array[5]);
	dVector p3(face_array[6], face_array[7], face_array[8]);
	dVector normal = (p2 - p1) * (p3 - p1);
	dFloat normal_mag = Util::get_vector_magnitude(normal);
	if (normal_mag < EPSILON) return;
	Util::normalize_vector(normal);
	// Transform points with respect to face normal
	std::vector<dVector> vertices;
	dMatrix face_matrix = Util::matrix_from_pin_dir(ORIGIN, normal);
	for (int i = 0; i < vertex_count; ++i) {
		dVector vertex = face_matrix.UntransformVector(dVector(face_array[i*3+0], face_array[i*3+1], face_array[i*3+2]));
		vertices.push_back(vertex);
	}
	// Calculate face area and centroid
	dVector centroid(0.0f, 0.0f, vertices[0].m_z);
	dFloat signed_area = 0.0f;
	for (int i = 0; i < vertex_count; ++i) {
		dFloat x0 = vertices[i].m_x;
		dFloat y0 = vertices[i].m_y;
		dFloat x1 = vertices[(i+1) % vertex_count].m_x;
		dFloat y1 = vertices[(i+1) % vertex_count].m_y;
		dFloat a = x0*y1 - x1*y0;
		signed_area += a;
		centroid.m_x += (x0 + x1)*a;
		centroid.m_y += (y0 + y1)*a;
	}
	signed_area *= 0.5f;
	centroid.m_x /= 6.0f * signed_area;
	centroid.m_y /= 6.0f * signed_area;
	centroid = face_matrix.TransformVector(centroid);
	// Calculate point force
	dVector point_veloc;
	NewtonBodyGetPointVelocity(data->body, &centroid[0], &point_veloc[0]);
	if (!Util::is_vector_valid(point_veloc)) return;
	dFloat veloc_mag = Util::get_vector_magnitude(point_veloc);
	if (veloc_mag < EPSILON) return;
	dFloat cos_theta = (normal % point_veloc) / veloc_mag;
	if (cos_theta < EPSILON) return;
	Util::normalize_vector(point_veloc);
	dMatrix veloc_matrix = Util::matrix_from_pin_dir(ORIGIN, point_veloc);
	dVector loc_normal = veloc_matrix.UnrotateVector(normal);
	//if (loc_normal.m_z < EPSILON) return;
	dVector point_force(
		-loc_normal.m_x * dAbs(signed_area) * veloc_mag * data->density,
		-loc_normal.m_y * dAbs(signed_area) * veloc_mag * data->density,
		-loc_normal.m_z * dAbs(signed_area) * veloc_mag * data->density);
	point_force = veloc_matrix.RotateVector(point_force);
	//dVector loc_point_force(0.0f, 0.0f, -loc_normal.m_z * dAbs(signed_area) * data->density * veloc_mag);
	//dVector point_force = veloc_matrix.RotateVector(loc_point_force);
	// Add force and torque
	data->force += point_force;
	dVector r = centroid - data->centre;
	data->torque += r * point_force;
	// Add aditional rotational force.
	if (1.0f - cos_theta < EPSILON) return;
	dVector side_force = normal * point_veloc;
	data->torque += r * side_force.Scale(dAbs(signed_area) * veloc_mag * data->density);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::Body::c_clear_non_collidable_bodies(const NewtonBody* body) {
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	for (std::map<const NewtonBody*, bool>::iterator it = data->non_collidable_bodies.begin(); it != data->non_collidable_bodies.end(); ++it) {
		const NewtonBody* other_body = it->first;
		if (valid_bodies.find(other_body) != valid_bodies.end()) {
			BodyData* other_data = (BodyData*)NewtonBodyGetUserData(other_body);
			if (other_data->non_collidable_bodies.find(body) != other_data->non_collidable_bodies.end())
				other_data->non_collidable_bodies.erase(body);
		}
	}
	data->non_collidable_bodies.clear();
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::Body::is_valid(VALUE self, VALUE v_body) {
	return Util::is_body_valid((const NewtonBody*)Util::value_to_ll(v_body)) ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::create_dynamic(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_id) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	const NewtonCollision* collision = Util::value_to_collision(v_collision);
	dFloat sx, sy, sz;
	NewtonCollisionGetScale(collision, &sx, &sy, &sz);
	dMatrix col_matrix;
	NewtonCollisionGetMatrix(collision, &col_matrix[0][0]);
	dMatrix matrix = Util::value_to_matrix(v_matrix, world_data->scale);
	dVector scale = Util::get_matrix_scale(matrix);
	int id = Util::value_to_int(v_id);
	if (Util::is_matrix_flipped(matrix)) {
		matrix.m_front.m_x *= -1;
		matrix.m_front.m_y *= -1;
		matrix.m_front.m_z *= -1;
		scale.m_x *= -1;
	}
	Util::extract_matrix_scale(matrix);
	NewtonBody* body = NewtonCreateDynamicBody(world, collision, &matrix[0][0]);
	valid_bodies[body] = true;
	BodyData* data = new BodyData;
	int col_type = NewtonCollisionGetType(collision);
	if (col_type == SERIALIZE_ID_NULL)
		data->volume = 1.0f;
	else
		data->volume = NewtonConvexCollisionCalculateVolume(collision);
	if (data->volume < MIN_VOLUME) {
		data->dynamic = false;
		data->bstatic = true;
		data->density = 0.0f;
		data->mass = 0.0f;
	}
	else {
		data->dynamic = true;
		data->bstatic = false;
		data->density = DEFAULT_DENSITY;
		data->mass = data->volume * data->density;
	}
	NewtonBodySetMassProperties(body, data->bstatic ? 0.0f : Util::clamp(data->mass, MIN_MASS, MAX_MASS), collision);
	data->elasticity = DEFAULT_ELASTICITY;
	data->softness = DEFAULT_SOFTNESS;
	data->static_friction = DEFAULT_STATIC_FRICTION;
	data->dynamic_friction = DEFAULT_DYNAMIC_FRICTION;
	data->friction_enabled = DEFAULT_ENABLE_FRICTION;
	data->add_force = dVector(0.0f, 0.0f, 0.0f);
	data->add_force_state = false;
	data->set_force = dVector(0.0f, 0.0f, 0.0f);
	data->set_force_state = false;
	data->add_torque = dVector(0.0f, 0.0f, 0.0f);
	data->add_torque_state = false;
	data->set_torque = dVector(0.0f, 0.0f, 0.0f);
	data->set_torque_state = false;
	data->add_force2 = dVector(0.0f, 0.0f, 0.0f);
	data->add_force2_state = false;
	data->set_force2 = dVector(0.0f, 0.0f, 0.0f);
	data->set_force2_state = false;
	data->add_torque2 = dVector(0.0f, 0.0f, 0.0f);
	data->add_torque2_state = false;
	data->set_torque2 = dVector(0.0f, 0.0f, 0.0f);
	data->set_torque2_state = false;
	data->collidable = true;
	data->record_touch_data = false;
	data->magnet_force = 0.0f;
	data->magnet_range = 0.0f;
	data->magnetic = false;
	data->destructor_proc = rb_ary_new();
	data->user_data = rb_ary_new();
	data->non_collidable_bodies;
	data->touchers;
	data->matrix_scale = scale;
	data->default_collision_scale = dVector(sx, sy, sz);
	data->collision_scale = dVector(sx, sy, sz);
	data->default_collision_offset = col_matrix.m_posit;
	data->matrix_changed = false;
	data->gravity_enabled = true;
	data->material_id = id;
	data->pick_and_drag;
	data->buoyancy;
	NewtonBodySetUserData(body, data);
	rb_gc_register_address(&data->destructor_proc);
	rb_gc_register_address(&data->user_data);
	NewtonBodySetForceAndTorqueCallback(body, force_and_torque_callback);
	NewtonBodySetDestructorCallback(body, destructor_callback);
	NewtonBodySetTransformCallback(body, transform_callback);
	NewtonBodySetMaterialGroupID(body, data->material_id);
	return Util::to_value(body);
}

VALUE MSNewton::Body::destroy(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	NewtonDestroyBody(body);
	return Qnil;
}

VALUE MSNewton::Body::get_world(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	return Util::to_value( NewtonBodyGetWorld(body) );
}

VALUE MSNewton::Body::get_collision(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	return Util::to_value( NewtonBodyGetCollision(body) );
}

VALUE MSNewton::Body::get_simulation_state(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	return NewtonBodyGetSimulationState(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::set_simulation_state(VALUE self, VALUE v_body, VALUE v_state) {
	const NewtonBody* body = Util::value_to_body(v_body);
	NewtonBodySetSimulationState(body, RTEST(v_state) ? 1 : 0);
	return NewtonBodyGetSimulationState(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::get_continuous_collision_state(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	return NewtonBodyGetContinuousCollisionMode(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::set_continuous_collision_state(VALUE self, VALUE v_body, VALUE v_state) {
	const NewtonBody* body = Util::value_to_body(v_body);
	NewtonBodySetContinuousCollisionMode(body, Util::value_to_bool(v_state) ? 1 : 0);
	return NewtonBodyGetContinuousCollisionMode(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::get_matrix(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	//const NewtonCollision* collision = NewtonBodyGetCollision(body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	const dVector& dcs = body_data->default_collision_scale;
	const dVector& ms = body_data->matrix_scale;
	//dFloat csx, csy, csz;
	//NewtonCollisionGetScale(collision, &csx, &csy, &csz);
	dFloat csx = body_data->collision_scale.m_x;
	dFloat csy = body_data->collision_scale.m_y;
	dFloat csz = body_data->collision_scale.m_z;
	dVector actual_matrix_scale(ms.m_x * csx / dcs.m_x, ms.m_y * csy / dcs.m_y, ms.m_z * csz / dcs.m_z);
	Util::set_matrix_scale(matrix, actual_matrix_scale);
	return Util::matrix_to_value(matrix, world_data->inverse_scale);
}

VALUE MSNewton::Body::get_normal_matrix(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	if (body_data->matrix_scale.m_x < 0) {
		matrix.m_front.m_x *= -1;
		matrix.m_front.m_y *= -1;
		matrix.m_front.m_z *= -1;
	}
	return Util::matrix_to_value(matrix, world_data->inverse_scale);
}

VALUE MSNewton::Body::set_matrix(VALUE self, VALUE v_body, VALUE v_matrix) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	//const NewtonCollision* collision = NewtonBodyGetCollision(body);
	dMatrix matrix = Util::value_to_matrix(v_matrix, world_data->scale);
	if (Util::is_matrix_flipped(matrix)) {
		matrix.m_front.m_x *= -1;
		matrix.m_front.m_y *= -1;
		matrix.m_front.m_z *= -1;
	}
	Util::extract_matrix_scale(matrix);
	NewtonBodySetMatrix(body, &matrix[0][0]);
	const dVector& dcs = body_data->default_collision_scale;
	const dVector& ms = body_data->matrix_scale;
	//dFloat csx, csy, csz;
	//NewtonCollisionGetScale(collision, &csx, &csy, &csz);
	dFloat csx = body_data->collision_scale.m_x;
	dFloat csy = body_data->collision_scale.m_y;
	dFloat csz = body_data->collision_scale.m_z;
	dVector actual_matrix_scale(ms.m_x * csx / dcs.m_x, ms.m_y * csy / dcs.m_y, ms.m_z * csz / dcs.m_z);
	Util::set_matrix_scale(matrix, actual_matrix_scale);
	return Util::matrix_to_value(matrix, world_data->inverse_scale);
}

VALUE MSNewton::Body::get_position(VALUE self, VALUE v_body, VALUE v_mode) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	if (Util::value_to_int(v_mode) == 0)
		return Util::point_to_value(dVector(matrix[3][0], matrix[3][1], matrix[3][2]), world_data->inverse_scale);
	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	return Util::point_to_value(matrix.TransformVector(com), world_data->inverse_scale);
}

VALUE MSNewton::Body::set_position(VALUE self, VALUE v_body, VALUE v_position, VALUE v_mode) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector position = Util::value_to_point(v_position, world_data->scale);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	if (Util::value_to_int(v_mode) == 1) {
		dVector com;
		NewtonBodyGetCentreOfMass(body, &com[0]);
		position = matrix.TransformVector(matrix.UntransformVector(position) - com);
	}
	matrix[3] = position;
	NewtonBodySetMatrix(body, &matrix[0][0]);
	return Util::point_to_value(position, world_data->inverse_scale);
}

VALUE MSNewton::Body::get_rotation(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	dFloat rotation[4];
	NewtonBodyGetRotation(body, rotation);
	VALUE v_rotation = rb_ary_new2(4);
	for (int i = 0; i < 4; ++i)
		rb_ary_store(v_rotation, i, Util::to_value(rotation[i]));
	return v_rotation;
}

VALUE MSNewton::Body::get_euler_angles(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	dVector angles0;
	dVector angles1;
	NewtonGetEulerAngle(&matrix[0][0], &angles0[0], &angles1[0]);
	return Util::vector_to_value(angles0);
}

VALUE MSNewton::Body::set_euler_angles(VALUE self, VALUE v_body, VALUE v_angles) {
	const NewtonBody* body = Util::value_to_body(v_body);
	dVector angles = Util::value_to_vector(v_angles);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	NewtonSetEulerAngle(&angles[0], &matrix[0][0]);
	NewtonBodySetMatrix(body, &matrix[0][0]);
	dVector angles0;
	dVector angles1;
	NewtonGetEulerAngle(&matrix[0][0], &angles0[0], &angles1[0]);
	return Util::vector_to_value(angles0);
}

VALUE MSNewton::Body::get_velocity(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector velocity;
	NewtonBodyGetVelocity(body, &velocity[0]);
	return Util::vector_to_value(velocity, world_data->inverse_scale);
}

VALUE MSNewton::Body::set_velocity(VALUE self, VALUE v_body, VALUE v_velocity) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector velocity = Util::value_to_vector(v_velocity, world_data->scale);
	NewtonBodySetVelocity(body, &velocity[0]);
	return Util::vector_to_value(velocity, world_data->inverse_scale);
}

VALUE MSNewton::Body::get_omega(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	dVector omega;
	NewtonBodyGetOmega(body, &omega[0]);
	return Util::vector_to_value(omega);
}

VALUE MSNewton::Body::set_omega(VALUE self, VALUE v_body, VALUE v_omega) {
	const NewtonBody* body = Util::value_to_body(v_body);
	dVector omega = Util::value_to_vector(v_omega);
	NewtonBodySetOmega(body, &omega[0]);
	return Util::vector_to_value(omega);
}

VALUE MSNewton::Body::get_centre_of_mass(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	com.m_x /= data->matrix_scale.m_x;
	com.m_y /= data->matrix_scale.m_y;
	com.m_z /= data->matrix_scale.m_z;
	return Util::point_to_value(com, world_data->inverse_scale);
}

VALUE MSNewton::Body::set_centre_of_mass(VALUE self, VALUE v_body, VALUE v_com) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector com = Util::value_to_point(v_com, world_data->scale);
	com.m_x *= data->matrix_scale.m_x;
	com.m_y *= data->matrix_scale.m_y;
	com.m_z *= data->matrix_scale.m_z;
	NewtonBodySetCentreOfMass(body, &com[0]);
	com.m_x /= data->matrix_scale.m_x;
	com.m_y /= data->matrix_scale.m_y;
	com.m_z /= data->matrix_scale.m_z;
	return Util::point_to_value(com, world_data->inverse_scale);
}

VALUE MSNewton::Body::get_mass(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value(body_data->mass * world_data->inverse_scale3);
}

VALUE MSNewton::Body::set_mass(VALUE self, VALUE v_body, VALUE v_mass) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (!body_data->dynamic) return Qnil;
	dFloat mass = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_mass) * world_data->scale3, MIN_MASS);
	body_data->mass = mass;
	body_data->density = mass / body_data->volume;
	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	NewtonBodySetMassProperties(body, body_data->bstatic ? 0.0f : Util::clamp(body_data->mass, MIN_MASS, MAX_MASS), NewtonBodyGetCollision(body));
	NewtonBodySetCentreOfMass(body, &com[0]);
	return Util::to_value(body_data->mass * world_data->inverse_scale3);
}

VALUE MSNewton::Body::get_density(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(body_data->density);
}

VALUE MSNewton::Body::set_density(VALUE self, VALUE v_body, VALUE v_density) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	if (!body_data->dynamic) return Qnil;
	body_data->density = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_density), MIN_DENSITY);
	body_data->mass = body_data->density * body_data->volume;
	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	NewtonBodySetMassProperties(body, body_data->bstatic ? 0.0f : Util::clamp(body_data->mass, MIN_MASS, MAX_MASS), NewtonBodyGetCollision(body));
	NewtonBodySetCentreOfMass(body, &com[0]);
	return Util::to_value(body_data->density);
}

VALUE MSNewton::Body::get_volume(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value(body_data->volume * world_data->inverse_scale3);
}

VALUE MSNewton::Body::set_volume(VALUE self, VALUE v_body, VALUE v_volume) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (!body_data->dynamic) return Qnil;
	body_data->volume = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_volume) * world_data->scale3, MIN_VOLUME);
	body_data->mass = body_data->density * body_data->volume;
	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	NewtonBodySetMassProperties(body, body_data->bstatic ? 0.0f : Util::clamp(body_data->mass, MIN_MASS, MAX_MASS), NewtonBodyGetCollision(body));
	NewtonBodySetCentreOfMass(body, &com[0]);
	return Util::to_value(body_data->volume * world_data->inverse_scale3);
}

VALUE MSNewton::Body::reset_mass_properties(VALUE self, VALUE v_body, VALUE v_density) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	dFloat density = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_density), MIN_DENSITY);
	if (!body_data->dynamic) return Qfalse;
	const NewtonCollision* collision = NewtonBodyGetCollision(body);
	body_data->density = density;
	body_data->volume = NewtonConvexCollisionCalculateVolume(collision);
	body_data->mass = body_data->density * body_data->volume;
	NewtonBodySetMassProperties(body, body_data->bstatic ? 0.0f : Util::clamp(body_data->mass, MIN_MASS, MAX_MASS), collision);
	return Qtrue;
}

VALUE MSNewton::Body::is_static(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(body_data->bstatic);
}

VALUE MSNewton::Body::set_static(VALUE self, VALUE v_body, VALUE v_static) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	if (!body_data->dynamic) return Qtrue;
	body_data->bstatic = Util::value_to_bool(v_static);
	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	NewtonBodySetMassProperties(body, body_data->bstatic ? 0.0f : Util::clamp(body_data->mass, MIN_MASS, MAX_MASS), NewtonBodyGetCollision(body));
	NewtonBodySetCentreOfMass(body, &com[0]);
	return Util::to_value(body_data->bstatic);
}

VALUE MSNewton::Body::is_collidable(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(body_data->collidable);
}

VALUE MSNewton::Body::set_collidable(VALUE self, VALUE v_body, VALUE v_collidable) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	body_data->collidable = Util::value_to_bool(v_collidable);
	NewtonBodySetCollidable(body, body_data->collidable ? 1 : 0);
	return Util::to_value(body_data->collidable);
}

VALUE MSNewton::Body::is_frozen(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	return NewtonBodyGetFreezeState(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::set_frozen(VALUE self, VALUE v_body, VALUE v_state) {
	const NewtonBody* body = Util::value_to_body(v_body);
	NewtonBodySetFreezeState(body, Util::value_to_bool(v_state) ? 1 : 0);
	return NewtonBodyGetFreezeState(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::is_sleeping(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	return NewtonBodyGetSleepState(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::set_sleeping(VALUE self, VALUE v_body, VALUE v_state) {
	const NewtonBody* body = Util::value_to_body(v_body);
	NewtonBodySetSleepState(body, Util::value_to_bool(v_state) ? 1 : 0);
	return NewtonBodyGetSleepState(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::get_auto_sleep_state(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	return NewtonBodyGetAutoSleep(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::set_auto_sleep_state(VALUE self, VALUE v_body, VALUE v_state) {
	const NewtonBody* body = Util::value_to_body(v_body);
	NewtonBodySetAutoSleep(body, RTEST(v_state) ? 1 : 0);
	return NewtonBodyGetAutoSleep(body) == 1 ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::is_non_collidable_with(VALUE self, VALUE v_body, VALUE v_other_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonBody* other_body = Util::value_to_body(v_other_body);
	Util::validate_two_bodies(body, other_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return body_data->non_collidable_bodies.find(other_body) != body_data->non_collidable_bodies.end() ? Qtrue : Qfalse;
}

VALUE MSNewton::Body::set_non_collidable_with(VALUE self, VALUE v_body, VALUE v_other_body, VALUE v_state) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonBody* other_body = Util::value_to_body(v_other_body);
	bool state = Util::value_to_bool(v_state);
	Util::validate_two_bodies(body, other_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	BodyData* other_body_data = (BodyData*)NewtonBodyGetUserData(other_body);
	if (state) {
		body_data->non_collidable_bodies[other_body] = true;
		other_body_data->non_collidable_bodies[body] = true;
	}
	else {
		if (body_data->non_collidable_bodies.find(other_body) != body_data->non_collidable_bodies.end())
			body_data->non_collidable_bodies.erase(other_body);
		if (other_body_data->non_collidable_bodies.find(body) != other_body_data->non_collidable_bodies.end())
			other_body_data->non_collidable_bodies.erase(body);
	}
	return Util::to_value(state);
}

VALUE MSNewton::Body::get_non_collidable_bodies(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	VALUE non_collidable_bodies = rb_ary_new();
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	for (std::map<const NewtonBody*, bool>::iterator it = body_data->non_collidable_bodies.begin(); it != body_data->non_collidable_bodies.end(); ++it)
		rb_ary_push(non_collidable_bodies, Util::to_value(it->first));
	return non_collidable_bodies;
}

VALUE MSNewton::Body::clear_non_collidable_bodies(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	unsigned int count = (unsigned int)body_data->non_collidable_bodies.size();
	c_clear_non_collidable_bodies(body);
	return Util::to_value(count);
}

VALUE MSNewton::Body::get_elasticity(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(body_data->elasticity);
}

VALUE MSNewton::Body::set_elasticity(VALUE self, VALUE v_body, VALUE v_elasticity) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	body_data->elasticity = Util::clamp<dFloat>(Util::value_to_dFloat(v_elasticity), 0.01f, 2.00f);
	return Util::to_value(body_data->elasticity);
}

VALUE MSNewton::Body::get_softness(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(body_data->softness);
}

VALUE MSNewton::Body::set_softness(VALUE self, VALUE v_body, VALUE v_softness) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	body_data->softness = Util::clamp<dFloat>(Util::value_to_dFloat(v_softness), 0.01f, 1.00f);
	return Util::to_value(body_data->softness);
}

VALUE MSNewton::Body::get_static_friction(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(body_data->static_friction);
}

VALUE MSNewton::Body::set_static_friction(VALUE self, VALUE v_body, VALUE v_friction) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	body_data->static_friction = Util::clamp<dFloat>(Util::value_to_dFloat(v_friction), 0.01f, 2.00f);
	return Util::to_value(body_data->static_friction);
}

VALUE MSNewton::Body::get_dynamic_friction(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(body_data->dynamic_friction);
}

VALUE MSNewton::Body::set_dynamic_friction(VALUE self, VALUE v_body, VALUE v_friction) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	body_data->static_friction = Util::clamp<dFloat>(Util::value_to_dFloat(v_friction), 0.01f, 2.00f);
	return Util::to_value(body_data->dynamic_friction);
}

VALUE MSNewton::Body::get_friction_state(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(body_data->friction_enabled);
}

VALUE MSNewton::Body::set_friction_state(VALUE self, VALUE v_body, VALUE v_state) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	body_data->friction_enabled = Util::value_to_bool(v_state);
	return Util::to_value(body_data->friction_enabled);
}

VALUE MSNewton::Body::get_magnet_force(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value(body_data->magnet_force * world_data->inverse_scale3);
}

VALUE MSNewton::Body::set_magnet_force(VALUE self, VALUE v_body, VALUE v_force) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	body_data->magnet_force = Util::value_to_dFloat(v_force) * world_data->scale3;
	return Util::to_value(body_data->magnet_force * world_data->inverse_scale3);
}

VALUE MSNewton::Body::get_magnet_range(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value(body_data->magnet_range * world_data->inverse_scale);
}

VALUE MSNewton::Body::set_magnet_range(VALUE self, VALUE v_body, VALUE v_range) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	body_data->magnet_range = Util::value_to_dFloat(v_range) * world_data->scale;
	return Util::to_value(body_data->magnet_range * world_data->inverse_scale);
}

VALUE MSNewton::Body::is_magnetic(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(body_data->magnetic);
}

VALUE MSNewton::Body::set_magnetic(VALUE self, VALUE v_body, VALUE v_state) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	body_data->magnetic = Util::value_to_bool(v_state);
	return Util::to_value(body_data->magnetic);
}

VALUE MSNewton::Body::get_aabb(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector min;
	dVector max;
	NewtonBodyGetAABB(body, &min[0], &max[0]);
	return rb_ary_new3(2, Util::point_to_value(min, world_data->inverse_scale), Util::point_to_value(max, world_data->inverse_scale));
}

VALUE MSNewton::Body::get_linear_damping(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	return Util::to_value( NewtonBodyGetLinearDamping(body) );
}

VALUE MSNewton::Body::set_linear_damping(VALUE self, VALUE v_body, VALUE v_damp) {
	const NewtonBody* body = Util::value_to_body(v_body);
	NewtonBodySetLinearDamping(body, Util::value_to_dFloat(v_damp));
	return Util::to_value( NewtonBodyGetLinearDamping(body) );
}

VALUE MSNewton::Body::get_angular_damping(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	dVector damp;
	NewtonBodyGetAngularDamping(body, &damp[0]);
	return Util::vector_to_value(damp);
}

VALUE MSNewton::Body::set_angular_damping(VALUE self, VALUE v_body, VALUE v_damp) {
	const NewtonBody* body = Util::value_to_body(v_body);
	dVector damp = Util::value_to_vector(v_damp);
	NewtonBodySetAngularDamping(body, &damp[0]);
	NewtonBodyGetAngularDamping(body, &damp[0]);
	return Util::vector_to_value(damp);
}

VALUE MSNewton::Body::get_point_velocity(VALUE self, VALUE v_body, VALUE v_point) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector point = Util::value_to_point(v_point, world_data->scale);
	dVector velocity;
	NewtonBodyGetPointVelocity(body, &point[0], &velocity[0]);
	return Util::vector_to_value(velocity, world_data->inverse_scale);
}

VALUE MSNewton::Body::add_point_force(VALUE self, VALUE v_body, VALUE v_point, VALUE v_force) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (body_data->bstatic) return Qfalse;
	dMatrix matrix;
	dVector centre;
	dVector point = Util::value_to_point(v_point, world_data->scale);
	dVector force = Util::value_to_vector(v_force, world_data->scale3);
	NewtonBodyGetCentreOfMass(body, &centre[0]);
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	centre = matrix.TransformVector(centre);
	dVector r = point - centre;
	dVector torque = r * force;
	body_data->add_force += force;
	body_data->add_force_state = true;
	body_data->add_torque += torque;
	body_data->add_torque_state = true;
	return Qtrue;
}

VALUE MSNewton::Body::add_impulse(VALUE self, VALUE v_body, VALUE v_center, VALUE v_delta_vel) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (body_data->bstatic) return Qfalse;
	dVector center = Util::value_to_point(v_center, world_data->scale);
	dVector delta_vel = Util::value_to_vector(v_delta_vel, world_data->scale);
	NewtonBodyAddImpulse(body, &center[0], &delta_vel[0]);
	return Qtrue;
}

VALUE MSNewton::Body::get_force(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector force;
	NewtonBodyGetForce(body, &force[0]);
	if (world_data->gravity_enabled && body_data->gravity_enabled) {
		for (int i = 0; i < 3; ++i)
			force[i] *= world_data->inverse_scale;
	}
	return Util::vector_to_value(force, world_data->inverse_scale3);
}

VALUE MSNewton::Body::get_force_acc(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector force;
	NewtonBodyGetForceAcc(body, &force[0]);
	if (world_data->gravity_enabled && body_data->gravity_enabled) {
		for (int i = 0; i < 3; ++i)
			force[i] *= world_data->inverse_scale;
	}
	return Util::vector_to_value(force, world_data->inverse_scale3);
}

VALUE MSNewton::Body::add_force(VALUE self, VALUE v_body, VALUE v_force) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (body_data->bstatic) return Qfalse;
	body_data->add_force += Util::value_to_vector(v_force, world_data->scale3);
	body_data->add_force_state = true;
	return Qtrue;
}

VALUE MSNewton::Body::add_force2(VALUE self, VALUE v_body, VALUE v_force) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (body_data->bstatic) return Qfalse;
	body_data->add_force2 += Util::value_to_vector(v_force, world_data->scale3);
	body_data->add_force2_state = true;
	return Qtrue;
}

VALUE MSNewton::Body::set_force(VALUE self, VALUE v_body, VALUE v_force) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (body_data->bstatic) return Qfalse;
	body_data->set_force = Util::value_to_vector(v_force, world_data->scale3);
	body_data->set_force_state = true;
	return Qtrue;
}

VALUE MSNewton::Body::set_force2(VALUE self, VALUE v_body, VALUE v_force) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (body_data->bstatic) return Qfalse;
	body_data->set_force2 = Util::value_to_vector(v_force, world_data->scale3);
	body_data->set_force2_state = true;
	return Qtrue;
}

VALUE MSNewton::Body::get_torque(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector torque;
	NewtonBodyGetTorque(body, &torque[0]);
	if (world_data->gravity_enabled && body_data->gravity_enabled) {
		for (int i = 0; i < 3; ++i)
			torque[i] *= world_data->inverse_scale;
	}
	return Util::vector_to_value(torque, world_data->inverse_scale4);
}

VALUE MSNewton::Body::get_torque_acc(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector torque;
	NewtonBodyGetTorqueAcc(body, &torque[0]);
	if (world_data->gravity_enabled && body_data->gravity_enabled) {
		for (int i = 0; i < 3; ++i)
			torque[i] *= world_data->inverse_scale;
	}
	return Util::vector_to_value(torque, world_data->inverse_scale4);
}

VALUE MSNewton::Body::add_torque(VALUE self, VALUE v_body, VALUE v_torque) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (body_data->bstatic) return Qfalse;
	body_data->add_torque += Util::value_to_vector(v_torque, world_data->scale4);
	body_data->add_torque_state = true;
	return Qtrue;
}

VALUE MSNewton::Body::add_torque2(VALUE self, VALUE v_body, VALUE v_torque) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (body_data->bstatic) return Qfalse;
	body_data->add_torque2 += Util::value_to_vector(v_torque, world_data->scale4);
	body_data->add_torque2_state = true;
	return Qtrue;
}

VALUE MSNewton::Body::set_torque(VALUE self, VALUE v_body, VALUE v_torque) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (body_data->bstatic) return Qfalse;
	body_data->set_torque = Util::value_to_vector(v_torque, world_data->scale4);
	body_data->set_torque_state = true;
	return Qtrue;
}

VALUE MSNewton::Body::set_torque2(VALUE self, VALUE v_body, VALUE v_torque) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (body_data->bstatic) return Qfalse;
	body_data->set_torque2 = Util::value_to_vector(v_torque, world_data->scale4);
	body_data->set_torque2_state = true;
	return Qtrue;
}

VALUE MSNewton::Body::get_net_contact_force(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector net_force(0.0f, 0.0f, 0.0f, 0.0f);
	for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint)) {
		for (void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact)) {
			NewtonMaterial* material = NewtonContactGetMaterial(contact);
			dVector force;
			NewtonMaterialGetContactForce(material, body, &force[0]);
			net_force += force;
		}
	}
	if (world_data->gravity_enabled && body_data->gravity_enabled) {
		for (int i = 0; i < 3; ++i)
			net_force[i] *= world_data->inverse_scale;
	}
	return Util::vector_to_value(net_force, world_data->inverse_scale3);
}

VALUE MSNewton::Body::get_contacts(VALUE self, VALUE v_body, VALUE v_inc_non_collidable) {
	const NewtonBody* body = Util::value_to_body(v_body);
	bool inc_non_collidable = Util::value_to_bool(v_inc_non_collidable);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	VALUE v_contacts = rb_ary_new();
	for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint)) {
		NewtonBody* touching_body = NewtonJointGetBody0(joint);
		if (touching_body == body)
			touching_body = NewtonJointGetBody1(joint);
		BodyData* touching_body_data = (BodyData*)NewtonBodyGetUserData(touching_body);
		for (void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact)) {
			NewtonMaterial* material = NewtonContactGetMaterial(contact);
			dVector point;
			dVector normal;
			dVector force;
			NewtonMaterialGetContactPositionAndNormal(material, body, &point[0], &normal[0]);
			NewtonMaterialGetContactForce(material, body, &force[0]);
			if (world_data->gravity_enabled && (body_data->gravity_enabled || touching_body_data->gravity_enabled)) {
				for (int i = 0; i < 3; ++i)
					force[i] *= world_data->inverse_scale;
			}
			dFloat speed = NewtonMaterialGetContactNormalSpeed(material) * world_data->inverse_scale;
			rb_ary_push(v_contacts, rb_ary_new3(5, Util::to_value(touching_body), Util::point_to_value(point, world_data->inverse_scale), Util::vector_to_value(normal), Util::vector_to_value(force, world_data->inverse_scale3), Util::to_value(speed)));
		}
	}
	if (inc_non_collidable) {
		NewtonCollision* colA = NewtonBodyGetCollision(body);
		NewtonCollision* colB;
		dMatrix matA;
		dMatrix matB;
		NewtonBodyGetMatrix(body, &matA[0][0]);
		dFloat points[3*NON_COL_CONTACTS_CAPACITY];
		dFloat normals[3*NON_COL_CONTACTS_CAPACITY];
		dFloat penetrations[3*NON_COL_CONTACTS_CAPACITY];
		long long attrA[NON_COL_CONTACTS_CAPACITY];
		long long attrB[NON_COL_CONTACTS_CAPACITY];
		for (const NewtonBody* tbody = NewtonWorldGetFirstBody(world); tbody; tbody = NewtonWorldGetNextBody(world, tbody)) {
			if (tbody == body || Util::bodies_collidable(tbody, body) || !Util::bodies_aabb_overlap(tbody, body)) continue;
			colB = NewtonBodyGetCollision(tbody);
			NewtonBodyGetMatrix(tbody, &matB[0][0]);
			int count = NewtonCollisionCollide(world, NON_COL_CONTACTS_CAPACITY, colA, &matA[0][0], colB, &matB[0][0], points, normals, penetrations, attrA, attrB, 0);
			if (count == 0) continue;
			for (int i = 0; i < count*3; i += 3) {
				dVector point(points[i+0], points[i+1], points[i+2]);
				dVector normal(normals[i+0], normals[i+1], normals[i+2]);
				dVector force(0,0,0);
				dFloat speed = 0;
				rb_ary_push(
					v_contacts,
					rb_ary_new3(
						5,
						Util::to_value(tbody),
						Util::point_to_value(point, world_data->inverse_scale),
						Util::vector_to_value(normal),
						Util::vector_to_value(force),
						Util::to_value(speed)
					)
				);
			}
		}
	}
	return v_contacts;
}

VALUE MSNewton::Body::get_touching_bodies(VALUE self, VALUE v_body, VALUE v_inc_non_collidable) {
	const NewtonBody* body = Util::value_to_body(v_body);
	bool inc_non_collidable = Util::value_to_bool(v_inc_non_collidable);
	VALUE v_touching_bodies = rb_ary_new();
	for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint)) {
		NewtonBody* other_body = NewtonJointGetBody0(joint);
		if (!inc_non_collidable && !Util::bodies_collidable(other_body, body)) continue;
		if (other_body == body)
			other_body = NewtonJointGetBody1(joint);
		rb_ary_push(v_touching_bodies, Util::to_value(other_body));
	}
	/*if (inc_non_collidable) {
		NewtonWorld* world = NewtonBodyGetWorld(body);
		NewtonCollision* colA = NewtonBodyGetCollision(body);
		NewtonCollision* colB;
		dMatrix matA;
		dMatrix matB;
		NewtonBodyGetMatrix(body, &matA[0][0]);
		for (const NewtonBody* tbody = NewtonWorldGetFirstBody(world); tbody; tbody = NewtonWorldGetNextBody(world, tbody)) {
			if (tbody == body || Util::bodies_collidable(tbody, body) || !Util::bodies_aabb_overlap(tbody, body)) continue;
			colB = NewtonBodyGetCollision(tbody);
			NewtonBodyGetMatrix(tbody, &matB[0][0]);
			if (NewtonCollisionIntersectionTest(world, colA, &matA[0][0], colB, &matB[0][0], 0) == 1)
				rb_ary_push(v_touching_bodies, Util::to_value(tbody));
		}
	}*/
	return v_touching_bodies;
}

VALUE MSNewton::Body::get_contact_points(VALUE self, VALUE v_body, VALUE v_inc_non_collidable) {
	const NewtonBody* body = Util::value_to_body(v_body);
	bool inc_non_collidable = Util::value_to_bool(v_inc_non_collidable);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	VALUE v_contact_points = rb_ary_new();
	for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body); joint; joint = NewtonBodyGetNextContactJoint(body, joint)) {
		for (void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact)) {
			NewtonMaterial* material = NewtonContactGetMaterial(contact);
			dVector point;
			dVector normal;
			NewtonMaterialGetContactPositionAndNormal(material, body, &point[0], &normal[0]);
			rb_ary_push(v_contact_points, Util::point_to_value(point, world_data->inverse_scale));
		}
	}
	if (inc_non_collidable) {
		NewtonCollision* colA = NewtonBodyGetCollision(body);
		NewtonCollision* colB;
		dMatrix matA;
		dMatrix matB;
		dFloat points[3*NON_COL_CONTACTS_CAPACITY];
		dFloat normals[3*NON_COL_CONTACTS_CAPACITY];
		dFloat penetrations[3*NON_COL_CONTACTS_CAPACITY];
		long long attrA[NON_COL_CONTACTS_CAPACITY];
		long long attrB[NON_COL_CONTACTS_CAPACITY];
		NewtonBodyGetMatrix(body, &matA[0][0]);
		for (const NewtonBody* tbody = NewtonWorldGetFirstBody(world); tbody; tbody = NewtonWorldGetNextBody(world, tbody)) {
			if (tbody == body || Util::bodies_collidable(tbody, body) || !Util::bodies_aabb_overlap(tbody, body)) continue;
			colB = NewtonBodyGetCollision(tbody);
			NewtonBodyGetMatrix(tbody, &matB[0][0]);
			int count = NewtonCollisionCollide(world, NON_COL_CONTACTS_CAPACITY, colA, &matA[0][0], colB, &matB[0][0], points, normals, penetrations, attrA, attrB, 0);
			if (count == 0) continue;
			for (int i = 0; i < count*3; i += 3) {
				dVector point(points[i+0], points[i+1], points[i+2]);
				rb_ary_push(v_contact_points, Util::point_to_value(point, world_data->inverse_scale));
			}
		}
	}
	return v_contact_points;
}

VALUE MSNewton::Body::get_collision_faces(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	NewtonCollision* collision = NewtonBodyGetCollision(body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	CollisionIteratorData iterator_data;
	iterator_data.faces = rb_ary_new();
	iterator_data.scale = world_data->inverse_scale;
	NewtonCollisionForEachPolygonDo(collision, &matrix[0][0], collision_iterator, (void*)&iterator_data);
	return iterator_data.faces;
}

VALUE MSNewton::Body::get_collision_faces2(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	NewtonCollision* collision = NewtonBodyGetCollision(body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	CollisionIteratorData iterator_data;
	iterator_data.faces = rb_ary_new();
	iterator_data.scale = world_data->inverse_scale;
	NewtonCollisionForEachPolygonDo(collision, &matrix[0][0], collision_iterator2, (void*)&iterator_data);
	return iterator_data.faces;
}

VALUE MSNewton::Body::get_collision_faces3(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	NewtonCollision* collision = NewtonBodyGetCollision(body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	CollisionIteratorData iterator_data;
	iterator_data.faces = rb_ary_new();
	iterator_data.scale = world_data->inverse_scale;
	NewtonCollisionForEachPolygonDo(collision, &matrix[0][0], collision_iterator3, (void*)&iterator_data);
	return iterator_data.faces;
}

VALUE MSNewton::Body::apply_pick_and_drag(VALUE self, VALUE v_body, VALUE v_pick_pt, VALUE v_dest_pt, VALUE v_stiffness, VALUE v_damp) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector pick_pt = Util::value_to_point(v_pick_pt, world_data->scale);
	dVector dest_pt = Util::value_to_point(v_dest_pt, world_data->scale);
	dFloat stiffness = Util::value_to_dFloat(v_stiffness);
	dFloat damp = Util::value_to_dFloat(v_damp);
	if (body_data->bstatic) return Qfalse;
	// Get data
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	dVector loc_pick_pt = matrix.UntransformVector(pick_pt);
	// Queue pick and drag settings
	PickAndDragData pd_data;
	pd_data.loc_pick_pt = loc_pick_pt;
	pd_data.dest_pt = dest_pt;
	pd_data.stiffness = stiffness;
	pd_data.damper = damp;
	body_data->pick_and_drag.push_back(pd_data);
	// Make sure body is not frozen
	NewtonBodySetFreezeState(body, 0);
	// Return success
	return Qtrue;
}

VALUE MSNewton::Body::apply_pick_and_drag2(VALUE self, VALUE v_body, VALUE v_pick_pt, VALUE v_dest_pt, VALUE v_stiffness, VALUE v_angular_damp, VALUE v_timestep) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector pick_pt = Util::value_to_point(v_pick_pt, world_data->scale);
	dVector dest_pt = Util::value_to_point(v_dest_pt, world_data->scale);
	dFloat stiffness = Util::value_to_dFloat(v_stiffness);
	dFloat angular_damp = Util::value_to_dFloat(v_angular_damp);
	dFloat timestep = Util::value_to_dFloat(v_timestep);
	if (body_data->bstatic) return Qfalse;
	dFloat inv_timestep = 1.0f / timestep;
	dVector com;
	dMatrix matrix;
	dVector omega0;
	dVector veloc0;
	dVector omega1;
	dVector veloc1;
	dVector point_veloc;

	NewtonWorldCriticalSectionLock(world, 0);

	// Calculate the desired impulse
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	NewtonBodyGetOmega(body, &omega0[0]);
	NewtonBodyGetVelocity(body, &veloc0[0]);
	NewtonBodyGetPointVelocity(body, &pick_pt[0], &point_veloc[0]);

	dVector delta_veloc(dest_pt - pick_pt);
	delta_veloc = delta_veloc.Scale(stiffness * inv_timestep) - point_veloc;
	for (int i = 0; i < 3; ++i) {
		dVector veloc(0.0f, 0.0f, 0.0f, 0.0f);
		veloc[i] = delta_veloc[i];
		NewtonBodyAddImpulse(body, &veloc[0], &pick_pt[0]);
	}

	// Damp angular velocity
	NewtonBodyGetOmega(body, &omega1[0]);
	NewtonBodyGetVelocity(body, &veloc1[0]);
	omega1 = omega1.Scale(angular_damp);

	// Restore body linear and angular velocity
	NewtonBodySetOmega(body, &omega0[0]);
	NewtonBodySetVelocity(body, &veloc0[0]);

	// Convert the delta velocity change to an external force and torque
	dFloat Ixx, Iyy, Izz, mass;
	NewtonBodyGetMassMatrix(body, &mass, &Ixx, &Iyy, &Izz);

	dVector angular_momentum(Ixx, Iyy, Izz, 0.0f);
	angular_momentum = matrix.RotateVector( angular_momentum.CompProduct(matrix.UnrotateVector(omega1 - omega0)) );

	dVector force( (veloc1 - veloc0).Scale(mass * inv_timestep) );
	dVector torque( angular_momentum.Scale(inv_timestep) );

	// Add force and torque
	body_data->add_force += force;
	body_data->add_force_state = true;
	body_data->add_torque += torque;
	body_data->add_torque_state = true;

	// Make sure body is not frozen
	NewtonBodySetFreezeState(body, 0);

	NewtonWorldCriticalSectionUnlock(world);
	return Qtrue;
}

VALUE MSNewton::Body::apply_buoyancy(VALUE self, VALUE v_body, VALUE v_plane_origin, VALUE v_plane_normal, VALUE v_plane_current, VALUE v_density, VALUE v_linear_viscosity, VALUE v_angular_viscosity) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector origin = Util::value_to_point(v_plane_origin, world_data->scale);
	dVector normal = Util::value_to_vector(v_plane_normal);
	dVector current = Util::value_to_vector(v_plane_current, world_data->scale);
	dFloat density = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_density), MIN_DENSITY);
	dFloat linear_viscosity = Util::clamp<dFloat>(Util::value_to_dFloat(v_linear_viscosity), 0.0f, 1.0f);
	dFloat angular_viscosity = Util::clamp<dFloat>(Util::value_to_dFloat(v_angular_viscosity), 0.0f, 1.0f);
	if (body_data->bstatic)
		return Qfalse;
	else {
		dMatrix plane_matrix = Util::matrix_from_pin_dir(origin, normal);
		normal.m_w = plane_matrix.UntransformVector(ORIGIN).m_z;
		BuoyancyData buoyancy_data;
		buoyancy_data.density = density;
		buoyancy_data.plane = normal;
		buoyancy_data.current = current;
		buoyancy_data.linear_ratio = 1.0f - linear_viscosity;
		buoyancy_data.angular_ratio = 1.0f - angular_viscosity;
		body_data->buoyancy.push_back(buoyancy_data);
		return Qtrue;
	}
}

VALUE MSNewton::Body::apply_fluid_resistance(VALUE self, VALUE v_body, VALUE v_density) {
	const NewtonBody* body = Util::value_to_body(v_body);
	dFloat density = Util::value_to_dFloat(v_density);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	if (body_data->bstatic == true || body_data->dynamic == false) return Qnil;
	const NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	NewtonCollision* collision = NewtonBodyGetCollision(body);
	dMatrix matrix;
	NewtonBodyGetMatrix(body, &matrix[0][0]);
	dVector centre;
	NewtonBodyGetCentreOfMass(body, &centre[0]);
	CollisionIteratorData2 iterator_data;
	iterator_data.body = body;
	iterator_data.centre = matrix.TransformVector(centre);
	iterator_data.density = density;
	iterator_data.force = dVector(0.0f, 0.0f, 0.0f);
	iterator_data.torque = dVector(0.0f, 0.0f, 0.0f);
	NewtonCollisionForEachPolygonDo(collision, &matrix[0][0], collision_iterator4, (void*)&iterator_data);

	body_data->add_force += iterator_data.force;
	body_data->add_force_state = true;
	body_data->add_torque += iterator_data.torque;
	body_data->add_torque_state = true;

	return rb_ary_new3(2, Util::vector_to_value(iterator_data.force, world_data->inverse_scale3), Util::vector_to_value(iterator_data.torque, world_data->inverse_scale4));
}

VALUE MSNewton::Body::copy(VALUE self, VALUE v_body, VALUE v_matrix, VALUE v_reapply_forces) {
	const NewtonBody* body = Util::value_to_body(v_body);
	bool reapply_forces = Util::value_to_bool(v_reapply_forces);

	NewtonWorld* world = NewtonBodyGetWorld(body);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);

	dMatrix matrix;
	if (v_matrix == Qnil)
		NewtonBodyGetMatrix(body, &matrix[0][0]);
	else
		matrix = Util::value_to_matrix(v_matrix, world_data->scale);
	if (Util::is_matrix_flipped(matrix)) {
		matrix.m_front.m_x *= -1;
		matrix.m_front.m_y *= -1;
		matrix.m_front.m_z *= -1;
	}

	NewtonCollision* new_col = NewtonCollisionCreateInstance(NewtonBodyGetCollision(body));

	Util::extract_matrix_scale(matrix);
	NewtonBody* new_body = NewtonCreateDynamicBody(world, new_col, &matrix[0][0]);
	valid_bodies[new_body] = true;

	//dFloat mass, ixx, iyy, izz;
	//NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);
	//NewtonBodySetMassMatrix(new_body, mass, ixx, iyy, izz);
	NewtonBodySetMassProperties(new_body, body_data->bstatic ? 0.0f : Util::clamp(body_data->mass, MIN_MASS, MAX_MASS), new_col);
	NewtonDestroyCollision(new_col);

	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	NewtonBodySetCentreOfMass(new_body, &com[0]);

	BodyData* new_data = new BodyData;
	new_data->add_force = dVector(0.0f, 0.0f, 0.0f);
	new_data->add_force_state = false;
	new_data->set_force = dVector(0.0f, 0.0f, 0.0f);
	new_data->set_force_state = false;
	new_data->add_torque = dVector(0.0f, 0.0f, 0.0f);
	new_data->add_torque_state = false;
	new_data->set_torque = dVector(0.0f, 0.0f, 0.0f);
	new_data->set_torque_state = false;
	new_data->add_force2 = dVector(0.0f, 0.0f, 0.0f);
	new_data->add_force2_state = false;
	new_data->set_force2 = dVector(0.0f, 0.0f, 0.0f);
	new_data->set_force2_state = false;
	new_data->add_torque2 = dVector(0.0f, 0.0f, 0.0f);
	new_data->add_torque2_state = false;
	new_data->set_torque2 = dVector(0.0f, 0.0f, 0.0f);
	new_data->set_torque2_state = false;
	new_data->dynamic = body_data->dynamic;
	new_data->bstatic = body_data->bstatic;
	new_data->density = body_data->density;
	new_data->volume = body_data->volume;
	new_data->mass = body_data->mass;
	new_data->elasticity = body_data->elasticity;
	new_data->softness = body_data->softness;
	new_data->static_friction = body_data->static_friction;
	new_data->dynamic_friction = body_data->dynamic_friction;
	new_data->friction_enabled = body_data->friction_enabled;
	new_data->collidable = body_data->collidable;
	new_data->non_collidable_bodies = std::map<const NewtonBody*, bool>(body_data->non_collidable_bodies);
	new_data->record_touch_data = false;
	new_data->magnet_force = body_data->magnet_force;
	new_data->magnet_range = body_data->magnet_range;
	new_data->magnetic = body_data->magnetic;
	new_data->touchers;
	new_data->destructor_proc = rb_ary_new();
	new_data->user_data = rb_ary_new();
	new_data->matrix_scale = dVector(body_data->matrix_scale);
	new_data->default_collision_scale = dVector(body_data->default_collision_scale);
	new_data->collision_scale = dVector(body_data->collision_scale);
	new_data->default_collision_offset = dVector(body_data->default_collision_offset);
	new_data->matrix_changed = false;
	new_data->gravity_enabled = body_data->gravity_enabled;
	new_data->material_id = body_data->material_id;
	new_data->pick_and_drag;
	new_data->buoyancy;

	NewtonBodySetUserData(new_body, new_data);
	rb_gc_register_address(&new_data->destructor_proc);
	rb_gc_register_address(&new_data->user_data);

	NewtonBodySetMaterialGroupID(new_body, body_data->material_id);

	NewtonBodySetForceAndTorqueCallback(new_body, force_and_torque_callback);
	NewtonBodySetDestructorCallback(new_body, destructor_callback);
	NewtonBodySetTransformCallback(new_body, transform_callback);

	NewtonBodySetLinearDamping(new_body, NewtonBodyGetLinearDamping(body));
	dVector angular_damp;
	NewtonBodyGetAngularDamping(body, &angular_damp[0]);
	NewtonBodySetAngularDamping(new_body, &angular_damp[0]);

	NewtonBodySetCollidable(new_body, body_data->collidable ? 1 : 0);

	NewtonBodySetSimulationState(new_body, NewtonBodyGetSimulationState(body));
	NewtonBodySetContinuousCollisionMode(new_body, NewtonBodyGetContinuousCollisionMode(body));

	NewtonBodySetFreezeState(new_body, NewtonBodyGetFreezeState(body));
	NewtonBodySetAutoSleep(new_body, NewtonBodyGetAutoSleep(body));

	if (reapply_forces) {
		dVector omega;
		dVector velocity;
		NewtonBodyGetOmega(body, &omega[0]);
		NewtonBodyGetVelocity(body, &velocity[0]);
		NewtonBodySetOmega(new_body, &omega[0]);
		NewtonBodySetVelocity(new_body, &velocity[0]);
	}

	return Util::to_value(new_body);
}

VALUE MSNewton::Body::get_destructor_proc(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if (RARRAY_LEN(data->destructor_proc) == 0) return Qnil;
	return rb_ary_entry(data->destructor_proc, 0);
}

VALUE MSNewton::Body::set_destructor_proc(VALUE self, VALUE v_body, VALUE v_proc) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if (v_proc == Qnil)
		rb_ary_clear(data->destructor_proc);
	else if (rb_class_of(v_proc) == rb_cProc)
		rb_ary_store(data->destructor_proc, 0, v_proc);
	else
		rb_raise(rb_eTypeError, "Expected nil or a Proc object!");
	return Qtrue;
}

VALUE MSNewton::Body::get_user_data(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if (RARRAY_LEN(data->user_data) == 0) return Qnil;
	return rb_ary_entry(data->user_data, 0);
}

VALUE MSNewton::Body::set_user_data(VALUE self, VALUE v_body, VALUE v_user_data) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	if (v_user_data == Qnil)
		rb_ary_clear(data->user_data);
	else
		rb_ary_store(data->user_data, 0, v_user_data);
	return Qtrue;
}

VALUE MSNewton::Body::get_record_touch_data_state(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(data->record_touch_data);
}

VALUE MSNewton::Body::set_record_touch_data_state(VALUE self, VALUE v_body, VALUE v_state) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->record_touch_data = Util::value_to_bool(v_state);
	if (!data->record_touch_data) data->touchers.clear();
	return Util::to_value(data->record_touch_data);
}

VALUE MSNewton::Body::get_matrix_scale(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::vector_to_value(data->matrix_scale);
}

VALUE MSNewton::Body::set_matrix_scale(VALUE self, VALUE v_body, VALUE v_scale) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->matrix_scale = Util::value_to_vector(v_scale);
	return Util::vector_to_value(data->matrix_scale);
}

VALUE MSNewton::Body::matrix_changed(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(data->matrix_changed);
}

VALUE MSNewton::Body::enable_gravity(VALUE self, VALUE v_body, VALUE v_state) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	data->gravity_enabled = Util::value_to_bool(v_state);
	return Util::to_value(data->gravity_enabled);
}

VALUE MSNewton::Body::is_gravity_enabled(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(data->gravity_enabled);
}

VALUE MSNewton::Body::get_contained_joints(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	VALUE v_contained_joints = rb_ary_new();
	for (std::map<JointData*, bool>::iterator it = valid_joints.begin(); it != valid_joints.end(); ++it)
		if (it->first->parent == body)
			rb_ary_push(v_contained_joints, Util::to_value(it->first));
	return v_contained_joints;
}

VALUE MSNewton::Body::get_connected_joints(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	VALUE v_connected_joints = rb_ary_new();
	for (std::map<JointData*, bool>::iterator it = valid_joints.begin(); it != valid_joints.end(); ++it)
		if (it->first->child == body)
			rb_ary_push(v_connected_joints, Util::to_value(it->first));
	return v_connected_joints;
}

VALUE MSNewton::Body::get_connected_bodies(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	VALUE v_connected_bodies = rb_ary_new();
	for (std::map<JointData*, bool>::iterator it = valid_joints.begin(); it != valid_joints.end(); ++it) {
		JointData* joint_data = it->first;
		if (joint_data->child == body && joint_data->parent != nullptr)
			rb_ary_push(v_connected_bodies, Util::to_value(joint_data->parent));
		else if (joint_data->parent == body && joint_data->child != nullptr)
			rb_ary_push(v_connected_bodies, Util::to_value(joint_data->child));
	}
	return v_connected_bodies;
}

VALUE MSNewton::Body::get_material_id(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::to_value(body_data->material_id);
}

VALUE MSNewton::Body::set_material_id(VALUE self, VALUE v_body, VALUE v_id) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	int id = Util::value_to_int(v_id);
	body_data->material_id = id;
	NewtonBodySetMaterialGroupID(body, id);
	return Util::to_value(body_data->material_id);
}

VALUE MSNewton::Body::get_collision_scale(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	//const NewtonCollision* collision = NewtonBodyGetCollision(body);
	//dFloat sx, sy, sz;
	//NewtonCollisionGetScale(collision, &sx, &sy, &sz);
	//return Util::vector_to_value(dVector(sx, sy, sz));
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::vector_to_value(body_data->collision_scale);
}

VALUE MSNewton::Body::set_collision_scale(VALUE self, VALUE v_body, VALUE v_scale) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	const NewtonCollision* collision = NewtonBodyGetCollision(body);
	if (NewtonCollisionGetType(collision) > 8)
		rb_raise(rb_eTypeError, "Only convex collisions can be scaled!");
	dVector scale = Util::value_to_vector(v_scale);
	scale.m_x = Util::clamp(scale.m_x, 0.01f, 100.0f);
	scale.m_y = Util::clamp(scale.m_y, 0.01f, 100.0f);
	scale.m_z = Util::clamp(scale.m_z, 0.01f, 100.0f);
	body_data->collision_scale = scale;
	const dVector& dco = body_data->default_collision_offset;
	const dVector& dcs = body_data->default_collision_scale;
	dMatrix col_matrix;
	NewtonCollisionGetMatrix(collision, &col_matrix[0][0]);
	col_matrix.m_posit.m_x = dco.m_x * scale.m_x / dcs.m_x;
	col_matrix.m_posit.m_y = dco.m_y * scale.m_y / dcs.m_y;
	col_matrix.m_posit.m_z = dco.m_z * scale.m_z / dcs.m_z;
	NewtonCollisionSetMatrix(collision, &col_matrix[0][0]);
	NewtonBodySetCollisionScale(body, scale.m_x, scale.m_y, scale.m_z);
	body_data->volume = NewtonConvexCollisionCalculateVolume(collision);
	body_data->mass = body_data->density * body_data->volume;
	dVector com;
	NewtonBodyGetCentreOfMass(body, &com[0]);
	NewtonBodySetMassProperties(body, body_data->bstatic ? 0.0f : Util::clamp(body_data->mass, MIN_MASS, MAX_MASS), collision);
	NewtonBodySetCentreOfMass(body, &com[0]);
	NewtonBodySetSleepState(body, 0);
	return Util::vector_to_value(scale);
}

VALUE MSNewton::Body::get_default_collision_scale(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	return Util::vector_to_value(body_data->default_collision_scale);
}

VALUE MSNewton::Body::get_actual_matrix_scale(VALUE self, VALUE v_body) {
	const NewtonBody* body = Util::value_to_body(v_body);
	BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
	//const NewtonCollision* collision = NewtonBodyGetCollision(body);
	const dVector& dcs = body_data->default_collision_scale;
	const dVector& ms = body_data->matrix_scale;
	//dFloat csx, csy, csz;
	//NewtonCollisionGetScale(collision, &csx, &csy, &csz);
	dFloat csx = body_data->collision_scale.m_x;
	dFloat csy = body_data->collision_scale.m_y;
	dFloat csz = body_data->collision_scale.m_z;
	dVector actual_matrix_scale(ms.m_x * csx / dcs.m_x, ms.m_y * csy / dcs.m_y, ms.m_z * csz / dcs.m_z);
	return Util::vector_to_value(actual_matrix_scale);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void Init_msp_body(VALUE mNewton) {
	VALUE mBody = rb_define_module_under(mNewton, "Body");

	rb_define_module_function(mBody, "is_valid?", VALUEFUNC(MSNewton::Body::is_valid), 1);
	rb_define_module_function(mBody, "create_dynamic", VALUEFUNC(MSNewton::Body::create_dynamic), 4);
	rb_define_module_function(mBody, "destroy", VALUEFUNC(MSNewton::Body::destroy), 1);
	rb_define_module_function(mBody, "get_world", VALUEFUNC(MSNewton::Body::get_world), 1);
	rb_define_module_function(mBody, "get_collision", VALUEFUNC(MSNewton::Body::get_collision), 1);
	rb_define_module_function(mBody, "get_simulation_state", VALUEFUNC(MSNewton::Body::get_simulation_state), 1);
	rb_define_module_function(mBody, "set_simulation_state", VALUEFUNC(MSNewton::Body::set_simulation_state), 2);
	rb_define_module_function(mBody, "get_continuous_collision_state", VALUEFUNC(MSNewton::Body::get_continuous_collision_state), 1);
	rb_define_module_function(mBody, "set_continuous_collision_state", VALUEFUNC(MSNewton::Body::set_continuous_collision_state), 2);
	rb_define_module_function(mBody, "get_matrix", VALUEFUNC(MSNewton::Body::get_matrix), 1);
	rb_define_module_function(mBody, "get_normal_matrix", VALUEFUNC(MSNewton::Body::get_normal_matrix), 1);
	rb_define_module_function(mBody, "set_matrix", VALUEFUNC(MSNewton::Body::set_matrix), 2);
	rb_define_module_function(mBody, "get_position", VALUEFUNC(MSNewton::Body::get_position), 2);
	rb_define_module_function(mBody, "set_position", VALUEFUNC(MSNewton::Body::set_position), 3);
	rb_define_module_function(mBody, "get_rotation", VALUEFUNC(MSNewton::Body::get_rotation), 1);
	rb_define_module_function(mBody, "get_euler_angles", VALUEFUNC(MSNewton::Body::get_euler_angles), 1);
	rb_define_module_function(mBody, "set_euler_angles", VALUEFUNC(MSNewton::Body::set_euler_angles), 2);
	rb_define_module_function(mBody, "get_velocity", VALUEFUNC(MSNewton::Body::get_velocity), 1);
	rb_define_module_function(mBody, "set_velocity", VALUEFUNC(MSNewton::Body::set_velocity), 2);
	rb_define_module_function(mBody, "get_omega", VALUEFUNC(MSNewton::Body::get_omega), 1);
	rb_define_module_function(mBody, "set_omega", VALUEFUNC(MSNewton::Body::set_omega), 2);
	rb_define_module_function(mBody, "get_centre_of_mass", VALUEFUNC(MSNewton::Body::get_centre_of_mass), 1);
	rb_define_module_function(mBody, "set_centre_of_mass", VALUEFUNC(MSNewton::Body::set_centre_of_mass), 2);
	rb_define_module_function(mBody, "get_mass", VALUEFUNC(MSNewton::Body::get_mass), 1);
	rb_define_module_function(mBody, "set_mass", VALUEFUNC(MSNewton::Body::set_mass), 2);
	rb_define_module_function(mBody, "get_density", VALUEFUNC(MSNewton::Body::get_density), 1);
	rb_define_module_function(mBody, "set_density", VALUEFUNC(MSNewton::Body::set_density), 2);
	rb_define_module_function(mBody, "get_volume", VALUEFUNC(MSNewton::Body::get_volume), 1);
	rb_define_module_function(mBody, "set_volume", VALUEFUNC(MSNewton::Body::set_volume), 2);
	rb_define_module_function(mBody, "reset_mass_properties", VALUEFUNC(MSNewton::Body::reset_mass_properties), 2);
	rb_define_module_function(mBody, "is_static?", VALUEFUNC(MSNewton::Body::is_static), 1);
	rb_define_module_function(mBody, "set_static", VALUEFUNC(MSNewton::Body::set_static), 2);
	rb_define_module_function(mBody, "is_collidable?", VALUEFUNC(MSNewton::Body::is_collidable), 1);
	rb_define_module_function(mBody, "set_collidable", VALUEFUNC(MSNewton::Body::set_collidable), 2);
	rb_define_module_function(mBody, "is_frozen?", VALUEFUNC(MSNewton::Body::is_frozen), 1);
	rb_define_module_function(mBody, "set_frozen", VALUEFUNC(MSNewton::Body::set_frozen), 2);
	rb_define_module_function(mBody, "is_sleeping?", VALUEFUNC(MSNewton::Body::is_sleeping), 1);
	rb_define_module_function(mBody, "set_sleeping", VALUEFUNC(MSNewton::Body::set_sleeping), 2);
	rb_define_module_function(mBody, "get_auto_sleep_state", VALUEFUNC(MSNewton::Body::get_auto_sleep_state), 1);
	rb_define_module_function(mBody, "set_auto_sleep_state", VALUEFUNC(MSNewton::Body::set_auto_sleep_state), 2);
	rb_define_module_function(mBody, "is_non_collidable_with?", VALUEFUNC(MSNewton::Body::is_non_collidable_with), 2);
	rb_define_module_function(mBody, "set_non_collidable_with", VALUEFUNC(MSNewton::Body::set_non_collidable_with), 3);
	rb_define_module_function(mBody, "get_non_collidable_bodies", VALUEFUNC(MSNewton::Body::get_non_collidable_bodies), 1);
	rb_define_module_function(mBody, "clear_non_collidable_bodies", VALUEFUNC(MSNewton::Body::clear_non_collidable_bodies), 1);
	rb_define_module_function(mBody, "get_elasticity", VALUEFUNC(MSNewton::Body::get_elasticity), 1);
	rb_define_module_function(mBody, "set_elasticity", VALUEFUNC(MSNewton::Body::set_elasticity), 2);
	rb_define_module_function(mBody, "get_softness", VALUEFUNC(MSNewton::Body::get_softness), 1);
	rb_define_module_function(mBody, "set_softness", VALUEFUNC(MSNewton::Body::set_softness), 2);
	rb_define_module_function(mBody, "get_static_friction", VALUEFUNC(MSNewton::Body::get_static_friction), 1);
	rb_define_module_function(mBody, "set_static_friction", VALUEFUNC(MSNewton::Body::set_static_friction), 2);
	rb_define_module_function(mBody, "get_dynamic_friction", VALUEFUNC(MSNewton::Body::get_dynamic_friction), 1);
	rb_define_module_function(mBody, "set_dynamic_friction", VALUEFUNC(MSNewton::Body::set_dynamic_friction), 2);
	rb_define_module_function(mBody, "get_friction_state", VALUEFUNC(MSNewton::Body::get_friction_state), 1);
	rb_define_module_function(mBody, "set_friction_state", VALUEFUNC(MSNewton::Body::set_friction_state), 2);
	rb_define_module_function(mBody, "get_magnet_force", VALUEFUNC(MSNewton::Body::get_magnet_force), 1);
	rb_define_module_function(mBody, "set_magnet_force", VALUEFUNC(MSNewton::Body::set_magnet_force), 2);
	rb_define_module_function(mBody, "get_magnet_range", VALUEFUNC(MSNewton::Body::get_magnet_range), 1);
	rb_define_module_function(mBody, "set_magnet_range", VALUEFUNC(MSNewton::Body::set_magnet_range), 2);
	rb_define_module_function(mBody, "is_magnetic?", VALUEFUNC(MSNewton::Body::is_magnetic), 1);
	rb_define_module_function(mBody, "set_magnetic", VALUEFUNC(MSNewton::Body::set_magnetic), 2);
	rb_define_module_function(mBody, "get_aabb", VALUEFUNC(MSNewton::Body::get_aabb), 1);
	rb_define_module_function(mBody, "get_linear_damping", VALUEFUNC(MSNewton::Body::get_linear_damping), 1);
	rb_define_module_function(mBody, "set_linear_damping", VALUEFUNC(MSNewton::Body::set_linear_damping), 2);
	rb_define_module_function(mBody, "get_angular_damping", VALUEFUNC(MSNewton::Body::get_angular_damping), 1);
	rb_define_module_function(mBody, "set_angular_damping", VALUEFUNC(MSNewton::Body::set_angular_damping), 2);
	rb_define_module_function(mBody, "get_point_velocity", VALUEFUNC(MSNewton::Body::get_point_velocity), 2);
	rb_define_module_function(mBody, "add_point_force", VALUEFUNC(MSNewton::Body::add_point_force), 3);
	rb_define_module_function(mBody, "add_impulse", VALUEFUNC(MSNewton::Body::add_impulse), 3);
	rb_define_module_function(mBody, "get_force", VALUEFUNC(MSNewton::Body::get_force), 1);
	rb_define_module_function(mBody, "get_force_acc", VALUEFUNC(MSNewton::Body::get_force_acc), 1);
	rb_define_module_function(mBody, "add_force", VALUEFUNC(MSNewton::Body::add_force), 2);
	rb_define_module_function(mBody, "add_force2", VALUEFUNC(MSNewton::Body::add_force2), 2);
	rb_define_module_function(mBody, "set_force", VALUEFUNC(MSNewton::Body::set_force), 2);
	rb_define_module_function(mBody, "set_force2", VALUEFUNC(MSNewton::Body::set_force2), 2);
	rb_define_module_function(mBody, "get_torque", VALUEFUNC(MSNewton::Body::get_torque), 1);
	rb_define_module_function(mBody, "get_torque_acc", VALUEFUNC(MSNewton::Body::get_torque_acc), 1);
	rb_define_module_function(mBody, "add_torque", VALUEFUNC(MSNewton::Body::add_torque), 2);
	rb_define_module_function(mBody, "add_torque2", VALUEFUNC(MSNewton::Body::add_torque2), 2);
	rb_define_module_function(mBody, "set_torque", VALUEFUNC(MSNewton::Body::set_torque), 2);
	rb_define_module_function(mBody, "set_torque2", VALUEFUNC(MSNewton::Body::set_torque2), 2);
	rb_define_module_function(mBody, "get_net_contact_force", VALUEFUNC(MSNewton::Body::get_net_contact_force), 1);
	rb_define_module_function(mBody, "get_contacts", VALUEFUNC(MSNewton::Body::get_contacts), 2);
	rb_define_module_function(mBody, "get_touching_bodies", VALUEFUNC(MSNewton::Body::get_touching_bodies), 2);
	rb_define_module_function(mBody, "get_contact_points", VALUEFUNC(MSNewton::Body::get_contact_points), 2);
	rb_define_module_function(mBody, "get_collision_faces", VALUEFUNC(MSNewton::Body::get_collision_faces), 1);
	rb_define_module_function(mBody, "get_collision_faces2", VALUEFUNC(MSNewton::Body::get_collision_faces2), 1);
	rb_define_module_function(mBody, "get_collision_faces3", VALUEFUNC(MSNewton::Body::get_collision_faces3), 1);
	rb_define_module_function(mBody, "apply_pick_and_drag", VALUEFUNC(MSNewton::Body::apply_pick_and_drag), 5);
	rb_define_module_function(mBody, "apply_pick_and_drag2", VALUEFUNC(MSNewton::Body::apply_pick_and_drag2), 6);
	rb_define_module_function(mBody, "apply_buoyancy", VALUEFUNC(MSNewton::Body::apply_buoyancy), 7);
	rb_define_module_function(mBody, "apply_fluid_resistance", VALUEFUNC(MSNewton::Body::apply_fluid_resistance), 2);
	rb_define_module_function(mBody, "copy", VALUEFUNC(MSNewton::Body::copy), 3);
	rb_define_module_function(mBody, "get_destructor_proc", VALUEFUNC(MSNewton::Body::get_destructor_proc), 1);
	rb_define_module_function(mBody, "set_destructor_proc", VALUEFUNC(MSNewton::Body::set_destructor_proc), 2);
	rb_define_module_function(mBody, "get_user_data", VALUEFUNC(MSNewton::Body::get_user_data), 1);
	rb_define_module_function(mBody, "set_user_data", VALUEFUNC(MSNewton::Body::set_user_data), 2);
	rb_define_module_function(mBody, "get_record_touch_data_state", VALUEFUNC(MSNewton::Body::get_record_touch_data_state), 1);
	rb_define_module_function(mBody, "set_record_touch_data_state", VALUEFUNC(MSNewton::Body::set_record_touch_data_state), 2);
	rb_define_module_function(mBody, "get_matrix_scale", VALUEFUNC(MSNewton::Body::get_matrix_scale), 1);
	rb_define_module_function(mBody, "set_matrix_scale", VALUEFUNC(MSNewton::Body::set_matrix_scale), 2);
	rb_define_module_function(mBody, "matrix_changed?", VALUEFUNC(MSNewton::Body::matrix_changed), 1);
	rb_define_module_function(mBody, "enable_gravity", VALUEFUNC(MSNewton::Body::enable_gravity), 2);
	rb_define_module_function(mBody, "is_gravity_enabled?", VALUEFUNC(MSNewton::Body::is_gravity_enabled), 1);
	rb_define_module_function(mBody, "get_contained_joints", VALUEFUNC(MSNewton::Body::get_contained_joints), 1);
	rb_define_module_function(mBody, "get_connected_joints", VALUEFUNC(MSNewton::Body::get_connected_joints), 1);
	rb_define_module_function(mBody, "get_connected_bodies", VALUEFUNC(MSNewton::Body::get_connected_bodies), 1);
	rb_define_module_function(mBody, "get_material_id", VALUEFUNC(MSNewton::Body::get_material_id), 1);
	rb_define_module_function(mBody, "set_material_id", VALUEFUNC(MSNewton::Body::set_material_id), 2);
	rb_define_module_function(mBody, "get_collision_scale", VALUEFUNC(MSNewton::Body::get_collision_scale), 1);
	rb_define_module_function(mBody, "set_collision_scale", VALUEFUNC(MSNewton::Body::set_collision_scale), 2);
	rb_define_module_function(mBody, "get_default_collision_scale", VALUEFUNC(MSNewton::Body::get_default_collision_scale), 1);
	rb_define_module_function(mBody, "get_actual_matrix_scale", VALUEFUNC(MSNewton::Body::get_actual_matrix_scale), 1);
}
