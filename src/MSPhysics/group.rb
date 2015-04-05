module MSPhysics

  # @since 1.0.0
  module Group
    class << self

      # Get group/component definition.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @return [Sketchup::ComponentDefinition]
      def get_definition(entity)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        if Sketchup.version.to_i >= 14
          entity.entities.parent
        else
          entity.model.definitions.find { |d|
            d.group? && d.instances.include?(entity)
          }
        end
      end

      # Get group/component entities.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @return [Sketchup::Entities]
      def get_entities(entity)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        entity.is_a?(Sketchup::ComponentInstance) ? entity.definition.entities : entity.entities
      end

      # Get group bounding box from faces.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] recursive Whether to include all the child groups and
      #   components.
      # @param [Geom::Transformation, nil] parent_tra The current coordinate
      #   system in which the entity is located in. Pass +nil+ to retrieve
      #   entity bounding box relative to the entity's coordinate system.
      # @yield A procedure to determine whether particular child group/component
      #   should be considered a part of the collection.
      # @yieldparam [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @yieldreturn [Boolean] Pass true to consider an entity as part of the
      #   collection. Pass false to not consider an entity as part of the
      #   collection.
      # @return [Geom::BoundingBox]
      def get_bounding_box_from_faces(entity, recursive = true, parent_tra = nil, &entity_validation)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        AMS.validate_type(parent_tra, Geom::Transformation, NilClass)
        bb = Geom::BoundingBox.new
        self.get_entities(entity).each { |e|
          if e.is_a?(Sketchup::Face)
            if parent_tra
              pts = []
              e.vertices.each { |v|
                pts << v.position.transform(parent_tra)
              }
              bb.add(pts)
            else
              bb.add(e.vertices)
            end
          elsif recursive && (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && entity_validation.call(e)
            bb.add self.get_bounding_box_from_faces(e, true, parent_tra ? parent_tra*e.transformation : e.transformation, &entity_validation)
          end
        }
        bb
      end

      # Get all edges of a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] recursive Whether to include all the child groups and
      #   components.
      # @param [Boolean] transform Whether to give points in global coordinates.
      # @yield A procedure to determine whether particular child group/component
      #   should be considered a part of the collection.
      # @yieldparam [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @yieldreturn [Boolean] Pass true to consider an entity as part of the
      #   collection. Pass false to not consider an entity as part of the
      #   collection.
      # @return [Array<Array<Geom::Point3d>>] An array of edges. Each edge
      #   represents an array of two points.
      def get_edges(entity, recursive = true, transform = false, &entity_validation)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        edges = []
        self.get_entities(entity).each { |e|
          if e.is_a?(Sketchup::Edge)
            edges << [e.start.position, e.end.position]
          elsif recursive && (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && entity_validation.call(e)
            edges.concat self.get_edges(e, true, true, &entity_validation)
          end
        }
        if transform
          tra = entity.transformation
          edges.each { |edge|
            edge.each { |pt| pt.transform!(tra) }
          }
        end
        edges
      end

      # Get vertices from all edges of a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] recursive Whether to include all the child groups and
      #   components.
      # @param [Boolean] transform Whether to give points in global coordinates.
      # @yield A procedure to determine whether particular child group/component
      #   should be considered a part of the collection.
      # @yieldparam [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @yieldreturn [Boolean] Pass true to consider an entity as part of the
      #   collection. Pass false to not consider an entity as part of the
      #   collection.
      # @return [Array<Geom::Point3d>] An array of points.
      def get_vertices_from_edges(entity, recursive = true, transform = false, &entity_validation)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        pts = []
        self.get_entities(entity).each { |e|
          if e.is_a?(Sketchup::Edge)
            pts << e.start.position
            pts << e.end.position
          elsif recursive && (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && entity_validation.call(e)
            pts.concat self.get_vertices_from_edges(e, true, true, &entity_validation)
          end
        }
        if transform
          tra = entity.transformation
          pts.each { |pt| pt.transform!(tra) }
        end
        pts
      end

      # Get all faces of a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] recursive Whether to include all the child groups and
      #   components.
      # @param [Boolean] transform Whether to give points in global coordinates.
      # @yield A procedure to determine whether particular child group/component
      #   should be considered a part of the collection.
      # @yieldparam [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @yieldreturn [Boolean] Pass true to consider an entity as part of the
      #   collection. Pass false to not consider an entity as part of the
      #   collection.
      # @return [Array<Array<Geom::Point3d>>] An array of faces. Each face
      #   represents an array of points.
      def get_faces(entity, recursive = true, transform = false, &entity_validation)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        faces = []
        self.get_entities(entity).each { |e|
          if e.is_a?(Sketchup::Face)
            face = []
            e.vertices.each { |v| face << v.position }
            faces << face
          elsif recursive && (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && entity_validation.call(e)
            faces.concat self.get_faces(e, true, true, &entity_validation)
          end
        }
        if transform
          tra = entity.transformation
          faces.each { |face|
            face.each { |pt| pt.transform!(tra) }
          }
        end
        faces
      end

      # Get vertices from all faces of a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] recursive Whether to include all the child groups and
      #   components.
      # @param [Boolean] transform Whether to give points in global coordinates.
      # @yield A procedure to determine whether particular child group/component
      #   should be considered a part of the collection.
      # @yieldparam [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @yieldreturn [Boolean] Pass true to consider an entity as part of the
      #   collection. Pass false to not consider an entity as part of the
      #   collection.
      # @return [Array<Geom::Point3d>] An array of points.
      def get_vertices_from_faces(entity, recursive = true, transform = false, &entity_validation)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        pts = []
        self.get_entities(entity).each { |e|
          if e.is_a?(Sketchup::Face)
            e.vertices.each { |v| pts << v.position }
          elsif recursive && (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && entity_validation.call(e)
            pts.concat self.get_vertices_from_faces(e, true, true, &entity_validation)
          end
        }
        if transform
          tra = entity.transformation
          pts.each { |pt| pt.transform!(tra) }
        end
        pts
      end

      # Get vertices from all faces of a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] recursive Whether to include all the child groups and
      #   components.
      # @param [Boolean] transform Whether to give points in global coordinates.
      # @yield A procedure to determine whether particular child group/component
      #   should be considered a part of the collection.
      # @yieldparam [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @yieldreturn [Boolean] Pass true to consider an entity as part of the
      #   collection. Pass false to not consider an entity as part of the
      #   collection.
      # @return [Array<Array<Geom::Point3d>>] An array of point collections
      #   from every group. Each point collection represents an array of points.
      def get_vertices_from_faces2(entity, recursive = true, transform = false, &entity_validation)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        collections = []
        points = []
        self.get_entities(entity).each { |e|
          if e.is_a?(Sketchup::Face)
            e.vertices.each { |v| points << v.position }
          elsif recursive && (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && entity_validation.call(e)
            collections.concat self.get_vertices_from_faces2(e, true, true, &entity_validation)
          end
        }
        collections << points
        if transform
          tra = entity.transformation
          collections.each { |collection| collection.each { |pt| pt.transform!(tra) } }
        end
        collections
      end

      # Get all construction points and lines of a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] recursive Whether to include all the child groups and
      #   components.
      # @param [Boolean] transform Whether to give points in global coordinates.
      # @yield A procedure to determine whether particular child group/component
      #   should be considered a part of the collection.
      # @yieldparam [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @yieldreturn [Boolean] Pass true to consider an entity as part of the
      #   collection. Pass false to not consider an entity as part of the
      #   collection.
      # @return [Array<Geom::Point3d>] An array of points.
      def get_construction(entity, recursive = true, transform = false, &entity_validation)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        pts = []
        self.get_entities(entity).each { |e|
          if e.is_a?(Sketchup::ConstructionPoint)
            pts << e.position
          elsif e.is_a?(Sketchup::ConstructionLine)
            pts << e.start unless e.start.nil?
            pts << e.end unless e.end.nil?
          elsif recursive && (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && entity_validation.call(e)
            pts.concat self.get_construction(e, true, true, &entity_validation)
          end
        }
        if transform
          tra = entity.transformation
          pts.each { |pt| pt.transform!(tra) }
        end
        pts
      end

      # Get triplets from all faces of a group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] recursive Whether to include all the child groups and
      #   components.
      # @param [Boolean] transform Whether to give points in global coordinates.
      # @yield A procedure to determine whether particular child group/component
      #   should be considered a part of the collection.
      # @yieldparam [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @yieldreturn [Boolean] Pass true to consider an entity as part of the
      #   collection. Pass false to not consider an entity as part of the
      #   collection.
      # @return [Array<Array<Geom::Point3d>>] An array of polygons. Each polygon
      #   represents an array of three points - a triplex.
      def get_polygons_from_faces(entity, recursive = true, transform = false, &entity_validation)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        triplets = []
        self.get_entities(entity).each { |e|
          if e.is_a?(Sketchup::Face)
            e.mesh.polygons.each_index{ |i|
              triplets << e.mesh.polygon_points_at(i+1)
            }
          elsif recursive && (e.is_a?(Sketchup::Group) || e.is_a?(Sketchup::ComponentInstance)) && entity_validation.call(e)
            triplets.concat self.get_polygons_from_faces(e, true, true, &entity_validation)
          end
        }
        if transform
          tra = entity.transformation
          flipped = MSPhysics::Geometry.is_matrix_flipped?(tra)
          for i in 0...triplets.size
            triplets[i].each { |pt| pt.transform!(tra) }
            triplets[i].reverse! if flipped
          end
        end
        triplets
      end

      # Get triangular mesh of the group.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] recursive Whether to include all the child groups and
      #   components.
      # @param [Boolean] transform Whether to give points in global coordinates.
      # @yield A procedure to determine whether particular child group/component
      #   should be considered a part of the collection.
      # @yieldparam [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @yieldreturn [Boolean] Pass true to consider an entity as part of the
      #   collection. Pass false to not consider an entity as part of the
      #   collection.
      # @return [Geom::PolygonMesh]
      def get_triangular_mesh(entity, recursive = true, transform = false, &entity_validation)
        AMS.validate_type(entity, ::Sketchup::Group, ::Sketchup::ComponentInstance)
        mesh = Geom::PolygonMesh.new
        self.get_entities(entity).each { |e|
          if e.is_a?(::Sketchup::Face)
            e.mesh.polygons.each_index{ |i|
              pts = e.mesh.polygon_points_at(i+1)
              pts.reverse! if transform && MSPhysics::Geometry.is_matrix_flipped?(entity.transformation)
              mesh.add_polygon(pts)
            }
          elsif recursive && (e.is_a?(::Sketchup::Group) || e.is_a?(::Sketchup::ComponentInstance)) && entity_validation.call(e)
            mesh2 = self.get_triangular_mesh(e, true, true, &entity_validation)
            mesh2.polygons.each_index { |i|
              mesh.add_polygon(mesh2.polygon_points_at(i+1))
            }
          end
        }
        mesh.transform!(entity.transformation) if transform
        mesh
      end

      # Calculate centre of mass of group/component relative to its coordinate
      # system.
      # @note This method doesn't return proper centre of mass in some cases.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] recursive Whether to include all the child groups and
      #   components.
      # @yield A procedure to determine whether particular child group/component
      #   should be considered a part of the collection.
      # @yieldparam [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @yieldreturn [Boolean] Pass true to consider an entity as part of the
      #   collection. Pass false to not consider an entity as part of the
      #   collection.
      # @return [Geom::Point3d]
      def calc_centre_of_mass(entity, recursive = true, &entity_validation)
        tx = 0
        ty = 0
        tz = 0
        total_area = 0
        triplets = self.get_polygons_from_faces(entity, recursive, false){ |e|
          entity_validation.call(e)
        }
        triplets.each { |triplet|
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

    end # class << self
  end # module Group
end # module MSPhysics
