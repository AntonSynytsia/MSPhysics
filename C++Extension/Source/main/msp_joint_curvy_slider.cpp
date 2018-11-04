/*
 * ---------------------------------------------------------------------------------------------------------------------
 *
 * Copyright (C) 2018, Anton Synytsia
 *
 * ---------------------------------------------------------------------------------------------------------------------
 */

#include "msp_joint_curvy_slider.h"
#include "msp_world.h"

/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Constants
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

const dFloat MSP::CurvySlider::DEFAULT_LINEAR_FRICTION(0.0f);
const dFloat MSP::CurvySlider::DEFAULT_ANGULAR_FRICTION(0.0f);
const dFloat MSP::CurvySlider::DEFAULT_ALIGNMENT_POWER(0.0f);
const dFloat MSP::CurvySlider::DEFAULT_CONTROLLER(1.0f);
const bool MSP::CurvySlider::DEFAULT_LOOP_ENABLED(false);
const bool MSP::CurvySlider::DEFAULT_ALIGNMENT_ENABLED(true);
const bool MSP::CurvySlider::DEFAULT_ROTATION_ENABLED(true);


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Helper Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::CurvySlider::c_clear_curve_edges(MSP::Joint::JointData* joint_data) {
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    for (std::map<unsigned int, EdgeData*>::iterator it = cj_data->m_edges.begin(); it != cj_data->m_edges.end(); ++it) {
        delete it->second;
    }
    cj_data->m_edges.clear();
    cj_data->m_curve_len = 0.0f;
}

void MSP::CurvySlider::c_update_curve_edges(MSP::Joint::JointData* joint_data) {
    if (!joint_data->m_connected) return;
    c_clear_curve_edges(joint_data);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    unsigned int points_size = (unsigned int)cj_data->m_points.size();
    if (points_size < 2) return;
    std::map<unsigned int, EdgeData*> temp_edges;
    // Calculate all the succeeding edge normals from the current index.
    dMatrix last_matrix(joint_data->m_local_matrix1 * joint_data->m_local_matrix2.Inverse());
    unsigned int pt_index = cj_data->m_initial_edge_index;
    for (unsigned int i = cj_data->m_initial_edge_index; i < (cj_data->m_loop ? points_size : points_size - 1); ++i) {
        const dVector& pt1 = cj_data->m_points[pt_index];
        const dVector& pt2 = cj_data->m_points[(i == points_size - 1 && cj_data->m_loop) ? 0 : i + 1];
        dVector dir(pt2 - pt1);
        dFloat mag = Util::get_vector_magnitude(dir);
        if (mag > M_EPSILON) {
            Util::scale_vector(dir, 1.0f / mag);
            EdgeData *edge_data = new EdgeData(mag, 0.0f, pt_index, i + 1);
            Util::rotate_matrix_to_dir(last_matrix, dir, edge_data->m_normal_matrix);
            edge_data->m_normal_matrix.m_posit = cj_data->m_points[pt_index];
            temp_edges[pt_index] = edge_data;
            last_matrix = edge_data->m_normal_matrix;
            pt_index = i + 1;
            cj_data->m_curve_len += mag;
        }
    }
    // As well as all the preceding edge normals from the current index.
    last_matrix = joint_data->m_local_matrix1 * joint_data->m_local_matrix2.Inverse();
    pt_index = cj_data->m_initial_edge_index;
    for (unsigned int i = cj_data->m_initial_edge_index; i > 0; --i) {
        const dVector& pt1 = cj_data->m_points[i - 1];
        const dVector& pt2 = cj_data->m_points[pt_index];
        dVector dir(pt2 - pt1);
        dFloat mag = Util::get_vector_magnitude(dir);
        if (mag > M_EPSILON) {
            Util::scale_vector(dir, 1.0f / mag);
            EdgeData *edge_data = new EdgeData(mag, 0.0f, i - 1, pt_index);
            Util::rotate_matrix_to_dir(last_matrix, dir, edge_data->m_normal_matrix);
            edge_data->m_normal_matrix.m_posit = cj_data->m_points[i - 1];
            temp_edges[i - 1] = edge_data;
            last_matrix = edge_data->m_normal_matrix;
            pt_index = i - 1;
            cj_data->m_curve_len += mag;
        }
    }
    // Update the edges map so there is no jumps in indexes
    dFloat elapsed_curve_len = 0.0f;
    unsigned int index = 0;
    for (std::map<unsigned int, EdgeData*>::iterator it = temp_edges.begin(); it != temp_edges.end(); ++it) {
        cj_data->m_edges[index] = it->second;
        it->second->m_preceding_curve_length = elapsed_curve_len;
        ++index;
        elapsed_curve_len += it->second->m_length;
    }
    temp_edges.clear();
}

bool MSP::CurvySlider::c_calc_curve_data_at_position(const MSP::Joint::JointData* joint_data, dFloat position, dMatrix& normal_matrix, dFloat &distance, dFloat &overpass) {
    if (!joint_data->m_connected) return false;
    const CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    if (cj_data->m_loop) {
        distance = fmod(position, cj_data->m_curve_len);
        if (distance < 0.0f) distance += cj_data->m_curve_len;
        overpass = 0.0f;
    }
    else {
        distance = Util::clamp_float(position, 0.0f, cj_data->m_curve_len);
        overpass = position - distance;
        if (overpass < -M_EPSILON) {
            normal_matrix = cj_data->m_edges.begin()->second->m_normal_matrix;
            return true;
        }
        else if (overpass > M_EPSILON) {
            const EdgeData* edge_data = cj_data->m_edges.rbegin()->second;
            normal_matrix = edge_data->m_normal_matrix;
            normal_matrix.m_posit = normal_matrix.m_posit + normal_matrix.m_right.Scale(edge_data->m_length);
            return true;
        }
    }
    dFloat traveled_dist = 0.0f;
    for (std::map<unsigned int, EdgeData*>::const_iterator it = cj_data->m_edges.begin(); it != cj_data->m_edges.end(); ++it) {
        const EdgeData* edge_data = it->second;
        if (traveled_dist + edge_data->m_length >= distance) {
            normal_matrix = edge_data->m_normal_matrix;
            normal_matrix.m_posit = normal_matrix.m_posit + normal_matrix.m_right.Scale(distance - traveled_dist);
            return true;
        }
        traveled_dist += edge_data->m_length;
    }
    const EdgeData* edge_data = cj_data->m_edges.rbegin()->second;
    normal_matrix = edge_data->m_normal_matrix;
    normal_matrix.m_posit = normal_matrix.m_posit + normal_matrix.m_right.Scale(edge_data->m_length);
    return true;
}

