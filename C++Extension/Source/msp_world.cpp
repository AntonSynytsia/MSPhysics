#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::World::destructor_callback(const NewtonWorld* const world) {
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	valid_worlds.erase(world);
	world_data->touch_data.clear();
	world_data->touching_data.clear();
	world_data->untouch_data.clear();
	world_data->joints_to_disconnect.clear();
	if (RARRAY_LEN(world_data->destructor_proc) == 1 && rb_ary_entry(world_data->destructor_proc, 0) != Qnil)
		rb_rescue2(RUBY_METHOD_FUNC(Util::call_proc), rb_ary_entry(world_data->destructor_proc, 0), RUBY_METHOD_FUNC(Util::rescue_proc), Qnil, rb_eException, (VALUE)0);
	rb_ary_clear(world_data->destructor_proc);
	rb_ary_clear(world_data->user_data);
	rb_gc_unregister_address(&world_data->destructor_proc);
	rb_gc_unregister_address(&world_data->user_data);
	#ifndef RUBY_VERSION18
		rb_ary_free(world_data->destructor_proc);
		rb_ary_free(world_data->user_data);
	#endif
	for (std::map<JointData*, bool>::iterator it = valid_joints.begin(); it != valid_joints.end();) {
		JointData* joint_data = it->first;
		++it;
		if (joint_data->world == world)
			MSNewton::Joint::c_destroy(joint_data);
	}
	delete world_data;
}

int MSNewton::World::aabb_overlap_callback(const NewtonMaterial* const material, const NewtonBody* const body0, const NewtonBody* const body1, int thread_index) {
	BodyData* data0 = (BodyData*)NewtonBodyGetUserData(body0);
	BodyData* data1 = (BodyData*)NewtonBodyGetUserData(body1);
	NewtonWorld* world = NewtonBodyGetWorld(body0);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if ((data0->collidable == false || data1->collidable == false) ||
		(data0->bstatic == true && data1->bstatic == true) ||
		(NewtonBodyGetFreezeState(body0) == 1 && NewtonBodyGetFreezeState(body1) == 1) ||
		(data0->bstatic == true && NewtonBodyGetFreezeState(body1) == 1) ||
		(data1->bstatic == true && NewtonBodyGetFreezeState(body0) == 1) ||
		(data0->non_collidable_bodies.find(body1) != data0->non_collidable_bodies.end()) ||
		(data1->non_collidable_bodies.find(body0) != data1->non_collidable_bodies.end())) {
		if (NewtonBodyGetContinuousCollisionMode(body0) == 1) {
			NewtonBodySetContinuousCollisionMode(body0, 0);
			world_data->temp_cccd_bodies.push_back(body0);
		}
		if (NewtonBodyGetContinuousCollisionMode(body1) == 1) {
			NewtonBodySetContinuousCollisionMode(body1, 0);
			world_data->temp_cccd_bodies.push_back(body1);
		}
		return 0;
	}
	else {
		if (NewtonBodyGetFreezeState(body0) == 1)
			NewtonBodySetFreezeState(body0, 0);
		if (NewtonBodyGetFreezeState(body1) == 1)
			NewtonBodySetFreezeState(body1, 0);
		return 1;
	}
}

void MSNewton::World::contact_callback(const NewtonJoint* const contact_joint, dFloat timestep, int thread_index) {
	const NewtonBody* body0 = NewtonJointGetBody0(contact_joint);
	const NewtonBody* body1 = NewtonJointGetBody1(contact_joint);
	BodyData* data0 = (BodyData*)NewtonBodyGetUserData(body0);
	BodyData* data1 = (BodyData*)NewtonBodyGetUserData(body1);
	for (void* contact = NewtonContactJointGetFirstContact(contact_joint); contact; contact = NewtonContactJointGetNextContact(contact_joint, contact)) {
		NewtonMaterial* material = NewtonContactGetMaterial(contact);
		if (data0->friction_enabled == true && data1->friction_enabled == true) {
			dFloat sfc = (data0->static_friction + data1->static_friction) * 0.5f;
			dFloat kfc = (data0->dynamic_friction + data1->dynamic_friction) * 0.5f;
			NewtonMaterialSetContactFrictionCoef(material, sfc, kfc, 0);
			NewtonMaterialSetContactFrictionCoef(material, sfc, kfc, 1);
		}
		else {
			NewtonMaterialSetContactFrictionState(material, 0, 0);
			NewtonMaterialSetContactFrictionState(material, 0, 1);
		}
		dFloat cor = (data0->elasticity + data1->elasticity) * 0.5f;
		dFloat sft = (data0->softness + data1->softness) * 0.5f;
		NewtonMaterialSetContactElasticity(material, cor);
		NewtonMaterialSetContactSoftness(material, sft);
	}
	const NewtonWorld* world = NewtonBodyGetWorld(body0);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (world_data->process_info) {
		void* contact = NewtonContactJointGetFirstContact(contact_joint);
		const NewtonMaterial* material = NewtonContactGetMaterial(contact);
		if (data0->record_touch_data) {
			if (data0->touchers.find(body1) == data0->touchers.end()) {
				dVector point;
				dVector normal;
				dVector force;
				NewtonMaterialGetContactPositionAndNormal(material, body0, &point[0], &normal[0]);
				NewtonMaterialGetContactForce(material, body0, &force[0]);
				BodyTouchData touch_data;
				touch_data.body0 = body0;
				touch_data.body1 = body1;
				touch_data.point = point;
				touch_data.normal = normal;
				touch_data.force = force;
				touch_data.speed = NewtonMaterialGetContactNormalSpeed(material);
				world_data->touch_data.push_back(touch_data);
				data0->touchers[body1] = 0;
			}
			else
				data0->touchers[body1] = 2;
		}
		if (data1->record_touch_data) {
			if (data1->touchers.find(body0) == data1->touchers.end()) {
				dVector point;
				dVector normal;
				dVector force;
				NewtonMaterialGetContactPositionAndNormal(material, body1, &point[0], &normal[0]);
				NewtonMaterialGetContactForce(material, body1, &force[0]);
				BodyTouchData touch_data;
				touch_data.body0 = body1;
				touch_data.body1 = body0;
				touch_data.point = point;
				touch_data.normal = normal;
				touch_data.force = force;
				touch_data.speed = NewtonMaterialGetContactNormalSpeed(material);
				world_data->touch_data.push_back(touch_data);
				data1->touchers[body0] = 0;
			}
			else
				data1->touchers[body0] = 2;
		}
	}
}

