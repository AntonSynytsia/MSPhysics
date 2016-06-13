#include "msp_joint_curvy_slider.h"

/*
 ///////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSNewton::CurvySlider::DEFAULT_LINEAR_FRICTION = 0.0f;
const dFloat MSNewton::CurvySlider::DEFAULT_ANGULAR_FRICTION = 0.0f;
const dFloat MSNewton::CurvySlider::DEFAULT_CONTROLLER = 1.0f;
const bool MSNewton::CurvySlider::DEFAULT_LOOP_ENABLED = false;
const bool MSNewton::CurvySlider::DEFAULT_ALIGNMENT_ENABLED = true;
const bool MSNewton::CurvySlider::DEFAULT_ROTATION_ENABLED = true;


/*
 ///////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

dFloat MSNewton::CurvySlider::c_calc_curve_length(std::vector<dVector>& curve_points, bool loop) {
	dFloat curve_length = 0.0f;
	unsigned int points_size = (unsigned int)curve_points.size();
	if (points_size < 2) return curve_length;
	for (unsigned int i = 0; i < (points_size - 1); ++i) {
		const dVector& pt1 = curve_points[i];
		const dVector& pt2 = curve_points[i+1];
		curve_length += Util::get_vector_magnitude(pt2 - pt1);
	}
	if (loop && points_size > 2)
		curve_length += Util::get_vector_magnitude(curve_points[points_size-1] - curve_points[0]);
	return curve_length;
}

bool MSNewton::CurvySlider::c_calc_curve_data_at_location(const CurvySliderData* cj_data, const dVector& location, dVector& point, dVector& vector, dFloat& distance, dVector& min_pt, dVector& max_pt, dFloat& min_len, dFloat& max_len) {
	unsigned int points_size = (unsigned int)cj_data->points.size();
	if (points_size < 2)
		return false;
	dFloat closest_dist = 0.0f;
	dFloat closest_left_over = 0.0f;
	bool closest_set = false;
	dFloat traveled_dist = 0.0f;
	for (unsigned int i = 0; i < (cj_data->loop ? points_size : points_size - 1); ++i) {
		const dVector& pt1 = cj_data->points[i];
		const dVector& pt2 = cj_data->points[(i == points_size - 1 && cj_data->loop == true) ? 0 : i+1];
		dVector edge_dir = pt2 - pt1;
		dFloat edge_len = Util::get_vector_magnitude(edge_dir);
		if (edge_len < 1.0e-8f) continue;
		Util::normalize_vector(edge_dir);
		dMatrix tra;
		Util::matrix_from_pin_dir(pt1, edge_dir, tra);
		dVector lpoint = tra.UntransformVector(location);
		dVector cpoint;
		dFloat section_dist;
		dFloat left_over;
		if (lpoint.m_z < 0.0f) {
			cpoint = pt1;
			section_dist = 0.0f;
			left_over = lpoint.m_z;
		}
		else if (lpoint.m_z > edge_len) {
			cpoint = pt2;
			section_dist = edge_len;
			left_over = lpoint.m_z - edge_len;
		}
		else {
			cpoint = tra.TransformVector(dVector(0.0f, 0.0f, lpoint.m_z));
			section_dist = lpoint.m_z;
			left_over = 0.0f;
		}
		dFloat cdist = Util::get_vector_magnitude(location - cpoint);
		if (closest_set == false || cdist < closest_dist) {
			closest_dist = cdist;
			if (left_over == 0.0f) {
				point = dVector(cpoint);
				vector = edge_dir;
				min_pt = dVector(pt1);
				max_pt = dVector(pt2);
				min_len = traveled_dist;
				max_len = traveled_dist + edge_len;
			}
			distance = traveled_dist + section_dist;
			closest_left_over = left_over;
			closest_set = true;
			if (closest_dist < 1.0e-8f)
				break;
		}
		traveled_dist += edge_len;
	}
	if (!closest_set)
		return false;
	if (closest_left_over != 0.0f) {
		distance += closest_left_over;
		if (cj_data->loop) {
			distance = fmod(distance, cj_data->curve_len);
			if (distance < 0.0f) distance += cj_data->curve_len;
		}
		else
			distance = Util::clamp(distance, 0.0f, cj_data->curve_len);
		traveled_dist = 0.0f;
		for (unsigned int i = 0; i < (cj_data->loop ? points_size : points_size - 1); ++i) {
			const dVector& pt1 = cj_data->points[i];
			const dVector& pt2 = cj_data->points[(i == points_size - 1 && cj_data->loop == true) ? 0 : i+1];
			dVector edge_dir(pt2 - pt1);
			dFloat edge_len = Util::get_vector_magnitude(edge_dir);
			if (edge_len < 1.0e-8f) continue;
			if (traveled_dist + edge_len >= distance) {
				Util::normalize_vector(edge_dir);
				point = pt1 + edge_dir.Scale(distance - traveled_dist);
				vector = edge_dir;
				min_pt = dVector(pt1);
				max_pt = dVector(pt2);
				min_len = traveled_dist;
				max_len = traveled_dist + edge_len;
				break;
			}
			traveled_dist += edge_len;
		}
	}
	return true;
}

bool MSNewton::CurvySlider::c_calc_curve_line_by_pos(const CurvySliderData* cj_data, dFloat distance, dVector& point, dVector& vector) {
	unsigned int points_size = (unsigned int)cj_data->points.size();
	if (cj_data->curve_len == 0.0f) return false;
	if (cj_data->loop) {
		distance = fmod(distance, cj_data->curve_len);
		if (distance < 0.0f) distance += cj_data->curve_len;
	}
	else
		distance = Util::clamp(distance, 0.0f, cj_data->curve_len);
	dFloat traveled_dist = 0.0f;
	for (unsigned int i = 0; i < (cj_data->loop ? points_size : points_size - 1); ++i) {
		const dVector& pt1 = cj_data->points[i];
		const dVector& pt2 = cj_data->points[(i == points_size - 1 && cj_data->loop == true) ? 0 : i+1];
		dVector edge_dir(pt2 - pt1);
		dFloat edge_len = Util::get_vector_magnitude(edge_dir);
		if (edge_len < 1.0e-8f) continue;
		if (traveled_dist + edge_len >= distance) {
			Util::normalize_vector(edge_dir);
			point = pt1 + edge_dir.Scale(distance - traveled_dist);
			vector = edge_dir;
			break;
		}
		traveled_dist += edge_len;
	}
	return true;
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

void MSNewton::CurvySlider::submit_constraints(const NewtonJoint* joint, dgFloat32 timestep, int thread_index) {
	JointData* joint_data = (JointData*)NewtonJointGetUserData(joint);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;

	// Calculate position of pivot points and Jacobian direction vectors in global space.
	dMatrix matrix0, matrix1, matrix2;
	MSNewton::Joint::c_calculate_global_matrix(joint_data, matrix0, matrix1, matrix2);

	dVector location = matrix2.UntransformVector(matrix0.m_posit);
	dVector point, vector, min_pt, max_pt;
	dFloat distance, min_len, max_len;
	if (!c_calc_curve_data_at_location(cj_data, location, point, vector, distance, min_pt, max_pt, min_len, max_len)) {
		cj_data->cur_data_set = false;
		return;
	}

	point = matrix2.TransformVector(point);
	vector = matrix2.RotateVector(vector);
	min_pt = matrix2.TransformVector(min_pt);
	max_pt = matrix2.TransformVector(max_pt);

	cj_data->cur_point = point;
	cj_data->cur_vector = vector;
	cj_data->cur_tangent = (1.0f - dAbs(vector.m_z) < EPSILON) ? Y_AXIS * vector : Z_AXIS * vector;
	cj_data->cur_data_set = true;

	dFloat last_pos = cj_data->cur_pos;
	dFloat last_vel = cj_data->cur_vel;
	if (cj_data->loop) {
		dFloat diff1 = distance - cj_data->last_dist;
		dFloat diff2 = diff1 + (diff1 > 0 ? -cj_data->curve_len : cj_data->curve_len);
		if (dAbs(diff1) < dAbs(diff2))
			cj_data->cur_pos += diff1;
		else
			cj_data->cur_pos += diff2;
	}
	else
		cj_data->cur_pos = distance;
	cj_data->cur_vel = (cj_data->cur_pos - last_pos) / timestep;
	cj_data->cur_accel = (cj_data->cur_vel - last_vel) / timestep;
	cj_data->last_dist = distance;

	dMatrix matrix3;
	Util::matrix_from_pin_dir(point, vector, matrix3);

	const dVector& p0 = matrix0.m_posit;
	const dVector& p1 = matrix3.m_posit;
	dVector p00(p0 + matrix0.m_right.Scale(MIN_JOINT_PIN_LENGTH));
	dVector p11(p1 + matrix3.m_right.Scale(MIN_JOINT_PIN_LENGTH));

	// Restrict movement on the pivot point along the normal and bi normal of the path.
	NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix3.m_front[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &matrix3.m_up[0]);
	if (joint_data->ctype == CT_FLEXIBLE)
		NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
	else if (joint_data->ctype == CT_ROBUST)
		NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
	NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

	// Align to curve
	if (cj_data->align) {
		NewtonUserJointAddLinearRow(joint, &p00[0], &p11[0], &matrix3.m_front[0]);
		if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		else
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

		NewtonUserJointAddLinearRow(joint, &p00[0], &p11[0], &matrix3.m_up[0]);
		if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		else
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}

	// Add linear friction or limits
	dFloat min_posit = matrix3.UntransformVector(min_pt).m_z;
	dFloat max_posit = matrix3.UntransformVector(max_pt).m_z;
	dFloat cur_posit = matrix3.UntransformVector(p0).m_z;
	dFloat margin = EPSILON + 0.01f * dAbs(cj_data->cur_vel);
	if (cur_posit < min_posit - margin || (cur_posit < min_posit - Joint::LINEAR_LIMIT_EPSILON && dAbs(min_len) < EPSILON && cj_data->loop == false)) {
		NewtonUserJointAddLinearRow(joint, &p0[0], &min_pt[0], &matrix3.m_right[0]);
		NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else if (cur_posit > max_posit + margin || (cur_posit > max_posit + Joint::LINEAR_LIMIT_EPSILON && dAbs(max_len - cj_data->curve_len) < EPSILON && cj_data->loop == false)) {
		NewtonUserJointAddLinearRow(joint, &p0[0], &max_pt[0], &matrix3.m_right[0]);
		NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		dVector point(matrix3.UntransformVector(matrix0.m_posit));
		point.m_z = 0.0f;
		point = matrix3.TransformVector(point);
		NewtonUserJointAddLinearRow(joint, &point[0], &matrix3.m_posit[0], &matrix3.m_right[0]);
		dFloat power = cj_data->linear_friction * cj_data->controller;
		NewtonUserJointSetRowMinimumFriction(joint, -power);
		NewtonUserJointSetRowMaximumFriction(joint, power);
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}

	// Add angular friction or limits
	if (cj_data->rotate) {
		if (cj_data->align) {
			NewtonUserJointAddAngularRow(joint, 0.0f, &matrix3.m_right[0]);
			dFloat power = cj_data->angular_friction * cj_data->controller;
			NewtonUserJointSetRowMinimumFriction(joint, -power);
			NewtonUserJointSetRowMaximumFriction(joint, power);
			NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
		}
		else {
			dFloat cur_cone_angle_cos = matrix0.m_right % cj_data->last_dir;
			if (dAbs(cur_cone_angle_cos) < 0.99995f) {
				dVector lateral_dir =  matrix0.m_right  * cj_data->last_dir;
				Util::normalize_vector(lateral_dir);
				NewtonUserJointAddAngularRow(joint, 0.0f, &lateral_dir[0]);
				dFloat power = cj_data->angular_friction * cj_data->controller;
				NewtonUserJointSetRowMinimumFriction(joint, -power);
				NewtonUserJointSetRowMaximumFriction(joint, power);
				NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
			}
		}
	}
	else if (cj_data->align) {
		NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle(matrix0.m_front, matrix3.m_front, matrix3.m_right), &matrix3.m_right[0]);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	else {
		// Get a point along the pin axis at some reasonable large distance from the pivot.
		dVector q0(p0 + matrix0.m_right.Scale(MIN_JOINT_PIN_LENGTH));
		dVector q1(p1 + matrix1.m_right.Scale(MIN_JOINT_PIN_LENGTH));
		// Get the ankle point.
		dVector r0(p0 + matrix0.m_front.Scale(MIN_JOINT_PIN_LENGTH));
		dVector r1(p1 + matrix1.m_front.Scale(MIN_JOINT_PIN_LENGTH));
		// Restrict rotation along all three orthonormal directions
		NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_front[0]);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

		NewtonUserJointAddLinearRow(joint, &q0[0], &q1[0], &matrix0.m_up[0]);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);

		NewtonUserJointAddLinearRow(joint, &r0[0], &r1[0], &matrix0.m_up[0]);
		if (joint_data->ctype == CT_FLEXIBLE)
			NewtonUserJointSetRowSpringDamperAcceleration(joint, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
		else if (joint_data->ctype == CT_ROBUST)
			NewtonUserJointSetRowAcceleration(joint, NewtonUserCalculateRowZeroAccelaration(joint));
		NewtonUserJointSetRowStiffness(joint, joint_data->stiffness);
	}
	cj_data->last_dir = matrix0.m_right;
}

void MSNewton::CurvySlider::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
}

void MSNewton::CurvySlider::on_destroy(JointData* joint_data) {
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	delete cj_data;
}

void MSNewton::CurvySlider::on_disconnect(JointData* joint_data) {
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	cj_data->cur_pos = 0.0f;
	cj_data->cur_vel = 0.0f;
	cj_data->cur_accel = 0.0f;
	cj_data->last_dist = 0.0f;
	cj_data->cur_data_set = false;
}

void MSNewton::CurvySlider::adjust_pin_matrix_proc(JointData* joint_data, dMatrix& pin_matrix) {
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	// Recalculate pin matrix so its on curve rather than on the joint origin.
	dMatrix matrix;
	dVector centre, point, vector, min_pt, max_pt;
	dFloat distance, min_len, max_len;
	NewtonBodyGetMatrix(joint_data->child, &matrix[0][0]);
	NewtonBodyGetCentreOfMass(joint_data->child, &centre[0]);
	centre = matrix.TransformVector(centre);
	MSNewton::Joint::c_get_pin_matrix(joint_data, matrix);
	centre = matrix.UntransformVector(centre);
	bool success = c_calc_curve_data_at_location(cj_data, centre, point, vector, distance, min_pt, max_pt, min_len, max_len);
	if (success) {
		point = matrix.TransformVector(point);
		vector = matrix.RotateVector(vector);
		Util::matrix_from_pin_dir(point, vector, pin_matrix);
		cj_data->cur_pos = distance;
		cj_data->last_dist = distance;
		cj_data->last_dir = vector;
	}
}


/*
 ///////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////
*/

