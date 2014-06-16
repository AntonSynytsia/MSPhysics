module MSPhysics
  module Collision

    # @!visibility private
    VALID_TYPES = [Sketchup::Group, Sketchup::ComponentInstance]
    # @!visibility private
    INVALID_OBJECT = 'The specified object is not a group or a component.'
    # @!visibility private
    INVALID_SHAPE = 'The specified entity has an invalid shape.'

    SHAPES = [
      :box,
      :sphere,
      :cone,
      :cylinder,
      :chamfer_cylinder,
      :capsule,
      :convex_hull,
      :null,
      :compound,
      :compound_from_mesh,
      :static_mesh
    ]

    # @!visibility private
    @progress = nil
    @data ||= {}

    # @!visibility private
    PROGRESS_REPORT = Proc.new { |progress|
      progress = (progress*100).to_i
      next true if @progress == progress
      printf("%d\%\n", progress)
      @progress = progress
      true
    }

    module_function

    # @!visibility private
    def get_collision_instance(world, ent, shape)
      return unless ent.is_a?(Sketchup::ComponentInstance)
      return unless @data[ent.definition].is_a?(Array)
      col = nil
      @data[ent.definition].each { |data|
        next if data[0] != world.address
        next if data[1] != shape
        col = data[2]
        break
      }
      return unless col
      c = Newton.collisionCreateInstance(col)
      scale = Geometry.get_scale(ent.transformation)
      if Newton.collisionGetType(c).between?(0,7)
        orig_scale = Array.new(3){ 0.chr*4 }
        Newton.collisionGetScale(c, *orig_scale)
        buffer = 0.chr*64
        Newton.collisionGetMatrix(c, buffer)
        offset_matrix = buffer.unpack('F*')
        for i in 0..2
          offset_matrix[12+i] *= scale[i]/orig_scale[i].unpack('F')[0]
        end
        Newton.collisionSetMatrix(c, offset_matrix.pack('F*'))
      end
      Newton.collisionSetScale(c, *scale)
      c
    end

    # @!visibility private
    def save_collision(world, ent, shape, col)
      return false unless ent.is_a?(Sketchup::ComponentInstance)
      return false if Newton.collisionGetType(col) == 11
      ent = ent.definition
      @data[ent] ||= []
      @data[ent].each { |data|
        next if data[0] != world.address
        next if data[1] != shape
        return false
      }
      c = Newton.collisionCreateInstance(col)
      @data[ent] << [world.address, shape, c]
      true
    end

    # Clear all saved collisions.
    # @api private
    def reset_data
      @data.clear
    end

    # Optimize shape name.
    # @param [String, Symbol] name
    # @return [Symbol, NilClass] Proper name if successful.
    def optimize_shape_name(name)
      name = name.to_s.downcase.gsub(/\s|_/, '')
      SHAPES.each { |shape|
        return shape if shape.to_s.gsub(/_/, '') == name
      }
      nil
    end

    # Create collision.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Symbol, String] shape Collision shape. See {SHAPES}.
    # @param [Boolean] transform
    # @return [AMS::FFI::Pointer] Pointer to the newton collision.
    def create(world, ent, shape = :convex_hull, transform = false)
      unless VALID_TYPES.include?(ent.class)
        raise ArgumentError, INVALID_OBJECT
      end
      shape = optimize_shape_name(shape)
      unless shape
        raise ArgumentError, 'The specified shape is invalid.'
      end
      unless transform
        col = get_collision_instance(world, ent, shape)
        return col if col
      end
      col = case shape
        when :box
          create_box(world, ent, transform)
        when :sphere
          create_sphere(world, ent, transform)
        when :cone
          create_cone(world, ent, transform)
        when :cylinder
          create_cylinder(world, ent, transform)
        when :chamfer_cylinder
          create_chamfer_cylinder(world, ent, transform)
        when :capsule
          create_capsule(world, ent, transform)
        when :convex_hull
          create_convex_hull(world, ent, transform)
        when :null
          create_null(world)
        when :compound
          child_data = {}
          handle = 'MSPhysics Body'
          definition = ent.respond_to?(:definition) ? ent.definition : ent
          definition.entities.each{ |e|
            next unless VALID_TYPES.include?(e.class)
            ignore = e.get_attribute(handle, 'Ignore', false)
            next if ignore
            shape = e.get_attribute(handle, 'Shape', 'Convex Hull')
            no_collision = e.get_attribute(handle, 'No Collision', false)
            if shape != 'Convex Hull'
              if ['Null', 'Compound', 'Compound From Mesh', 'Static Mesh'].include?(shape)
                shape = 'Convex Hull'
              end
              scale = Geometry.get_scale(e.transformation)
              shape = 'Convex Hull' if scale != [1,1,1]
            end
            child_data[e] = [shape, no_collision]
          }
          create_compound(world, ent, child_data)
        when :compound_from_mesh
          create_compound_from_mesh(world, ent, true, 2)
        when :static_mesh
          create_static_mesh(world, ent, true, 2)
        when :deformable
          create_deformable(world, ent, true, 2)
        else
          raise 'Failed to create collision!'
      end
      save_collision(world, ent, shape, col)
      col
    end

    # @!visibility private
    def init_collision(world, ent, transform = false)
      unless VALID_TYPES.include?(ent.class)
        raise ArgumentError, INVALID_OBJECT
      end
      bb = MSPhysics::Group.get_bounding_box_from_faces(ent, true, false)
      if bb.depth.zero? or bb.height.zero? or bb.width.zero?
        raise TypeError, INVALID_SHAPE
      end
      scale = Geometry.get_scale(ent.transformation)
      pt = bb.center
      w = bb.width.to_m
      h = bb.height.to_m
      d = bb.depth.to_m
      if transform
        pt.transform!(ent.transformation)
        w *= scale[0]
        h *= scale[1]
        d *= scale[2]
      else
        pt.x *= scale[0]
        pt.y *= scale[1]
        pt.z *= scale[2]
      end
      c = Conversion.convert_point(pt, :in, :m)
      if transform
        tra = Geometry.extract_scale(ent.transformation)
        offset_matrix = Geom::Transformation.new(tra.xaxis, tra.yaxis, tra.zaxis, c)
      else
        offset_matrix = Geom::Transformation.new(c)
      end
      binding
    end

    # Create box.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] transform
    # @return [AMS::FFI::Pointer]
    def create_box(world, ent, transform = false)
      binding = init_collision(world, ent, transform)
      code = "col = Newton.createBox(world, w, h, d, 0, offset_matrix.to_a.pack('F*'));"
      code << "Newton.collisionSetScale(col, *scale) unless transform; col"
      eval(code, binding)
    end

    # Create sphere.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] transform
    # @return [AMS::FFI::Pointer]
    def create_sphere(world, ent, transform = false)
      binding = init_collision(world, ent, transform)
      code = "diam = w; diam = h if h > diam; diam = d if d > diam;"
      code << "col = Newton.createSphere(world, diam*0.5, 0, offset_matrix.to_a.pack('F*'));"
      code << "Newton.collisionSetScale(col, *scale) unless transform; col"
      eval(code, binding)
    end

    # Create cone.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] transform
    # @return [AMS::FFI::Pointer]
    def create_cone(world, ent, transform = false)
      binding = init_collision(world, ent, transform)
      code = "diam = h; diam = d if d > diam;"
      code << "col = Newton.createCone(world, diam*0.5, w, 0, offset_matrix.to_a.pack('F*'));"
      code << "Newton.collisionSetScale(col, *scale) unless transform; col"
      eval(code, binding)
    end

    # Create cylinder.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] transform
    # @return [AMS::FFI::Pointer]
    def create_cylinder(world, ent, transform = false)
      binding = init_collision(world, ent, transform)
      code = "diam = h; diam = d if d > diam;"
      code << "col = Newton.createCylinder(world, diam*0.5, w, 0, offset_matrix.to_a.pack('F*'));"
      code << "Newton.collisionSetScale(col, *scale) unless transform; col"
      eval(code, binding)
    end

    # Create chamfer cylinder.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] transform
    # @return [AMS::FFI::Pointer]
    def create_chamfer_cylinder(world, ent, transform = false)
      binding = init_collision(world, ent, transform)
      code = "diam = h; diam = d if d > diam;"
      code << "col = Newton.createChamferCylinder(world, (diam-w)*0.5, w, 0, offset_matrix.to_a.pack('F*'));"
      code << "Newton.collisionSetScale(col, *scale) unless transform; col"
      eval(code, binding)
    end

    # Create capsule.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] transform
    # @return [AMS::FFI::Pointer]
    def create_capsule(world, ent, transform = false)
      binding = init_collision(world, ent, transform)
      code = "diam = h; diam = d if d > diam;"
      code << "col = Newton.createCapsule(world, diam*0.5, w-diam, 0, offset_matrix.to_a.pack('F*'));"
      code << "Newton.collisionSetScale(col, *scale) unless transform; col"
      eval(code, binding)
    end

    # Create convex hull.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] transform
    # @return [AMS::FFI::Pointer]
    def create_convex_hull(world, ent, transform = false)
      unless VALID_TYPES.include?(ent.class)
        raise ArgumentError, INVALID_OBJECT
      end
      pts = MSPhysics::Group.get_vertices_from_faces(ent, true, transform)
      if Geometry.points_coplanar?(pts)
        raise TypeError, INVALID_SHAPE
      end
      for i in 0...pts.size
        pts[i] = Conversion.convert_point(pts[i], :in, :m).to_a
      end
      col = Newton.createConvexHull(world, pts.size, pts.flatten.pack('F*'), 12, 0.0, 0, nil)
      unless transform
        scale = Geometry.get_scale(ent.transformation)
        Newton.collisionSetScale(col, *scale)
      end
      col
    end

    # Create null.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @return [AMS::FFI::Pointer]
    def create_null(world)
      Newton.createNull(world)
    end

    # Create compound.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Hash{entity => [shape, no_collision]}] child_data
    # @return [AMS::FFI::Pointer]
    def create_compound(world, ent, child_data = {})
      col_data = []
      child_data.each { |e, data|
        shape = data[0]
        no_collision = data[1]
        begin
          c = create(world, e, shape, true)
          Newton.collisionSetCollisonMode(c, 0) if no_collision
          col_data << c
        rescue Exception => error
          # puts "#{error}\n#{$@[0]}"
        end
      }
      if col_data.empty?
        raise TypeError, INVALID_SHAPE
      end
      col = Newton.createCompoundCollision(world, 0)
      Newton.compoundCollisionBeginAddRemove(col)
      col_data.each { |c|
        Newton.compoundCollisionAddSubCollision(col, c)
        Newton.destroyCollision(c)
      }
      Newton.compoundCollisionEndAddRemove(col)
      scale = Geometry.get_scale(ent.transformation)
      Newton.collisionSetScale(col, *scale)
      col
    end

    # Create compound from mesh.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] simplify Whether to remove unused edges.
    # @param [Fixnum] optimize Optimization mode:
    #   +0+ - none,
    #   +1+ - triangulate,
    #   +2+ - polygonize.
    # @return [AMS::FFI::Pointer]
    def create_compound_from_mesh(world, ent, simplify = true, optimize = 0)
      unless VALID_TYPES.include?(ent.class)
        raise ArgumentError, INVALID_OBJECT
      end
      faces = MSPhysics::Group.get_polygons_from_faces(ent, true, false)
      if Geometry.points_coplanar?(faces.flatten(1))
        raise TypeError, INVALID_SHAPE
      end
      mesh = Newton.meshCreate(world)
      Newton.meshBeginFace(mesh)
      faces.each { |face|
        pts = []
        face.each { |pt|
          pts.push Conversion.convert_point(pt, :in, :m).to_a
        }
        Newton.meshAddFace(mesh, pts.size, pts.flatten.pack('F*'), 12, 0)
      }
      Newton.meshEndFace(mesh)
      if simplify
        vertex_remap_array = 0.chr*4*faces.flatten.size
        Newton.removeUnusedVertices(mesh, vertex_remap_array)
      end
      if optimize == 1
        Newton.meshTriangulate(mesh)
      elsif optimize == 2
        Newton.meshPolygonize(mesh)
      end
      Newton.meshFixTJoints(mesh)
      index = Sketchup.active_model.entities.to_a.index(ent)
      puts "Generating convex approximation for entities[#{index}]..."
      # mesh, max_concavity, back_face_distance_factor, max_count, max_vertex_per_hull, progress_report_callback, report_data
      convex_approximation = Newton.meshApproximateConvexDecomposition(mesh, 0.01, 0.2, 256, 100, PROGRESS_REPORT, nil)
      col = Newton.createCompoundCollisionFromMesh(world, convex_approximation, 0.001, 0, 0)
      Newton.meshDestroy(mesh)
      Newton.meshDestroy(convex_approximation)
      save_collision(world, ent, :compound_from_mesh, col)
      scale = Geometry.get_scale(ent.transformation)
      Newton.collisionSetScale(col, *scale)
      col
    end

    # Create static mesh.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] simplify Whether to remove unused edges.
    # @param [Fixnum] optimize Optimization mode:
    #   +0+ - none,
    #   +1+ - triangulate,
    #   +2+ - polygonize.
    # @return [AMS::FFI::Pointer]
    def create_static_mesh(world, ent, simplify = true, optimize = 0)
      unless VALID_TYPES.include?(ent.class)
        raise ArgumentError, INVALID_OBJECT
      end
      faces = MSPhysics::Group.get_polygons_from_faces(ent, true, true)
      if faces.size.zero?
        raise TypeError, INVALID_SHAPE
      end
      tra = Geometry.extract_scale(ent.transformation).inverse
      mesh = Newton.meshCreate(world)
      Newton.meshBeginFace(mesh)
      faces.each { |face|
        pts = []
        face.each { |pt|
          pt.transform!(tra)
          pts.push Conversion.convert_point(pt, :in, :m).to_a
        }
        Newton.meshAddFace(mesh, pts.size, pts.flatten.pack('F*'), 12, 1)
      }
      Newton.meshEndFace(mesh)
      if simplify
        vertex_remap_array = 0.chr*4*faces.flatten.size
        Newton.removeUnusedVertices(mesh, vertex_remap_array)
      end
      Newton.meshSimplify(mesh, faces.flatten.size, PROGRESS_REPORT, nil)
      if optimize == 1
        Newton.meshTriangulate(mesh)
      elsif optimize == 2
        Newton.meshPolygonize(mesh)
      end
      Newton.meshFixTJoints(mesh)
      col = Newton.createTreeCollisionFromMesh(world, mesh, 0)
      Newton.meshDestroy(mesh)
      col
    end

    # Create soft collision.
    # @param [AMS::FFI::Pointer] world A pointer to the newton world.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    # @param [Boolean] simplify Whether to remove unused edges.
    # @param [Fixnum] optimize Optimization mode:
    #   +0+ - none,
    #   +1+ - triangulate,
    #   +2+ - polygonize.
    # @return [AMS::FFI::Pointer]
    def create_deformable(world, ent, simplify = true, optimize = 0)
      unless VALID_TYPES.include?(ent.class)
        raise ArgumentError, INVALID_OBJECT
      end
      faces = MSPhysics::Group.get_polygons_from_faces(ent, true, true)
      if faces.size.zero?
        raise TypeError, INVALID_SHAPE
      end
      tra = Geometry.extract_scale(ent.transformation).inverse
      mesh = Newton.meshCreate(world)
      Newton.meshBeginFace(mesh)
      faces.each { |face|
        pts = []
        face.each { |pt|
          pt.transform!(tra)
          pts.push Conversion.convert_point(pt, :in, :m).to_a
        }
        Newton.meshAddFace(mesh, pts.size, pts.flatten.pack('F*'), 12, 0)
      }
      Newton.meshEndFace(mesh)
      if simplify
        vertex_remap_array = 0.chr*4*faces.flatten.size
        Newton.removeUnusedVertices(mesh, vertex_remap_array)
      end
      Newton.meshSimplify(mesh, faces.flatten.size, PROGRESS_REPORT, nil)
      if optimize == 1
        Newton.meshTriangulate(mesh)
      elsif optimize == 2
        Newton.meshPolygonize(mesh)
      end
      Newton.meshFixTJoints(mesh)
      col = Newton.createDeformableMesh(world, mesh, 0)
      Newton.meshDestroy(mesh)
      Newton.deformableMeshSetSkinThickness(col, 0.05)
      Newton.deformableMeshCreateClusters(col, 8, 0.15)
      col
    end

  end # module Collision
end # module MSPhysics