unsigned MSNewton::World::ray_prefilter_callback(const NewtonBody* const body, const NewtonCollision* const collision, void* const user_data) {
	bool* continuous = (bool*)user_data;
	return (*continuous == true) ? 1 : 0;
}

dFloat MSNewton::World::ray_filter_callback(const NewtonBody* const body, const NewtonCollision* const shape_hit, const dFloat* const hit_contact, const dFloat* const hit_normal, dLong collision_id, void* const user_data, dFloat intersect_param) {
	Hit* hit = (Hit*)user_data;
	hit->body = body;
	hit->point = dVector(hit_contact);
	hit->normal = dVector(hit_normal);
	return intersect_param;
}

dFloat MSNewton::World::continuous_ray_filter_callback(const NewtonBody* const body, const NewtonCollision* const shape_hit, const dFloat* const hit_contact, const dFloat* const hit_normal, dLong collision_id, void* const user_data, dFloat intersect_param) {
	RayData* ray_data = (RayData*)user_data;
	Hit hit;
	hit.body = body;
	hit.point = dVector(hit_contact);
	hit.normal = dVector(hit_normal);
	ray_data->hits.push_back(hit);
	return 1.0f;
}

int MSNewton::World::body_iterator(const NewtonBody* const body, void* const user_data) {
	rb_ary_push((VALUE)user_data, Util::to_value(body));
	return 1;
}

void MSNewton::World::collision_copy_constructor_callback(const NewtonWorld* const world, NewtonCollision* const collision, const NewtonCollision* const source_collision) {
	valid_collisions[collision] = dVector(valid_collisions[source_collision]);
}

void MSNewton::World::collision_destructor_callback(const NewtonWorld* const world, const NewtonCollision* const collision) {
	valid_collisions.erase(collision);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::World::c_update_magnets(const NewtonWorld* world) {
	dMatrix matrix;
	dVector com;
	dMatrix other_matrix;
	dVector other_com;
	dVector dir;
	for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
		BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
		if (data->magnet_force == 0.0f || data->magnet_range == 0.0f) continue;
		NewtonBodyGetMatrix(body, &matrix[0][0]);
		NewtonBodyGetCentreOfMass(body, &com[0]);
		com = matrix.TransformVector(com);
		for (const NewtonBody* other_body = NewtonWorldGetFirstBody(world); other_body; other_body = NewtonWorldGetNextBody(world, other_body)) {
			if (other_body == body) continue;
			BodyData* other_data = (BodyData*)NewtonBodyGetUserData(other_body);
			if (!other_data->magnetic) continue;
			NewtonBodyGetMatrix(other_body, &other_matrix[0][0]);
			NewtonBodyGetCentreOfMass(other_body, &other_com[0]);
			other_com = other_matrix.TransformVector(other_com);
			dir = other_com - com;
			dFloat dist = sqrt(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
			if( dist == 0.0f || dist >= data->magnet_range ) continue;
			dir = dir.Scale( (data->magnet_range - dist) * data->magnet_force / (data->magnet_range * dist) );
			other_data->add_force -= dir;
			other_data->add_force_state = true;
			// For every action there is an equal and opposite reaction!
			data->add_force += dir;
			data->add_force_state = true;
		}
	}
}

void MSNewton::World::c_process_touch_events(const NewtonWorld* world) {
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);

	// Generate onTouch events for non-collidable bodies.
	for (const NewtonBody* body0 = NewtonWorldGetFirstBody(world); body0; body0 = NewtonWorldGetNextBody(world, body0)) {
		BodyData* body0_data = (BodyData*)NewtonBodyGetUserData(body0);
		if (body0_data->record_touch_data == false) continue;
		NewtonCollision* colA = NewtonBodyGetCollision(body0);
		NewtonCollision* colB;
		dMatrix matA;
		dMatrix matB;
		NewtonBodyGetMatrix(body0, &matA[0][0]);
		dFloat points[3];
		dFloat normals[3];
		dFloat penetrations[3];
		long long attrA[1];
		long long attrB[1];
		for (const NewtonBody* body1 = NewtonWorldGetFirstBody(world); body1; body1 = NewtonWorldGetNextBody(world, body1)) {
			if (body0 == body1 || Util::bodies_collidable(body0, body1) == true || Util::bodies_aabb_overlap(body0, body1) == false) continue;
			colB = NewtonBodyGetCollision(body1);
			NewtonBodyGetMatrix(body1, &matB[0][0]);
			int count = NewtonCollisionCollide(world, 1, colA, &matA[0][0], colB, &matB[0][0], points, normals, penetrations, attrA, attrB, 0);
			if (count == 0) continue;
			BodyTouchData touch_data;
			touch_data.body0 = body0;
			touch_data.body1 = body1;
			touch_data.point = dVector(points);
			touch_data.normal = dVector(normals);
			touch_data.force = dVector(0.0f, 0.0f, 0.0f);
			touch_data.speed = 0.0f;
			world_data->touch_data.push_back(touch_data);
			body0_data->touchers[body1] = 0;
		}
	}

	// Generate onTouching and onUntouch events for all bodies with touchers.
	for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
		BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
		if (body_data->record_touch_data == false || body_data->touchers.empty() == true) continue;
		NewtonCollision* colA = NewtonBodyGetCollision(body);
		dMatrix matrixA;
		NewtonBodyGetMatrix(body, &matrixA[0][0]);
		std::vector<const NewtonBody*> to_erase;
		for (std::map<const NewtonBody* ,short>::iterator it = body_data->touchers.begin(); it != body_data->touchers.end(); ++it) {
			if (it->second == 0) {
				body_data->touchers[it->first] = 1;
				continue;
			}
			bool touching = false;
			if (it->second == 1) {
				NewtonCollision* colB = NewtonBodyGetCollision(it->first);
				dMatrix matrixB;
				NewtonBodyGetMatrix(it->first, &matrixB[0][0]);
				touching = NewtonCollisionIntersectionTest(world, colA, &matrixA[0][0], colB, &matrixB[0][0], 0) == 1;
				if (touching == false && NewtonCollisionGetType(colA) < 11 && NewtonCollisionGetType(colB) < 11) {
					dVector pointA;
					dVector pointB;
					dVector normalAB;
					NewtonCollisionClosestPoint(world, colA, &matrixA[0][0], colB, &matrixB[0][0], &pointA[0], &pointB[0], &normalAB[0], 0);
					dVector diff = pointB - pointA;
					dFloat dist = sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]) - world_data->material_thinkness;
					if (dist < MIN_TOUCH_DISTANCE) touching = true;
				}
			}
			else if (it->second == 2) {
				touching = true;
				body_data->touchers[it->first] = 1;
			}
			if (touching) {
				BodyTouchingData touching_data;
				touching_data.body0 = body;
				touching_data.body1 = it->first;
				world_data->touching_data.push_back(touching_data);
			}
			else {
				BodyUntouchData untouch_data;
				untouch_data.body0 = body;
				untouch_data.body1 = it->first;
				world_data->untouch_data.push_back(untouch_data);
				to_erase.push_back(it->first);
			}
		}
		for (std::vector<const NewtonBody*>::iterator it = to_erase.begin(); it != to_erase.end(); ++it)
			body_data->touchers.erase(*it);
		//for (unsigned int i = 0; i < to_erase.size(); ++i)
			//body_data->touchers.erase(to_erase[i]);
	}
}