bool MSP::CurvySlider::c_calc_curve_data_at_point(const MSP::Joint::JointData* joint_data, const dVector &location, dMatrix& normal_matrix, dFloat &distance, dFloat &overpass) {
    if (!joint_data->m_connected) return false;
    const CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    // Find the closest point on curve
    dVector closest_point;
    dFloat closest_distance = 0.0f;
    dFloat closest_left_over = 0.0f;
    unsigned int closest_normal1_index;
    unsigned int closest_normal2_index;
    bool closest_set = false;
    for (std::map<unsigned int, EdgeData*>::const_iterator it = cj_data->m_edges.begin(); it != cj_data->m_edges.end(); ++it) {
        const EdgeData* edge_data = it->second;
        dFloat lpointz = (location - edge_data->m_normal_matrix.m_posit).DotProduct3(edge_data->m_normal_matrix.m_right);
        dVector cpoint;
        dFloat section_dist;
        dFloat left_over;
        unsigned int normal1_index;
        unsigned int normal2_index;
        if (lpointz < 0.0f) {
            normal1_index = it->first - 1;
            normal2_index = it->first;
            cpoint = cj_data->m_points[edge_data->m_start_index];
            section_dist = 0.0f;
            left_over = lpointz;
        }
        else if (lpointz > edge_data->m_length) {
            normal1_index = it->first;
            normal2_index = it->first + 1;
            cpoint = cj_data->m_points[edge_data->m_end_index];
            section_dist = edge_data->m_length;
            left_over = lpointz - edge_data->m_length;
        }
        else {
            normal1_index = it->first;
            normal2_index = it->first;
            cpoint = edge_data->m_normal_matrix.m_posit + edge_data->m_normal_matrix.m_right.Scale(lpointz);
            section_dist = lpointz;
            left_over = 0.0f;
        }
        dFloat cdist = Util::get_vector_magnitude2(location - cpoint);
        if (!closest_set || cdist < closest_distance) {
            distance = edge_data->m_preceding_curve_length + section_dist;
            closest_point = cpoint;
            closest_distance = cdist;
            closest_normal1_index = normal1_index;
            closest_normal2_index = normal2_index;
            closest_left_over = left_over;
            closest_set = true;
            if (closest_distance < M_EPSILON) break;
        }
    }
    if (!closest_set)
        return false;
    // Calculate the normal at point
    std::map<unsigned int, EdgeData*>::const_iterator it1(cj_data->m_edges.find(closest_normal1_index));
    std::map<unsigned int, EdgeData*>::const_iterator it2(cj_data->m_edges.find(closest_normal2_index));
    if (it1 == cj_data->m_edges.end()) {
        if (cj_data->m_loop)
            it1 = cj_data->m_edges.find(cj_data->m_edges.rbegin()->first);
        else
            it1 = it2;
    }
    else if (it2 == cj_data->m_edges.end()) {
        if (cj_data->m_loop)
            it2 = cj_data->m_edges.begin();
        else
            it2 = it1;
    }
    dFloat cos_angle = it1->second->m_normal_matrix.m_right.DotProduct3(it2->second->m_normal_matrix.m_right);
    if (dAbs(cos_angle) > 0.999995f) {
        normal_matrix = it1->second->m_normal_matrix;
        if (it1 == it2 && !cj_data->m_loop)
            overpass = closest_left_over;
        else
            overpass = 0.0f;
    }
    else {
        if (dSqrt(closest_distance) > M_EPSILON) {
            dVector udir(it1->second->m_normal_matrix.m_right.CrossProduct(it2->second->m_normal_matrix.m_right));
            dVector vdir(location - closest_point);
            dVector zdir(udir.CrossProduct(vdir));
            dFloat mag = Util::get_vector_magnitude(zdir);
            if (mag > M_EPSILON)
                Util::rotate_matrix_to_dir(it1->second->m_normal_matrix, zdir, normal_matrix);
            else
                normal_matrix = it1->second->m_normal_matrix;
        }
        else
            normal_matrix = it1->second->m_normal_matrix;
        overpass = 0.0f;
    }
    normal_matrix.m_posit = closest_point;
    return closest_set;
}

bool MSP::CurvySlider::c_calc_curve_data_at_point2(const MSP::Joint::JointData* joint_data, const dVector& location, dVector& point, dVector& vector, dFloat &distance, unsigned int &edge_index) {
    const CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    unsigned int points_size = (unsigned int)cj_data->m_points.size();
    if (points_size < 2) return false;
    dFloat closest_dist = 0.0f;
    dFloat closest_left_over = 0.0f;
    bool closest_set = false;
    dFloat traveled_dist = 0.0f;
    unsigned int pt1_index = 0;
    for (unsigned int i = 0; i < (cj_data->m_loop ? points_size : points_size - 1); ++i) {
        const dVector& pt1 = cj_data->m_points[pt1_index];
        const dVector& pt2 = cj_data->m_points[(i == points_size - 1 && cj_data->m_loop) ? 0 : i + 1];
        dVector edge_dir(pt2 - pt1);
        dFloat edge_len = Util::get_vector_magnitude(edge_dir);
        if (edge_len > M_EPSILON) {
            Util::scale_vector(edge_dir, 1.0f / edge_len);
            dFloat lpointz = (location - pt1).DotProduct3(edge_dir);
            dVector cpoint;
            dFloat section_dist;
            dFloat left_over;
            if (lpointz < 0.0f) {
                cpoint = pt1;
                section_dist = 0.0f;
                left_over = lpointz;
            }
            else if (lpointz > edge_len) {
                cpoint = pt2;
                section_dist = edge_len;
                left_over = lpointz - edge_len;
            }
            else {
                cpoint = pt1 + edge_dir.Scale(lpointz);
                section_dist = lpointz;
                left_over = 0.0f;
            }
            dFloat cdist = Util::get_vector_magnitude2(location - cpoint);
            if (!closest_set || cdist < closest_dist) {
                closest_dist = cdist;
                closest_left_over = left_over;
                distance = traveled_dist + section_dist;
                if (dAbs(left_over) < M_EPSILON) {
                    point = cpoint;
                    vector = edge_dir;
                    edge_index = i;
                }
                closest_set = true;
            }
            traveled_dist += edge_len;
            pt1_index = i + 1;
        }
    }
    if (!closest_set) return false;
    if (dAbs(closest_left_over) > M_EPSILON) {
        dFloat temp_curve_len = 0.0f;
        pt1_index = 0;
        for (unsigned int i = 0; i < (cj_data->m_loop ? points_size : points_size - 1); ++i) {
            const dVector& pt1 = cj_data->m_points[pt1_index];
            const dVector& pt2 = cj_data->m_points[(i == points_size - 1 && cj_data->m_loop) ? 0 : i + 1];
            dVector edge_dir(pt2 - pt1);
            dFloat edge_len = Util::get_vector_magnitude(edge_dir);
            if (edge_len > M_EPSILON) {
                temp_curve_len += edge_len;
                pt1_index = i + 1;
            }
        }
        distance += closest_left_over;
        if (cj_data->m_loop) {
            distance = fmod(distance, temp_curve_len);
            if (distance < 0.0f) distance += temp_curve_len;
        }
        else
            distance = Util::clamp_float(distance, 0.0f, temp_curve_len);
        traveled_dist = 0.0f;
        pt1_index = 0;
        for (unsigned int i = 0; i < (cj_data->m_loop ? points_size : points_size - 1); ++i) {
            const dVector& pt1 = cj_data->m_points[pt1_index];
            const dVector& pt2 = cj_data->m_points[(i == points_size - 1 && cj_data->m_loop) ? 0 : i + 1];
            dVector edge_dir(pt2 - pt1);
            dFloat edge_len = Util::get_vector_magnitude(edge_dir);
            if (edge_len > M_EPSILON) {
                if (traveled_dist + edge_len >= distance) {
                    Util::scale_vector(edge_dir, 1.0f / edge_len);
                    point = pt1 + edge_dir.Scale(distance - traveled_dist);
                    vector = edge_dir;
                    edge_index = pt1_index;
                    break;
                }
                traveled_dist += edge_len;
                pt1_index = i + 1;
            }
        }
    }
    return true;
}

