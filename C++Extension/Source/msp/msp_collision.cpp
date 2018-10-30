#include "msp_collision.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::Collision::MIN_SIZE(1.0e-4f);
const dFloat MSP::Collision::MAX_SIZE(1.0e5f);


/*
 ///////////////////////////////////////////////////////////////////////////////
  Variables
 ///////////////////////////////////////////////////////////////////////////////
*/

std::map<const NewtonCollision*, MSP::Collision::CollisionData*> MSP::Collision::s_valid_collisions;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

bool MSP::Collision::c_is_collision_valid(const NewtonCollision* address) {
	return s_valid_collisions.find(address) != s_valid_collisions.end();
}

const NewtonCollision* MSP::Collision::c_value_to_collision(VALUE v_collision) {
	const NewtonCollision* address = reinterpret_cast<NewtonCollision*>(rb_num2ull(v_collision));
	if (Util::s_validate_objects && s_valid_collisions.find(address) == s_valid_collisions.end())
		rb_raise(rb_eTypeError, "Given address doesn't reference a valid collision!");
	return address;
}

VALUE MSP::Collision::c_collision_to_value(const NewtonCollision* collision) {
	return rb_ull2inum(reinterpret_cast<unsigned long long>(collision));
}

bool MSP::Collision::c_is_collision_convex(const NewtonCollision* collision) {
	return NewtonCollisionGetType(collision) < 7;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Collision::rbf_create_null(VALUE self, VALUE v_world) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	const NewtonCollision* col = NewtonCreateNull(world);
	s_valid_collisions[col] = new CollisionData;
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_box(VALUE self, VALUE v_world, VALUE v_width, VALUE v_height, VALUE v_depth, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	const NewtonCollision* col = NewtonCreateBox(
		world,
		Util::clamp_float(Util::value_to_dFloat(v_width), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_height), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_depth), MIN_SIZE, MAX_SIZE),
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	s_valid_collisions[col] = new CollisionData;
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_sphere(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	const NewtonCollision* col = NewtonCreateSphere(
		world,
		Util::clamp_float(Util::value_to_dFloat(v_radius), MIN_SIZE, MAX_SIZE),
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	s_valid_collisions[col] = new CollisionData;
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_scaled_sphere(VALUE self, VALUE v_world, VALUE v_width, VALUE v_height, VALUE v_depth, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	dFloat w = Util::clamp_float(Util::value_to_dFloat(v_width), MIN_SIZE, MAX_SIZE);
	dFloat h = Util::clamp_float(Util::value_to_dFloat(v_height), MIN_SIZE, MAX_SIZE);
	dFloat d = Util::clamp_float(Util::value_to_dFloat(v_depth), MIN_SIZE, MAX_SIZE);
	dFloat r = Util::min_float(d, Util::min_float(h, w));
	dFloat ir = 1.0f / r;
	const NewtonCollision* col = NewtonCreateSphere(
		world,
		r * 0.5f,
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	dFloat sx = w * ir;
	dFloat sy = h * ir;
	dFloat sz = d * ir;
	NewtonCollisionSetScale(col, sx, sy, sz);
	s_valid_collisions[col] = new CollisionData(sx, sy, sz);
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_cone(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	const NewtonCollision* col = NewtonCreateCone(
		world,
		Util::clamp_float(Util::value_to_dFloat(v_radius), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_height), MIN_SIZE, MAX_SIZE),
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	s_valid_collisions[col] = new CollisionData;
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_scaled_cone(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	dFloat rx = Util::clamp_float(Util::value_to_dFloat(v_radiusx), MIN_SIZE, MAX_SIZE);
	dFloat ry = Util::clamp_float(Util::value_to_dFloat(v_radiusy), MIN_SIZE, MAX_SIZE);
	dFloat h = Util::clamp_float(Util::value_to_dFloat(v_height), MIN_SIZE, MAX_SIZE);
	dFloat r = Util::min_float(rx, ry);
	dFloat ir = 1.0f / r;
	const NewtonCollision* col = NewtonCreateCone(
		world,
		r,
		h,
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	dFloat sx = 1.0f;
	dFloat sy = ry * ir;
	dFloat sz = rx * ir;
	NewtonCollisionSetScale(col, sx, sy, sz);
	s_valid_collisions[col] = new CollisionData(sx, sy, sz);
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_cylinder(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	const NewtonCollision* col = NewtonCreateCylinder(
		world,
		Util::clamp_float(Util::value_to_dFloat(v_radius), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_radius), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_height), MIN_SIZE, MAX_SIZE),
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	s_valid_collisions[col] = new CollisionData;
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_scaled_cylinder(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	dFloat rx = Util::clamp_float(Util::value_to_dFloat(v_radiusx), MIN_SIZE, MAX_SIZE);
	dFloat ry = Util::clamp_float(Util::value_to_dFloat(v_radiusy), MIN_SIZE, MAX_SIZE);
	dFloat h = Util::clamp_float(Util::value_to_dFloat(v_height), MIN_SIZE, MAX_SIZE);
	dFloat r = Util::min_float(rx, ry);
	dFloat ir = 1.0f / r;
	const NewtonCollision* col = NewtonCreateCylinder(
		world,
		r,
		r,
		h,
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	dFloat sx = 1.0f;
	dFloat sy = ry * ir;
	dFloat sz = rx * ir;
	NewtonCollisionSetScale(col, sx, sy, sz);
	s_valid_collisions[col] = new CollisionData(sx, sy, sz);
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_capsule(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	const NewtonCollision* col = NewtonCreateCapsule(
		world,
		Util::clamp_float(Util::value_to_dFloat(v_radius), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_radius), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_height), 0.0f, MAX_SIZE),
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	s_valid_collisions[col] = new CollisionData;
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_scaled_capsule(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_total_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	dFloat rx = Util::clamp_float(Util::value_to_dFloat(v_radiusx), MIN_SIZE, MAX_SIZE);
	dFloat ry = Util::clamp_float(Util::value_to_dFloat(v_radiusy), MIN_SIZE, MAX_SIZE);
	dFloat th = Util::clamp_float(Util::value_to_dFloat(v_total_height), MIN_SIZE, MAX_SIZE);
	dFloat r = Util::min_float(rx, ry);
	dFloat ir = 1.0f / r;
	dFloat h = th - r * 2.0f;
	const NewtonCollision* col = NewtonCreateCapsule(
		world,
		r,
		r,
		Util::max_float(h, 0.0f),
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	dFloat sx = h < 0 ? th * 0.5f * ir : 1.0f;
	dFloat sy = ry * ir;
	dFloat sz = rx * ir;
	NewtonCollisionSetScale(col, sx, sy, sz);
	s_valid_collisions[col] = new CollisionData(sx, sy, sz);
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_tapered_capsule(VALUE self, VALUE v_world, VALUE v_radius0, VALUE v_radius1, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	const NewtonCollision* col = NewtonCreateCapsule(
		world,
		Util::clamp_float(Util::value_to_dFloat(v_radius0), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_radius1), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_height), 0.0f, MAX_SIZE),
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	s_valid_collisions[col] = new CollisionData;
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_tapered_cylinder(VALUE self, VALUE v_world, VALUE v_radius0, VALUE v_radius1, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	const NewtonCollision* col = NewtonCreateCylinder(
		world,
		Util::clamp_float(Util::value_to_dFloat(v_radius0), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_radius1), MIN_SIZE, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_height), MIN_SIZE, MAX_SIZE),
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	s_valid_collisions[col] = new CollisionData;
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_chamfer_cylinder(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	const NewtonCollision* col = NewtonCreateChamferCylinder(
		world,
		Util::clamp_float(Util::value_to_dFloat(v_radius), 0.0f, MAX_SIZE),
		Util::clamp_float(Util::value_to_dFloat(v_height), 0.0f, MAX_SIZE),
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	s_valid_collisions[col] = new CollisionData;
	return c_collision_to_value(col);
}


VALUE MSP::Collision::rbf_create_scaled_chamfer_cylinder(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	dFloat rx = Util::clamp_float(Util::value_to_dFloat(v_radiusx), MIN_SIZE, MAX_SIZE);
	dFloat ry = Util::clamp_float(Util::value_to_dFloat(v_radiusy), MIN_SIZE, MAX_SIZE);
	dFloat h = Util::clamp_float(Util::value_to_dFloat(v_height), MIN_SIZE, MAX_SIZE);
	dFloat r = Util::min_float(rx, ry);
	const NewtonCollision* col = NewtonCreateChamferCylinder(
		world,
		r,
		h,
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	dFloat sx = 1.0f;
	dFloat sy = ry / (h * 0.5f + r);
	dFloat sz = rx / (h * 0.5f + r);
	NewtonCollisionSetScale(col, sx, sy, sz);
	s_valid_collisions[col] = new CollisionData(sx, sy, sz);
	return c_collision_to_value(col);
}

VALUE MSP::Collision::rbf_create_convex_hull(VALUE self, VALUE v_world, VALUE v_vertices, VALUE v_tolerance, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	Check_Type(v_vertices, T_ARRAY);
	unsigned int vertex_count = (unsigned int)RARRAY_LEN(v_vertices);
	dFloat* vertex_cloud = new dFloat[vertex_count * 3];
	unsigned int j = 0;
	for (unsigned int i = 0; i < vertex_count; ++i) {
		dVector point(Util::value_to_point(rb_ary_entry(v_vertices, i)));
		vertex_cloud[j] = point.m_x;
		vertex_cloud[j + 1] = point.m_y;
		vertex_cloud[j + 2] = point.m_z;
		j += 3;
	}
	const NewtonCollision* col = NewtonCreateConvexHull(
		world,
		vertex_count,
		vertex_cloud,
		sizeof(dFloat) * 3,
		Util::value_to_dFloat(v_tolerance),
		Util::value_to_int(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix)[0][0]);
	delete[] vertex_cloud;
	if (col != NULL) {
		s_valid_collisions[col] = new CollisionData;
		return c_collision_to_value(col);
	}
	else
		return Qnil;
}

VALUE MSP::Collision::rbf_create_compound(VALUE self, VALUE v_world, VALUE v_convex_collisions, VALUE v_id) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	Check_Type(v_convex_collisions, T_ARRAY);
	int id = Util::value_to_int(v_id);
	NewtonCollision* compound = NewtonCreateCompoundCollision(world, id);
	NewtonCompoundCollisionBeginAddRemove(compound);
	unsigned int collisions_count = (unsigned int)RARRAY_LEN(v_convex_collisions);
	for (unsigned int i = 0; i < collisions_count; ++i) {
		const NewtonCollision* col = reinterpret_cast<NewtonCollision*>(Util::value_to_ull(rb_ary_entry(v_convex_collisions, i)));
		if (c_is_collision_valid(col) && c_is_collision_convex(col))
			NewtonCompoundCollisionAddSubCollision(compound, col);
	}
	NewtonCompoundCollisionEndAddRemove(compound);
	s_valid_collisions[compound] = new CollisionData;
	return c_collision_to_value(compound);
}

VALUE MSP::Collision::rbf_create_compound_from_cd(
	VALUE self,
	VALUE v_world,
	VALUE v_polygons,
	VALUE v_max_concavity, // 0.01
	VALUE v_back_face_distance_factor, // 0.2
	VALUE v_max_hull_count, // 256
	VALUE v_max_vertices_per_hull, // 100
	VALUE v_hull_tolerance, // 0.001
	VALUE v_id)
{
	return Qnil;
#if 0
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	MSP::World::WorldData* world_data = reinterpret_cast<MSP::World::WorldData*>(NewtonWorldGetUserData(world));
	Check_Type(v_polygons, T_ARRAY);
	int id = Util::value_to_int(v_id);
	NewtonMesh* mesh = NewtonMeshCreate(world);
	/*NewtonMeshBeginFace(mesh);
	unsigned int polygon_count = (unsigned int)RARRAY_LEN(v_polygons);
	for (unsigned int i = 0; i < polygon_count; ++i) {
		VALUE v_polygon = rb_ary_entry(v_polygons, i);
		unsigned int vertex_count = (unsigned int)RARRAY_LEN(v_polygon);
		if (vertex_count > 0) {
			dFloat* vertex_cloud = new dFloat[vertex_count * 3];
			for (unsigned int j = 0; j < vertex_count; ++j) {
				dVector point(Util::value_to_point(rb_ary_entry(v_polygon, j), world_data->scale));
				vertex_cloud[j * 3 + 0] = point.m_x;
				vertex_cloud[j * 3 + 1] = point.m_y;
				vertex_cloud[j * 3 + 2] = point.m_z;
			}
			NewtonMeshAddFace(mesh, vertex_count, vertex_cloud, sizeof(dFloat) * 3, 0);
			delete[] vertex_cloud;
		}
	}
	NewtonMeshEndFace(mesh);*/
	NewtonMeshBeginBuild(mesh);
	unsigned int polygon_count = (unsigned int)RARRAY_LEN(v_polygons);
	for (unsigned int i = 0; i < polygon_count; ++i) {
		VALUE v_polygon = rb_ary_entry(v_polygons, i);
		unsigned int vertex_count = (unsigned int)RARRAY_LEN(v_polygon);
		if (vertex_count > 0) {
			NewtonMeshBeginFace(mesh);
			for (unsigned int j = 0; j < vertex_count; ++j) {
				dVector point(Util::value_to_point(rb_ary_entry(v_polygon, j), world_data->scale));
				NewtonMeshAddPoint(mesh, point.m_x, point.m_y, point.m_z);
			}
			NewtonMeshEndFace(mesh);
		}
	}
	NewtonMeshEndBuild(mesh);
	NewtonRemoveUnusedVertices(mesh, NULL);
	NewtonMeshFixTJoints(mesh);
	dFloat max_concavity = Util::value_to_dFloat(v_max_concavity);
	dFloat back_face_dist_factor = Util::value_to_dFloat(v_back_face_distance_factor);
	unsigned int max_hull_count = Util::value_to_uint(v_max_hull_count);
	unsigned int max_vertices_per_hull = Util::value_to_uint(v_max_vertices_per_hull);
	dFloat hull_tolerance = Util::value_to_dFloat(v_hull_tolerance);
	NewtonMesh* convex_approximation = NewtonMeshApproximateConvexDecomposition(mesh, max_concavity, back_face_dist_factor, max_hull_count, max_vertices_per_hull, nullptr, nullptr);
	const NewtonCollision* collision = NewtonCreateCompoundCollisionFromMesh(world, convex_approximation, hull_tolerance, id, id);
	NewtonMeshDestroy(convex_approximation);
	NewtonMeshDestroy(mesh);
	s_valid_collisions[collision] = dVector(1.0f, 1.0f, 1.0f);
	return c_collision_to_value(collision);
#endif
}

VALUE MSP::Collision::rbf_create_static_mesh(VALUE self, VALUE v_world, VALUE v_polygons, VALUE v_optimize, VALUE v_id) {
	const NewtonWorld* world = MSP::World::c_value_to_world(v_world);
	Check_Type(v_polygons, T_ARRAY);
	bool optimize = Util::value_to_bool(v_optimize);
	int id = Util::value_to_int(v_id);

	NewtonCollision* collision = NewtonCreateTreeCollision(world, id);
	NewtonTreeCollisionBeginBuild(collision);
	unsigned int polygons_length = (unsigned int)RARRAY_LEN(v_polygons);
	for (unsigned int i = 0; i < polygons_length; ++i) {
		VALUE v_polygon = rb_ary_entry(v_polygons, i);
		if (TYPE(v_polygon) != T_ARRAY) continue;
		unsigned int vertex_count = (unsigned int)RARRAY_LEN(v_polygon);
		dFloat* vertex_cloud = new dFloat[vertex_count * 3];
		unsigned int k = 0;
		for (unsigned int j = 0; j < vertex_count; ++j) {
			dVector point(Util::value_to_point(rb_ary_entry(v_polygon, j)));
			vertex_cloud[k] = point.m_x;
			vertex_cloud[k + 1] = point.m_y;
			vertex_cloud[k + 2] = point.m_z;
			k += 3;
		}
		NewtonTreeCollisionAddFace(collision, 3, &vertex_cloud[0], 3 * sizeof(dFloat), 0);
		delete[] vertex_cloud;
	}
	NewtonTreeCollisionEndBuild(collision, optimize ? 1 : 0);
	s_valid_collisions[collision] = new CollisionData;
	return c_collision_to_value(collision);
}

VALUE MSP::Collision::rbf_get_type(VALUE self, VALUE v_collision) {
	const NewtonCollision* collision = c_value_to_collision(v_collision);
	return Util::to_value( NewtonCollisionGetType(collision) );
}

VALUE MSP::Collision::rbf_get_scale(VALUE self, VALUE v_collision) {
	const NewtonCollision* collision = c_value_to_collision(v_collision);
	return Util::vector_to_value(s_valid_collisions[collision]->m_scale);
}

VALUE MSP::Collision::rbf_set_scale(VALUE self, VALUE v_collision, VALUE v_scale) {
	const NewtonCollision* collision = c_value_to_collision(v_collision);
	dVector scale(Util::value_to_vector(v_scale));
	dVector& cscale = s_valid_collisions[collision]->m_scale;
	cscale.m_x = Util::clamp_float(scale.m_x, 0.01f, 100.0f);
	cscale.m_y = Util::clamp_float(scale.m_y, 0.01f, 100.0f);
	cscale.m_z = Util::clamp_float(scale.m_z, 0.01f, 100.0f);
	NewtonCollisionSetScale(collision, cscale.m_x, cscale.m_y, cscale.m_z);
	return Qnil;
}

VALUE MSP::Collision::rbf_is_valid(VALUE self, VALUE v_collision) {
	return c_is_collision_valid(reinterpret_cast<NewtonCollision*>(Util::value_to_ull(v_collision))) ? Qtrue : Qfalse;
}

VALUE MSP::Collision::rbf_destroy(VALUE self, VALUE v_collision) {
	const NewtonCollision* collision = c_value_to_collision(v_collision);
	NewtonDestroyCollision(collision);
	return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSP::Collision::init_ruby(VALUE mNewton) {
	VALUE mCollision = rb_define_module_under(mNewton, "Collision");

	rb_define_module_function(mCollision, "create_null", VALUEFUNC(MSP::Collision::rbf_create_null), 1);
	rb_define_module_function(mCollision, "create_box", VALUEFUNC(MSP::Collision::rbf_create_box), 6);
	rb_define_module_function(mCollision, "create_sphere", VALUEFUNC(MSP::Collision::rbf_create_sphere), 4);
	rb_define_module_function(mCollision, "create_scaled_sphere", VALUEFUNC(MSP::Collision::rbf_create_scaled_sphere), 6);
	rb_define_module_function(mCollision, "create_cone", VALUEFUNC(MSP::Collision::rbf_create_cone), 5);
	rb_define_module_function(mCollision, "create_scaled_cone", VALUEFUNC(MSP::Collision::rbf_create_scaled_cone), 6);
	rb_define_module_function(mCollision, "create_cylinder", VALUEFUNC(MSP::Collision::rbf_create_cylinder), 5);
	rb_define_module_function(mCollision, "create_scaled_cylinder", VALUEFUNC(MSP::Collision::rbf_create_scaled_cylinder), 6);
	rb_define_module_function(mCollision, "create_capsule", VALUEFUNC(MSP::Collision::rbf_create_capsule), 5);
	rb_define_module_function(mCollision, "create_scaled_capsule", VALUEFUNC(MSP::Collision::rbf_create_scaled_capsule), 6);
	rb_define_module_function(mCollision, "create_tapered_capsule", VALUEFUNC(MSP::Collision::rbf_create_tapered_capsule), 6);
	rb_define_module_function(mCollision, "create_tapered_cylinder", VALUEFUNC(MSP::Collision::rbf_create_tapered_cylinder), 6);
	rb_define_module_function(mCollision, "create_chamfer_cylinder", VALUEFUNC(MSP::Collision::rbf_create_chamfer_cylinder), 5);
	rb_define_module_function(mCollision, "create_scaled_chamfer_cylinder", VALUEFUNC(MSP::Collision::rbf_create_scaled_chamfer_cylinder), 6);
	rb_define_module_function(mCollision, "create_convex_hull", VALUEFUNC(MSP::Collision::rbf_create_convex_hull), 5);
	rb_define_module_function(mCollision, "create_compound", VALUEFUNC(MSP::Collision::rbf_create_compound), 3);
	//rb_define_module_function(mCollision, "create_compound_from_cd", VALUEFUNC(MSP::Collision::rbf_create_compound_from_cd), 8);
	rb_define_module_function(mCollision, "create_static_mesh", VALUEFUNC(MSP::Collision::rbf_create_static_mesh), 4);
	rb_define_module_function(mCollision, "get_type", VALUEFUNC(MSP::Collision::rbf_get_type), 1);
	rb_define_module_function(mCollision, "get_scale", VALUEFUNC(MSP::Collision::rbf_get_scale), 1);
	rb_define_module_function(mCollision, "set_scale", VALUEFUNC(MSP::Collision::rbf_set_scale), 2);
	rb_define_module_function(mCollision, "is_valid?", VALUEFUNC(MSP::Collision::rbf_is_valid), 1);
	rb_define_module_function(mCollision, "destroy", VALUEFUNC(MSP::Collision::rbf_destroy), 1);
}