void MSNewton::World::c_clear_touch_events(const NewtonWorld* world) {
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	data->touch_data.clear();
	data->touching_data.clear();
	data->untouch_data.clear();
}

void MSNewton::World::c_clear_matrix_change_record(const NewtonWorld* world) {
	for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
		BodyData* data = (BodyData*)NewtonBodyGetUserData(body);
		data->matrix_changed = false;
	}
}

void MSNewton::World::c_disconnect_flagged_joints(const NewtonWorld* world) {
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	for (std::vector<JointData*>::iterator it = world_data->joints_to_disconnect.begin(); it != world_data->joints_to_disconnect.end(); ++it)
		if (Util::is_joint_valid(*it)) {
			JointData* joint_data = *it;
			if (joint_data->constraint != nullptr)
				NewtonDestroyJoint(world, joint_data->constraint);
		}
	world_data->joints_to_disconnect.clear();
}

void MSNewton::World::c_enable_cccd_bodies(const NewtonWorld* world) {
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	for (std::vector<const NewtonBody*>::iterator it = world_data->temp_cccd_bodies.begin(); it != world_data->temp_cccd_bodies.end(); ++it)
		NewtonBodySetContinuousCollisionMode(*it, 1);
	world_data->temp_cccd_bodies.clear();
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::World::is_valid(VALUE self, VALUE v_world) {
	return Util::is_world_valid((const NewtonWorld*)Util::value_to_ll(v_world)) ? Qtrue : Qfalse;
}

VALUE MSNewton::World::create(VALUE self, VALUE v_world_scale) {
	dFloat world_scale = Util::clamp<dFloat>(Util::value_to_dFloat(v_world_scale), 0.1f, 100.0f);
	NewtonWorld* world = NewtonCreate();
	NewtonInvalidateCache(world);
	WorldData *data = new WorldData;
	valid_worlds[world] = true;
	data->solver_model = DEFAULT_SOLVER_MODEL;
	data->friction_model = DEFAULT_FRICTION_MODEL;
	data->material_thinkness = DEFAULT_MATERIAL_THICKNESS;
	data->gravity = dVector(DEFAULT_GRAVITY);
	data->destructor_proc = rb_ary_new();
	data->user_data = rb_ary_new();
	data->touch_data;
	data->touching_data;
	data->untouch_data;
	data->joints_to_disconnect;
	data->time = 0.0f;
	data->scale = world_scale;
	data->scale3 = world_scale * world_scale * world_scale;
	data->scale4 = data->scale3 * world_scale;
	data->inverse_scale = 1.0f / data->scale;
	data->inverse_scale3 = 1.0f / data->scale3;
	data->inverse_scale4 = 1.0f / data->scale4;
	data->gravity_enabled = false;
	data->process_info = true;
	data->temp_cccd_bodies;
	for (int i = 0; i < 3; ++i) {
		if (std::abs(data->gravity[i]) > EPSILON)
			data->gravity_enabled = true;
	}
	NewtonWorldSetUserData(world, data);
	rb_gc_register_address(&data->destructor_proc);
	rb_gc_register_address(&data->user_data);

	NewtonSetContactMergeTolerance(world, DEFAULT_CONTACT_MERGE_TOLERANCE);
	//int id = NewtonMaterialGetDefaultGroupID(world);
	int id = NewtonMaterialCreateGroupID(world);
	data->material_id = id;
	NewtonMaterialSetSurfaceThickness(world, id, id, DEFAULT_MATERIAL_THICKNESS);
	NewtonMaterialSetDefaultFriction(world, id, id, DEFAULT_STATIC_FRICTION, DEFAULT_DYNAMIC_FRICTION);
	NewtonMaterialSetDefaultElasticity(world, id, id, DEFAULT_ELASTICITY);
	NewtonMaterialSetDefaultSoftness(world, id, id, DEFAULT_SOFTNESS);
	NewtonSetSolverModel(world, DEFAULT_SOLVER_MODEL);
	NewtonSetFrictionModel(world, DEFAULT_FRICTION_MODEL);
	NewtonSetSolverConvergenceQuality(world, DEFAULT_CONVERGENCE_QUALITY);
	NewtonSelectBroadphaseAlgorithm(world, 0);

	NewtonMaterialSetCollisionCallback(world, id, id, aabb_overlap_callback, contact_callback);
	NewtonWorldSetDestructorCallback(world, destructor_callback);
	NewtonWorldSetCollisionConstructorDestructorCallback(world, collision_copy_constructor_callback, collision_destructor_callback);
	return Util::to_value(world);
}

VALUE MSNewton::World::destroy(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	NewtonDestroy(world);
	return Qnil;
}

VALUE MSNewton::World::get_max_threads_count(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	return Util::to_value( NewtonGetMaxThreadsCount(world) );
}

VALUE MSNewton::World::set_max_threads_count(VALUE self, VALUE v_world, VALUE v_count) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	NewtonSetThreadsCount(world, Util::clamp_min(Util::value_to_int(v_count), 1));
	return Util::to_value( NewtonGetThreadsCount(world) );
}