VALUE MSNewton::CurvySlider::is_valid(VALUE self, VALUE v_joint) {
	JointData* address = (JointData*)Util::value_to_ll(v_joint);
	bool valid = Util::is_joint_valid(address);
	if (valid && address->jtype != JT_CURVY_SLIDER) valid = false;
	return Util::to_value(valid);
}

VALUE MSNewton::CurvySlider::create(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_NONE);

	CurvySliderData* cj_data = new CurvySliderData;
	cj_data->points;
	cj_data->curve_len = 0.0f;
	cj_data->cur_pos = 0.0f;
	cj_data->cur_vel = 0.0f;
	cj_data->cur_accel = 0.0f;
	cj_data->last_dist = 0.0f;
	cj_data->cur_point;
	cj_data->cur_vector;
	cj_data->cur_tangent;
	cj_data->last_dir;
	cj_data->cur_data_set = false;
	cj_data->linear_friction = DEFAULT_LINEAR_FRICTION;
	cj_data->angular_friction = DEFAULT_ANGULAR_FRICTION;
	cj_data->controller = DEFAULT_CONTROLLER;
	cj_data->loop = DEFAULT_LOOP_ENABLED;
	cj_data->align = DEFAULT_ALIGNMENT_ENABLED;
	cj_data->rotate = DEFAULT_ROTATION_ENABLED;

	joint_data->dof = 6;
	joint_data->jtype = JT_CURVY_SLIDER;
	joint_data->cj_data = cj_data;
	joint_data->submit_constraints = submit_constraints;
	joint_data->get_info = get_info;
	joint_data->on_destroy = on_destroy;
	joint_data->on_disconnect = on_disconnect;
	joint_data->adjust_pin_matrix_proc = adjust_pin_matrix_proc;

	return Util::to_value(joint_data);
}