bool MSP::CurvySlider::c_calc_curve_data_at_point3(const MSP::Joint::JointData* joint_data, dFloat last_dist, const dVector &location, dMatrix& normal_matrix, dFloat &distance, dFloat &overpass) {
    if (!joint_data->m_connected) return false;
    const CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    // Find the current edge data.
    const EdgeData* cur_edge_data = nullptr;
    if (last_dist <= M_EPSILON)
        cur_edge_data = cj_data->m_edges.begin()->second;
    else if (last_dist >= cj_data->m_curve_len - M_EPSILON)
        cur_edge_data = cj_data->m_edges.rbegin()->second;
    else {
        for (std::map<unsigned int, EdgeData*>::const_iterator it = cj_data->m_edges.begin(); it != cj_data->m_edges.end(); ++it) {
            const EdgeData* edge_data = it->second;
            if (last_dist >= edge_data->m_preceding_curve_length && last_dist < edge_data->m_preceding_curve_length + edge_data->m_length) {
                cur_edge_data = edge_data;
                break;
            }
        }
        if (cur_edge_data == nullptr) return false;
    }
    // First check if the location denotes the current edge data.
    dFloat cur_lpointz = (location - cur_edge_data->m_normal_matrix.m_posit).DotProduct3(cur_edge_data->m_normal_matrix.m_right);
    if (cur_lpointz >= 0.0f && cur_lpointz <= cur_edge_data->m_length) {
        normal_matrix = cur_edge_data->m_normal_matrix;
        normal_matrix.m_posit += normal_matrix.m_right.Scale(cur_lpointz);
        distance = cur_edge_data->m_preceding_curve_length + cur_lpointz;
        overpass = 0.0f;
        return true;
    }
    // Otherwise check all the preceding and consequent edges.
    unsigned int total_edges_count = (unsigned int)cj_data->m_edges.size();
    bool found_potential_ref_point = false;
    if (cj_data->m_loop) {
        // Check all the preceding edges until reach the half curve length.
        // Do first run in reverse, starting from the current edge and proceeding until we reach the beginning.
        dFloat previous_left_over = 0.0f;
        const EdgeData* previous_edge_data = nullptr;
        bool previous_set = false;
        bool cur_edge_data_found = false;
        for (std::map<unsigned int, EdgeData*>::const_reverse_iterator it = cj_data->m_edges.rbegin(); it != cj_data->m_edges.rend(); ++it) {
            // Skip until we reach the current edge index
            if (!cur_edge_data_found) {
                if (it->second == cur_edge_data)
                    cur_edge_data_found = true;
                else
                    continue;
            }
            // Otherwise do the processing
            const EdgeData* edge_data = it->second;
            dFloat lpointz = (location - edge_data->m_normal_matrix.m_posit).DotProduct3(edge_data->m_normal_matrix.m_right);
            dFloat left_over = 0.0f;
            if (lpointz < 0.0f)
                left_over = lpointz;
            else if (lpointz > edge_data->m_length)
                left_over = lpointz - edge_data->m_length;
            else {
                // Snap to edge when left over is zero
                normal_matrix = edge_data->m_normal_matrix;
                normal_matrix.m_posit = edge_data->m_normal_matrix.m_posit + edge_data->m_normal_matrix.m_right.Scale(lpointz);
                distance = edge_data->m_preceding_curve_length + lpointz;
                overpass = 0.0f;
                found_potential_ref_point = true;
                break;
            }
            // Snap to point of the previous edge in case left_over changes from negative to positive
            if (previous_set && previous_left_over < 0.0f && left_over > 0.0f) {
                c_calc_pivot_normal(edge_data->m_normal_matrix, previous_edge_data->m_normal_matrix, previous_edge_data->m_normal_matrix.m_posit, location, normal_matrix);
                distance = previous_edge_data->m_preceding_curve_length;
                overpass = 0.0f;
                found_potential_ref_point = true;
                break;
            }
            // Update previous data
            previous_edge_data = edge_data;
            previous_left_over = left_over;
            previous_set = true;
        }
        // Do the second run in reverse, starting from the end and proceeding until we reach the current edge.
        if (!found_potential_ref_point) {
            //~ previous_set = false;
            for (std::map<unsigned int, EdgeData*>::const_reverse_iterator it = cj_data->m_edges.rbegin(); it != cj_data->m_edges.rend(); ++it) {
                // Break when we reach the current edge index
                if (it->second == cur_edge_data) break;
                // Otherwise do the processing
                const EdgeData* edge_data = it->second;
                dFloat lpointz = (location - edge_data->m_normal_matrix.m_posit).DotProduct3(edge_data->m_normal_matrix.m_right);
                dFloat left_over = 0.0f;
                if (lpointz < 0.0f)
                    left_over = lpointz;
                else if (lpointz > edge_data->m_length)
                    left_over = lpointz - edge_data->m_length;
                else {
                    // Snap to edge when left over is zero
                    normal_matrix = edge_data->m_normal_matrix;
                    normal_matrix.m_posit = edge_data->m_normal_matrix.m_posit + edge_data->m_normal_matrix.m_right.Scale(lpointz);
                    distance = edge_data->m_preceding_curve_length + lpointz;
                    overpass = 0.0f;
                    found_potential_ref_point = true;
                    break;
                }
                // Snap to point of the previous edge in case left_over changes from negative to positive
                if (previous_set && previous_left_over < 0.0f && left_over > 0.0f) {
                    c_calc_pivot_normal(edge_data->m_normal_matrix, previous_edge_data->m_normal_matrix, previous_edge_data->m_normal_matrix.m_posit, location, normal_matrix);
                    distance = previous_edge_data->m_preceding_curve_length;
                    overpass = 0.0f;
                    found_potential_ref_point = true;
                    break;
                }
                // Update previous data
                previous_edge_data = edge_data;
                previous_left_over = left_over;
                previous_set = true;
            }
        }
        // Check all the succeeding edges until reach the half curve length.
        // Do first run, starting from the current edge and proceeding until we reach the end.
        previous_set = false;
        cur_edge_data_found = false;
        for (std::map<unsigned int, EdgeData*>::const_iterator it = cj_data->m_edges.begin(); it != cj_data->m_edges.end(); ++it) {
            // Skip until we reach the current edge index
            if (!cur_edge_data_found) {
                if (it->second == cur_edge_data)
                    cur_edge_data_found = true;
                else
                    continue;
            }
            // Otherwise do the processing
            const EdgeData* edge_data = it->second;
            dFloat lpointz = (location - edge_data->m_normal_matrix.m_posit).DotProduct3(edge_data->m_normal_matrix.m_right);
            dFloat left_over = 0.0f;
            if (lpointz < 0.0f)
                left_over = lpointz;
            else if (lpointz > edge_data->m_length)
                left_over = lpointz - edge_data->m_length;
            else {
                // Snap to edge when left over is zero
                if (found_potential_ref_point) {
                    dFloat original_dist1 = dAbs(last_dist - distance);
                    dFloat original_dist2 = cj_data->m_curve_len - original_dist1;
                    dFloat current_dist1 = dAbs(edge_data->m_preceding_curve_length + lpointz - last_dist);
                    dFloat current_dist2 = cj_data->m_curve_len - current_dist1;
                    dFloat min_original_dist = (original_dist1 < original_dist2) ? original_dist1 : original_dist2;
                    dFloat min_current_dist = (current_dist1 < current_dist2) ? current_dist1 : current_dist2;
                    if (min_original_dist <= min_current_dist) return true;
                }
                normal_matrix = edge_data->m_normal_matrix;
                normal_matrix.m_posit = edge_data->m_normal_matrix.m_posit + edge_data->m_normal_matrix.m_right.Scale(lpointz);
                distance = edge_data->m_preceding_curve_length + lpointz;
                overpass = 0.0f;
                return true;
            }
            // Snap to point of the current edge in case left_over changes from positive to negative
            if (previous_set && previous_left_over > 0.0f && left_over < 0.0f) {
                if (found_potential_ref_point) {
                    dFloat original_dist1 = dAbs(last_dist - distance);
                    dFloat original_dist2 = cj_data->m_curve_len - original_dist1;
                    dFloat current_dist1 = dAbs(edge_data->m_preceding_curve_length - last_dist);
                    dFloat current_dist2 = cj_data->m_curve_len - current_dist1;
                    dFloat min_original_dist = (original_dist1 < original_dist2) ? original_dist1 : original_dist2;
                    dFloat min_current_dist = (current_dist1 < current_dist2) ? current_dist1 : current_dist2;
                    if (min_original_dist <= min_current_dist) return true;
                }
                c_calc_pivot_normal(previous_edge_data->m_normal_matrix, edge_data->m_normal_matrix, edge_data->m_normal_matrix.m_posit, location, normal_matrix);
                distance = edge_data->m_preceding_curve_length;
                overpass = 0.0f;
                return true;
            }
            // Update previous data
            previous_edge_data = edge_data;
            previous_left_over = left_over;
            previous_set = true;
        }
        // Do second run, starting from the beginning and proceeding until we reach the current edge.
        //~ previous_set = false;
        for (std::map<unsigned int, EdgeData*>::const_iterator it = cj_data->m_edges.begin(); it != cj_data->m_edges.end(); ++it) {
            // Break when we reach the current edge index
            if (it->second == cur_edge_data) break;
            // Otherwise do the processing
            const EdgeData* edge_data = it->second;
            dFloat lpointz = (location - edge_data->m_normal_matrix.m_posit).DotProduct3(edge_data->m_normal_matrix.m_right);
            dFloat left_over = 0.0f;
            if (lpointz < 0.0f)
                left_over = lpointz;
            else if (lpointz > edge_data->m_length)
                left_over = lpointz - edge_data->m_length;
            else {
                // Snap to edge when left over is zero
                if (found_potential_ref_point) {
                    dFloat original_dist1 = dAbs(last_dist - distance);
                    dFloat original_dist2 = cj_data->m_curve_len - original_dist1;
                    dFloat current_dist1 = dAbs(edge_data->m_preceding_curve_length + lpointz - last_dist);
                    dFloat current_dist2 = cj_data->m_curve_len - current_dist1;
                    dFloat min_original_dist = (original_dist1 < original_dist2) ? original_dist1 : original_dist2;
                    dFloat min_current_dist = (current_dist1 < current_dist2) ? current_dist1 : current_dist2;
                    if (min_original_dist <= min_current_dist) return true;
                }
                normal_matrix = edge_data->m_normal_matrix;
                normal_matrix.m_posit = edge_data->m_normal_matrix.m_posit + edge_data->m_normal_matrix.m_right.Scale(lpointz);
                distance = edge_data->m_preceding_curve_length + lpointz;
                overpass = 0.0f;
                return true;
            }
            // Snap to point of the current edge in case left_over changes from positive to negative
            if (previous_set && previous_left_over > 0.0f && left_over < 0.0f) {
                if (found_potential_ref_point) {
                    dFloat original_dist1 = dAbs(last_dist - distance);
                    dFloat original_dist2 = cj_data->m_curve_len - original_dist1;
                    dFloat current_dist1 = dAbs(edge_data->m_preceding_curve_length - last_dist);
                    dFloat current_dist2 = cj_data->m_curve_len - current_dist1;
                    dFloat min_original_dist = (original_dist1 < original_dist2) ? original_dist1 : original_dist2;
                    dFloat min_current_dist = (current_dist1 < current_dist2) ? current_dist1 : current_dist2;
                    if (min_original_dist <= min_current_dist) return true;
                }
                c_calc_pivot_normal(previous_edge_data->m_normal_matrix, edge_data->m_normal_matrix, edge_data->m_normal_matrix.m_posit, location, normal_matrix);
                distance = edge_data->m_preceding_curve_length;
                overpass = 0.0f;
                return true;
            }
            // Update previous data
            previous_edge_data = edge_data;
            previous_left_over = left_over;
            previous_set = true;
        }
        // If nothing found, snap to the starting point.
        if (!found_potential_ref_point) {
            const EdgeData* edge_data1 = cj_data->m_edges.rbegin()->second;
            const EdgeData* edge_data2 = cj_data->m_edges.begin()->second;
            c_calc_pivot_normal(edge_data1->m_normal_matrix, edge_data2->m_normal_matrix, edge_data2->m_normal_matrix.m_posit, location, normal_matrix);
            distance = 0.0f;
            overpass = 0.0f;
            return true;
        }
    }
    else { // Loop disabled
        // Find the closest preceding edge
        dFloat previous_left_over = 0.0f;
        const EdgeData* previous_edge_data = nullptr;
        bool previous_set = false;
        bool cur_edge_data_found = false;
        for (std::map<unsigned int, EdgeData*>::const_reverse_iterator it = cj_data->m_edges.rbegin(); it != cj_data->m_edges.rend(); ++it) {
            // Skip until we reach the current edge index
            if (!cur_edge_data_found) {
                if (it->second == cur_edge_data)
                    cur_edge_data_found = true;
                else
                    continue;
            }
            // Otherwise do the processing
            const EdgeData* edge_data = it->second;
            dFloat lpointz = (location - edge_data->m_normal_matrix.m_posit).DotProduct3(edge_data->m_normal_matrix.m_right);
            dFloat left_over = 0.0f;
            if (lpointz < 0.0f) {
                left_over = lpointz;
                // Minimum limit
                if (it->first == 0) {
                    normal_matrix = edge_data->m_normal_matrix;
                    distance = 0.0f;
                    overpass = left_over;
                    found_potential_ref_point = true;
                    break;
                }
            }
            else if (lpointz > edge_data->m_length)
                left_over = lpointz - edge_data->m_length;
            else {
                // Snap to edge when left over is zero
                normal_matrix = edge_data->m_normal_matrix;
                normal_matrix.m_posit = edge_data->m_normal_matrix.m_posit + edge_data->m_normal_matrix.m_right.Scale(lpointz);
                distance = edge_data->m_preceding_curve_length + lpointz;
                overpass = 0.0f;
                found_potential_ref_point = true;
                break;
            }
            // Snap to point of the previous edge in case left_over changes from negative to positive
            if (previous_set && previous_left_over < 0.0f && left_over > 0.0f) {
                c_calc_pivot_normal(edge_data->m_normal_matrix, previous_edge_data->m_normal_matrix, previous_edge_data->m_normal_matrix.m_posit, location, normal_matrix);
                distance = previous_edge_data->m_preceding_curve_length;
                overpass = 0.0f;
                found_potential_ref_point = true;
                break;
            }
            // Update previous data
            previous_edge_data = edge_data;
            previous_left_over = left_over;
            previous_set = true;
        }
        // Find the closest succeeding point
        previous_set = false;
        cur_edge_data_found = false;
        for (std::map<unsigned int, EdgeData*>::const_iterator it = cj_data->m_edges.begin(); it != cj_data->m_edges.end(); ++it) {
            // Skip until we reach the current edge index
            if (!cur_edge_data_found) {
                if (it->second == cur_edge_data)
                    cur_edge_data_found = true;
                else
                    continue;
            }
            // Otherwise do the processing
            const EdgeData* edge_data = it->second;
            dFloat lpointz = (location - edge_data->m_normal_matrix.m_posit).DotProduct3(edge_data->m_normal_matrix.m_right);
            dFloat left_over = 0.0f;
            if (lpointz < 0.0f)
                left_over = lpointz;
            else if (lpointz > edge_data->m_length) {
                left_over = lpointz - edge_data->m_length;
                // Maximum limit
                if (it->first == total_edges_count - 1) {
                    if (found_potential_ref_point) {
                        dFloat original_dist = dAbs(last_dist - distance);
                        dFloat current_dist = dAbs(cj_data->m_curve_len - last_dist);
                        if (original_dist <= current_dist) return true;
                    }
                    normal_matrix = edge_data->m_normal_matrix;
                    normal_matrix.m_posit = edge_data->m_normal_matrix.m_posit + edge_data->m_normal_matrix.m_right.Scale(edge_data->m_length);
                    distance = cj_data->m_curve_len;
                    overpass = left_over;
                    return true;
                }
            }
            else {
                // Snap to edge when left over is zero
                if (found_potential_ref_point) {
                    dFloat original_dist = dAbs(last_dist - distance);
                    dFloat current_dist = dAbs(edge_data->m_preceding_curve_length + lpointz - last_dist);
                    if (original_dist <= current_dist) return true;
                }
                normal_matrix = edge_data->m_normal_matrix;
                normal_matrix.m_posit = edge_data->m_normal_matrix.m_posit + edge_data->m_normal_matrix.m_right.Scale(lpointz);
                distance = edge_data->m_preceding_curve_length + lpointz;
                overpass = 0.0f;
                return true;
            }
            // Snap to point of the current edge in case left_over changes from positive to negative
            if (previous_set && previous_left_over > 0.0f && left_over < 0.0f) {
                if (found_potential_ref_point) {
                    dFloat original_dist = dAbs(last_dist - distance);
                    dFloat current_dist = dAbs(edge_data->m_preceding_curve_length - last_dist);
                    if (original_dist <= current_dist) return true;
                }
                c_calc_pivot_normal(previous_edge_data->m_normal_matrix, edge_data->m_normal_matrix, edge_data->m_normal_matrix.m_posit, location, normal_matrix);
                distance = edge_data->m_preceding_curve_length;
                overpass = 0.0f;
                return true;
            }
            // Update previous data
            previous_edge_data = edge_data;
            previous_left_over = left_over;
            previous_set = true;
        }
    }
    return found_potential_ref_point;
}