VALUE MSNewton::World::get_cur_threads_count(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	return Util::to_value( NewtonGetThreadsCount(world) );
}

VALUE MSNewton::World::destroy_all_bodies(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	int count = NewtonWorldGetBodyCount(world);
	NewtonDestroyAllBodies(world);
	return Util::to_value(count);
}

VALUE MSNewton::World::get_body_count(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	return Util::to_value( NewtonWorldGetBodyCount(world) );
}

VALUE MSNewton::World::get_constraint_count(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	return Util::to_value( NewtonWorldGetConstraintCount(world) );
}

VALUE MSNewton::World::update(int argc, VALUE* argv, VALUE self) {
	const NewtonWorld* world;
	dFloat timestep;
	int update_rate;
	if (argc == 2) {
		world = Util::value_to_world(argv[0]);
		timestep = Util::clamp<dFloat>(Util::value_to_dFloat(argv[1]), DG_MIN_TIMESTEP, DG_MAX_TIMESTEP);
		update_rate = 1;
	}
	else if (argc == 3) {
		world = Util::value_to_world(argv[0]);
		timestep = Util::clamp<dFloat>(Util::value_to_dFloat(argv[1]), DG_MIN_TIMESTEP, DG_MAX_TIMESTEP);
		update_rate = Util::clamp<int>(Util::value_to_int(argv[2]), 1, 100);
	}
	else
		rb_raise(rb_eArgError, "Wrong number of arguments! Expected 2..3 arguments.");
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	c_clear_matrix_change_record(world);
	world_data->process_info = false;
	for (int i = 0; i < update_rate - 1; ++i) {
		c_update_magnets(world);
		NewtonUpdate(world, timestep);
		c_enable_cccd_bodies(world);
		c_disconnect_flagged_joints(world);
	}
	world_data->process_info = true;
	c_clear_touch_events(world);
	c_update_magnets(world);
	NewtonUpdate(world, timestep);
	c_enable_cccd_bodies(world);
	c_disconnect_flagged_joints(world);
	c_process_touch_events(world);
	world_data->time += timestep * update_rate;
	return Util::to_value(timestep * update_rate);
}

VALUE MSNewton::World::update_async(int argc, VALUE* argv, VALUE self) {
	const NewtonWorld* world;
	dFloat timestep;
	int update_rate;
	if (argc == 2) {
		world = Util::value_to_world(argv[0]);
		timestep = Util::clamp<dFloat>(Util::value_to_dFloat(argv[1]), DG_MIN_TIMESTEP, DG_MAX_TIMESTEP);
		update_rate = 1;
	}
	else if (argc == 3) {
		world = Util::value_to_world(argv[0]);
		timestep = Util::clamp<dFloat>(Util::value_to_dFloat(argv[1]), DG_MIN_TIMESTEP, DG_MAX_TIMESTEP);
		update_rate = Util::clamp<int>(Util::value_to_int(argv[2]), 1, 100);
	}
	else
		rb_raise(rb_eArgError, "Wrong number of arguments! Expected 2..3 arguments.");
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	c_clear_matrix_change_record(world);
	world_data->process_info = false;
	for (int i = 0; i < update_rate - 1; ++i) {
		c_update_magnets(world);
		NewtonUpdateAsync(world, timestep);
		c_enable_cccd_bodies(world);
		c_disconnect_flagged_joints(world);
	}
	world_data->process_info = true;
	c_clear_touch_events(world);
	c_update_magnets(world);
	NewtonUpdate(world, timestep);
	c_enable_cccd_bodies(world);
	c_disconnect_flagged_joints(world);
	c_process_touch_events(world);
	world_data->time += timestep * update_rate;
	return Util::to_value(timestep * update_rate);
}