VALUE MSNewton::CurvySlider::add_point(VALUE self, VALUE v_joint, VALUE v_position) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	dVector point = Util::value_to_point(v_position, world_data->scale);
	dMatrix pin_matrix;
	MSNewton::Joint::c_get_pin_matrix(joint_data, pin_matrix);
	cj_data->points.push_back(pin_matrix.UntransformVector(point));
	cj_data->curve_len = c_calc_curve_length(cj_data->points, cj_data->loop);
	return Util::to_value(cj_data->points.size() - 1);
}

VALUE MSNewton::CurvySlider::remove_point(VALUE self, VALUE v_joint, VALUE v_point_index) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	unsigned int point_index = Util::value_to_uint(v_point_index);
	if (point_index < cj_data->points.size()) {
		cj_data->points.erase(cj_data->points.begin() + point_index);
		cj_data->curve_len = c_calc_curve_length(cj_data->points, cj_data->loop);
		return Qtrue;
	}
	else
		return Qfalse;
}

VALUE MSNewton::CurvySlider::get_points(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	dMatrix pin_matrix;
	MSNewton::Joint::c_get_pin_matrix(joint_data, pin_matrix);
	VALUE v_points = rb_ary_new();
	for(std::vector<dVector>::iterator it = cj_data->points.begin(); it != cj_data->points.end(); ++it) {
		dVector point = pin_matrix.TransformVector(*it);
		rb_ary_push(v_points, Util::point_to_value(point, world_data->inverse_scale));
	}
	return v_points;
}

