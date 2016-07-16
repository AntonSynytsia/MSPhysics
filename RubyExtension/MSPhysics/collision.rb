module MSPhysics

  # @since 1.0.0
  module Collision

    EPSILON = 1.0e-6

    class << self

      # Get collision type.
      # * Collision types 12-17 are not implemented.
      # * Collisions of type 8 and below are all convex collisions.
      # @param [Fixnum] address Reference to a valid collision.
      # @return [Fixnum] Collision type:
      #   * 0 - sphere
      #   * 1 - capsule
      #   * 2 - chamfer cylinder
      #   * 3 - tapered capsule
      #   * 4 - cylinder
      #   * 5 - tapered cylinder
      #   * 6 - box
      #   * 7 - cone
      #   * 8 - convex hull
      #   * 9 - null
      #   * 10 - compound
      #   * 11 - tree (static mesh)
      #   * 12 - height field
      #   * 13 - cloth patch
      #   * 14 - deformable solid
      #   * 15 - user mesh
      #   * 16 - scene
      #   * 17 - fractured compound
      # @example Determining if collision is convex.
      #   onStart {
      #     address = this.collision_address
      #     type = MSPhysics::Collision.type(address)
      #     convex = (type < 9)
      #   }
      def type(address)
        MSPhysics::Newton::Collision.get_type(address)
      end

      # Determine if collision is convex.
      # @param [Fixnum] address Reference to a valid collision.
      # @return [Boolean]
      def convex?(address)
        MSPhysics::Newton::Collision.get_type(address) < 9
      end

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
        if s.x < EPSILON || s.y  < EPSILON || s.z < EPSILON
          raise(TypeError, "Entity #{entity} has one of the axis scaled to zero. Zero scaled shapes are not acceptable!", caller)
        end
      end

      # Create a physics collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Symbol, String] shape Use one of the provided shapes:
      #   * box
      #   * sphere
      #   * cone
      #   * cylinder
      #   * chamfer_cylinder
      #   * capsule
      #   * convex_hull
      #   * null
      #   * compound
      #   * compound_from_cd
      #   * static_mesh
      # @param [Boolean] transform Whether to offset the collision. Usually this
      #   parameter is set true if the entity is a sub-collision of some parent
      #   collision.
      # @return [Fixnum] Collision address
      def create(world, entity, shape = :convex_hull, transform = false)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        shape = shape.to_s.downcase.gsub(' ', '_').to_sym
        case shape
          when :box
            create_box(world, entity, transform)
          when :sphere
            create_sphere(world, entity, transform)
          when :cone
            create_cone(world, entity, transform)
          when :cylinder
            create_cylinder(world, entity, transform)
          when :chamfer_cylinder
            create_chamfer_cylinder(world, entity, transform)
          when :capsule
            create_capsule(world, entity, transform)
          when :convex_hull
            create_convex_hull(world, entity, transform)
          when :null
            create_null(world)
          when :compound
            create_compound2(world, entity)
          when :compound_from_cd
            create_compound_from_cd(world, entity)
          when :static_mesh
            create_static_mesh2(world, entity)
        else
          raise(TypeError, "The specified collision shape, \"#{shape}\", does not exist!", caller)
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
      # @param [Boolean] transform Whether to offset the collision. Usually this
      #   parameter is set true if the entity is a sub-collision of some parent
      #   collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_box(world, entity, transform = false)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
        if bb.width < EPSILON || bb.height < EPSILON || bb.depth < EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        center = bb.center
        if transform
          center.transform!(tra)
          offset_matrix = Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, center)
        else
          center.x *= AMS::Geometry.is_matrix_flipped?(tra) ? -s.x : s.x
          center.y *= s.y
          center.z *= s.z
          offset_matrix = Geom::Transformation.new(center)
        end
        MSPhysics::Newton::Collision.create_box(world.address, bb.width*s.x, bb.height*s.y, bb.depth*s.z, 0, offset_matrix)
      end

      # Create a sphere collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] transform Whether to offset the collision. Usually this
      #   parameter is set true if the entity is a sub-collision of some parent
      #   collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_sphere(world, entity, transform = false)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
        if bb.width < EPSILON || bb.height < EPSILON || bb.depth < EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        center = bb.center
        if transform
          center.transform!(tra)
          offset_matrix = Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, center)
        else
          center.x *= AMS::Geometry.is_matrix_flipped?(tra) ? -s.x : s.x
          center.y *= s.y
          center.z *= s.z
          offset_matrix = Geom::Transformation.new(center)
        end
        #d = bb.width*s.x
        #d = bb.height*s.y if bb.height*s.y > d
        #d = bb.height*s.z if bb.height*s.z > d
        #MSPhysics::Newton::Collision.create_sphere(world.address, d*0.5, 0, offset_matrix)
        MSPhysics::Newton::Collision.create_scaled_sphere(world.address, bb.width*s.x, bb.height*s.y, bb.depth*s.z, 0, offset_matrix)
      end

      # Create a cone collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] transform Whether to offset the collision. Usually this
      #   parameter is set true if the entity is a sub-collision of some parent
      #   collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_cone(world, entity, transform = false)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
        if bb.width < EPSILON || bb.height < EPSILON || bb.depth < EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        center = bb.center
        if transform
          center.transform!(tra)
          offset_matrix = Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, center)
        else
          center.x *= AMS::Geometry.is_matrix_flipped?(tra) ? -s.x : s.x
          center.y *= s.y
          center.z *= s.z
          offset_matrix = Geom::Transformation.new(center)
        end
        MSPhysics::Newton::Collision.create_scaled_cone(world.address, bb.depth*s.z * 0.5, bb.height*s.y * 0.5, bb.width*s.x, 0, offset_matrix)
      end

      # Create a cylinder collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] transform Whether to offset the collision. Usually this
      #   parameter is set true if the entity is a sub-collision of some parent
      #   collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_cylinder(world, entity, transform = false)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
        if bb.width < EPSILON || bb.height < EPSILON || bb.depth < EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        center = bb.center
        if transform
          center.transform!(tra)
          offset_matrix = Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, center)
        else
          center.x *= AMS::Geometry.is_matrix_flipped?(tra) ? -s.x : s.x
          center.y *= s.y
          center.z *= s.z
          offset_matrix = Geom::Transformation.new(center)
        end
        MSPhysics::Newton::Collision.create_scaled_cylinder(world.address, bb.depth*s.z * 0.5, bb.height*s.y * 0.5, bb.width*s.x, 0, offset_matrix)
      end

      # Create a capsule collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] transform Whether to offset the collision. Usually this
      #   parameter is set true if the entity is a sub-collision of some parent
      #   collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_capsule(world, entity, transform = false)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
        if bb.width < EPSILON || bb.height < EPSILON || bb.depth < EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        center = bb.center
        if transform
          center.transform!(tra)
          offset_matrix = Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, center)
        else
          center.x *= AMS::Geometry.is_matrix_flipped?(tra) ? -s.x : s.x
          center.y *= s.y
          center.z *= s.z
          offset_matrix = Geom::Transformation.new(center)
        end
        #d = bb.depth*s.z
        #d = bb.height*s.y if bb.height*s.y > d
        #l = bb.width*s.x-d
        #l = 0 if l < 0
        #MSPhysics::Newton::Collision.create_capsule(world.address, d*0.5, l, 0, offset_matrix)
        MSPhysics::Newton::Collision.create_scaled_capsule(world.address, bb.depth*s.z * 0.5, bb.height*s.y * 0.5, bb.width*s.x, 0, offset_matrix)
      end

      # Create a chamfer cylinder collision.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @param [Boolean] transform Whether to offset the collision. Usually this
      #   parameter is set true if the entity is a sub-collision of some parent
      #   collision.
      # @return [Fixnum] Collision address
      # @raise [TypeError] if calculated bounding box turns out flat.
      def create_chamfer_cylinder(world, entity, transform = false)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        bb = AMS::Group.get_bounding_box_from_faces(entity, true) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
        if bb.width < EPSILON || bb.height < EPSILON || bb.depth < EPSILON
          raise(TypeError, "Entity #{entity} has a flat bounding box. Flat collisions are invalid!", caller)
        end
        tra = entity.transformation
        s = AMS::Geometry.get_matrix_scale(tra)
        center = bb.center
        if transform
          center.transform!(tra)
          offset_matrix = Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, center)
        else
          center.x *= AMS::Geometry.is_matrix_flipped?(tra) ? -s.x : s.x
          center.y *= s.y
          center.z *= s.z
          offset_matrix = Geom::Transformation.new(center)
        end
        #l = bb.width*s.x
        #d = bb.depth*s.z
        #d = bb.height*s.y if bb.height*s.y > d
        #d -= l*2
        #d = 0 if d < 0
        #MSPhysics::Newton::Collision.create_chamfer_cylinder(world.address, d*0.5, l, 0, offset_matrix)
        MSPhysics::Newton::Collision.create_scaled_chamfer_cylinder(world.address, bb.depth*s.z * 0.5, bb.height*s.y * 0.5, bb.width*s.x, 0, offset_matrix)
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
      def create_convex_hull(world, entity, transform = false)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        vertices = AMS::Group.get_vertices_from_faces(entity, true, false) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
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
        MSPhysics::Newton::Collision.create_convex_hull(world.address, vertices, 0.0, 0, offset_matrix)
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
        point_collections = AMS::Group.get_vertices_from_faces2(entity, true, false) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
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
          convex_collisions << MSPhysics::Newton::Collision.create_convex_hull(world.address, vertices, 0.0, 0, nil)
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
        meshes = AMS::Group.get_triangular_meshes(entity, true, stra) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
        convex_collisions = []
        meshes.each { |mesh|
          vertices = mesh.points
          next if vertices.size < 4 || AMS::Geometry.points_coplanar?(vertices)
          convex_collisions << MSPhysics::Newton::Collision.create_convex_hull(world.address, vertices, 0.0, 0, nil)
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
        triplets = AMS::Group.get_polygons_from_faces(entity, true, false) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
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
        MSPhysics::Newton::Collision.create_static_mesh(world.address, triplets, true, 0, 0)
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
        mesh = AMS::Group.get_triangular_mesh(entity, true, stra, nil) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
        if mesh.count_polygons == 0
          raise(TypeError, "Entity #{entity} doesn't have any faces. At least one face is required for an entity to be a valid tree collision!", caller)
        end
        triplets = Array.new(mesh.count_polygons)
        for i in 0...mesh.count_polygons
          triplets[i] = mesh.polygon_points_at(i+1)
        end
        MSPhysics::Newton::Collision.create_static_mesh(world.address, triplets, true, 0, 0)
      end

      # Create a compound collision from convex decomposition using convex
      # approximation algorithm by Julio Jerez.
      # @param [World] world
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @return [Fixnum] Collision address
      # @raise [TypeError] if entity has too few vertices.
      # @raise [TypeError] if entity is flat.
      def create_compound_from_cd(world, entity)
        MSPhysics::World.validate(world)
        validate_entity(entity)
        triplets = AMS::Group.get_polygons_from_faces(entity, true, false) { |e|
          e.get_attribute('MSPhysics', 'Type', 'Body') == 'Body' && !e.get_attribute('MSPhysics Body', 'Ignore')
        }
        if triplets.size < 4
          raise(TypeError, "Entity #{entity} doesn't have enough faces. At least four faces is required for an entity to be a valid compound from mesh collision!", caller)
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
        MSPhysics::Newton::Collision.create_compound_from_cd(world.address, triplets, 0.01, 0.2, 512, 1024, 0)
      end

    end # class << self
  end # module Collision
end # module MSPhysics
