module MSPhysics

  # @since 1.0.0
  module Geometry

    EPSILON = 1.0e-6

    class << self

      # Get an array of unique points from an array of Point3d objects.
      # @param [Array<Geom::Point3d>] points
      # @return [Array<Geom::Point3d>]
      def get_unique_points(points)
        unique_points = []
        points.each { |point|
          found = false
          unique_points.each { |unique_point|
            if unique_point == point
              found = true
              break
            end
          }
          unique_points << point unless found
        }
        unique_points
      end

      # Determine whether an array of points lie on the same line.
      # @param [Array<Geom::Point3d>] points
      # @return [Boolean]
      def points_collinear?(points)
        return true if points.size < 3
        pt1 = points[0]
        pt2 = nil
        points.each { |pt|
          next if (pt1 == pt)
          pt2 = pt
          break
        }
        return false unless pt2
        v1 = pt2 - pt1
        points.each { |pt|
          next if (pt1 == pt || pt2 == pt)
          pt3 = pt
          v2 = pt3 - pt1
          return false unless v2.parallel?(v1)
        }
        true
      end

      # Get three non-collinear points from an array of three or more points.
      # @param [Array<Geom::Point3d>] points
      # @return [Array<Geom::Point3d>, nil] An array of three non-collinear
      #   points if successful.
      def get_noncollinear_points(points)
        pt1 = points[0]
        pt2 = nil
        points.each { |pt|
          next if (pt1 == pt)
          pt2 = pt
          break
        }
        return unless pt2
        v1 = pt2 - pt1
        points.each { |pt|
          next if (pt1 == pt || pt2 == pt)
          pt3 = pt
          v2 = pt3 - pt1
          return [pt1, pt2, pt3] unless v2.parallel?(v1)
        }
        nil
      end

      # Get plane normal.
      # @param [Array<Geom::Point3d>] plane An array of
      #   three, non-collinear points on the plane.
      # @return [Geom::Vector3d]
      def get_plane_normal(plane)
        u = plane[1] - plane[0]
        v = plane[2] - plane[0]
        (u*v)
      end

      # Determine whether an array of points lie on the same plane.
      # @param [Array<Geom::Point3d>] points
      # @return [Boolean]
      def points_coplanar?(points)
        plane = get_noncollinear_points(points)
        return true if plane.nil?
        v1 = get_plane_normal(plane)
        points.each { |pt|
          pl = [plane[0], plane[1], pt]
          next if points_collinear?(pl)
          v2 = get_plane_normal(pl)
          return false unless v2.parallel?(v1)
        }
        true
      end

      # Sort an array of points in a counter clockwise direction.
      # @param [Array<Geom::Point3d>] points
      # @return [Array<Geom::Point3d>, nil] An array of sorted points if
      #   successful.
      def sort_polygon_points(points)
        plane = get_noncollinear_points(points)
        return unless plane
        normal = get_plane_normal(plane)
        center = calc_center(points)
        itra = Geom::Transformation.new(center, normal).inverse
        points2 = []
        points.each { |pt| points2 << pt.transform(itra) }
        points2 = {}
        data = {}
        points2.each { |pt|
          theta = Math.atan2(pt.y, pt.x)
          theta += Math::PI*2 if theta < 0
          data[theta] = pt
        }
        sorted_points = []
        data.sort.each { |theta, pt| sorted_points << pt }
        sorted_points
      end

      # Get center from an array of points.
      # @param [Array<Geom::Point3d>] points
      # @return [Geom::Point3d]
      def calc_center(points)
        c = Geom::Point3d.new(0, 0, 0)
        points.each { |pt|
          c.x += pt.x
          c.y += pt.y
          c.z += pt.z
        }
        for i in 0..2; c[i] /= points.size.to_f end
        c
      end

      # Calculate edge centre of mass.
      # @param [Sketchup::Edge] edge
      # @return [Geom::Point3d]
      def calc_edge_centre(edge)
        AMS.validate_type(edge, Sketchup::Edge)
        a = edge.start.position
        b = edge.end.position
        x = (a.x + b.x) / 2.0
        y = (a.y + b.y) / 2.0
        z = (a.z + b.z) / 2.0
        Geom::Point3d.new(x,y,z)
      end

      # Calculate face centre of mass.
      # @param [Sketchup::Face] face
      # @return [Geom::Point3d]
      def calc_face_centre(face)
        AMS.validate_type(face, Sketchup::Face)
        tx = 0
        ty = 0
        tz = 0
        total_area = 0
        face.mesh.polygons.each_index { |i|
          triplet = face.mesh.polygon_points_at(i+1)
          # Use Heron's formula to calculate the area of the triangle.
          a = triplet[0].distance(triplet[1])
          b = triplet[0].distance(triplet[2])
          c = triplet[1].distance(triplet[2])
          s = (a + b + c)*0.5
          area = Math.sqrt(s * (s-a) * (s-b) * (s-c))
          total_area += area
          # Identify triangle centroid.
          cx = (triplet[0].x + triplet[1].x + triplet[2].x) / 3.0
          cy = (triplet[0].y + triplet[1].y + triplet[2].y) / 3.0
          cz = (triplet[0].z + triplet[1].z + triplet[2].z) / 3.0
          # Add point to centre.
          tx += cx * area
          ty += cy * area
          tz += cz * area
        }
        # Compute centre.
        Geom::Point3d.new(tx.to_f/total_area, ty.to_f/total_area, tz.to_f/total_area)
      end

      # Determine whether particular point in on edge.
      # @param [Geom::Point3d] point
      # @param [Sketchup::Edge] edge
      # @return [Boolean]
      def is_point_on_edge?(point, edge)
        AMS.validate_type(edge, Sketchup::Edge)
        a = edge.start.position
        b = edge.end.position
        return true if point == a || point == b
        v1 = a.vector_to(b)
        v2 = a.vector_to(point)
        return false unless v1.samedirection?(v2)
        v1.length >= v2.length
      end

      # Determine whether particular point is on face.
      # @param [Geom::Point3d] point
      # @param [Sketchup::Face] face
      # @return [Boolean]
      def is_point_on_face?(point, face)
        AMS.validate_type(face, Sketchup::Face)
        # 1. Divide face into triangles using polygon mesh.
        # 2. Check if point is within one of the triangles.
        face.mesh.polygons.each_index { |i|
          triplet = face.mesh.polygon_points_at(i+1)
          return true if is_point_on_triangle?(point, *triplet)
        }
        false
      end

      # Determine whether particular point is within the triangle.
      # @param [Geom::Point3d] pt The point to test.
      # @param [Geom::Point3d] pt1 One of the triangle vertices.
      # @param [Geom::Point3d] pt2 One of the triangle vertices.
      # @param [Geom::Point3d] pt3 One of the triangle vertices.
      # @return [Boolean]
      def is_point_on_triangle?(pt, pt1, pt2, pt3)
        v1 = pt.vector_to(pt1)
        return true if v1.length.zero?
        v2 = pt.vector_to(pt2)
        return true if v2.length.zero?
        v3 = pt.vector_to(pt3)
        return true if v3.length.zero?
        angle = v1.angle_between(v2) + v2.angle_between(v3) + v3.angle_between(v1)
        Math::PI * 2 - angle < EPSILON
      end

      # Get distance between two points.
      # @param [Array<Geom::Point3d>] a
      # @param [Array<Geom::Point3d>] b
      # @return [Numeric]
      def distance(a, b)
        a.distance(b)
      end

      # Get scale ratios of a transformation matrix.
      # @param [Geom::Transformation, Array<Numeric>] tra
      # @return [Geom::Vector3d]
      def get_matrix_scale(tra)
        tra = Geom::Transformation.new(tra) unless tra.is_a?(Geom::Transformation)
        Geom::Vector3d.new(
          X_AXIS.transform(tra).length,
          Y_AXIS.transform(tra).length,
          Z_AXIS.transform(tra).length)
      end

      # Set the scale ratios of a transformation matrix.
      # @param [Geom::Transformation, Array<Numeric>] tra
      # @param [Geom::Vector3d, Array<Numeric>] scale An array of three numeric
      #   values containing the scale ratios of the X-axis, Y-axis, and Z-axis.
      # @return [Geom::Transformation]
      def set_matrix_scale(tra, scale)
        s = Geom::Transformation.scaling(scale.x, scale.y, scale.z)
        extract_matrix_scale(tra)*s
      end

      # Normalize scale of a transformation matrix.
      # @param [Geom::Transformation, Array<Numeric>] tra
      # @return [Geom::Transformation]
      def extract_matrix_scale(tra)
        tra = Geom::Transformation.new(tra) unless tra.is_a?(Geom::Transformation)
        Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, tra.origin)
      end

      # Determine whether transformation matrix is flipped.
      # @param [Geom::Transformation, Array<Numeric>] tra
      # @return [Boolean]
      def is_matrix_flipped?(tra)
        tra = Geom::Transformation.new(tra) unless tra.is_a?(Geom::Transformation)
        (tra.xaxis * tra.yaxis) % tra.zaxis < 0
      end

      # Determine whether transformation matrix is uniform. A uniform
      # transformation matrix has all axis perpendicular to each other.
      # @param [Geom::Transformation, Array<Numeric>] tra
      # @return [Boolean]
      def is_matrix_uniform?(tra)
        tra = Geom::Transformation.new(tra) unless tra.is_a?(Geom::Transformation)
        tra.xaxis.perpendicular?(tra.yaxis) && tra.xaxis.perpendicular?(tra.zaxis) && tra.yaxis.perpendicular?(tra.zaxis)
      end

      # Get points on a 2D circle.
      # @param [Array<Numeric>] origin
      # @param [Numeric] radius
      # @param [Fixnum] num_seg Number of segments.
      # @param [Numeric] rot_angle Rotate angle in degrees.
      # @return [Array<Array<Numeric>>] An array of points on circle.
      def get_points_on_circle2d(origin, radius, num_seg = 16, rot_angle = 0)
        ra = rot_angle.degrees
        offset = Math::PI*2/num_seg.to_i
        pts = []
        for n in 0...num_seg.to_i
          angle = ra + (n*offset)
          pts << [Math.cos(angle)*radius + origin.x, Math.sin(angle)*radius + origin.y]
        end
        pts
      end

      # Get points on a 3D circle.
      # @param [Array<Numeric>, Geom::Point3d] origin
      # @param [Array<Numeric>, Geom::Vector3d] normal
      # @param [Numeric] radius
      # @param [Fixnum] num_seg Number of segments.
      # @param [Numeric] rot_angle Rotate angle in degrees.
      # @return [Array<Geom::Point3d>] An array of points on circle.
      def get_points_on_circle3d(origin, radius, normal = [0,0,1], num_seg = 16, rot_angle = 0)
        # Get the x and y axes
        origin = Geom::Point3d.new(origin) unless origin.is_a?(Geom::Point3d)
        normal = Geom::Vector3d.new(normal) unless normal.is_a?(Geom::Vector3d)
        xaxis = normal.axes[0]
        yaxis = normal.axes[1]
        xaxis.length = radius
        yaxis.length = radius
        # Compute points
        ra = rot_angle.degrees
        offset = Math::PI*2/num_seg.to_i
        pts = []
        for n in 0...num_seg.to_i
          angle = ra + (n*offset)
          vec = Geom.linear_combination(Math.cos(angle), xaxis, Math.sin(angle), yaxis)
          pts << origin + vec
        end
        pts
      end

      # Blend colors.
      # @param [Numeric] ratio between 0.0 and 1.0.
      # @param [Array<Sketchup::Color, String, Array>] colors An array of colors to blend.
      # @return [Sketchup::Color]
      def blend_colors(ratio, colors = ['white', 'black'])
        if colors.empty?
          raise(TypeError, 'Expected at least one color, but got none.', caller)
        end
        return Sketchup::Color.new(colors[0]) if colors.size == 1
        ratio = MSPhysics.clamp(ratio, 0, 1)
        cr = (colors.length-1)*ratio
        dec = cr-cr.to_i
        if dec == 0
          Sketchup::Color.new(colors[cr])
        else
          a = colors[cr.to_i].to_a
          b = colors[cr.ceil].to_a
          a[3] = 255 unless a[3]
          b[3] = 255 unless b[3]
          Sketchup::Color.new(((b[0]-a[0])*dec+a[0]).to_i, ((b[1]-a[1])*dec+a[1]).to_i, ((b[2]-a[2])*dec+a[2]).to_i, ((b[3]-a[3])*dec+a[3]).to_i)
        end
      end

      # Determine if ray intersects triangle.
      # @param [Geom::Point3d] origin Ray origin.
      # @param [Geom::Vector3d] dir Ray direction.
      # @param [Geom::Point3d] pt0 One of the triangle vertices.
      # @param [Geom::Point3d] pt1 One of the triangle vertices.
      # @param [Geom::Point3d] pt2 One of the triangle vertices.
      # @return [Geom::Point3d, nil]
      # @see Source http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
      def intersect_ray_triangle(origin, dir, pt0, pt1, pt2)
        dir = dir.normalize
        # Find vectors for two edges sharing pt0.
        edge1 = pt1 - pt0
        edge2 = pt2 - pt0
        # Begin calculate determinant
        pvec = dir.cross(edge2)
        # If determinant is near zero, ray lies in plane of triangle.
        det = edge1.dot(pvec)