VALUE MSNewton::CurvySlider::get_points_size(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	return Util::to_value(cj_data->points.size());
}

VALUE MSNewton::CurvySlider::clear_points(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	unsigned int count = (unsigned int)cj_data->points.size();
	cj_data->points.clear();
	cj_data->curve_len = 0.0f;
	return count;
}

VALUE MSNewton::CurvySlider::get_point_position(VALUE self, VALUE v_joint, VALUE v_point_index) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	unsigned int point_index = Util::value_to_uint(v_point_index);
	if (point_index < cj_data->points.size()) {
		dMatrix pin_matrix;
		MSNewton::Joint::c_get_pin_matrix(joint_data, pin_matrix);
		dVector point = pin_matrix.TransformVector(cj_data->points[point_index]);
		return Util::point_to_value(point, world_data->inverse_scale);
	}
	else
		return Qnil;
}

VALUE MSNewton::CurvySlider::set_point_position(VALUE self, VALUE v_joint, VALUE v_point_index, VALUE v_position) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	unsigned int point_index = Util::value_to_uint(v_point_index);
	if (point_index < cj_data->points.size()) {
		dVector point = Util::value_to_point(v_position, world_data->scale);
		dMatrix pin_matrix;
		MSNewton::Joint::c_get_pin_matrix(joint_data, pin_matrix);
		cj_data->points[point_index] = pin_matrix.UntransformVector(point);
		cj_data->curve_len = c_calc_curve_length(cj_data->points, cj_data->loop);
		return Qtrue;
	}
	else
		return Qfalse;
}

