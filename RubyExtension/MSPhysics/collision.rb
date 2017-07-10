module MSPhysics

  # @since 1.0.0
  module Collision

    ENTITY_VALIDATION_PROC = Proc.new { |entity|
      next true if (!entity.is_a?(Sketchup::Group) && !entity.is_a?(Sketchup::ComponentInstance))
      entity.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !entity.get_attribute('MSPhysics Body', 'Ignore')
    }

    class << self

      # Verify that entity is valid for collision generation.
      # @api private
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @raise [TypeError] if entity is deleted.
      # @raise [TypeError] if entity has a transformation matrix with
      #   non-perpendicular axis.
      # @raise [TypeError] if entity has at least one of the axis scaled to
      #   zero.
      # @return [void]
      def validate_entity(entity)
        AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
        raise(TypeError, "Entity #{entity} is deleted!", caller) unless entity.valid?
        unless AMS::Geometry.is_matrix_uniform?(entity.transformation)
          raise(TypeError, "Entity #{entity} has a non-uniform transformation matrix. Some or all matrix axis are not perpendicular to each other.", caller)
        end
        s = AMS::Geometry.get_matrix_scale(entity.transformation)
        if s.x.to_f < MSPhysics::EPSILON || s.y.to_f  < MSPhysics::EPSILON || s.z.to_f < MSPhysics::EPSILON
          raise(TypeError, "Entity #{entity} has one of the axis scaled to zero. Zero scaled shapes are not acceptable!", caller)
        end
      end

      # Create a physics collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Fixnum] shape_id Shape ID. See {MSPhysics::SHAPE_NAMES}
      # @param [Geom::Transformation, nil] transformation A local transform to
      #   apply to the collision.
      # @return [Fixnum] Collision address
      def create(world, entity, shape_id, transformation = nil)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        case shape_id
          when 0
            create_null(world)
          when 1
            create_box(world, entity, transformation)
          when 2
            create_sphere(world, entity, transformation)
          when 3
            create_cone(world, entity,transformation)
          when 4
            create_cylinder(world, entity, transformation)
          when 5
            create_chamfer_cylinder(world, entity, transformation)
          when 6
            create_capsule(world, entity, transformation)
          when 7
            create_convex_hull(world, entity, false)
          when 8
            create_compound2(world, entity)
          when 9
            create_static_mesh2(world, entity)
        else
          raise(TypeError, "The given shape ID, \"#{shape_id}\", does not reference a valid shape!", caller)
        end
      end

      # Create a null collision.
      # @param [World] world
      # @return [Fixnum] Collision address
      def create_null(world)
        MSPhysics::World.validate(world)
        MSPhysics::Newton::Collision.create_null(world.address)
      end

      # Create a box collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Geom::Transformation, nil] transformation A transform to apply
      #   to the collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_box(world, entity, transformation)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true, nil, &ENTITY_VALIDATION_PROC)
        if bb.width.to_f < MSPhysics::EPSILON || bb.height.to_f < MSPhysics::EPSILON || bb.depth.to_f < MSPhysics::EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        ent_tra = entity.transformation
        scale = AMS::Geometry.get_matrix_scale(ent_tra)
        center = bb.center
        center.x *= AMS::Geometry.is_matrix_flipped?(ent_tra) ? -scale.x : scale.x
        center.y *= scale.y
        center.z *= scale.z
        offset_matrix = Geom::Transformation.new(center)
        params = Geom::Vector3d.new(bb.width * scale.x, bb.height * scale.y, bb.depth * scale.z)
        if transformation
          offset_matrix = offset_matrix * transformation
          params.transform!(transformation)
        end
        MSPhysics::Newton::Collision.create_box(world.address, params.x.abs, params.y.abs, params.z.abs, 0, offset_matrix)
      end

      # Create a sphere collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Geom::Transformation, nil] transformation A transform to apply
      #   to the collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_sphere(world, entity, transformation)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true, nil, &ENTITY_VALIDATION_PROC)
        if bb.width.to_f < MSPhysics::EPSILON || bb.height.to_f < MSPhysics::EPSILON || bb.depth.to_f < MSPhysics::EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        ent_tra = entity.transformation
        scale = AMS::Geometry.get_matrix_scale(ent_tra)
        center = bb.center
        center.x *= AMS::Geometry.is_matrix_flipped?(ent_tra) ? -scale.x : scale.x
        center.y *= scale.y
        center.z *= scale.z
        offset_matrix = Geom::Transformation.new(center)
        params = Geom::Vector3d.new(bb.width * scale.x, bb.height * scale.y, bb.depth * scale.z)
        if transformation
          offset_matrix = offset_matrix * transformation
          params.transform!(transformation)
        end
        MSPhysics::Newton::Collision.create_scaled_sphere(world.address, params.x.abs, params.y.abs, params.z.abs, 0, offset_matrix)
      end

      # Create a cone collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Geom::Transformation, nil] transformation A transform to apply
      #   to the collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_cone(world, entity, transformation)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true, nil, &ENTITY_VALIDATION_PROC)
        if bb.width.to_f < MSPhysics::EPSILON || bb.height.to_f < MSPhysics::EPSILON || bb.depth.to_f < MSPhysics::EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        ent_tra = entity.transformation
        scale = AMS::Geometry.get_matrix_scale(ent_tra)
        center = bb.center
        center.x *= AMS::Geometry.is_matrix_flipped?(ent_tra) ? -scale.x : scale.x
        center.y *= scale.y
        center.z *= scale.z
        offset_matrix = Geom::Transformation.new(center)
        params = Geom::Vector3d.new(bb.width * scale.x, bb.height * scale.y, bb.depth * scale.z)
        if transformation
          offset_matrix = offset_matrix * transformation
          params.transform!(transformation)
        end
        MSPhysics::Newton::Collision.create_scaled_cone(world.address, params.z.abs * 0.5 , params.y.abs * 0.5, params.x.abs, 0, offset_matrix)
      end

      # Create a cylinder collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Geom::Transformation, nil] transformation A transform to apply
      #   to the collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_cylinder(world, entity, transformation)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true, nil, &ENTITY_VALIDATION_PROC)
        if bb.width.to_f < MSPhysics::EPSILON || bb.height.to_f < MSPhysics::EPSILON || bb.depth.to_f < MSPhysics::EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        ent_tra = entity.transformation
        scale = AMS::Geometry.get_matrix_scale(ent_tra)
        center = bb.center
        center.x *= AMS::Geometry.is_matrix_flipped?(ent_tra) ? -scale.x : scale.x
        center.y *= scale.y
        center.z *= scale.z
        offset_matrix = Geom::Transformation.new(center)
        params = Geom::Vector3d.new(bb.width * scale.x, bb.height * scale.y, bb.depth * scale.z)
        if transformation
          offset_matrix = offset_matrix * transformation
          params.transform!(transformation)
        end
        MSPhysics::Newton::Collision.create_scaled_cylinder(world.address, params.z.abs * 0.5 , params.y.abs * 0.5, params.x.abs, 0, offset_matrix)
      end

      # Create a capsule collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Geom::Transformation, nil] transformation A transform to apply
      #   to the collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_capsule(world, entity, transformation)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true, nil, &ENTITY_VALIDATION_PROC)
        if bb.width.to_f < MSPhysics::EPSILON || bb.height.to_f < MSPhysics::EPSILON || bb.depth.to_f < MSPhysics::EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        ent_tra = entity.transformation
        scale = AMS::Geometry.get_matrix_scale(ent_tra)
        center = bb.center
        center.x *= AMS::Geometry.is_matrix_flipped?(ent_tra) ? -scale.x : scale.x
        center.y *= scale.y
        center.z *= scale.z
        offset_matrix = Geom::Transformation.new(center)
        params = Geom::Vector3d.new(bb.width * scale.x, bb.height * scale.y, bb.depth * scale.z)
        if transformation
          offset_matrix = offset_matrix * transformation
          params.transform!(transformation)
        end
        MSPhysics::Newton::Collision.create_scaled_capsule(world.address, params.z.abs * 0.5 , params.y.abs * 0.5, params.x.abs, 0, offset_matrix)
      end

      # Create a chamfer cylinder collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Geom::Transformation, nil] transformation A transform to apply
      #   to the collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_chamfer_cylinder(world, entity, transformation)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true, nil, &ENTITY_VALIDATION_PROC)
        if bb.width.to_f < MSPhysics::EPSILON || bb.height.to_f < MSPhysics::EPSILON || bb.depth.to_f < MSPhysics::EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        ent_tra = entity.transformation
        scale = AMS::Geometry.get_matrix_scale(ent_tra)
        center = bb.center
        center.x *= AMS::Geometry.is_matrix_flipped?(ent_tra) ? -scale.x : scale.x
        center.y *= scale.y
        center.z *= scale.z
        offset_matrix = Geom::Transformation.new(center)
        params = Geom::Vector3d.new(bb.width * scale.x, bb.height * scale.y, bb.depth * scale.z)
        if transformation
          offset_matrix = offset_matrix * transformation
          params.transform!(transformation)
        end
        MSPhysics::Newton::Collision.create_scaled_chamfer_cylinder(world.address, params.z.abs * 0.5 , params.y.abs * 0.5, params.x.abs, 0, offset_matrix)
      end

      # Create a convex hull collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] transform Whether to offset the collision. Usually this
      #   parameter is set true if the entity is a sub-collision of some parent
      #   collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if entity has too few vertices.
      # @raise [TypeError] if entity has all vertices coplanar.
      # @raise [TypeError] if the engine failed to generate convex hull.
      def create_convex_hull(world, entity, transform = false)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        vertices = AMS::Group.get_vertices_from_faces(entity, true, nil, &ENTITY_VALIDATION_PROC)
        if vertices.size < 4
          raise(TypeError, "Entity #{entity} has too few vertices. At least four non-coplanar vertices are expected!", caller)
        end
        if AMS::Geometry.points_coplanar?(vertices)
          raise(TypeError, "Entity #{entity} has all vertices coplanar. Flat collisions are not acceptable!", caller)
        end
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        s.x *= -1 if AMS::Geometry.is_matrix_flipped?(tra)
        vertices.each { |v|
          v.x *= s.x
          v.y *= s.y
          v.z *= s.z
        }
        offset_matrix = transform ? AMS::Geometry.extract_matrix_scale(tra) : nil
        collision = MSPhysics::Newton::Collision.create_convex_hull(world.address, vertices, 1.0e-4, 0, offset_matrix)
        unless collision
          raise(TypeError, "The engine failed to generate convex collision for entity, #{entity}, as the entity is probably too small!", caller)
        end
        collision
      end

      # Create a compound collision. In a compound collision every sub-group
      # is considered a convex collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @return [Fixnum] Collision address
      # @raise [TypeError] if entity doesn't have any valid sub-collisions.
      def create_compound(world, entity)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        point_collections = AMS::Group.get_vertices_from_faces2(entity, true, nil, &ENTITY_VALIDATION_PROC)
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        s.x *= -1 if AMS::Geometry.is_matrix_flipped?(tra)
        convex_collisions = []
        point_collections.each { |vertices|
          next if vertices.size < 4
          next if AMS::Geometry.points_coplanar?(vertices)
          vertices.each { |v|
            v.x *= s.x
            v.y *= s.y
            v.z *= s.z
          }
          sub_col = MSPhysics::Newton::Collision.create_convex_hull(world.address, vertices, 1.0e-4, 0, nil)
          convex_collisions << sub_col if sub_col
        }
        if convex_collisions.empty?
          raise(TypeError, "Entity #{entity} doesn't have any valid sub-collisions, making it an invalid compound collision!", caller)
        end
        collision = MSPhysics::Newton::Collision.create_compound(world.address, convex_collisions, 0)
        convex_collisions.each { |sub_col|
          MSPhysics::Newton::Collision.destroy(sub_col)
        }
        collision
      end

      # Create a compound collision. In a compound collision every sub-group
      # is considered a convex collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @return [Fixnum] Collision address
      # @raise [TypeError] if entity doesn't have any valid sub-collisions.
      def create_compound2(world, entity)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        s.x *= -1 if AMS::Geometry.is_matrix_flipped?(tra)
        stra = Geom::Transformation.new([s.x,0,0,0, 0,s.y,0,0, 0,0,s.z,0, 0,0,0,1])
        meshes = AMS::Group.get_triangular_meshes(entity, true, stra, &ENTITY_VALIDATION_PROC)
        convex_collisions = []
        meshes.each { |mesh|
          vertices = mesh.points
          next if vertices.size < 4 || AMS::Geometry.points_coplanar?(vertices)
          sub_col = MSPhysics::Newton::Collision.create_convex_hull(world.address, vertices, 1.0e-4, 0, nil)
          convex_collisions << sub_col if sub_col
        }
        if convex_collisions.empty?
          raise(TypeError, "Entity #{entity} doesn't have any valid sub-collisions, making it an invalid compound collision!", caller)
        end
        collision = MSPhysics::Newton::Collision.create_compound(world.address, convex_collisions, 0)
        convex_collisions.each { |sub_col|
          MSPhysics::Newton::Collision.destroy(sub_col)
        }
        collision
      end

      # Create a static tree/scene collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @return [Fixnum] Collision address
      # @raise [TypeError] if entity has no faces.
      def create_static_mesh(world, entity)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        triplets = AMS::Group.get_polygons_from_faces(entity, true, nil, &ENTITY_VALIDATION_PROC)
        if triplets.empty?
          raise(TypeError, "Entity #{entity} doesn't have any faces. At least one face is required for an entity to be a valid tree collision!", caller)
        end
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        flipped = AMS::Geometry.is_matrix_flipped?(tra)
        s.x *= -1 if flipped
        for i in 0...triplets.size
          triplets[i].each { |point|
            point.x *= s.x
            point.y *= s.y
            point.z *= s.z
          }
          triplets[i].reverse! if flipped
        end
        MSPhysics::Newton::Collision.create_static_mesh(world.address, triplets, false, 0)
      end

      # Create a static tree/scene collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @return [Fixnum] Collision address
      # @raise [TypeError] if entity has no faces.
      def create_static_mesh2(world, entity)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        flipped = AMS::Geometry.is_matrix_flipped?(tra)
        s.x *= -1 if flipped
        stra = Geom::Transformation.new([s.x,0,0,0, 0,s.y,0,0, 0,0,s.z,0, 0,0,0,1])
        mesh = AMS::Group.get_triangular_mesh(entity, true, stra, &ENTITY_VALIDATION_PROC)
        if mesh.count_polygons == 0
          raise(TypeError, "Entity #{entity} doesn't have any faces. At least one face is required for an entity to be a valid tree collision!", caller)
        end
        triplets = Array.new(mesh.count_polygons)
        for i in 0...mesh.count_polygons
          triplets[i] = mesh.polygon_points_at(i+1)
        end
        c = MSPhysics::Newton::Collision.create_static_mesh(world.address, triplets, false, 0)
      end

    end # class << self
  end # module Collision
end # module MSPhysics
