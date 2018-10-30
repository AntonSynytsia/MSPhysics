#include "msp_bodies.h"
#include "msp_world.h"
#include "msp_body.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::Bodies::rbf_aabb_overlap(VALUE self, VALUE v_body1, VALUE v_body2) {
	const NewtonBody* body1 = MSP::Body::c_value_to_body(v_body1);
	const NewtonBody* body2 = MSP::Body::c_value_to_body(v_body2);
	MSP::Body::c_validate_two_bodies(body1, body2);
	return MSP::Body::c_bodies_aabb_overlap(body1, body2) ? Qtrue : Qfalse;
}

VALUE MSP::Bodies::rbf_collidable(VALUE self, VALUE v_body1, VALUE v_body2) {
	const NewtonBody* body1 = MSP::Body::c_value_to_body(v_body1);
	const NewtonBody* body2 = MSP::Body::c_value_to_body(v_body2);
	MSP::Body::c_validate_two_bodies(body1, body2);
	return MSP::Body::c_bodies_collidable(body1, body2) ? Qtrue : Qfalse;
}

VALUE MSP::Bodies::rbf_touching(VALUE self, VALUE v_body1, VALUE v_body2) {
	const NewtonBody* body1 = MSP::Body::c_value_to_body(v_body1);
	const NewtonBody* body2 = MSP::Body::c_value_to_body(v_body2);
	MSP::Body::c_validate_two_bodies(body1, body2);
	const NewtonWorld* world = NewtonBodyGetWorld(body1);
	const NewtonCollision* colA = NewtonBodyGetCollision(body1);
	const NewtonCollision* colB = NewtonBodyGetCollision(body2);
	dMatrix matrixA;
	dMatrix matrixB;
	NewtonBodyGetMatrix(body1, &matrixA[0][0]);
	NewtonBodyGetMatrix(body2, &matrixB[0][0]);
	return NewtonCollisionIntersectionTest(world, colA, &matrixA[0][0], colB, &matrixB[0][0], 0) == 1 ? Qtrue : Qfalse;
}

VALUE MSP::Bodies::rbf_get_closest_points(VALUE self, VALUE v_body1, VALUE v_body2) {
	const NewtonBody* body1 = MSP::Body::c_value_to_body(v_body1);
	const NewtonBody* body2 = MSP::Body::c_value_to_body(v_body2);
	MSP::Body::c_validate_two_bodies(body1, body2);
	const NewtonWorld* world = NewtonBodyGetWorld(body1);
	const NewtonCollision* colA = NewtonBodyGetCollision(body1);
	const NewtonCollision* colB = NewtonBodyGetCollision(body2);
	dMatrix matrixA;
	dMatrix matrixB;
	NewtonBodyGetMatrix(body1, &matrixA[0][0]);
	NewtonBodyGetMatrix(body2, &matrixB[0][0]);
	dVector pointA;
	dVector pointB;
	dVector normalAB;
	if (NewtonCollisionClosestPoint(world, colA, &matrixA[0][0], colB, &matrixB[0][0], &pointA[0], &pointB[0], &normalAB[0], 0) == 0)
		return Qnil;
	return rb_ary_new3(2, Util::point_to_value(pointA), Util::point_to_value(pointB));
}

VALUE MSP::Bodies::rbf_get_force_in_between(VALUE self, VALUE v_body1, VALUE v_body2) {
	const NewtonBody* body1 = MSP::Body::c_value_to_body(v_body1);
	const NewtonBody* body2 = MSP::Body::c_value_to_body(v_body2);
	MSP::Body::c_validate_two_bodies(body1, body2);
	dVector net_force(0.0f);
	for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(body1); joint; joint = NewtonBodyGetNextContactJoint(body1, joint)) {
		if (NewtonJointGetBody0(joint) == body2 || NewtonJointGetBody1(joint) == body2) {
			for (void* contact = NewtonContactJointGetFirstContact(joint); contact; contact = NewtonContactJointGetNextContact(joint, contact)) {
				NewtonMaterial* material = NewtonContactGetMaterial(contact);
				dVector force;
				NewtonMaterialGetContactForce(material, body1, &force[0]);
				net_force += force;
			}
		}
	}
	return Util::vector_to_value(net_force.Scale(M_INCH_TO_METER));
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSP::Bodies::init_ruby(VALUE mNewton) {
	VALUE mBodies = rb_define_module_under(mNewton, "Bodies");

	rb_define_module_function(mBodies, "aabb_overlap?", VALUEFUNC(MSP::Bodies::rbf_aabb_overlap), 2);
	rb_define_module_function(mBodies, "collidable?", VALUEFUNC(MSP::Bodies::rbf_collidable), 2);
	rb_define_module_function(mBodies, "touching?", VALUEFUNC(MSP::Bodies::rbf_touching), 2);
	rb_define_module_function(mBodies, "get_closest_points", VALUEFUNC(MSP::Bodies::rbf_get_closest_points), 2);
	rb_define_module_function(mBodies, "get_force_in_between", VALUEFUNC(MSP::Bodies::rbf_get_force_in_between), 2);
}