=begin
        # Determine if ray intersects a triangle.
        return if (det < EPSILON)
        # Calculate distance from vert to ray origin
        tvec = origin - pt0
        # Calculate U parameter and test bounds
        u = tvec.dot(pvec)
        return if (u < 0.0 || u > det)
        # Prepare to test V parameter
        qvec = tvec.cross(edge1)
        # Calculate V parameter and test bounds
        v = dir.dot(qvec)
        return if (v < 0.0 || u + v > det)
        # Calculate t, scale parameters, ray intersects triangle
        t = edge2.dot(qvec)
        inv_det = 1.0 / det
        t *= inv_det
        u *= inv_det
        v *= inv_det
=end
        # Determine if ray intersects a two sided triangle.
        return if (det > -EPSILON && det < EPSILON)
        inv_det = 1.0 / det
        # Calculate dostance from pt0 to ray origin
        tvec = origin - pt0
        # Calculate U parameter and test bounds
        u = tvec.dot(pvec) * inv_det
        return if (u < 0.0 || u > 1.0)
        # Prepare to test V parameter
        qvec = tvec.cross(edge1)
        # Calculate V parameter and test bounds
        v = dir.dot(qvec) * inv_det
        return if (v < 0.0 || u + v > 1.0)
        # Calculate T parameter
        t = edge2.dot(qvec) * inv_det

        # If t is greater than zero, ray intersects triangle
        return if t <= 0
        dir.length = t
        return origin + dir
      end

      # Rotate t1 so that its Z-axis aligns with Z-axis of t2.
      # @param [Geom::Transformation] t1
      # @param [Geom::Transformation] t2
      # @return [Geom::Transformation] Rotated matrix.
      def rotate_matrix1_to_matrix2(t1, t2)
        ta = Geom::Transformation.new(ORIGIN, t1.zaxis)
        tb = Geom::Transformation.new(ORIGIN, t2.zaxis)
        tb * (ta.inverse * t1)
      end

    end # class << self
  end # module Geometry
end # module MSPhysics