VALUE MSNewton::World::get_gravity(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::vector_to_value(world_data->gravity, world_data->inverse_scale);
}

VALUE MSNewton::World::set_gravity(VALUE self, VALUE v_world, VALUE v_gravity) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	world_data->gravity = Util::value_to_vector(v_gravity, world_data->scale);
	world_data->gravity_enabled = false;
	for (int i = 0; i < 3; ++i) {
		if (std::abs(world_data->gravity[i]) > EPSILON)
			world_data->gravity_enabled = true;
	}
	return Util::vector_to_value(world_data->gravity, world_data->inverse_scale);
}

VALUE MSNewton::World::get_bodies(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	VALUE v_bodies = rb_ary_new2(NewtonWorldGetBodyCount(world));
	int i = 0;
	for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
		rb_ary_store(v_bodies, i, Util::to_value(body));
		++i;
	}
	return v_bodies;
}

VALUE MSNewton::World::get_body_datas(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	VALUE v_body_datas = rb_ary_new();
	for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
		BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
		VALUE v_user_data = rb_ary_entry(body_data->user_data, 0);
		if (v_user_data != Qnil) rb_ary_push(v_body_datas, v_user_data);
	}
	return v_body_datas;
}

VALUE MSNewton::World::get_bodies_in_aabb(VALUE self, VALUE v_world, VALUE v_min_pt, VALUE v_max_pt) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	VALUE v_bodies = rb_ary_new();
	NewtonWorldForEachBodyInAABBDo(world, &Util::value_to_point(v_min_pt, world_data->scale)[0], &Util::value_to_point(v_max_pt, world_data->scale)[1], body_iterator, (void* const)v_bodies);
	return v_bodies;
}

VALUE MSNewton::World::get_first_body(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	NewtonBody* body = NewtonWorldGetFirstBody(world);
	return body ? Util::to_value(body) : Qnil;
}

VALUE MSNewton::World::get_next_body(VALUE self, VALUE v_world, VALUE v_body) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	const NewtonBody* body = Util::value_to_body(v_body);
	NewtonBody* next_body = NewtonWorldGetNextBody(world, body);
	return next_body ? Util::to_value(next_body) : Qnil;
}

VALUE MSNewton::World::get_solver_model(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value( data->solver_model );
}

VALUE MSNewton::World::set_solver_model(VALUE self, VALUE v_world, VALUE v_solver_model) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	data->solver_model = Util::clamp(Util::value_to_int(v_solver_model), 0, 256);
	NewtonSetSolverModel(world, data->solver_model);
	return Util::to_value( data->solver_model );
}

VALUE MSNewton::World::get_friction_model(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value( data->friction_model );
}

VALUE MSNewton::World::set_friction_model(VALUE self, VALUE v_world, VALUE v_friction_model) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	data->friction_model = Util::clamp(Util::value_to_int(v_friction_model), 0, 1);
	NewtonSetFrictionModel(world, data->friction_model);
	return Util::to_value( data->friction_model );
}

VALUE MSNewton::World::get_material_thickness(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value( data->material_thinkness );
}

VALUE MSNewton::World::set_material_thickness(VALUE self, VALUE v_world, VALUE v_material_thinkness) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* data = (WorldData*)NewtonWorldGetUserData(world);
	data->material_thinkness = Util::clamp<dFloat>(Util::value_to_dFloat(v_material_thinkness), 0.0f, 1.0f/32.0f);
	NewtonMaterialSetSurfaceThickness(world, data->material_id, data->material_id, data->material_thinkness);
	return Util::to_value( data->material_thinkness );
}

VALUE MSNewton::World::ray_cast(VALUE self, VALUE v_world, VALUE v_point1, VALUE v_point2) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector point1 = Util::value_to_point(v_point1, world_data->scale);
	dVector point2 = Util::value_to_point(v_point2, world_data->scale);
	Hit* hit = new Hit();
	NewtonWorldRayCast(world, &point1[0], &point2[0], ray_filter_callback, (void*)hit, NULL, 0);
	VALUE v_res =  hit->body ? rb_ary_new3(3, Util::to_value(hit->body), Util::point_to_value(hit->point, world_data->inverse_scale), Util::vector_to_value(hit->normal)) : Qnil;
	delete hit;
	return v_res;
}

VALUE MSNewton::World::continuous_ray_cast(VALUE self, VALUE v_world, VALUE v_point1, VALUE v_point2) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dVector point1 = Util::value_to_point(v_point1, world_data->scale);
	dVector point2 = Util::value_to_point(v_point2, world_data->scale);
	RayData* ray_data = new RayData();
	NewtonWorldRayCast(world, &point1[0], &point2[0], continuous_ray_filter_callback, (void*)ray_data, NULL, 0);
	VALUE hits = rb_ary_new2( (long)ray_data->hits.size() );
	for (unsigned int i = 0; i < ray_data->hits.size(); ++i) {
		VALUE hit = rb_ary_new3(3, Util::to_value(ray_data->hits[i].body), Util::point_to_value(ray_data->hits[i].point, world_data->inverse_scale), Util::vector_to_value(ray_data->hits[i].normal));
		rb_ary_store(hits, i, hit);
	}
	delete ray_data;
	return hits;
}

