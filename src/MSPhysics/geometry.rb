module MSPhysics
  module Geometry

    module_function

    # Convert an array of Point3d objects and array of arrays to just Point3d
    # objects.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] pts
    # @return [Array<Geom::Point3d>]
    # @since 1.0.0
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
    # @since 1.0.0
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
    # @since 1.0.0
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
    # @since 1.0.0
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
    # @since 1.0.0
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
    # @since 1.0.0
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
    # @since 1.0.0
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
    # @since 1.0.0
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

    # Get distance between two points.
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] a
    # @param [Array<Geom::Point3d>, Array<Array<Numeric>>] b
    # @return [Numeric]
    # @since 1.0.0
    def distance(a, b)
      ((b[0]-a[0])**2 + (b[1]-a[1])**2 + (b[2]-a[2])**2)**0.5
    end

    # Get the scale ratios of a transformation matrix.
    # @param [Array<Numeric>, Geom::Transformation] tra
    # @return [Array<Numeric>] An array of three numeric values containing the
    #   scale of the x_axis, y_axis, and z_axis.
    def get_scale(tra)
      m = Geom::Transformation.new(tra.to_a).to_a
      scale = []
      sign = m[2] < 0 ? -1 : 1
      scale[0] = Geom::Vector3d.new(m[0,3]).length * sign
      sign = m[6] < 0 ? -1 : 1
      scale[1] = Geom::Vector3d.new(m[4,3]).length * sign
      sign = m[10] < 0 ? -1 : 1
      scale[2] = Geom::Vector3d.new(m[8,3]).length * sign
      scale
    end

    # Set the scale ratios of a transformation matrix.
    # @param [Array<Numeric>, Geom::Transformation] tra
    # @param [Array<Numeric>] scale An array of three numeric values containing
    #   the scale ratios of the x_axis, y_axis, and z_axis.
    # @return [Geom::Transformation]
    def set_scale(tra, scale)
      tra = Geom::Transformation.new(tra)
      matrix = [1,0,0, 0, 0,0,1, 0, 0,0,1, 0, 0,0,0, 1]
      xaxis = tra.xaxis
      xaxis.length = scale[0]
      yaxis = tra.yaxis
      yaxis.length = scale[1]
      zaxis = tra.zaxis
      zaxis.length = scale[2]
      origin = tra.origin
      matrix[0,3] = xaxis.to_a
      matrix[4,3] = yaxis.to_a
      matrix[8,3] = zaxis.to_a
      matrix[12,3] = origin.to_a
      Geom::Transformation.new(matrix)
    end

    # Normalize the scale of a transformation matrix.
    # @param [Array<Numeric>, Geom::Transformation] tra
    # @return [Geom::Transformation]
    def extract_scale(tra)
      tra = Geom::Transformation.new(tra.to_a)
      xaxis = tra.xaxis.normalize
      yaxis = tra.yaxis.normalize
      zaxis = tra.zaxis.normalize
      origin = tra.origin
      Geom::Transformation.new(xaxis, yaxis, zaxis, origin)
    end

  end # module Geometry
end # module MSPhysics