void MSP::CurvySlider::c_calc_pivot_normal(const dMatrix& normal_matrix1, const dMatrix& normal_matrix2, const dVector& pivot_point, const dVector& location, dMatrix& normal_matrix_out) {
    dFloat cos_angle = normal_matrix1.m_right.DotProduct3(normal_matrix2.m_right);
    if (dAbs(cos_angle) > 0.999995f)
        normal_matrix_out = normal_matrix1;
    else {
        if (Util::get_vector_magnitude2(location - pivot_point) > M_EPSILON) {
            dVector udir(normal_matrix1.m_right.CrossProduct(normal_matrix2.m_right));
            dVector vdir(location - pivot_point);
            dVector zdir(udir.CrossProduct(vdir));
            dFloat mag = Util::get_vector_magnitude2(zdir);
            if (mag > M_EPSILON)
                Util::rotate_matrix_to_dir(normal_matrix1, zdir, normal_matrix_out);
            else
                normal_matrix_out = normal_matrix1;
        }
        else
            normal_matrix_out = normal_matrix1;
    }
    normal_matrix_out.m_posit = pivot_point;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Callback Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::CurvySlider::submit_constraints(const NewtonJoint* joint, dFloat timestep, int thread_index) {
    MSP::Joint::JointData* joint_data = reinterpret_cast<MSP::Joint::JointData*>(NewtonJointGetUserData(joint));
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);

    dFloat inv_timestep = 1.0f / timestep;

    // Calculate position of pivot points and Jacobian direction vectors in global space.
    dMatrix matrix0, matrix1, matrix2;
    MSP::Joint::c_calculate_global_matrix2(joint_data, matrix0, matrix1, matrix2);

    dVector location(matrix2.UntransformVector(matrix0.m_posit));
    dMatrix loc_normal_matrix;
    dFloat distance, overpass;
    if (!c_calc_curve_data_at_point3(joint_data, cj_data->m_cur_dist, location, loc_normal_matrix, distance, overpass)) {
        cj_data->m_cur_normal_matrix_set = false;
        return;
    }

    dFloat last_pos = cj_data->m_cur_pos;
    dFloat last_vel = cj_data->m_cur_vel;

    if (cj_data->m_loop) {
        dFloat diff1 = distance - cj_data->m_cur_dist;
        dFloat diff2 = diff1 + (diff1 > 0 ? -cj_data->m_curve_len : cj_data->m_curve_len);
        cj_data->m_cur_pos += (dAbs(diff1) < dAbs(diff2)) ? diff1 : diff2;
    }
    else
        cj_data->m_cur_pos = distance;

    dMatrix normal_matrix(loc_normal_matrix * matrix2);
    cj_data->m_cur_normal_matrix = normal_matrix;
    cj_data->m_cur_normal_matrix_set = true;

    cj_data->m_cur_vel = (cj_data->m_cur_pos - last_pos) * inv_timestep;
    cj_data->m_cur_accel = (cj_data->m_cur_vel - last_vel) * inv_timestep;
    cj_data->m_cur_dist = distance;

    const dVector& p0 = matrix0.m_posit;
    const dVector& p1 = normal_matrix.m_posit;
    dVector p00(p0 + matrix0.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));
    dVector p11(p1 + normal_matrix.m_right.Scale(MSP::Joint::MIN_PIN_LENGTH));

    // Restrict movement on the pivot point along the normal and bi normal of the path.
    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &normal_matrix.m_front[0]);
    NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    NewtonUserJointAddLinearRow(joint, &p0[0], &p1[0], &normal_matrix.m_up[0]);
    NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::LINEAR_STIFF, Joint::LINEAR_DAMP);
    NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

    // Align to curve
    if (cj_data->m_align) {
        NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, normal_matrix.m_right, normal_matrix.m_front), &normal_matrix.m_front[0]);
        NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP2);
        if (cj_data->m_alignment_power < M_EPSILON) {
            NewtonUserJointSetRowMinimumFriction(joint, -Joint::CUSTOM_LARGE_VALUE);
            NewtonUserJointSetRowMaximumFriction(joint, Joint::CUSTOM_LARGE_VALUE);
        }
        else {
            NewtonUserJointSetRowMinimumFriction(joint, -cj_data->m_alignment_power);
            NewtonUserJointSetRowMaximumFriction(joint, cj_data->m_alignment_power);
        }
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

        NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, normal_matrix.m_right, normal_matrix.m_up), &normal_matrix.m_up[0]);
        NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP2);
        if (cj_data->m_alignment_power < M_EPSILON) {
            NewtonUserJointSetRowMinimumFriction(joint, -Joint::CUSTOM_LARGE_VALUE);
            NewtonUserJointSetRowMaximumFriction(joint, Joint::CUSTOM_LARGE_VALUE);
        }
        else {
            NewtonUserJointSetRowMinimumFriction(joint, -cj_data->m_alignment_power);
            NewtonUserJointSetRowMaximumFriction(joint, cj_data->m_alignment_power);
        }
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }

    // Add linear friction or limits
    if (!cj_data->m_loop && overpass < -M_EPSILON) {
        dVector p2(normal_matrix.m_posit + normal_matrix.m_right.Scale(Joint::LINEAR_LIMIT_EPSILON));
        NewtonUserJointAddLinearRow(joint, &p0[0], &p2[0], &normal_matrix.m_right[0]);
        NewtonUserJointSetRowMinimumFriction(joint, 0.0f);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }
    else if (!cj_data->m_loop && overpass > M_EPSILON) {
        dVector p2(normal_matrix.m_posit + normal_matrix.m_right.Scale(-Joint::LINEAR_LIMIT_EPSILON));
        NewtonUserJointAddLinearRow(joint, &p0[0], &p2[0], &normal_matrix.m_right[0]);
        NewtonUserJointSetRowMaximumFriction(joint, 0.0f);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }
    else {
        dVector point(normal_matrix.UntransformVector(matrix0.m_posit));
        point.m_z = 0.0f;
        point = normal_matrix.TransformVector(point);
        NewtonUserJointAddLinearRow(joint, &point[0], &normal_matrix.m_posit[0], &normal_matrix.m_right[0]);
        NewtonUserJointSetRowAcceleration(joint, -cj_data->m_cur_vel * inv_timestep);
        dFloat power = cj_data->m_linear_friction * cj_data->m_controller;
        NewtonUserJointSetRowMinimumFriction(joint, -power);
        NewtonUserJointSetRowMaximumFriction(joint, power);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }

    // Add angular friction or limits
    if (cj_data->m_rotate) {
        dVector omega0(0.0f);
        dVector omega1(0.0f);
        NewtonBodyGetOmega(joint_data->m_child, &omega0[0]);
        if (joint_data->m_parent != nullptr)
            NewtonBodyGetOmega(joint_data->m_parent, &omega1[0]);
        dVector rel_omega(omega0 - omega1);
        dFloat friction = cj_data->m_angular_friction * cj_data->m_controller;
        if (cj_data->m_align) {
            NewtonUserJointAddAngularRow(joint, 0.0f, &normal_matrix.m_right[0]);
            NewtonUserJointSetRowAcceleration(joint, -rel_omega.DotProduct3(normal_matrix.m_right) * inv_timestep);
            NewtonUserJointSetRowMinimumFriction(joint, -friction);
            NewtonUserJointSetRowMaximumFriction(joint, friction);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
        else {
            NewtonUserJointAddAngularRow(joint, 0.0f, &normal_matrix.m_front[0]);
            NewtonUserJointSetRowAcceleration(joint, -rel_omega.DotProduct3(normal_matrix.m_front) * inv_timestep);
            NewtonUserJointSetRowMinimumFriction(joint, -friction);
            NewtonUserJointSetRowMaximumFriction(joint, friction);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

            NewtonUserJointAddAngularRow(joint, 0.0f, &normal_matrix.m_up[0]);
            NewtonUserJointSetRowAcceleration(joint, -rel_omega.DotProduct3(normal_matrix.m_up) * inv_timestep);
            NewtonUserJointSetRowMinimumFriction(joint, -friction);
            NewtonUserJointSetRowMaximumFriction(joint, friction);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

            NewtonUserJointAddAngularRow(joint, 0.0f, &normal_matrix.m_right[0]);
            NewtonUserJointSetRowAcceleration(joint, -rel_omega.DotProduct3(normal_matrix.m_right) * inv_timestep);
            NewtonUserJointSetRowMinimumFriction(joint, -friction);
            NewtonUserJointSetRowMaximumFriction(joint, friction);
            NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
        }
    }
    else if (cj_data->m_align) {
        NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_front, normal_matrix.m_front, normal_matrix.m_right), &normal_matrix.m_right[0]);
        NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP2);
        if (cj_data->m_alignment_power < M_EPSILON) {
            NewtonUserJointSetRowMinimumFriction(joint, -Joint::CUSTOM_LARGE_VALUE);
            NewtonUserJointSetRowMaximumFriction(joint, Joint::CUSTOM_LARGE_VALUE);
        }
        else {
            NewtonUserJointSetRowMinimumFriction(joint, -cj_data->m_alignment_power);
            NewtonUserJointSetRowMaximumFriction(joint, cj_data->m_alignment_power);
        }
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }
    else {
        NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_front), &matrix1.m_front[0]);
        NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP2);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

        NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_up[0]);
        NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP2);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);

        NewtonUserJointAddAngularRow(joint, Joint::c_calculate_angle2(matrix0.m_front, matrix1.m_front, matrix1.m_right), &matrix1.m_right[0]);
        NewtonUserJointSetRowSpringDamperAcceleration(joint, joint_data->m_stiffness, Joint::ANGULAR_STIFF, Joint::ANGULAR_DAMP2);
        NewtonUserJointSetRowStiffness(joint, joint_data->m_stiffness);
    }
}

