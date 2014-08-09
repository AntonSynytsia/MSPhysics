module MSPhysics
  # Newton cloth. Newton cloth is not fully complete yet. Collisions don't work,
  # positions are unchangeable yet.
  # @example
  #   onStart {
  #     tra = Geom::Transformation.new([0,0,0], [0,0,1])
  #     @cloth = Cloth.new(tra, 10, 16, 16, 0.5, 100, 5000, 10, 200)
  #     @cloth.connect(0,0)
  #     @cloth.connect(16,0)
  #   }
  #   onUpdate {
  #     @cloth.update
  #   }
  #   onDraw { |view, bb|
  #     @cloth.render(view, bb) if key('e') == 1
  #   }
  class Cloth

    # @param [Geom::Transformation, Array<Numeric>] tra Cloth transformation.
    #   Cloth width extends on the xaxis.
    #   Cloth height extends on the yaxis.
    # @param [Numeric] spacing grid offset
    # @param [Fixnum] grid_x grid width is determined by spacing multiplied by
    #   the grid_x parameter.
    # @param [Fixnum] grid_y grid height is determined by spacing multiplied by
    #   the grid_y parameter.
    # @param [Numeric] thickness skin thickness
    # @param [Numeric] s_damper structural damper
    # @param [Numeric] s_stiffness structural stiffness
    # @param [Numeric] b_damper bend damper
    # @param [Numeric] b_stiffness bend stiffness
    def initialize(tra, spacing = 1, grid_x = 24, grid_y = 24, thickness = 0.5, s_damper = 100, s_stiffness = 5000, b_damper = 10, b_stiffness = 200)
      mat = Geom::Transformation.new(tra).to_a
      mat[12..14] = Conversion.convert_point(mat[12..14], :in, :m).to_a
      tra = Geom::Transformation.new(mat)

      sim = SimulationTool.instance.simulation
      world = sim.world_ptr

      @spacing = MSPhysics.clamp(spacing, 0.1, 1000)
      @grid_x = MSPhysics.clamp(grid_x.to_i, 1, 10000)
      @grid_y = MSPhysics.clamp(grid_y.to_i, 1, 10000)
      @s_damper = MSPhysics.clamp(s_damper, 0.1, nil)
      @s_stiffness = MSPhysics.clamp(s_stiffness, 0.1, nil)
      @b_damper = MSPhysics.clamp(b_damper, 0.1, nil)
      @b_stiffness = MSPhysics.clamp(b_stiffness, 0.1, nil)
      @thickness = MSPhysics.clamp(thickness, 0.01, 100)
      @width = @grid_x * @spacing
      @height = @grid_y * @spacing
      @connected = []

      points = Array.new(@grid_x+1){ Array.new(@grid_y+1) }

      enumerator = 0
      for i in 0...(@grid_x+1)
        x = @spacing.to_m * i
        for j in 0...(@grid_y+1)
          y = @spacing.to_m * j
          pt = Geom::Point3d.new(x,y,0).transform(tra)
          points[i][j] = [pt.x, pt.y, pt.z, enumerator]
          enumerator += 1
        end
      end

      face = Array.new(3){ Array.new(4) }

      # Create mesh
      mesh = Newton.meshCreate(world)
      Newton.meshBeginFace(mesh)
      for i in 0...@grid_x
        for j in 0...@grid_y
          face[0][0] = points[i][j][0]
          face[0][1] = points[i][j][1]
          face[0][2] = points[i][j][2]
          face[0][3] = points[i][j][3]

          face[1][0] = points[i][j+1][0]
          face[1][1] = points[i][j+1][1]
          face[1][2] = points[i][j+1][2]
          face[1][3] = points[i][j+1][3]

          face[2][0] = points[i+1][j+1][0]
          face[2][1] = points[i+1][j+1][1]
          face[2][2] = points[i+1][j+1][2]
          face[2][3] = points[i+1][j+1][3]

          Newton.meshAddFace(mesh, 3, face.flatten.pack('F*'), 16, 0)

          face[0][0] = points[i][j][0]
          face[0][1] = points[i][j][1]
          face[0][2] = points[i][j][2]
          face[0][3] = points[i][j][3]

          face[1][0] = points[i+1][j+1][0]
          face[1][1] = points[i+1][j+1][1]
          face[1][2] = points[i+1][j+1][2]
          face[1][3] = points[i+1][j+1][3]

          face[2][0] = points[i+1][j][0]
          face[2][1] = points[i+1][j][1]
          face[2][2] = points[i+1][j][2]
          face[2][3] = points[i+1][j][3]

          Newton.meshAddFace(mesh, 3, face.flatten.pack('F*'), 16, 0)
        end
      end
      Newton.meshEndFace(mesh)

      # Create collision
      structural_mat = Newton::ClothPatchMaterial.new
      structural_mat[:damper] = @s_damper
      structural_mat[:stiffness] = @s_stiffness

      bend_mat = Newton::ClothPatchMaterial.new
      bend_mat[:damper] = @b_damper
      bend_mat[:stiffness] = @b_stiffness

      soft_collision = Newton.createClothPatch(world, mesh, 0, structural_mat, bend_mat)
      Newton.meshDestroy(mesh)

      @particle_count = Newton.deformableMeshGetParticleCount(soft_collision)

      @render_pts = Array.new(@particle_count)

      # Organize vertex indices
      @indices = Array.new(@grid_x+1){ Array.new(@grid_y+1) }
      for index in 0...@particle_count
        buf = 0.chr*12
        Newton.deformableMeshGetParticlePosition(soft_collision, index, buf)
        pos = Geom::Point3d.new(buf.unpack('F*')).transform(tra.inverse)
        i = (pos.x.to_f/@spacing.to_m).round
        j = (pos.y.to_f/@spacing.to_m).round
        @indices[i][j] = index
      end
      Newton.deformableMeshSetSkinThickness(soft_collision, @thickness.to_m)

      @soft_col = soft_collision
      connect(0,0)
      connect(@grid_x,0)

      # Create body
      @body_ptr = Newton.createDeformableBody(world, soft_collision, tra.to_a.pack('F*'))
      Newton.destroyCollision(soft_collision)
      @soft_col = nil
      @col_ptr = Newton.bodyGetCollision(@body_ptr)

      mass = 8
      Newton.bodySetMassProperties(@body_ptr, mass, @col_ptr)

      Newton.bodySetForceAndTorqueCallback(@body_ptr, sim.gravity_callback)

      @destructor_callback = Proc.new { |body_ptr|
        @body_ptr = nil
        Newton.destroyCollision(@col_ptr)
        @col_ptr = nil
        @edges.each { |e| e.erase! if e.valid? }
        @edges.clear
        @vertices.clear
      }
      Newton.bodySetDestructorCallback(@body_ptr, @destructor_callback)

      # Create visual mesh
      # Create faces from the polygon mesh.
      mesh = Geom::PolygonMesh.new
      for i in 0...@grid_x
        for j in 0...@grid_y
          pts = []
          [[i,j], [i,j+1], [i+1,j+1]].each { |x,y|
            pts << Conversion.convert_point(points[x][y], :m, :in)
          }
          mesh.add_polygon(pts)
          pts = []
          [[i,j], [i+1,j+1], [i+1,j]].each { |x,y|
            pts << Conversion.convert_point(points[x][y], :m, :in)
          }
          mesh.add_polygon(pts)
        end
      end
      ents = Sketchup.active_model.entities
      count = ents.size
      ents.add_faces_from_mesh(mesh, 4, nil, nil)
      @edges = ents.to_a[count...ents.size].grep(Sketchup::Edge)
      @edges.uniq!
      # Get all vertices
      verts = []
      @edges.each { |e| verts.concat(e.vertices) }
      verts.uniq!
      # Organize all vertices.
      @vertices = {}
      for i in 0...@particle_count
        buf = 0.chr*12
        Newton.deformableMeshGetParticlePosition(@col_ptr, i, buf)
        pt = Conversion.convert_point(buf.unpack('F*'), :m, :in)
        verts.each { |v|
          if v.position.distance(pt) < 0.01
            @vertices[i] = v
            break
          end
        }
      end
    end


    # @!attribute [r] body_ptr
    #   @return [AMS::FFI::Pointer, NilClass]

    # @!attribute [r] width
    #   Get cloth width in inches (grid_x * spacing).
    #   @return [Fixnum]

    # @!attribute [r] height
    #   Get cloth height in inches (grid_y * spacing).
    #   @return [Fixnum]

    # @!attribute [r] spacing
    #   Get cloth grid spacing in inches.
    #   @return [Numeric]

    # @!attribute [r] grid_x
    #   Get cloth grid x count.
    #   @return [Fixnum]

    # @!attribute [r] grid_y
    #   Get cloth grid y count.
    #   @return [Fixnum]


    attr_reader :body_ptr, :width, :height, :spacing, :grid_x, :grid_y

    private

    def check_validity
      raise 'The cloth is destroyed!' unless valid?
    end

    def get_index_by_pos(i, j)

    end

    public

    # Get cloth material thickness in inches.
    # @return [Numeric]
    def thickness
      @thickness
    end

    # Set cloth material thickness in inches.
    # @param [Numeric] value
    def thickness=(value)
      @thickness = MSPhysics.clamp(value, 0.01, 100)
      Newton.deformableMeshSetSkinThickness(@col_ptr, @thickness.to_m)
    end

    # Draw all vertex points.
    def render(view, bb)
      check_validity
