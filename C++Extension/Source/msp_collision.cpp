#include "msp_collision.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::Collision::MIN_SIZE = 1.0e-4f;
const dFloat MSNewton::Collision::MAX_SIZE = 1.0e4f;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::Collision::create_null(VALUE self, VALUE v_world) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	const NewtonCollision* col = NewtonCreateNull(world);
	valid_collisions[col] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_box(VALUE self, VALUE v_world, VALUE v_width, VALUE v_height, VALUE v_depth, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	const NewtonCollision* col = NewtonCreateBox(
		world,
		Util::value_to_dFloat2(v_width, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_height, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_depth, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	valid_collisions[col] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_sphere(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	const NewtonCollision* col = NewtonCreateSphere(
		world,
		Util::value_to_dFloat2(v_radius, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	valid_collisions[col] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_scaled_sphere(VALUE self, VALUE v_world, VALUE v_width, VALUE v_height, VALUE v_depth, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	dFloat w = Util::value_to_dFloat2(v_width, scale, MIN_SIZE, MAX_SIZE);
	dFloat h = Util::value_to_dFloat2(v_height, scale, MIN_SIZE, MAX_SIZE);
	dFloat d = Util::value_to_dFloat2(v_depth, scale, MIN_SIZE, MAX_SIZE);
	dFloat r = h < w ? h : w;
	if (d < r) r = d;
	dFloat ir = 1.0f / r;
	const NewtonCollision* col = NewtonCreateSphere(
		world,
		r * 0.5f,
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	dFloat sx = w * ir;
	dFloat sy = h * ir;
	dFloat sz = d * ir;
	NewtonCollisionSetScale(col, sx, sy, sz);
	valid_collisions[col] = dVector(sx, sy, sz);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_cone(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	const NewtonCollision* col = NewtonCreateCone(
		world,
		Util::value_to_dFloat2(v_radius, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_height, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	valid_collisions[col] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_scaled_cone(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	dFloat rx = Util::value_to_dFloat2(v_radiusx, scale, MIN_SIZE, MAX_SIZE);
	dFloat ry = Util::value_to_dFloat2(v_radiusy, scale, MIN_SIZE, MAX_SIZE);
	dFloat h = Util::value_to_dFloat2(v_height, scale, MIN_SIZE, MAX_SIZE);
	dFloat r = rx < ry ? rx : ry;
	dFloat ir = 1.0f / r;
	const NewtonCollision* col = NewtonCreateCone(
		world,
		r,
		h,
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	dFloat sx = 1.0f;
	dFloat sy = ry * ir;
	dFloat sz = rx * ir;
	NewtonCollisionSetScale(col, sx, sy, sz);
	valid_collisions[col] = dVector(sx, sy, sz);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_cylinder(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	const NewtonCollision* col = NewtonCreateCylinder(
		world,
		Util::value_to_dFloat2(v_radius, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_radius, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_height, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	valid_collisions[col] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_scaled_cylinder(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	dFloat rx = Util::value_to_dFloat2(v_radiusx, scale, MIN_SIZE, MAX_SIZE);
	dFloat ry = Util::value_to_dFloat2(v_radiusy, scale, MIN_SIZE, MAX_SIZE);
	dFloat h = Util::value_to_dFloat2(v_height, scale, MIN_SIZE, MAX_SIZE);
	dFloat r = rx < ry ? rx : ry;
	dFloat ir = 1.0f / r;
	const NewtonCollision* col = NewtonCreateCylinder(
		world,
		r,
		r,
		h,
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	dFloat sx = 1.0f;
	dFloat sy = ry * ir;
	dFloat sz = rx * ir;
	NewtonCollisionSetScale(col, sx, sy, sz);
	valid_collisions[col] = dVector(sx, sy, sz);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_capsule(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	const NewtonCollision* col = NewtonCreateCapsule(
		world,
		Util::value_to_dFloat2(v_radius, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_radius, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_height, scale, 0.0f, MAX_SIZE),
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	valid_collisions[col] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_scaled_capsule(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_total_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	dFloat rx = Util::value_to_dFloat2(v_radiusx, scale, MIN_SIZE, MAX_SIZE);
	dFloat ry = Util::value_to_dFloat2(v_radiusy, scale, MIN_SIZE, MAX_SIZE);
	dFloat th = Util::value_to_dFloat2(v_total_height, scale, MIN_SIZE, MAX_SIZE);
	dFloat r = rx < ry ? rx : ry;
	dFloat ir = 1.0f / r;
	dFloat h = th - r * 2;
	const NewtonCollision* col = NewtonCreateCapsule(
		world,
		r,
		r,
		h < 0 ? 0 : h,
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	dFloat sx = h < 0 ? th * 0.5f * ir : 1.0f;
	dFloat sy = ry * ir;
	dFloat sz = rx * ir;
	NewtonCollisionSetScale(col, sx, sy, sz);
	valid_collisions[col] = dVector(sx, sy, sz);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_tapered_capsule(VALUE self, VALUE v_world, VALUE v_radius0, VALUE v_radius1, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	const NewtonCollision* col = NewtonCreateCapsule(
		world,
		Util::value_to_dFloat2(v_radius0, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_radius1, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_height, scale, 0.0f, MAX_SIZE),
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	valid_collisions[col] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_tapered_cylinder(VALUE self, VALUE v_world, VALUE v_radius0, VALUE v_radius1, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	const NewtonCollision* col = NewtonCreateCylinder(
		world,
		Util::value_to_dFloat2(v_radius0, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_radius1, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_dFloat2(v_height, scale, MIN_SIZE, MAX_SIZE),
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	valid_collisions[col] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_chamfer_cylinder(VALUE self, VALUE v_world, VALUE v_radius, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	const NewtonCollision* col = NewtonCreateChamferCylinder(
		world,
		Util::value_to_dFloat2(v_radius, scale, 0.0f, MAX_SIZE),
		Util::value_to_dFloat2(v_height, scale, 0.0f, MAX_SIZE),
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	valid_collisions[col] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(col);
}


VALUE MSNewton::Collision::create_scaled_chamfer_cylinder(VALUE self, VALUE v_world, VALUE v_radiusx, VALUE v_radiusy, VALUE v_height, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	dFloat scale = INCH_TO_METER * world_data->scale;
	dFloat rx = Util::value_to_dFloat2(v_radiusx, scale, MIN_SIZE, MAX_SIZE);
	dFloat ry = Util::value_to_dFloat2(v_radiusy, scale, MIN_SIZE, MAX_SIZE);
	dFloat h = Util::value_to_dFloat2(v_height, scale, MIN_SIZE, MAX_SIZE);
	dFloat r = rx < ry ? rx : ry;
	const NewtonCollision* col = NewtonCreateChamferCylinder(
		world,
		r,
		h,
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	dFloat sx = 1.0f;
	dFloat sy = ry / (h * 0.5f + r);
	dFloat sz = rx / (h * 0.5f + r);
	NewtonCollisionSetScale(col, sx, sy, sz);
	valid_collisions[col] = dVector(sx, sy, sz);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_convex_hull(VALUE self, VALUE v_world, VALUE v_vertices, VALUE v_tolerance, VALUE v_id, VALUE v_offset_matrix) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	Check_Type(v_vertices, T_ARRAY);
	std::vector<dFloat> vertex_cloud;
	for (int i = 0; i < RARRAY_LEN(v_vertices); ++i) {
		dVector point = Util::value_to_point(rb_ary_entry(v_vertices, i), world_data->scale);
		vertex_cloud.push_back(point.m_x);
		vertex_cloud.push_back(point.m_y);
		vertex_cloud.push_back(point.m_z);
	}
	const NewtonCollision* col = NewtonCreateConvexHull(
		world,
		RARRAY_LEN(v_vertices),
		&vertex_cloud[0],
		sizeof(dFloat) * 3,
		Util::value_to_dFloat(v_tolerance),
		Util::value_to_long(v_id),
		v_offset_matrix == Qnil ? NULL : &Util::value_to_matrix(v_offset_matrix, world_data->scale)[0][0]);
	valid_collisions[col] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(col);
}

VALUE MSNewton::Collision::create_compound(VALUE self, VALUE v_world, VALUE v_convex_collisions, VALUE v_id) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	Check_Type(v_convex_collisions, T_ARRAY);
	int id = Util::value_to_long(v_id);
	NewtonCollision* compound = NewtonCreateCompoundCollision(world, id);
	NewtonCompoundCollisionBeginAddRemove(compound);
	for (int i = 0; i < RARRAY_LEN(v_convex_collisions); ++i) {
		const NewtonCollision* col = (const NewtonCollision*)Util::value_to_ll(rb_ary_entry(v_convex_collisions, i));
		if (Util::is_collision_valid(col) && Util::is_collision_convex(col))
			NewtonCompoundCollisionAddSubCollision(compound, col);
	}
	NewtonCompoundCollisionEndAddRemove(compound);
	valid_collisions[compound] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(compound);
}

VALUE MSNewton::Collision::create_compound_from_cd1(
	VALUE self,
	VALUE v_world,
	VALUE v_polygons,
	VALUE v_max_concavity,
	VALUE v_back_face_distance_factor,
	VALUE v_max_hull_count,
	VALUE v_max_vertices_per_hull,
	VALUE v_id)
{
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	Check_Type(v_polygons, T_ARRAY);
	int id = Util::value_to_long(v_id);
	NewtonMesh* mesh = NewtonMeshCreate(world);
	unsigned int total_vertex_count = 0;
	NewtonMeshBeginFace(mesh);
	for (unsigned int i = 0; i < (unsigned int)RARRAY_LEN(v_polygons); ++i) {
		VALUE v_polygon = rb_ary_entry(v_polygons, i);
		std::vector<dFloat> vertex_cloud;
		unsigned int vertex_count = 0;
		for (unsigned int j = 0; j < (unsigned int)RARRAY_LEN(v_polygon); ++j) {
			dVector point = Util::value_to_point(rb_ary_entry(v_polygon, j), world_data->scale);
			vertex_cloud.push_back(point.m_x);
			vertex_cloud.push_back(point.m_y);
			vertex_cloud.push_back(point.m_z);
			++vertex_count;
		}
		total_vertex_count += vertex_count;
		NewtonMeshAddFace(mesh, vertex_count, &vertex_cloud[0], sizeof(dFloat)*3, 0);
	}
	NewtonMeshEndFace(mesh);
	NewtonRemoveUnusedVertices(mesh, NULL);
	NewtonMeshFixTJoints(mesh);
	dFloat max_concavity = Util::value_to_dFloat(v_max_concavity);
	dFloat back_face_dist_factor = Util::value_to_dFloat(v_back_face_distance_factor);
	unsigned int max_hull_count = Util::value_to_uint(v_max_hull_count);
	unsigned int max_vertices_per_hull = Util::value_to_uint(v_max_vertices_per_hull);
	NewtonMesh* convex_approximation = NewtonMeshApproximateConvexDecomposition(mesh, max_concavity, back_face_dist_factor, max_hull_count, max_vertices_per_hull, NULL, NULL);
	NewtonMeshDestroy(mesh);
	const NewtonCollision* collision = NewtonCreateCompoundCollisionFromMesh(world, convex_approximation, max_concavity, id, id);
	NewtonMeshDestroy(convex_approximation);
	valid_collisions[collision] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(collision);
}

VALUE MSNewton::Collision::create_compound_from_cd2(
	VALUE self,
	VALUE v_world,
	VALUE v_points,
	VALUE v_indices,
	VALUE v_params,
	VALUE v_id)
{
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	Check_Type(v_points, T_ARRAY);
	Check_Type(v_indices, T_ARRAY);
	Check_Type(v_params, T_HASH);
	int id = Util::value_to_long(v_id);

	// Convert polygons to an array of triangles and points.
	unsigned int nTriangles = RARRAY_LEN(v_indices);
	unsigned int nPoints = RARRAY_LEN(v_points);

	if (nTriangles == 0 || nPoints == 0) return Qnil;

	std::vector<int> triangles(nTriangles*3, 0); // array of indices
	std::vector<double> points(nPoints*3, 0.0f); // array of coordinates
	for (unsigned int i = 0; i < nTriangles; ++i) {
		VALUE v_triangle = rb_ary_entry(v_indices, i);
		for (unsigned int j = 0; j < 3; ++j)
			triangles[i*3+j] = Util::value_to_int(rb_ary_entry(v_triangle, j));
	}

	for (unsigned int i = 0; i < nPoints; ++i) {
		dVector point = Util::value_to_point(rb_ary_entry(v_points, i), world_data->scale);
		for (unsigned int j = 0; j < 3; ++j)
			points[i*3+j] = point[j];
	}

	if (nPoints == 0) return Qnil;

	// V-HACD parameters
	VHACD::IVHACD::Parameters params;
	VALUE val;
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("resolution")) );
	if( val != Qnil ) params.m_resolution = Util::clamp<unsigned int>(Util::value_to_uint(val), 10000, 64000000);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("depth")) );
	if( val != Qnil ) params.m_depth = Util::clamp(Util::value_to_int(val), 1, 32);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("concavity")) );
	if( val != Qnil ) params.m_concavity = Util::clamp<double>(Util::value_to_double(val), 0.0f, 1.0f);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("plane_downsampling")) );
	if( val != Qnil ) params.m_planeDownsampling = Util::clamp(Util::value_to_int(val), 1, 16);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("convex_hull_downsampling")) );
	if( val != Qnil ) params.m_convexhullDownsampling = Util::clamp(Util::value_to_int(val), 1, 16);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("alpha")) );
	if( val != Qnil ) params.m_alpha = Util::clamp<double>(Util::value_to_double(val), 0.0f, 1.0f);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("beta")) );
	if( val != Qnil ) params.m_beta = Util::clamp<double>(Util::value_to_double(val), 0.0f, 1.0f);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("gamma")) );
	if( val != Qnil ) params.m_gamma = Util::clamp<double>(Util::value_to_double(val), 0.0f, 1.0f);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("delta")) );
	if( val != Qnil ) params.m_delta = Util::clamp<double>(Util::value_to_double(val), 0.0f, 1.0f);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("pca")) );
	if( val != Qnil ) params.m_pca = Util::clamp(Util::value_to_int(val), 0, 1);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("mode")) );
	if( val != Qnil ) params.m_mode = Util::clamp(Util::value_to_int(val), 0, 1);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("max_num_vertices_per_ch")) );
	if( val != Qnil ) params.m_maxNumVerticesPerCH = Util::clamp<unsigned int>(Util::value_to_uint(val), 4, 1024);
	val = rb_hash_aref( v_params, ID2SYM(rb_intern("min_volume_per_ch")) );
	if( val != Qnil ) params.m_minVolumePerCH = Util::clamp<double>(Util::value_to_double(val), 0.0f, 0.01f);

	// Create interface
	VHACD::IVHACD * interfaceVHACD = VHACD::CreateVHACD();

	// Compute approximate convex decomposition
	bool res = interfaceVHACD->Compute(&points[0], 3, nPoints, &triangles[0], 3, nTriangles, params);

	if (!res) {
		interfaceVHACD->Clean();
		interfaceVHACD->Release();
		return Qnil;
	}

	// Get number of convex hulls
	unsigned int nConvexHulls = interfaceVHACD->GetNConvexHulls();
	VHACD::IVHACD::ConvexHull ch;
	// Create compound collision
	NewtonCollision* compound = NewtonCreateCompoundCollision(world, id);
	NewtonCompoundCollisionBeginAddRemove(compound);
	// Process all convex hulls
	for (unsigned int i = 0; i < nConvexHulls; ++i) {
		// Get the i-th convex-hull information
		interfaceVHACD->GetConvexHull(i, ch);
		// Create compound sub collision
		std::vector<dFloat> vertex_cloud;
		for (unsigned int j = 0; j < ch.m_nPoints; ++j) {
			vertex_cloud.push_back( (dFloat)ch.m_points[j*3+0] );
			vertex_cloud.push_back( (dFloat)ch.m_points[j*3+1] );
			vertex_cloud.push_back( (dFloat)ch.m_points[j*3+2] );
		}
		const NewtonCollision* col = NewtonCreateConvexHull(world, ch.m_nPoints, &vertex_cloud[0], sizeof(dFloat)*3, 0.0f, id, NULL);
		NewtonCompoundCollisionAddSubCollision(compound, col);
		NewtonDestroyCollision(col);
	}
	NewtonCompoundCollisionEndAddRemove(compound);

	interfaceVHACD->Clean();
	interfaceVHACD->Release();

	valid_collisions[compound] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(compound);
}

VALUE MSNewton::Collision::create_compound_from_cd3(VALUE self, VALUE v_world, VALUE v_points, VALUE v_indices, VALUE v_max_concavity_angle, VALUE v_id) {
	return Qnil;
}

VALUE MSNewton::Collision::create_static_mesh(VALUE self, VALUE v_world, VALUE v_polygons, VALUE v_simplify, VALUE v_optimize, VALUE v_id) {
	const NewtonWorld* world = Util::value_to_world(v_world);
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(world);
	Check_Type(v_polygons, T_ARRAY);
	bool simplify = Util::value_to_bool(v_simplify);
	int optimize = Util::value_to_int(v_optimize);
	int id = Util::value_to_long(v_id);
	NewtonMesh* mesh = NewtonMeshCreate(world);
	unsigned int total_vertex_count = 0;
	NewtonMeshBeginFace(mesh);
	for (unsigned int i = 0; i < (unsigned int)RARRAY_LEN(v_polygons); ++i) {
		VALUE v_polygon = rb_ary_entry(v_polygons, i);
		if (TYPE(v_polygon) != T_ARRAY) continue;
		std::vector<dFloat> vertex_cloud;
		unsigned int vertex_count = 0;
		for (unsigned int j = 0; j < (unsigned int)RARRAY_LEN(v_polygon); ++j) {
			dVector point = Util::value_to_point(rb_ary_entry(v_polygon, j), world_data->scale);
			vertex_cloud.push_back(point.m_x);
			vertex_cloud.push_back(point.m_y);
			vertex_cloud.push_back(point.m_z);
			++vertex_count;
		}
		total_vertex_count += vertex_count;
		NewtonMeshAddFace(mesh, vertex_count, &vertex_cloud[0], sizeof(dFloat)*3, 0);
	}
	NewtonMeshEndFace(mesh);
	if (simplify)
		NewtonRemoveUnusedVertices(mesh, NULL);
	NewtonMeshFixTJoints(mesh);
	if (optimize == 1)
		NewtonMeshTriangulate(mesh);
	else if (optimize == 2)
		NewtonMeshPolygonize(mesh);
	NewtonCollision* collision = NewtonCreateTreeCollisionFromMesh(world, mesh, id);
	NewtonMeshDestroy(mesh);
	valid_collisions[collision] = dVector(1.0f, 1.0f, 1.0f);
	return Util::to_value(collision);
}

VALUE MSNewton::Collision::get_type(VALUE self, VALUE v_collision) {
	const NewtonCollision* collision = Util::value_to_collision(v_collision);
	return Util::to_value( NewtonCollisionGetType(collision) );
}

VALUE  MSNewton::Collision::get_scale(VALUE self, VALUE v_collision) {
	const NewtonCollision* collision = Util::value_to_collision(v_collision);
	return Util::vector_to_value(valid_collisions[collision]);
}

VALUE  MSNewton::Collision::set_scale(VALUE self, VALUE v_collision, VALUE v_scale) {
	const NewtonCollision* collision = Util::value_to_collision(v_collision);
	if (NewtonCollisionGetType(collision) > 8)
		rb_raise(rb_eTypeError, "Only convex collisions can be scaled!");
	dVector scale = Util::value_to_vector(v_scale);
	scale.m_x = Util::clamp(scale.m_x, 0.01f, 100.0f);
	scale.m_y = Util::clamp(scale.m_y, 0.01f, 100.0f);
	scale.m_z = Util::clamp(scale.m_z, 0.01f, 100.0f);
	NewtonCollisionSetScale(collision, scale.m_x, scale.m_y, scale.m_z);
	valid_collisions[collision] = scale;
	return Util::vector_to_value(scale);
}

VALUE MSNewton::Collision::is_valid(VALUE self, VALUE v_collision) {
	return Util::is_collision_valid((const NewtonCollision*)Util::value_to_ll(v_collision)) ? Qtrue : Qfalse;
}

VALUE MSNewton::Collision::destroy(VALUE self, VALUE v_collision) {
	const NewtonCollision* collision = Util::value_to_collision(v_collision);
	NewtonDestroyCollision(collision);
	return Qnil;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void Init_msp_collision(VALUE mNewton) {
	VALUE mCollision = rb_define_module_under(mNewton, "Collision");

	rb_define_module_function(mCollision, "create_null", VALUEFUNC(MSNewton::Collision::create_null), 1);
	rb_define_module_function(mCollision, "create_box", VALUEFUNC(MSNewton::Collision::create_box), 6);
	rb_define_module_function(mCollision, "create_sphere", VALUEFUNC(MSNewton::Collision::create_sphere), 4);
	rb_define_module_function(mCollision, "create_scaled_sphere", VALUEFUNC(MSNewton::Collision::create_scaled_sphere), 6);
	rb_define_module_function(mCollision, "create_cone", VALUEFUNC(MSNewton::Collision::create_cone), 5);
	rb_define_module_function(mCollision, "create_scaled_cone", VALUEFUNC(MSNewton::Collision::create_scaled_cone), 6);
	rb_define_module_function(mCollision, "create_cylinder", VALUEFUNC(MSNewton::Collision::create_cylinder), 5);
	rb_define_module_function(mCollision, "create_scaled_cylinder", VALUEFUNC(MSNewton::Collision::create_scaled_cylinder), 6);
	rb_define_module_function(mCollision, "create_capsule", VALUEFUNC(MSNewton::Collision::create_capsule), 5);
	rb_define_module_function(mCollision, "create_scaled_capsule", VALUEFUNC(MSNewton::Collision::create_scaled_capsule), 6);
	rb_define_module_function(mCollision, "create_tapered_capsule", VALUEFUNC(MSNewton::Collision::create_tapered_capsule), 6);
	rb_define_module_function(mCollision, "create_tapered_cylinder", VALUEFUNC(MSNewton::Collision::create_tapered_cylinder), 6);
	rb_define_module_function(mCollision, "create_chamfer_cylinder", VALUEFUNC(MSNewton::Collision::create_chamfer_cylinder), 5);
	rb_define_module_function(mCollision, "create_scaled_chamfer_cylinder", VALUEFUNC(MSNewton::Collision::create_scaled_chamfer_cylinder), 6);
	rb_define_module_function(mCollision, "create_convex_hull", VALUEFUNC(MSNewton::Collision::create_convex_hull), 5);
	rb_define_module_function(mCollision, "create_compound", VALUEFUNC(MSNewton::Collision::create_compound), 3);
	rb_define_module_function(mCollision, "create_compound_from_cd1", VALUEFUNC(MSNewton::Collision::create_compound_from_cd1), 7);
	rb_define_module_function(mCollision, "create_compound_from_cd2", VALUEFUNC(MSNewton::Collision::create_compound_from_cd2), 5);
	rb_define_module_function(mCollision, "create_compound_from_cd3", VALUEFUNC(MSNewton::Collision::create_compound_from_cd3), 5);
	rb_define_module_function(mCollision, "create_static_mesh", VALUEFUNC(MSNewton::Collision::create_static_mesh), 5);
	rb_define_module_function(mCollision, "get_type", VALUEFUNC(MSNewton::Collision::get_type), 1);
	rb_define_module_function(mCollision, "get_scale", VALUEFUNC(MSNewton::Collision::get_scale), 1);
	rb_define_module_function(mCollision, "set_scale", VALUEFUNC(MSNewton::Collision::set_scale), 2);
	rb_define_module_function(mCollision, "is_valid?", VALUEFUNC(MSNewton::Collision::is_valid), 1);
	rb_define_module_function(mCollision, "destroy", VALUEFUNC(MSNewton::Collision::destroy), 1);
}