void MSP::CurvySlider::get_info(const NewtonJoint* const joint, NewtonJointRecord* const info) {
}

void MSP::CurvySlider::on_destroy(MSP::Joint::JointData* joint_data) {
    delete (reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data));
}

void MSP::CurvySlider::on_connect(MSP::Joint::JointData* joint_data) {
    c_update_curve_edges(joint_data);
}

void MSP::CurvySlider::on_disconnect(MSP::Joint::JointData* joint_data) {
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    cj_data->m_cur_pos = 0.0f;
    cj_data->m_cur_vel = 0.0f;
    cj_data->m_cur_accel = 0.0f;
    cj_data->m_cur_dist = 0.0f;
    cj_data->m_cur_normal_matrix_set = false;
    c_clear_curve_edges(joint_data);
}

void MSP::CurvySlider::adjust_pin_matrix_proc(MSP::Joint::JointData* joint_data, dMatrix& pin_matrix) {
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    // Recalculate pin matrix so its on curve rather than on the joint origin.
    dMatrix matrix;
    dVector point, vector;
    NewtonBodyGetMatrix(joint_data->m_child, &matrix[0][0]);
    dVector origin(pin_matrix.UntransformVector(matrix.m_posit));
    if (c_calc_curve_data_at_point2(joint_data, origin, point, vector, cj_data->m_cur_pos, cj_data->m_initial_edge_index)) {
        point = pin_matrix.TransformVector(point);
        vector = pin_matrix.RotateVector(vector);
        Util::matrix_from_pin_dir(point, vector, pin_matrix);
        cj_data->m_cur_dist = cj_data->m_cur_pos;
    }
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Ruby Functions
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

VALUE MSP::CurvySlider::rbf_is_valid(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* address = reinterpret_cast<MSP::Joint::JointData*>(Util::value_to_ull(v_joint));
    return (MSP::Joint::c_is_joint_valid(address) && address->m_jtype == MSP::Joint::CURVY_SLIDER) ? Qtrue : Qfalse;
}

VALUE MSP::CurvySlider::rbf_create(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::NONE);

    joint_data->m_dof = 6;
    joint_data->m_jtype = MSP::Joint::CURVY_SLIDER;
    joint_data->m_cj_data = new CurvySliderData();
    joint_data->m_submit_constraints = submit_constraints;
    joint_data->m_get_info = get_info;
    joint_data->m_on_destroy = on_destroy;
    joint_data->m_on_connect = on_connect;
    joint_data->m_on_disconnect = on_disconnect;
    joint_data->m_adjust_pin_matrix_proc = adjust_pin_matrix_proc;

    return MSP::Joint::c_joint_to_value(joint_data);
}