=begin
      # Regenerate the soft body mesh for rendering.
      Newton.deformableMeshUpdateRenderNormals(@col_ptr)
      # Update the vertex array.
      count = Newton.deformableMeshGetVertexCount(@col_ptr)
      vertices = 0.chr*12*count
      normals = 0.chr*12*count
      uv = 0.chr*8*count
      Newton.deformableMeshGetVertexStreams(@col_ptr, 12, vertices, 12, normals, 8, uv)
      vertices = vertices.unpack('F*')
      normals = normals.unpack('F*')
      view.drawing_color = 'purple'
      view.line_width = 1
      view.line_stipple = ''
      for i in 0...(vertices.size/3)
        x = vertices[i*3+0]
        y = vertices[i*3+1]
        z = vertices[i*3+2]
        pt1 = Conversion.convert_point([x,y,z], :m, :in)
        x = normals[i*3+0]
        y = normals[i*3+1]
        z = normals[i*3+2]
        v = MSPhysics.scale_vector([x,y,z], 3)
        pt2 = pt1 + v
        view.draw(GL_LINES, [pt1, pt2])
        #view.draw_points(pt, 10, 0, 'red')
      end
=end
      for i in 0...@particle_count
        buf = 0.chr*12
        Newton.deformableMeshGetParticlePosition(@col_ptr, i, buf)
        @render_pts[i] = Conversion.convert_point(buf.unpack('F*'), :m, :in)
      end
      bb.add @render_pts
      view.draw_points(@render_pts, 5, 5, 'purple')
      nil
    end

    # Transform all faces.
    def update
      check_validity
      translations = Array.new(@particle_count)
      for i in 0...@particle_count
        buf = 0.chr*12
        Newton.deformableMeshGetParticlePosition(@col_ptr, i, buf)
        pt = Conversion.convert_point(buf.unpack('F*'), :m, :in)
        v = @vertices[i]
        if v.deleted?
          destroy
          return
        end
        translations[i] = v.position.vector_to(pt)
      end
      Sketchup.active_model.entities.transform_by_vectors(@vertices.values, translations)
      # Smooth edges
      @edges.each { |e|
        if e.deleted?
          destroy
          return
        end
        next if e.faces.size != 2
        e.smooth = true
      }
      nil
    end

    # Constraint vertex.
    # @param [Fixnum] i This value is clamped between 0 and grid_x.
    # @param [Fixnum] j This value is clamped between 0 and grid_y.
    # @param [Geom::Point3d] pos Attach position. Pas nil to attach at vertex
    #   location.
    # @param [Body] parent Parent body. Pas nil to attach to space.
    # @return [Boolean] success
    def connect(i, j, pos = nil, parent = nil)
      #check_validity
      i = MSPhysics.clamp(i.to_i, 0, @grid_x)
      j = MSPhysics.clamp(j.to_i, 0, @grid_y)
      index = @indices[i][j]
      return false if @connected.include?(index)
      if pos
        pos = MSPhysics.convert_point(pos, :in, :m).to_a.pack('F*')
      else
        pos = 0.chr*12
        Newton.deformableMeshGetParticlePosition(@soft_col, index, pos)
      end
      if parent
        MSPhysics.validate_type(parent, MSPhysics::Body)
        parent.check_validity
        parent = parent._body_ptr
      else
        parent = nil
      end
      Newton.deformableMeshBeginConfiguration(@soft_col)
      Newton.deformableMeshConstraintParticle(@soft_col, index, pos, parent)
      Newton.deformableMeshEndConfiguration(@soft_col)
      @connected << index
      true
    end

    # Unconstraint particular vertex.
    # @param [Fixnum] i This value is clamped between 0 and grid_x.
    # @param [Fixnum] j This value is clamped between 0 and grid_y.
    # @return [Boolean] success
    def disconnect(i, j)
      check_validity
      i = MSPhysics.clamp(i.to_i, 0, @grid_x)
      j = MSPhysics.clamp(j.to_i, 0, @grid_y)
      index = @indices[i][j]
      return false unless @connected.include?(index)
      Newton.deformableMeshBeginConfiguration(@col_ptr)
      Newton.deformableMeshUnconstraintParticle(@col_ptr, index)
      Newton.deformableMeshEndConfiguration(@col_ptr)
      @connected.delete(index)
      true
    end

    # Unconstraint all vertices.
    # @return [Fixnum] the number of vertices disconnected.
    def disconnect_all
      check_validity
      count = @connected.size
      Newton.deformableMeshBeginConfiguration(@col_ptr)
      @connected.each { |index|
        Newton.deformableMeshUnconstraintParticle(@col_ptr, index)
      }
      Newton.deformableMeshEndConfiguration(@col_ptr)
      @connected.clear
      count
    end

    # Destroy cloth.
    def destroy
      check_validity
      Newton.destroyBody(@body_ptr)
    end

    # Determine whether the cloth is destroyed.
    # @return [Boolean]
    def valid?
      @body_ptr ? true : false
    end

  end # class Cloth
end # module MSPhysics
