module MSPhysics
  module Geometry

    module_function

    # Convert an array of Point3d objects and array of arrays to just Point3d
    # objects.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] pts
    # @return [Array<Geom::Point3d>]
    def convert_to_points(pts)
      pts = pts.dup
      for i in 0...pts.size
        next if pts[i].is_a?(Geom::Point3d)
        pts[i] = Geom::Point3d.new(pts[i])
      end
      pts
    end

    # Get an array of unique points from an array of Poin3d objects.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] pts
    # @return [Array<Geom::Point3d>]
    def get_unique_points(pts)
      for i in 0...pts.size
        next if pts[i].nil?
        for j in (i+1)...pts.size
          pts[j] = nil if (pts[j].to_a == pts[i].to_a)
        end
      end
      pts.compact
    end

    # Determine whether an array of points lie on the same line.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] pts
    # @return [Boolean]
    def points_collinear?(pts)
      return true if pts.size < 3
      pt1 = Geom::Point3d.new(pts[0])
      pt2 = nil
      pts.each { |pt|
        next if (pt1 == pt)
        pt2 = Geom::Point3d.new(pt)
        break
      }
      return nil if pt2.nil?
      v1 = pt2 - pt1
      pts.each { |pt|
        next if (pt1 == pt or pt2 == pt)
        pt3 = Geom::Point3d.new(pt)
        v2 = pt3 - pt1
        return false unless v2.parallel?(v1)
      }
      true
    end

    # Get three non-collinear points from an array of three or more points.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] pts
    # @return [Array<Geom::Point3d>, NilClass]
    def get_noncollinear_points(pts)
      return nil if pts.size < 3
      pt1 = Geom::Point3d.new(pts[0])
      pt2 = nil
      pts.each { |pt|
        next if (pt1 == pt)
        pt2 = Geom::Point3d.new(pt)
        break
      }
      return if pt2.nil?
      v1 = pt2 - pt1
      pts.each { |pt|
        next if (pt1 == pt or pt2 == pt)
        pt3 = Geom::Point3d.new(pt)
        v2 = pt3 - pt1
        return [pt1, pt2, pt3] unless v2.parallel?(v1)
      }
      nil
    end

    # Get plane normal.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] plane An array of
    #   three, non-collinear points on the plane.
    # @return [Geom::Vector3d]
    def get_plane_normal(plane)
      for i in 0..2
        next if plane[i].is_a?(Geom::Point3d)
        plane[i] = Geom::Point3d.new(plane[i])
      end
      u = plane[1] - plane[0]
      v = plane[2] - plane[0]
      (u*v)
    end

    # Determine whether an array of points lie on the same plane.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] pts
    # @return [Boolean]
    def points_coplanar?(pts)
      plane = get_noncollinear_points(pts)
      return true if plane.nil?
      v1 = get_plane_normal(plane)
      pts.each { |pt|
        pl = [plane[0], plane[1], pt]
        next if points_collinear?(pl)
        v2 = get_plane_normal(pl)
        return false unless v2.parallel?(v1)
      }
      true
    end

    # Sort an array of points in the counter clockwise direction.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] pts
    # @return [Array<Geom::Point3d>, NilClass] pts
    def sort_polygon_points(pts)
      plane = get_noncollinear_points(pts)
      return nil if plane.nil?
      normal = get_plane_normal(plane)
      center = calc_center(pts)
      tra = Geom::Transformation.new(center, normal)
      pts2 = []
      pts.each { |pt| pts2.push pt.transform(tra.inverse) }
      data = {}
      pts2.each { |pt|
        theta = Math.atan2(pt.y, pt.x)
        theta += Math::PI*2 if (theta < 0)
        data[theta] = pt
      }
      pts = []
      data.keys.sort.each { |theta|
        pts.push data[theta]
      }
      pts
    end

    # Get center from an array of points.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] pts
    # @return [Geom::Point3d]
    def calc_center(pts)
      c = Geom::Point3d.new(0,0,0)
      pts.each{ |pt|
        c.x += pt[0]
        c.y += pt[1]
        c.z += pt[2]
      }
      for i in 0..2; c[i] /= pts.size.to_f end
      c
    end

    # Calculate edge centre.
    # @param [Sketchup::Edge] edge
    # @return [Geom::Point3d]
    def calc_edge_centre(edge)
      MSPhysics.validate_type(edge, Sketchup::Edge)
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
      MSPhysics.validate_type(face, Sketchup::Face)
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
      point = Geom::Point3d.new(point.to_a)
      MSPhysics.validate_type(edge, Sketchup::Edge)
      a = edge.start.position
      b = edge.end.position
      return true if point == a or point == b
      v1 = a.vector_to(b)
      v2 = a.vector_to(point)
      return false unless v1.samedirection?(v2)
      v1.length >= v2.length
    end

    # Determine whether particular point is on face.
    # @return [Boolean]
    def is_point_on_face?(point, face)
      point = Geom::Point3d.new(point.to_a)
      MSPhysics.validate_type(face, Sketchup::Face)
      # 1. Divide face into triangles using polygon mesh.
      # 2. Check if point is within one of the triangles.
      face.mesh.polygons.each_index { |i|
        triplet = face.mesh.polygon_points_at(i+1)
        return true if is_point_in_triangle?(point, *triplet)
      }
      false
    end

    # Get cross product of three points.
    # @param [Geom::Point3d] p1
    # @param [Geom::Point3d] p2
    # @param [Geom::Point3d] p3
    # @return [Numeric]
    def sign(p1, p2, p3)
      p1 = Geom::Point3d.new(p1.to_a)
      p2 = Geom::Point3d.new(p2.to_a)
      p3 = Geom::Point3d.new(p3.to_a)
      return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y)
    end

    # Determine whether particular point is within the triangle.
    # @param [Geom::Point3d] pt The point to test.
    # @param [Geom::Point3d] v1 One of the triangle points.
    # @param [Geom::Point3d] v2 One of the triangle points.
    # @param [Geom::Point3d] v3 One of the triangle points.
    # @return [Boolean]
    def is_point_in_triangle?(pt, v1, v2, v3)
      b1 = sign(pt, v1, v2) < 0
      b2 = sign(pt, v2, v3) < 0
      b3 = sign(pt, v3, v1) < 0
      (b1 == b2 && b2 == b3)
    end

    # Get distance between two points.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] a
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] b
    # @return [Numeric]
    def distance(a, b)
      ((b[0]-a[0])**2 + (b[1]-a[1])**2 + (b[2]-a[2])**2)**0.5
    end

    # Get the scale ratios of a transformation matrix.
    # @param [Array<Numeric>, Geom::Transformation] tra
    # @return [Array<Numeric>] An array of three numeric values containing the
    #   scale of the x_axis, y_axis, and z_axis.
    def get_scale(tra)
      mat = Geom::Transformation.new(tra.to_a).to_a
      sx = Geom::Vector3d.new(mat[0,3]).length
      sy = Geom::Vector3d.new(mat[4,3]).length
      sz = Geom::Vector3d.new(mat[8,3]).length
      [sx,sy,sz]
    end

    # Set the scale ratios of a transformation matrix.
    # @param [Array<Numeric>, Geom::Transformation] tra
    # @param [Array<Numeric>] scale An array of three numeric values containing
    #   the scale ratios of the x_axis, y_axis, and z_axis.
    # @return [Geom::Transformation]
    def set_scale(tra, scale)
      s = Geom::Transformation.scaling(scale[0], scale[1], scale[2])
      extract_scale(tra)*s
    end

    # Normalize scale of a transformation matrix.
    # @param [Array<Numeric>, Geom::Transformation] tra
    # @return [Geom::Transformation]
    def extract_scale(tra)
      tra = Geom::Transformation.new(tra.to_a)
      Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, tra.origin)
    end

  end # module Geometry
end # module MSPhysics