VALUE MSP::CurvySlider::rbf_add_point(VALUE self, VALUE v_joint, VALUE v_position) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    dVector point(Util::value_to_point(v_position));
    dMatrix pin_matrix;
    Joint::c_get_pin_matrix(joint_data, pin_matrix);
    cj_data->m_points.push_back(pin_matrix.UntransformVector(point));
    if (joint_data->m_connected)
        c_update_curve_edges(joint_data);
    return Util::to_value(cj_data->m_points.size() - 1);
}

VALUE MSP::CurvySlider::rbf_remove_point(VALUE self, VALUE v_joint, VALUE v_point_index) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    unsigned int point_index = Util::value_to_uint(v_point_index);
    if (point_index < cj_data->m_points.size()) {
        cj_data->m_points.erase(cj_data->m_points.begin() + point_index);
        if (joint_data->m_connected)
            c_update_curve_edges(joint_data);
        return Qtrue;
    }
    else
        return Qfalse;
}

VALUE MSP::CurvySlider::rbf_get_points(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    dMatrix pin_matrix;
    MSP::Joint::c_get_pin_matrix(joint_data, pin_matrix);
    VALUE v_points = rb_ary_new2((unsigned int)cj_data->m_points.size());
    unsigned int i = 0;
    for (std::vector<dVector>::iterator it = cj_data->m_points.begin(); it != cj_data->m_points.end(); ++it) {
        dVector point(pin_matrix.TransformVector(*it));
        rb_ary_store(v_points, i, Util::point_to_value(point));
        ++i;
    }
    return v_points;
}