VALUE MSNewton::CurvySlider::get_length(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->curve_len * world_data->inverse_scale);
}

VALUE MSNewton::CurvySlider::get_cur_position(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_pos * world_data->inverse_scale);
}

VALUE MSNewton::CurvySlider::get_cur_velocity(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_vel * world_data->inverse_scale);
}

VALUE MSNewton::CurvySlider::get_cur_acceleration(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->cur_accel * world_data->inverse_scale);
}

VALUE MSNewton::CurvySlider::get_cur_point(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	if (cj_data->cur_data_set)
		return Util::point_to_value(cj_data->cur_point, world_data->inverse_scale);
	else
		return Qnil;
}

VALUE MSNewton::CurvySlider::get_cur_vector(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	if (cj_data->cur_data_set)
		return Util::vector_to_value(cj_data->cur_vector);
	else
		return Qnil;
}

VALUE MSNewton::CurvySlider::get_cur_tangent(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	if (cj_data->cur_data_set)
		return Util::vector_to_value(cj_data->cur_tangent);
	else
		return Qnil;
}

VALUE MSNewton::CurvySlider::get_linear_friction(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->linear_friction * world_data->inverse_scale4);
}

VALUE MSNewton::CurvySlider::set_linear_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->linear_friction = Util::clamp_min(Util::value_to_dFloat(v_friction), 0.0f) * world_data->scale4;
	return Util::to_value(cj_data->linear_friction * world_data->inverse_scale4);
}