VALUE MSNewton::World::convex_ray_cast(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_target) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	const NewtonCollision* collision = Util::value_to_collision(v_collision);
	dMatrix matrix = Util::value_to_matrix(v_matrix, world_data->scale);
	dVector target = Util::value_to_point(v_target, world_data->scale);
	bool* continuous = new bool;
	*continuous = false;
	dFloat hit_param;
	NewtonWorldConvexCastReturnInfo info[1];
	int hit_count = NewtonWorldConvexCast(world, &matrix[0][0], &target[0], collision, &hit_param, (void*)continuous, ray_prefilter_callback, &info[0], 1, 0);
	delete continuous;
	if (hit_count != 0)
		return rb_ary_new3(
			4,
			Util::to_value(info[0].m_hitBody),
			Util::point_to_value(dVector(info[0].m_point), world_data->inverse_scale),
			Util::vector_to_value(dVector(info[0].m_normal)),
			Util::to_value(info[0].m_penetration));
	else
		return Qnil;
}

VALUE MSNewton::World::continuous_convex_ray_cast(VALUE self, VALUE v_world, VALUE v_collision, VALUE v_matrix, VALUE v_target, VALUE v_max_hits) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	const NewtonCollision* collision = Util::value_to_collision(v_collision);
	dMatrix matrix = Util::value_to_matrix(v_matrix, world_data->scale);
	dVector target = Util::value_to_point(v_target, world_data->scale);
	unsigned int max_hits = Util::clamp<unsigned int>(Util::value_to_uint(v_max_hits), 1, 256);
	bool* continuous = new bool;
	*continuous = true;
	dFloat hit_param;
	std::vector<NewtonWorldConvexCastReturnInfo> info((size_t)max_hits);
	int hit_count = NewtonWorldConvexCast(world, &matrix[0][0], &target[0], collision, &hit_param, (void*)continuous, ray_prefilter_callback, &info[0], max_hits, 0);
	delete continuous;
	VALUE hits = rb_ary_new2((long)hit_count);
	for (int i = 0; i < hit_count; ++i) {
		VALUE hit = rb_ary_new3(
			4,
			Util::to_value(info[i].m_hitBody),
			Util::point_to_value(dVector(info[i].m_point), world_data->inverse_scale),
			Util::vector_to_value(dVector(info[i].m_normal)),
			Util::to_value(info[i].m_penetration));
		rb_ary_store(hits, i, hit);
	}
	return hits;
}

VALUE MSNewton::World::add_explosion(VALUE self, VALUE v_world, VALUE v_center, VALUE v_blast_radius, VALUE v_blast_force) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dMatrix matrix;
	dVector point1 = Util::value_to_point(v_center, world_data->scale);
	dVector point2;
	dFloat blast_radius = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_blast_radius) * world_data->scale, 0.0f);
	dFloat blast_force = Util::clamp_min<dFloat>(Util::value_to_dFloat(v_blast_force) * world_data->scale3, 0.0f);
	if (blast_radius == 0.0f || blast_force == 0.0f)
		return Qfalse;
	for (const NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
		dFloat mass, ixx, iyy, izz;
		NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);
		if (mass == 0.0f) continue;
		NewtonBodyGetMatrix(body, &matrix[0][0]);
		NewtonBodyGetCentreOfMass(body, &point2[0]);
		point2 = matrix.TransformVector(point2);
		Hit hit;
		NewtonWorldRayCast(world, &point1[0], &point2[0], ray_filter_callback, &hit, NULL, 0);
		if (hit.body != body) continue;
		dVector force = hit.point - point1;
		dFloat mag = sqrt(force[0]*force[0] + force[1]*force[1] + force[2]*force[2]);
		if (mag == 0.0f || mag > blast_radius) continue;
		force = force.Scale( (blast_radius - mag) * blast_force / (blast_radius * mag) );
		//dVector r = hit.point - point2;
		//dVector torque = r * force;
		BodyData* body_data = (BodyData*)NewtonBodyGetUserData(body);
		body_data->add_force += force;
		body_data->add_force_state = true;
		//body_data->add_torque += torque;
		//body_data->add_torque_state = true;
	}
	return Qtrue;
}

VALUE MSNewton::World::get_aabb(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (NewtonWorldGetBodyCount(world) == 0) return Qnil;
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
	return rb_ary_new3(2, Util::point_to_value(world_min, world_data->inverse_scale), Util::point_to_value(world_max, world_data->inverse_scale));
}

VALUE MSNewton::World::get_destructor_proc(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (RARRAY_LEN(world_data->destructor_proc) == 0) return Qnil;
	return rb_ary_entry(world_data->destructor_proc, 0);
}

VALUE MSNewton::World::set_destructor_proc(VALUE self, VALUE v_world, VALUE v_proc) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (v_proc == Qnil)
		rb_ary_clear(world_data->destructor_proc);
	else if (rb_class_of(v_proc) == rb_cProc)
		rb_ary_store(world_data->destructor_proc, 0, v_proc);
	else
		rb_raise(rb_eTypeError, "Expected nil or a Proc object!");
	return Qtrue;
}

VALUE MSNewton::World::get_user_data(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (RARRAY_LEN(world_data->user_data) == 0) return Qnil;
	return rb_ary_entry(world_data->user_data, 0);
}

VALUE MSNewton::World::set_user_data(VALUE self, VALUE v_world, VALUE v_user_data) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	if (v_user_data == Qnil)
		rb_ary_clear(world_data->user_data);
	else
		rb_ary_store(world_data->user_data, 0, v_user_data);
	return world_data->user_data;
}