VALUE MSP::CurvySlider::rbf_get_points_size(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_points.size());
}

VALUE MSP::CurvySlider::rbf_clear_points(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    unsigned int count = (unsigned int)cj_data->m_points.size();
    cj_data->m_points.clear();
    c_clear_curve_edges(joint_data);
    return count;
}

VALUE MSP::CurvySlider::rbf_get_point_position(VALUE self, VALUE v_joint, VALUE v_point_index) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    unsigned int point_index = Util::value_to_uint(v_point_index);
    if (point_index < cj_data->m_points.size()) {
        dMatrix pin_matrix;
        MSP::Joint::c_get_pin_matrix(joint_data, pin_matrix);
        dVector point(pin_matrix.TransformVector(cj_data->m_points[point_index]));
        return Util::point_to_value(point);
    }
    else
        return Qnil;
}

VALUE MSP::CurvySlider::rbf_set_point_position(VALUE self, VALUE v_joint, VALUE v_point_index, VALUE v_position) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    unsigned int point_index = Util::value_to_uint(v_point_index);
    if (point_index < cj_data->m_points.size()) {
        dVector point(Util::value_to_point(v_position));
        dMatrix pin_matrix;
        MSP::Joint::c_get_pin_matrix(joint_data, pin_matrix);
        cj_data->m_points[point_index] = pin_matrix.UntransformVector(point);
        if (joint_data->m_connected)
            c_update_curve_edges(joint_data);
        return Qtrue;
    }
    else
        return Qfalse;
}

VALUE MSP::CurvySlider::rbf_get_length(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_curve_len * M_INCH_TO_METER);
}

VALUE MSP::CurvySlider::rbf_get_cur_position(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_pos * M_INCH_TO_METER);
}

VALUE MSP::CurvySlider::rbf_get_cur_velocity(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_vel * M_INCH_TO_METER);
}

VALUE MSP::CurvySlider::rbf_get_cur_acceleration(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_cur_accel * M_INCH_TO_METER);
}

VALUE MSP::CurvySlider::rbf_get_cur_point(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    if (cj_data->m_cur_normal_matrix_set)
        return Util::point_to_value(cj_data->m_cur_normal_matrix.m_posit);
    else
        return Qnil;
}

VALUE MSP::CurvySlider::rbf_get_cur_vector(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    if (cj_data->m_cur_normal_matrix_set)
        return Util::vector_to_value(cj_data->m_cur_normal_matrix.m_right);
    else
        return Qnil;
}

VALUE MSP::CurvySlider::rbf_get_cur_normal_matrix(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    if (cj_data->m_cur_normal_matrix_set)
        return Util::matrix_to_value(cj_data->m_cur_normal_matrix);
    else
        return Qnil;
}

VALUE MSP::CurvySlider::rbf_get_linear_friction(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_linear_friction * M_INCH_TO_METER);
}

VALUE MSP::CurvySlider::rbf_set_linear_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    cj_data->m_linear_friction = Util::max_float(Util::value_to_dFloat(v_friction) * M_METER_TO_INCH, 0.0f);
    return Qnil;
}

VALUE MSP::CurvySlider::rbf_get_angular_friction(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_angular_friction * M_INCH2_TO_METER2);
}

VALUE MSP::CurvySlider::rbf_set_angular_friction(VALUE self, VALUE v_joint, VALUE v_friction) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    cj_data->m_angular_friction = Util::max_float(Util::value_to_dFloat(v_friction) * M_METER2_TO_INCH2, 0.0f);
    return Qnil;
}

VALUE MSP::CurvySlider::rbf_get_controller(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_controller);
}

VALUE MSP::CurvySlider::rbf_set_controller(VALUE self, VALUE v_joint, VALUE v_controller) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    cj_data->m_controller = Util::max_float(Util::value_to_dFloat(v_controller), 0.0f);
    return Qnil;
}

VALUE MSP::CurvySlider::rbf_loop_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_loop);
}

VALUE MSP::CurvySlider::rbf_enable_loop(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    bool last_loop = cj_data->m_loop;
    cj_data->m_loop = Util::value_to_bool(v_state);
    if (cj_data->m_loop != last_loop && joint_data->m_connected) {
        c_update_curve_edges(joint_data);
        if (!cj_data->m_loop) {
            cj_data->m_cur_pos = fmod(cj_data->m_cur_pos, cj_data->m_curve_len);
            if (cj_data->m_cur_pos < 0.0f)
                cj_data->m_cur_pos += cj_data->m_curve_len;
        }
    }
    return Qnil;
}

VALUE MSP::CurvySlider::rbf_alignment_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_align);
}

VALUE MSP::CurvySlider::rbf_enable_alignment(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    cj_data->m_align = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::CurvySlider::rbf_get_alignment_power(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_alignment_power * M_INCH2_TO_METER2);
}

VALUE MSP::CurvySlider::rbf_set_alignment_power(VALUE self, VALUE v_joint, VALUE v_power) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    cj_data->m_alignment_power = Util::max_float(Util::value_to_dFloat(v_power) * M_METER2_TO_INCH2, 0.0f);
    return Qnil;
}

VALUE MSP::CurvySlider::rbf_rotation_enabled(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    return Util::to_value(cj_data->m_rotate);
}

VALUE MSP::CurvySlider::rbf_enable_rotation(VALUE self, VALUE v_joint, VALUE v_state) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    cj_data->m_rotate = Util::value_to_bool(v_state);
    return Qnil;
}

