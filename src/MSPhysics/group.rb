module MSPhysics
  module Group

    # @!visibility private
    VALID_TYPES = [Sketchup::Group, Sketchup::ComponentInstance].freeze
    # @!visibility private
    INVALID_TYPE = 'The specified entity is not a group or a component!'.freeze

    module_function

    # Get entities of a group/component/definition.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, Sketchup::ComponentDefinition] ent
    # @return [Sketchup::Entities, NilClass]
    def get_entities(ent)
      case ent
      when Sketchup::Group, Sketchup::ComponentDefinition
        ent.entities
      when Sketchup::ComponentInstance
        ent.definition.entities
      else
        nil
      end
    end

    # Get all edges of a group/component.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] recursive Whether to include all the child groups and
    #   components.
    # @param [Boolean] transform Whether to give points in global coordinates.
    # @return [Array<Array<Geom::Point3d>>] An array of edges. Each edge
    #   represents an array of two points.
    def get_edges(ent, recursive = true, transform = true)
      unless VALID_TYPES.include?(ent.class)
        raise INVALID_TYPE
      end
      edges = []
      get_entities(ent).each { |e|
        if VALID_TYPES.include?(e.class) and recursive and MSPhysics.get_entity_type(e) == 'Body'
          edges.concat get_edges(e, true, true)
          next
        end
        next unless e.is_a?(Sketchup::Edge)
        edge = []
        e.vertices.each { |v| edge << v.position }
        edges << edge
      }
      if transform
        tra = ent.transformation
        edges.each { |edge|
          edge.each { |pt| pt.transform!(tra) }
        }
      end
      edges
    end

    # Get vertices from all edges of a group/component.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] recursive Whether to include all the child groups and
    #   components.
    # @param [Boolean] transform Whether to give points in global coordinates.
    # @return [Array<Geom::Point3d>] An array of points.
    def get_vertices_from_edges(ent, recursive = true, transform = true)
      unless VALID_TYPES.include?(ent.class)
        raise INVALID_TYPE
      end
      pts = []
      get_entities(ent).each { |e|
        if VALID_TYPES.include?(e.class) and recursive and MSPhysics.get_entity_type(e) == 'Body'
          pts.concat get_vertices_from_edges(e, true, true)
          next
        end
        next unless e.is_a?(Sketchup::Edge)
        e.vertices.each { |v| pts << v.position }
      }
      if transform
        tra = ent.transformation
        pts.each { |pt| pt.transform!(tra) }
      end
      pts
    end

    # Get all faces of a group/component.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] recursive Whether to include all the child groups and
    #   components.
    # @param [Boolean] transform Whether to give points in global coordinates.
    # @return [Array<Array<Geom::Point3d>>] An array of faces. Each face
    #   represents an array of points.
    def get_faces(ent, recursive = true, transform = true)
      unless VALID_TYPES.include?(ent.class)
        raise INVALID_TYPE
      end
      faces = []
      get_entities(ent).each { |e|
        if VALID_TYPES.include?(e.class) and recursive and MSPhysics.get_entity_type(e) == 'Body'
          faces.concat get_faces(e, true, true)
          next
        end
        next unless e.is_a?(Sketchup::Face)
        face = []
        e.vertices.each { |v| face << v.position }
        faces << face
      }
      if transform
        tra = ent.transformation
        faces.each { |face|
          face.each { |pt| pt.transform!(tra) }
        }
      end
      faces
    end

    # Get vertices from all faces of a group/component.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] recursive Whether to include all the child groups and
    #   components.
    # @param [Boolean] transform Whether to give points in global coordinates.
    # @return [Array<Array<Numeric>>] An array of points.
    def get_vertices_from_faces(ent, recursive = true, transform = true)
      unless VALID_TYPES.include?(ent.class)
        raise INVALID_TYPE
      end
      pts = []
      get_entities(ent).each { |e|
        if VALID_TYPES.include?(e.class) and recursive and MSPhysics.get_entity_type(e) == 'Body'
          pts.concat get_vertices_from_faces(e, true, true)
          next
        end
        next unless e.is_a?(Sketchup::Face)
        e.vertices.each { |v| pts << v.position.to_a }
      }
      if transform
        tra = ent.transformation
        pts.each { |pt| pt.transform!(tra) }
      end
      pts.uniq!
      pts
    end

    # Get all construction points and lines of a group/component.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] recursive Whether to include all the child groups and
    #   components.
    # @param [Boolean] transform Whether to give points in global coordinates.
    # @return [Array<Geom::Point3d>] An array of points.
    def get_construction(ent, recursive = true, transform = true)
      unless VALID_TYPES.include?(ent.class)
        raise INVALID_TYPE
      end
      pts = []
      get_entities(ent).each { |e|
        if VALID_TYPES.include?(e.class) and recursive and MSPhysics.get_entity_type(e) == 'Body'
          pts.concat get_construction(e, true, true)
          next
        end
        if e.is_a?(Sketchup::ConstructionPoint)
          pts << e.position
        elsif e.is_a?(Sketchup::ConstructionLine)
          pts << e.start unless e.start.nil?
          pts << e.end unless e.end.nil?
        end
      }
      if transform
        tra = ent.transformation
        pts.each { |pt| pt.transform!(tra) }
      end
      pts
    end

    # Get all polygons from all faces of a group/component.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] recursive Whether to include all the child groups and
    #   components.
    # @param [Boolean] transform Whether to give points in global coordinates.
    # @return [Array<Array<Geom::Point3d>>] An array of polygons. Each polygon
    #   represents an array of three points (a triplex).
    def get_polygons_from_faces(ent, recursive = true, transform = true)
      unless VALID_TYPES.include?(ent.class)
        raise INVALID_TYPE
      end
      faces = []
      get_entities(ent).each { |e|
        if VALID_TYPES.include?(e.class) and recursive and MSPhysics.get_entity_type(e) == 'Body'
          faces.concat get_polygons_from_faces(e, true, true)
          next
        end
        next unless e.is_a?(Sketchup::Face)
        e.mesh.polygons.each_index{ |i|
          faces << e.mesh.polygon_points_at(i+1)
        }
      }
      if transform
        tra = ent.transformation
        faces.each { |face|
          face.each { |pt| pt.transform!(tra) }
        }
      end
      faces
    end

    # Get centre of mass of group/component relative to its coordinate system.
    # This method doesn't work properly in some cases though.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] recursive Whether to include all the child groups and
    #   components.
    # @return [Geom::Point3d]
    def calc_centre_of_mass(ent, recursive = true)
      tx = 0
      ty = 0
      tz = 0
      total_area = 0
      get_polygons_from_faces(ent, recursive, false).each { |triplet|
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
      Geom::Point3d.new(tx.to_f/total_area, ty.to_f/total_area, tz.to_f/total_area)
    end

    # Get normal of the face with the biggest area.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @return [Geom::Vector3d]
    def get_biggest_face_normal(ent)
      unless VALID_TYPES.include?(ent.class)
        raise INVALID_TYPE
      end
      area = 0
      normal = Geom::Vector3d.new(0,0,1)
      get_entities(ent).each { |e|
        next unless e.is_a?(Sketchup::Face)
        next if e.area <= area
        normal = e.normal
        area = e.area
      }
      normal.transform(ent.transformation).normalize
    end

    # Get group bounding box from faces.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] recursive Whether to include all the child groups and
    #   components.
    # @param [Boolean] transform Whether to give points in global coordinates.
    # @return [Geom::BoundingBox]
    def get_bounding_box_from_faces(ent, recursive = true, transform = true)
      unless VALID_TYPES.include?(ent.class)
        raise INVALID_TYPE
      end
      bb = Geom::BoundingBox.new
      get_entities(ent).each { |e|
        if VALID_TYPES.include?(e.class) and recursive and MSPhysics.get_entity_type(e) == 'Body'
          bb.add get_bounding_box_from_faces(e, true, true)
          next
        end
        next unless e.is_a?(Sketchup::Face)
        bb.add(*e.vertices.to_a)
      }
      if transform
        tra = ent.transformation
        pts = []
        for i in 0..7
          pts << bb.corner(i).transform(tra)
        end
        bb = Geom::BoundingBox.new
        bb.add(pts)
      end
      bb
    end

  end # module Group
end # module MSPhysics