VALUE MSNewton::World::get_touch_data_at(VALUE self, VALUE v_world, VALUE v_index) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	unsigned int index = Util::value_to_uint(v_index);
	if (index < 0 || index >= world_data->touch_data.size()) return Qnil;
	BodyTouchData touch_data = world_data->touch_data[index];
	VALUE v_touch_data = rb_ary_new2(6);
	rb_ary_store(v_touch_data, 0, Util::to_value(touch_data.body0));
	rb_ary_store(v_touch_data, 1, Util::to_value(touch_data.body1));
	rb_ary_store(v_touch_data, 2, Util::point_to_value(touch_data.point, world_data->inverse_scale));
	rb_ary_store(v_touch_data, 3, Util::vector_to_value(touch_data.normal));
	BodyData* body0_data = (BodyData*)NewtonBodyGetUserData(touch_data.body0);
	BodyData* body1_data = (BodyData*)NewtonBodyGetUserData(touch_data.body1);
	dVector force(touch_data.force);
	for (int i = 0; i < 3; ++i)
		force[i] *= world_data->inverse_scale3;
	if (world_data->gravity_enabled && (body0_data->gravity_enabled || body1_data->gravity_enabled)) {
		for (int i = 0; i < 3; ++i)
			force[i] *= world_data->inverse_scale;
	}
	rb_ary_store(v_touch_data, 4, Util::vector_to_value(force));
	rb_ary_store(v_touch_data, 5, Util::to_value(touch_data.speed * world_data->inverse_scale));
	return v_touch_data;
}

VALUE MSNewton::World::get_touch_data_count(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value( (long)world_data->touch_data.size() );
}

VALUE MSNewton::World::get_touching_data_at(VALUE self, VALUE v_world, VALUE v_index) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	unsigned int index = Util::value_to_uint(v_index);
	if (index < 0 || index >= world_data->touching_data.size()) return Qnil;
	BodyTouchingData touching_data = world_data->touching_data[index];
	VALUE v_touching_data = rb_ary_new2(2);
	rb_ary_store(v_touching_data, 0, Util::to_value(touching_data.body0));
	rb_ary_store(v_touching_data, 1, Util::to_value(touching_data.body1));
	return v_touching_data;
}

VALUE MSNewton::World::get_touching_data_count(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value( (long)world_data->touching_data.size() );
}

VALUE MSNewton::World::get_untouch_data_at(VALUE self, VALUE v_world, VALUE v_index) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	unsigned int index = Util::value_to_uint(v_index);
	if (index < 0 || index >= world_data->untouch_data.size()) return Qnil;
	BodyUntouchData untouch_data = world_data->untouch_data[index];
	VALUE v_untouch_data = rb_ary_new2(2);
	rb_ary_store(v_untouch_data, 0, Util::to_value(untouch_data.body0));
	rb_ary_store(v_untouch_data, 1, Util::to_value(untouch_data.body1));
	return v_untouch_data;
}

VALUE MSNewton::World::get_untouch_data_count(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value( (long)world_data->untouch_data.size() );
}

VALUE MSNewton::World::get_time(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value( world_data->time );
}

VALUE MSNewton::World::serialize_to_file(VALUE self, VALUE v_world, VALUE v_full_path) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	NewtonSerializeToFile(world, Util::value_to_c_str(v_full_path), NULL, NULL);
	return Qnil;
}

VALUE MSNewton::World::get_contact_merge_tolerance(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	return Util::to_value( NewtonGetContactMergeTolerance(world) );
}

VALUE MSNewton::World::set_contact_merge_tolerance(VALUE self, VALUE v_world, VALUE v_tolerance) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	dFloat tolerance = Util::value_to_dFloat(v_tolerance);
	NewtonSetContactMergeTolerance(world, tolerance);
	return Util::to_value( NewtonGetContactMergeTolerance(world) );
}

VALUE MSNewton::World::get_scale(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value( world_data->scale );
}

VALUE MSNewton::World::get_joints(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	VALUE v_joints = rb_ary_new();
	for (std::map<JointData*, bool>::iterator it = valid_joints.begin(); it != valid_joints.end(); ++it)
		if (it->first->world == world)
			rb_ary_push(v_joints, Util::to_value(it->first));
	return v_joints;
}

VALUE MSNewton::World::get_joint_datas(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	VALUE v_joint_datas = rb_ary_new();
	for (std::map<JointData*, bool>::iterator it = valid_joints.begin(); it != valid_joints.end(); ++it)
		if (it->first->world == world) {
			VALUE v_user_data = rb_ary_entry(it->first->user_data, 0);
			if (v_user_data != Qnil) rb_ary_push(v_joint_datas, v_user_data);
		}
	return v_joint_datas;
}