VALUE MSP::CurvySlider::rbf_get_normal_martix_at_position(VALUE self, VALUE v_joint, VALUE v_position) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    dFloat position = Util::value_to_dFloat(v_position) * M_METER_TO_INCH;
    dMatrix normal_matrix;
    dFloat distance, overpass;
    if (c_calc_curve_data_at_position(joint_data, position, normal_matrix, distance, overpass)) {
        dMatrix pin_matrix;
        Joint::c_get_pin_matrix(joint_data, pin_matrix);
        return Util::matrix_to_value(normal_matrix * pin_matrix);
    }
    else
        return Qnil;
}

VALUE MSP::CurvySlider::rbf_get_normal_martix_at_point(VALUE self, VALUE v_joint, VALUE v_point) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    dVector point(Util::value_to_point(v_point));
    dMatrix pin_matrix;
    Joint::c_get_pin_matrix(joint_data, pin_matrix);
    dVector loc_point(pin_matrix.UntransformVector(point));
    dMatrix normal_matrix;
    dFloat distance, overpass;
    if (c_calc_curve_data_at_point(joint_data, loc_point, normal_matrix, distance, overpass))
        return rb_ary_new3(2, Util::matrix_to_value(normal_matrix * pin_matrix), Util::to_value(overpass * M_INCH_TO_METER));
    else
        return Qnil;
}

VALUE MSP::CurvySlider::rbf_get_normal_matrices(VALUE self, VALUE v_joint) {
    MSP::Joint::JointData* joint_data = MSP::Joint::c_value_to_joint2(v_joint, MSP::Joint::CURVY_SLIDER);
    CurvySliderData* cj_data = reinterpret_cast<CurvySliderData*>(joint_data->m_cj_data);
    VALUE v_normal_matrices = rb_ary_new2((unsigned int)cj_data->m_edges.size());
    dMatrix pin_matrix;
    Joint::c_get_pin_matrix(joint_data, pin_matrix);
    for (std::map<unsigned int, EdgeData*>::iterator it = cj_data->m_edges.begin(); it != cj_data->m_edges.end(); ++it)
        rb_ary_store(v_normal_matrices, it->first, Util::matrix_to_value(it->second->m_normal_matrix * pin_matrix));
    return v_normal_matrices;
}


/*
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Main
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

void MSP::CurvySlider::init_ruby(VALUE mNewton) {
    VALUE mCurvySlider = rb_define_module_under(mNewton, "CurvySlider");

    rb_define_module_function(mCurvySlider, "is_valid?", VALUEFUNC(MSP::CurvySlider::rbf_is_valid), 1);
    rb_define_module_function(mCurvySlider, "create", VALUEFUNC(MSP::CurvySlider::rbf_create), 1);
    rb_define_module_function(mCurvySlider, "add_point", VALUEFUNC(MSP::CurvySlider::rbf_add_point), 2);
    rb_define_module_function(mCurvySlider, "remove_point", VALUEFUNC(MSP::CurvySlider::rbf_remove_point), 2);
    rb_define_module_function(mCurvySlider, "get_points", VALUEFUNC(MSP::CurvySlider::rbf_get_points), 1);
    rb_define_module_function(mCurvySlider, "get_points_size", VALUEFUNC(MSP::CurvySlider::rbf_get_points), 1);
    rb_define_module_function(mCurvySlider, "clear_points", VALUEFUNC(MSP::CurvySlider::rbf_clear_points), 2);
    rb_define_module_function(mCurvySlider, "get_point_position", VALUEFUNC(MSP::CurvySlider::rbf_get_point_position), 2);
    rb_define_module_function(mCurvySlider, "set_point_position", VALUEFUNC(MSP::CurvySlider::rbf_set_point_position), 3);
    rb_define_module_function(mCurvySlider, "get_length", VALUEFUNC(MSP::CurvySlider::rbf_get_length), 1);
    rb_define_module_function(mCurvySlider, "get_cur_position", VALUEFUNC(MSP::CurvySlider::rbf_get_cur_position), 1);
    rb_define_module_function(mCurvySlider, "get_cur_velocity", VALUEFUNC(MSP::CurvySlider::rbf_get_cur_velocity), 1);
    rb_define_module_function(mCurvySlider, "get_cur_acceleration", VALUEFUNC(MSP::CurvySlider::rbf_get_cur_acceleration), 1);
    rb_define_module_function(mCurvySlider, "get_cur_point", VALUEFUNC(MSP::CurvySlider::rbf_get_cur_point), 1);
    rb_define_module_function(mCurvySlider, "get_cur_vector", VALUEFUNC(MSP::CurvySlider::rbf_get_cur_vector), 1);
    rb_define_module_function(mCurvySlider, "get_cur_normal_matrix", VALUEFUNC(MSP::CurvySlider::rbf_get_cur_normal_matrix), 1);
    rb_define_module_function(mCurvySlider, "get_linear_friction", VALUEFUNC(MSP::CurvySlider::rbf_get_linear_friction), 1);
    rb_define_module_function(mCurvySlider, "set_linear_friction", VALUEFUNC(MSP::CurvySlider::rbf_set_linear_friction), 2);
    rb_define_module_function(mCurvySlider, "get_angular_friction", VALUEFUNC(MSP::CurvySlider::rbf_get_angular_friction), 1);
    rb_define_module_function(mCurvySlider, "set_angular_friction", VALUEFUNC(MSP::CurvySlider::rbf_set_angular_friction), 2);
    rb_define_module_function(mCurvySlider, "get_controller", VALUEFUNC(MSP::CurvySlider::rbf_get_controller), 1);
    rb_define_module_function(mCurvySlider, "set_controller", VALUEFUNC(MSP::CurvySlider::rbf_set_controller), 2);
    rb_define_module_function(mCurvySlider, "loop_enabled?", VALUEFUNC(MSP::CurvySlider::rbf_loop_enabled), 1);
    rb_define_module_function(mCurvySlider, "enable_loop", VALUEFUNC(MSP::CurvySlider::rbf_enable_loop), 2);
    rb_define_module_function(mCurvySlider, "alignment_enabled?", VALUEFUNC(MSP::CurvySlider::rbf_alignment_enabled), 1);
    rb_define_module_function(mCurvySlider, "enable_alignment", VALUEFUNC(MSP::CurvySlider::rbf_enable_alignment), 2);
    rb_define_module_function(mCurvySlider, "get_alignment_power", VALUEFUNC(MSP::CurvySlider::rbf_get_alignment_power), 1);
    rb_define_module_function(mCurvySlider, "set_alignment_power", VALUEFUNC(MSP::CurvySlider::rbf_set_alignment_power), 2);
    rb_define_module_function(mCurvySlider, "rotation_enabled?", VALUEFUNC(MSP::CurvySlider::rbf_rotation_enabled), 1);
    rb_define_module_function(mCurvySlider, "enable_rotation", VALUEFUNC(MSP::CurvySlider::rbf_enable_rotation), 2);
    rb_define_module_function(mCurvySlider, "get_normal_martix_at_position", VALUEFUNC(MSP::CurvySlider::rbf_get_normal_martix_at_position), 2);
    rb_define_module_function(mCurvySlider, "get_normal_martix_at_point", VALUEFUNC(MSP::CurvySlider::rbf_get_normal_martix_at_point), 2);
    rb_define_module_function(mCurvySlider, "get_normal_matrices", VALUEFUNC(MSP::CurvySlider::rbf_get_normal_matrices), 1);
}