VALUE MSNewton::CurvySlider::get_angular_friction(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	return Util::to_value(cj_data->angular_friction * world_data->inverse_scale5);
}

VALUE MSNewton::CurvySlider::set_angular_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	cj_data->angular_friction = Util::clamp_min(Util::value_to_dFloat(v_friction), 0.0f) * world_data->scale5;
	return Util::to_value(cj_data->angular_friction * world_data->inverse_scale5);
}

VALUE MSNewton::CurvySlider::get_controller(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	return Util::to_value(cj_data->controller);
}

VALUE MSNewton::CurvySlider::set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	cj_data->controller = Util::clamp_min(Util::value_to_dFloat(v_controller), 0.0f);
	return Util::to_value(cj_data->controller);
}

VALUE MSNewton::CurvySlider::loop_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	return Util::to_value(cj_data->loop);
}

VALUE MSNewton::CurvySlider::enable_loop(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	bool last_loop = cj_data->loop;
	cj_data->loop = Util::value_to_bool(v_state);
	if (cj_data->loop != last_loop) {
		cj_data->curve_len = c_calc_curve_length(cj_data->points, cj_data->loop);
		if (!cj_data->loop) {
			cj_data->cur_pos = fmod(cj_data->cur_pos, cj_data->curve_len);
			if (cj_data->cur_pos < 0.0f) cj_data->cur_pos += cj_data->curve_len;
		}
	}
	return Util::to_value(cj_data->loop);
}

VALUE MSNewton::CurvySlider::alignment_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	return Util::to_value(cj_data->align);
}

VALUE MSNewton::CurvySlider::enable_alignment(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	cj_data->align = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->align);
}

VALUE MSNewton::CurvySlider::rotation_enabled(VALUE self, VALUE v_joint) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	return Util::to_value(cj_data->rotate);
}

VALUE MSNewton::CurvySlider::enable_rotation(VALUE self, VALUE v_joint, VALUE v_state) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	cj_data->rotate = Util::value_to_bool(v_state);
	return Util::to_value(cj_data->rotate);
}

VALUE MSNewton::CurvySlider::get_info_by_pos(VALUE self, VALUE v_joint, VALUE v_pos) {
	JointData* joint_data = Util::value_to_joint2(v_joint, JT_CURVY_SLIDER);
	CurvySliderData* cj_data = (CurvySliderData*)joint_data->cj_data;
	WorldData* world_data = (WorldData*)NewtonWorldGetUserData(joint_data->world);
	dFloat pos = Util::value_to_dFloat(v_pos) * world_data->scale;
	dVector point, vector;
	bool state = c_calc_curve_line_by_pos(cj_data, pos, point, vector);
	if (state) {
		dMatrix pin_matrix;
		MSNewton::Joint::c_get_pin_matrix(joint_data, pin_matrix);
		point = pin_matrix.TransformVector(point);
		return rb_ary_new3(2, Util::point_to_value(point, world_data->inverse_scale), Util::vector_to_value(vector));
	}
	else
		return Qnil;
}