VALUE MSNewton::World::get_default_material_id(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	return Util::to_value(world_data->material_id);
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void Init_msp_world(VALUE mNewton) {
	VALUE mWorld = rb_define_module_under(mNewton, "World");

	rb_define_module_function(mWorld, "is_valid?", VALUEFUNC(MSNewton::World::is_valid), 1);
	rb_define_module_function(mWorld, "create", VALUEFUNC(MSNewton::World::create), 1);
	rb_define_module_function(mWorld, "destroy", VALUEFUNC(MSNewton::World::destroy), 1);
	rb_define_module_function(mWorld, "get_max_threads_count", VALUEFUNC(MSNewton::World::get_max_threads_count), 1);
	rb_define_module_function(mWorld, "set_max_threads_count", VALUEFUNC(MSNewton::World::set_max_threads_count), 2);
	rb_define_module_function(mWorld, "get_cur_threads_count", VALUEFUNC(MSNewton::World::get_cur_threads_count), 1);
	rb_define_module_function(mWorld, "destroy_all_bodies", VALUEFUNC(MSNewton::World::destroy_all_bodies), 1);
	rb_define_module_function(mWorld, "get_body_count", VALUEFUNC(MSNewton::World::get_body_count), 1);
	rb_define_module_function(mWorld, "get_constraint_count", VALUEFUNC(MSNewton::World::get_constraint_count), 1);
	rb_define_module_function(mWorld, "update", VALUEFUNC(MSNewton::World::update), -1);
	rb_define_module_function(mWorld, "update_async", VALUEFUNC(MSNewton::World::update_async), -1);
	rb_define_module_function(mWorld, "get_gravity", VALUEFUNC(MSNewton::World::get_gravity), 1);
	rb_define_module_function(mWorld, "set_gravity", VALUEFUNC(MSNewton::World::set_gravity), 2);
	rb_define_module_function(mWorld, "get_bodies", VALUEFUNC(MSNewton::World::get_bodies), 1);
	rb_define_module_function(mWorld, "get_body_datas", VALUEFUNC(MSNewton::World::get_body_datas), 1);
	rb_define_module_function(mWorld, "get_bodies_in_aabb", VALUEFUNC(MSNewton::World::get_bodies_in_aabb), 3);
	rb_define_module_function(mWorld, "get_first_body", VALUEFUNC(MSNewton::World::get_first_body), 1);
	rb_define_module_function(mWorld, "get_next_body", VALUEFUNC(MSNewton::World::get_next_body), 2);
	rb_define_module_function(mWorld, "get_solver_model", VALUEFUNC(MSNewton::World::get_solver_model), 1);
	rb_define_module_function(mWorld, "set_solver_model", VALUEFUNC(MSNewton::World::set_solver_model), 2);
	rb_define_module_function(mWorld, "get_friction_model", VALUEFUNC(MSNewton::World::get_friction_model), 1);
	rb_define_module_function(mWorld, "set_friction_model", VALUEFUNC(MSNewton::World::set_friction_model), 2);
	rb_define_module_function(mWorld, "get_material_thickness", VALUEFUNC(MSNewton::World::get_material_thickness), 1);
	rb_define_module_function(mWorld, "set_material_thickness", VALUEFUNC(MSNewton::World::set_material_thickness), 2);
	rb_define_module_function(mWorld, "ray_cast", VALUEFUNC(MSNewton::World::ray_cast), 3);
	rb_define_module_function(mWorld, "continuous_ray_cast", VALUEFUNC(MSNewton::World::continuous_ray_cast), 3);
	rb_define_module_function(mWorld, "convex_ray_cast", VALUEFUNC(MSNewton::World::convex_ray_cast), 4);
	rb_define_module_function(mWorld, "continuous_convex_ray_cast", VALUEFUNC(MSNewton::World::continuous_convex_ray_cast), 5);
	rb_define_module_function(mWorld, "add_explosion", VALUEFUNC(MSNewton::World::add_explosion), 4);
	rb_define_module_function(mWorld, "get_aabb", VALUEFUNC(MSNewton::World::get_aabb), 1);
	rb_define_module_function(mWorld, "get_destructor_proc", VALUEFUNC(MSNewton::World::get_destructor_proc), 1);
	rb_define_module_function(mWorld, "set_destructor_proc", VALUEFUNC(MSNewton::World::set_destructor_proc), 2);
	rb_define_module_function(mWorld, "get_user_data", VALUEFUNC(MSNewton::World::get_user_data), 1);
	rb_define_module_function(mWorld, "set_user_data", VALUEFUNC(MSNewton::World::set_user_data), 2);
	rb_define_module_function(mWorld, "get_touch_data_at", VALUEFUNC(MSNewton::World::get_touch_data_at), 2);
	rb_define_module_function(mWorld, "get_touch_data_count", VALUEFUNC(MSNewton::World::get_touch_data_count), 1);
	rb_define_module_function(mWorld, "get_touching_data_at", VALUEFUNC(MSNewton::World::get_touching_data_at), 2);
	rb_define_module_function(mWorld, "get_touching_data_count", VALUEFUNC(MSNewton::World::get_touching_data_count), 1);
	rb_define_module_function(mWorld, "get_untouch_data_at", VALUEFUNC(MSNewton::World::get_untouch_data_at), 2);
	rb_define_module_function(mWorld, "get_untouch_data_count", VALUEFUNC(MSNewton::World::get_untouch_data_count), 1);
	rb_define_module_function(mWorld, "get_time", VALUEFUNC(MSNewton::World::get_time), 1);
	rb_define_module_function(mWorld, "serialize_to_file", VALUEFUNC(MSNewton::World::serialize_to_file), 2);
	rb_define_module_function(mWorld, "get_contact_merge_tolerance", VALUEFUNC(MSNewton::World::get_contact_merge_tolerance), 1);
	rb_define_module_function(mWorld, "set_contact_merge_tolerance", VALUEFUNC(MSNewton::World::set_contact_merge_tolerance), 2);
	rb_define_module_function(mWorld, "get_scale", VALUEFUNC(MSNewton::World::get_scale), 1);
	rb_define_module_function(mWorld, "get_joints", VALUEFUNC(MSNewton::World::get_joints), 1);
	rb_define_module_function(mWorld, "get_joint_datas", VALUEFUNC(MSNewton::World::get_joint_datas), 1);
	rb_define_module_function(mWorld, "get_default_material_id", VALUEFUNC(MSNewton::World::get_default_material_id), 1);
}
