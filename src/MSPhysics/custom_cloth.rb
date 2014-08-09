module MSPhysics

  # CustomCloth is not a real cloth from Newton, but it still behaves quite
  # similar. Custom cloth is made of grid spaced spheres connected with the ball
  # joints. Sketchup faces play role in making the grid look like cloth.
  class CustomCloth

    # @!visibility private
    @@instances = []

    class << self

      # Update all created cloth.
      def update_all
        @@instances.each { |inst| inst.update }
      end

      # Destroy all created cloth.
      def destroy_all
        @@instances.dup.each { |inst| inst.destroy if inst.valid? }
        @@instances.clear
      end

    end

    # Create custom cloth.
    # @param [Geom::Transformation] tra Cloth position and orientation.
    #   The upper left corner of the cloth represents its origin.
    #   The width of the cloth extends on the x-axis.
    #   The height of the cloth extends on the y-axis.
    # @param [Numeric] width Cloth width in inches.
    # @param [Numeric] height Cloth height in inches.
    # @param [Fixnum] div_x The number of spheres to divide the cloth width.
    #   This value must be at least two.
    # @param [Fixnum] div_y The number of spheres to divide the cloth height.
    #   This value must be at least two.
    # @param [Numeric] thickness Cloth thickness. Cloth thickness determines the
    #   diameter of spheres. This value must be at least 0.1 inches.
    # @param [Numeric] mass Cloth mass in Kilograms.
    # @param [Sketchup::Material, NilClass] mat Set cloth material and color.
    #   Pass nil to use default material.
    def initialize(tra, width, height, div_x = 10, div_y = 10, thickness = 0.5, mass = 1, mat = nil)
      if SimulationTool.inactive?
        raise 'MSPhysics Simulation must be running in order to create cloth.'
      end
      ents = Sketchup.active_model.entities
      @tra = Geom::Transformation.new(tra)
      @width = MSPhysics.clamp(width, 1, nil)
      @height = MSPhysics.clamp(height, 1, nil)
      @div_x = MSPhysics.clamp(div_x.to_i, 2, nil)
      @div_y = MSPhysics.clamp(div_y.to_i, 2, nil)
      @thickness = MSPhysics.clamp(thickness, 0.1, @width*0.9/@div_x)
      @thickness = MSPhysics.clamp(@thickness, 0.1, @height*0.9/@div_y)
      @mass = MSPhysics.clamp(mass, 0.01, nil)
      @material = mat.is_a?(Sketchup::Material) ? mat : nil
      @visible = true
      @collidable = true
      @valid = true
      @sx = -@width.to_f/(@div_x-1)
      @sy = -@height.to_f/(@div_y-1)
      @sim = SimulationTool.instance.simulation
      # Create ball collision.
      diam = @thickness.to_m
      rad = diam/2.0
      #col1 = Newton.createSphere(@sim.world_ptr, rad, 0, nil)
      col1 = Newton.createBox(@sim.world_ptr, diam, diam, diam, 0, nil)
      col2x = Newton.createBox(@sim.world_ptr, rad, rad, @sx.to_m+diam, 0, nil)
      col2y = Newton.createBox(@sim.world_ptr, rad, rad, @sy.to_m+diam, 0, nil)
      # Create bodies with collision.
      matrix = @tra.to_a
      matrix[12..14] = Conversion.convert_point(matrix[12..14], :in, :m).to_a
      tra2 = Geom::Transformation.new(matrix)
      @bodies = []
      @body_count = @div_x*@div_y + (@div_y-1)*@div_x + (@div_x-1)*@div_y
      pmass = @mass.to_f/@body_count
      for i in 0...(@div_x*2-1)
        @bodies[i] ||= []
        for j in 0...(@div_y*2-1)
          if i%2 == 0 and j%2 == 0
            # create vertex
            col = col1
            dir = Geom::Vector3d.new(0,0,1)
          elsif i%2 == 1 and j%2 == 0
            # create horizontal stick
            col = col2x
            dir = Geom::Vector3d.new(1,0,0)
          elsif i%2 == 0 and j%2 == 1
            # create vertical stick
            col = col2y
            dir = Geom::Vector3d.new(0,1,0)
          else
            next
          end
          origin = Geom::Point3d.new(i*@sx.to_m/2.0, j*@sy.to_m/2.0, 0)
          tra = Geom::Transformation.new(origin, dir)
          buf = (tra2*tra).to_a.pack('F*')
          @bodies[i][j] = Newton.createDynamicBody(@sim.world_ptr, col, buf)
          Newton.bodySetMassProperties(@bodies[i][j], pmass, col)
          Newton.bodySetForceAndTorqueCallback(@bodies[i][j], @sim.gravity_callback)
        end
      end
      # Clear memory from collision.
      Newton.destroyCollision(col1)
      Newton.destroyCollision(col2x)
      Newton.destroyCollision(col2y)
      # Connect bodies with the ball joint.
      pin_dir = @tra.zaxis.to_a.pack('F*')
      for i in 0...(@div_x*2)
        for j in 0...(@div_y*2)
          next if i%2 != 0 or j%2 != 0
          [[i-1, j], [i, j-1], [i+1, j], [i, j+1]].each { |x, y|
            next if x < 0 or y < 0 or x > @div_x*2-2 or y > @div_y*2-2
            parent = @bodies[i][j]
            child = @bodies[x][y]
            matrix = 0.chr*64
            Newton.bodyGetMatrix(parent, matrix)
            pivot_pt = matrix.unpack('F*')[12..14].pack('F*')
            joint = Newton.constraintCreateBall(@sim.world_ptr, pivot_pt, child, parent)
            Newton.ballSetConeLimits(joint, pin_dir, 30.degrees, 30.degrees)
          }
        end
      end
      # Create faces from the polygon mesh.
      mesh = Geom::PolygonMesh.new
      for i in 0...(@div_x-1)
        for j in 0...(@div_y-1)
          pts = []
          [[i,j], [i+1,j], [i,j+1]].each { |x, y|
            pts << get_vertex_pos(x, y)
          }
          mesh.add_polygon(pts)
          pts = []
          [[i,j+1], [i+1,j], [i+1,j+1]].each { |x, y|
            pts << get_vertex_pos(x, y)
          }
          mesh.add_polygon(pts)
        end
      end
      count = ents.size
      ents.add_faces_from_mesh(mesh, 4, @material, @material)
      @edges = []
      @faces = []
      ents.to_a[count...ents.size].each { |e|
        @edges << e if e.is_a?(Sketchup::Edge)
        @faces << e if e.is_a?(Sketchup::Face)
      }
      @edges.uniq!
      @faces.uniq!
      # Get all vertices
      verts = []
      @faces.each { |e| verts.concat(e.vertices) }
      verts.uniq!
      # Organize all vertices.
      @vertices = []
      verts.each { |v|
        pos = v.position.transform(@tra.inverse)
        i = (pos.x.to_f/@sx).round
        j = (pos.y.to_f/@sy).round
        @vertices[i] ||= []
        @vertices[i][j] = v
      }
      @flatten_vertices = @vertices.flatten
      @collision = []
      @collision_iterator = Proc.new { |user_data, vertex_count, face_array, face_id|
        vc = face_array.get_array_of_float(0, vertex_count*3)
        face = []
        for i in 0...vertex_count
          face << Conversion.convert_point(vc[i*3,3], :m, :in)
        end
        @collision << face
      }
      position_texture
      @joints = []
      @joint_destructor_callback = Proc.new { |joint_ptr|
        n = @joints.flatten.index(joint_ptr)
        i = n % @div_x
        j = n / @div_x
        @joints[j][i] = nil
      }
      # Add instance to the queue.
      @@instances << self
    end

    # @!attribute [r] div_x
    #   Get the number of spheres dividing cloth width.
    #   @return [Fixnum]

    # @!attribute [r] div_y
    #   Get the number of spheres dividing cloth height.
    #   @return [Fixnum]

    # @!attribute [r] width
    #   Get cloth width in inches.
    #   @return [Numeric]

    # @!attribute [r] height
    #   Get cloth height in inches.
    #   @return [Numeric]

    # @!attribute [r] thickness
    #   Get cloth thickness in inches.
    #   @return [Numeric]


    attr_reader :div_x, :div_y, :width, :height, :thickness

    private

    def check_validity
      raise "The cloth is destroyed." unless @valid
    end

    def position_texture
      return unless @material
      return if @material.texture.nil?
      @faces.each { |e|
        if e.deleted?
          destroy
          return
        end
        pts = []
        count = 0
        e.vertices.each { |vert|
          n = @flatten_vertices.index(vert)
          i = n / @div_y
          j = n % @div_y
          pts << vert.position
          u = i * @sx / @material.texture.width.to_f
          v = j * @sy / @material.texture.height.to_f
          pts << [u,v]
        }
        e.position_material(@material, pts, true)
        e.position_material(@material, pts, false)
      }
    end

    public

    # Update cloth.
    # @api private
    def update
      check_validity
      # Transform vertices
      for i in 0...@div_x
        for j in 0...@div_y
          v = @vertices[i][j]
          if v.deleted?
            destroy
            return
          end
          desired_pos = get_vertex_pos(i, j)
          translation = v.position.vector_to(desired_pos)
          Sketchup.active_model.entities.transform_entities(translation, v)
        end
      end
      # Smooth edges
      @edges.each { |e|
        if e.deleted?
          destroy
          return
        end
        next if e.faces.size != 2
        #e.soft = true
        e.smooth = true
      }
      position_texture
    end

    # Drawn cloth collision.
    # @param [Sketchup::View] view
    def debug_collision(view)
      view.line_width = 1
      view.line_stipple = ''
      bb = MSPhysics::SimulationTool.instance.simulation.bb
      @bodies.flatten.each { |body|
        next unless body
        matrix = 0.chr*64
        Newton.bodyGetMatrix(body, matrix)
        col = Newton.bodyGetCollision(body)
        @collision.clear
        Newton.collisionForEachPolygonDo(col, matrix, @collision_iterator, nil)
        view.drawing_color = Newton.bodyGetSleepState(body) == 1 ? 'white' : 'blue'
        @collision.each { |face|
          bb.add(face)
          view.draw(GL_LINE_LOOP, face)
        }
      }
    end

    # Attach vertex to a body or space.
    # @param [Fixnum] i This value is clamped between +0+ and <tt>div_x-1</tt>.
    # @param [Fixnum] j This value is clamped between +0+ and <tt>div_y-1</tt>.
    # @param [Body, NilClass] parent
    # @return [Boolean] +true+ if connected, +false+ if vertex is already connected.
    def connect_vertex(i,j, parent = nil)
      i = MSPhysics.clamp(i.to_i, 0, @div_x-1)
      j = MSPhysics.clamp(j.to_i, 0, @div_y-1)
      @joints[j] ||= []
      return false if @joints[j][i]
      pos = Conversion.convert_point(get_vertex_pos(i,j), :in, :m).to_a.pack('F*')
      @joints[j][i] = Newton.constraintCreateBall(@sim.world_ptr, pos, @bodies[i*2][j*2], parent ? parent._body_ptr : nil)
      Newton.jointSetDestructor(@joints[j][i], @joint_destructor_callback)
      true
    end

    # Disconnect vertex from a body or space.
    # @param [Fixnum] i This value is clamped between +0+ and <tt>div_x-1</tt>.
    # @param [Fixnum] j This value is clamped between +0+ and <tt>div_y-1</tt>.
    # @return [Boolean] success
    def disconnect_vertex(i,j)
      i = MSPhysics.clamp(i.to_i, 0, @div_x-1)
      j = MSPhysics.clamp(j.to_i, 0, @div_y-1)
      @joints[j] ||= []
      return false unless @joints[j][i]
      Newton.destroyJoint(@sim.world_ptr, @joints[j][i])
      true
    end

    # Disconnect all connected joints.
    # @return [Numeric] count
    def disconnect_all
      count = 0
      @joints.flatten.compact.each { |jnt|
        Newton.destroyJoint(@sim.world_ptr, jnt)
        count += 1
      }
      @joints.clear
      count
    end

    # Get position of the particular body on the grid.
    # @param [Fixnum] i This value is clamped between +0+ and <tt>div_x-1</tt>.
    # @param [Fixnum] j This value is clamped between +0+ and <tt>div_y-1</tt>.
    # @return [Geom::Point3d, NilClass]
    def get_vertex_pos(i, j)
      check_validity
      i = MSPhysics.clamp(i.to_i, 0, @div_x-1)
      j = MSPhysics.clamp(j.to_i, 0, @div_y-1)
      return unless @bodies[i*2][j*2]
      matrix = 0.chr*64
      Newton.bodyGetMatrix(@bodies[i*2][j*2], matrix)
      Conversion.convert_point(matrix.unpack('F*')[12..14], :m, :in)
    end

    # Set position of the particular body on the grid.
    # @param [Fixnum] i This value is clamped between +0+ and <tt>div_x-1</tt>.
    # @param [Fixnum] j This value is clamped between +0+ and <tt>div_y-1</tt>.
    # @param [Geom::Point3d] pos
    # @return [Boolean] success
    def set_vertex_pos(i, j, pos)
      check_validity
      i = MSPhysics.clamp(i.to_i, 0, @div_x-1)
      j = MSPhysics.clamp(j.to_i, 0, @div_y-1)
      return false unless @bodies[i*2][j*2]
      matrix = Geom::Transformation.new().to_a
      matrix[12..14] = Conversion.convert_point(pos, :in, :m).to_a
      Newton.bodySetMatrix(@bodies[i*2][j*2], matrix.to_a.pack('F*'))
      true
    end

    # Get cloth mass in Kilograms.
    # @return [Numeric]
    def mass
      check_validity
      @mass
    end

    # Set cloth mass in Kilograms.
    # @param [Numeric] value
    def mass=(value)
      check_validity
      @mass = MSPhysics.clamp(mass, 0.01, nil)
      pmass = @mass.to_f/@body_count
      @bodies.flatten.each{ |body|
        col = Newton.bodyGetCollision(body)
        Newton.bodySetMassProperties(@bodies[i][j], pmass, col)
      }
    end

    # Get cloth material.
    # @return [Sketchup::Material, NilClass]
    def material
      check_validity
      @material
    end

    # Set cloth material.
    # @param [Sketchup::Material, NilClass] mat
    def material=(mat)
      check_validity
      @material = mat.is_a?(Sketchup::Material) ? mat : nil
      @faces.each { |face|
        face.back_material = @material
        face.material = @material
      }
    end

    # Show/Hide cloth.
    # @param [Boolean] state
    def visible=(state)
      check_validity
      @visible = state ? true : false
      @faces.each { |face| face.visible = @visible }
      @edges.each { |edge| edge.visible = @visible }
    end

    # Determine whether the cloth is visible.
    # @return [Boolean]
    def visible?
      check_validity
      @visible
    end

    # Make cloth collidable or not collidable.
    # @param [Boolean] state
    def collidable=(state)
      check_validity
      @collidable = state ? true : false
      state = @collidable ? 1 : 0
      for i in 0...(@div_x*2-1)
        for j in 0...(@div_y*2-1)
          body_ptr = @bodies[i][j]
          next unless body_ptr
          col = Newton.bodyGetCollision(body_ptr)
          Newton.collisionSetCollisionMode(col, state)
        end
      end
    end

    # Determine whether the cloth is collidable.
    # @return [Boolean]
    def collidable?
      check_validity
      @collidable
    end

    # Destroy the cloth.
    def destroy
      check_validity
      @edges.each { |e| e.erase! if e.valid? }
      @edges.clear
      @faces.clear
      @vertices.clear
      @flatten_vertices.clear
      @bodies.flatten.each { |body|
        next unless body
        Newton.destroyBody(body)
      }
      @bodies.clear
      @joints.clear
      @valid = false
      @@instances.delete(self)
    end

    # Determine whether the cloth is not destroyed.
    def valid?
      @valid
    end

  end # class CustomCloth
end # module MSPhysics