void Init_msp_curvy_slider(VALUE mNewton) {
	VALUE mCurvySlider = rb_define_module_under(mNewton, "CurvySlider");

	rb_define_module_function(mCurvySlider, "is_valid?", VALUEFUNC(MSNewton::CurvySlider::is_valid), 1);
	rb_define_module_function(mCurvySlider, "create", VALUEFUNC(MSNewton::CurvySlider::create), 1);
	rb_define_module_function(mCurvySlider, "add_point", VALUEFUNC(MSNewton::CurvySlider::add_point), 2);
	rb_define_module_function(mCurvySlider, "remove_point", VALUEFUNC(MSNewton::CurvySlider::remove_point), 2);
	rb_define_module_function(mCurvySlider, "get_points", VALUEFUNC(MSNewton::CurvySlider::get_points), 1);
	rb_define_module_function(mCurvySlider, "get_points_size", VALUEFUNC(MSNewton::CurvySlider::get_points), 1);
	rb_define_module_function(mCurvySlider, "clear_points", VALUEFUNC(MSNewton::CurvySlider::clear_points), 2);
	rb_define_module_function(mCurvySlider, "get_point_position", VALUEFUNC(MSNewton::CurvySlider::get_point_position), 2);
	rb_define_module_function(mCurvySlider, "set_point_position", VALUEFUNC(MSNewton::CurvySlider::set_point_position), 3);
	rb_define_module_function(mCurvySlider, "get_length", VALUEFUNC(MSNewton::CurvySlider::get_length), 1);
	rb_define_module_function(mCurvySlider, "get_cur_position", VALUEFUNC(MSNewton::CurvySlider::get_cur_position), 1);
	rb_define_module_function(mCurvySlider, "get_cur_velocity", VALUEFUNC(MSNewton::CurvySlider::get_cur_velocity), 1);
	rb_define_module_function(mCurvySlider, "get_cur_acceleration", VALUEFUNC(MSNewton::CurvySlider::get_cur_acceleration), 1);
	rb_define_module_function(mCurvySlider, "get_cur_point", VALUEFUNC(MSNewton::CurvySlider::get_cur_point), 1);
	rb_define_module_function(mCurvySlider, "get_cur_vector", VALUEFUNC(MSNewton::CurvySlider::get_cur_vector), 1);
	rb_define_module_function(mCurvySlider, "get_cur_tangent", VALUEFUNC(MSNewton::CurvySlider::get_cur_tangent), 1);
	rb_define_module_function(mCurvySlider, "get_linear_friction", VALUEFUNC(MSNewton::CurvySlider::get_linear_friction), 1);
	rb_define_module_function(mCurvySlider, "set_linear_friction", VALUEFUNC(MSNewton::CurvySlider::set_linear_friction), 2);
	rb_define_module_function(mCurvySlider, "get_angular_friction", VALUEFUNC(MSNewton::CurvySlider::get_angular_friction), 1);
	rb_define_module_function(mCurvySlider, "set_angular_friction", VALUEFUNC(MSNewton::CurvySlider::set_angular_friction), 2);
	rb_define_module_function(mCurvySlider, "get_controller", VALUEFUNC(MSNewton::CurvySlider::get_controller), 1);
	rb_define_module_function(mCurvySlider, "set_controller", VALUEFUNC(MSNewton::CurvySlider::set_controller), 2);
	rb_define_module_function(mCurvySlider, "loop_enabled?", VALUEFUNC(MSNewton::CurvySlider::loop_enabled), 1);
	rb_define_module_function(mCurvySlider, "enable_loop", VALUEFUNC(MSNewton::CurvySlider::enable_loop), 2);
	rb_define_module_function(mCurvySlider, "alignment_enabled?", VALUEFUNC(MSNewton::CurvySlider::alignment_enabled), 1);
	rb_define_module_function(mCurvySlider, "enable_alignment", VALUEFUNC(MSNewton::CurvySlider::enable_alignment), 2);
	rb_define_module_function(mCurvySlider, "rotation_enabled?", VALUEFUNC(MSNewton::CurvySlider::rotation_enabled), 1);
	rb_define_module_function(mCurvySlider, "enable_rotation", VALUEFUNC(MSNewton::CurvySlider::enable_rotation), 2);
	rb_define_module_function(mCurvySlider, "get_info_by_pos", VALUEFUNC(MSNewton::CurvySlider::get_info_by_pos), 2);
}
