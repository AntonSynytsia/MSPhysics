module MSPhysics
  class Particle

    # @!visibility private
    @@def2d = {}
    # @!visibility private
    @@def3d = {}
    # @!visibility private
    @@instances = []

    class << self

      # Type 1 uses Sketchup face for rendering a 2d particle.
      # Pros:
      #   * This type works in all SU versions.
      #   * Blends with all particles.
      #   * Can be shadowed.
      # Cons:
      #   * This type is slower than the second type.
      # @param [Geom::Point3d] pos Particle centre position in global space.
      # @param [Geom::Vector3d] velocity Initial velocity in meters per second.
      #   Particle velocity will be affected by the applied gravity.
      # @param [Numeric] radius Initial radius.
      # @param [Numeric] life_time Particle life in frames.
      # @param [Numeric] gravity in meters per second per second.
      # @param [Numeric] scale Scale factor per frame. Particle radius will be
      #   multiplied by the scale factor each frame.
      # @param [Sketchup::Color, String, Array] color
      # @param [Numeric] alpha Initial color alpha. Alpha blends out as frame
      #   approach life end.
      # @param [Fixnum] num_seg Number of segments diving a circular particle.
      # @param [Boolean] _3d Pass +true+ to create a spherical particle, or
      #   +false+ to create a circular particle.
      # @return [Particle]
      def create_type1(pos, velocity = [0,0,10], radius = 4, life_time = 100, gravity = -9.8, scale = 1.01, color = 'Gray', alpha = 1.0, num_seg = 12, _3d = false)
        Particle.new(pos, velocity, radius, life_time, gravity, scale, color, alpha, num_seg, _3d, 1)
      end

      # Type 2 uses view GL_POLYGON for rendering a 2d particle.
      # Pros:
      #   * Fast and simple; avoids creating materials and geometry,
      #     resulting quite faster than type 1.
      # Cons:
      #   * The transparency and color are only supported in SU8+;
      #     in SU7 particles will be completely black.
      #   * Some particles may not blend with other particles.
      # @param [Geom::Point3d] pos Particle centre position in global space.
      # @param [Geom::Vector3d] velocity Initial velocity in meters per second.
      #   Particle velocity will be affected by the applied gravity.
      # @param [Numeric] radius Initial radius.
      # @param [Numeric] life_time Particle life in frames.
      # @param [Numeric] gravity in meters per second per second.
      # @param [Numeric] scale Scale factor per frame. Particle radius will be
      #   multiplied by the scale factor each frame.
      # @param [Sketchup::Color, String, Array] color
      # @param [Numeric] alpha Initial color alpha. Alpha blends out as frame
      #   approach life end.
      # @param [Fixnum] num_seg Number of segments diving a circular particle.
      # @param [Boolean] _3d Pass +true+ to create a spherical particle, or
      #   +false+ to create a circular particle.
      # @return [Particle]
      def create_type2(pos, velocity = [0,0,10], radius = 4, life_time = 100, gravity = -9.8, scale = 1.01, color = 'Gray', alpha = 1.0, num_seg = 12, _3d = false)
        Particle.new(pos, velocity, radius, life_time, gravity, scale, color, alpha, num_seg, _3d, 2)
      end

      # @api private
      # @param [Sketchup::View] view
      def draw_all(view)
        eye = view.camera.eye
        fx = {}
        @@instances.each { |inst|
          if inst.type == 1
            inst.draw(view)
          else
            dist = inst.pos.distance(view.camera.eye)#.round
            fx[dist] ||= []
            fx[dist] << inst
          end
        }
        keys = fx.keys
        keys.sort! { |x,y| y <=> x }
        keys.each { |dist|
          fx[dist].each { |inst | inst.draw(view) }
        }
      end

      # @api private
      def update_all
        @@instances.dup.each { |inst|
          inst.update
        }
      end

      # Remove all particles.
      def destroy_all
        @@instances.dup.each { |inst|
          inst.destroy
        }
        @@instances.clear
      end

      # Get count of all particles.
      # @return [Fixnum]
      def size
        @@instances.size
      end

    end # class << self

    # @param [Geom::Point3d] pos Particle centre position in global space.
    # @param [Geom::Vector3d] velocity Initial velocity in meters per second.
    #   Particle velocity will be affected by the applied gravity.
    # @param [Numeric] radius Initial radius.
    # @param [Numeric] life_time Particle life in frames.
    # @param [Numeric] gravity in meters per second per second.
    # @param [Numeric] scale Scale factor per frame. Particle radius will be
    #   multiplied by the scale factor each frame.
    # @param [Sketchup::Color, String, Array] color
    # @param [Numeric] alpha Initial color alpha. Alpha blends out as frame
    #   approach life end.
    # @param [Fixnum] num_seg Number of segments diving a circular particle.
    # @param [Boolean] _3d Pass +true+ to create a spherical particle, or
    #   +false+ to create a circular particle.
    # @param [Fixnum] type
    #   +1+ - use faces and geometry to render particles;
    #   +2+ - use view GL_POLYGON to render particles.
    def initialize(pos, velocity = [0,0,10], radius = 4, life_time = 100, gravity = -9.8, scale = 1.01, color = 'Gray', alpha = 1.0, num_seg = 12, _3d = false, type = 1)
      unless $msp_simulation
        raise 'You cannot create particles while MSPhysics simulation is inactive.'
      end
      @life_time = life_time.to_i
      @life_start = $msp_simulation.frame
      @life_end = @life_start + @life_time
      @pos = Geom::Point3d.new(pos)
      @velocity = Geom::Vector3d.new(velocity)
      @radius = MSPhysics.clamp(radius.to_f.abs, 0.1, nil)
      @gravity = gravity.to_f
      @scale = scale.to_f.abs
      @color = Sketchup::Color.new(color)
      @color.alpha = alpha
      @trate = @color.alpha / @life_time.to_f
      @alpha = @color.alpha
      @num_seg = MSPhysics.clamp(num_seg.to_i, 3, 200)
      @_3d = _3d ? true : false
      @num_seg += 1 if @_3d and @num_seg%2 != 0
      @type = (type == 1) ? 1 : 2
      @rotate = rand(7)*30
      @valid = true
      create_geom_instance if @type == 1
      @@instances << self
    end

    # @!attribute [r] pos
    #   @return [Geom::Point3d]

    # @!attribute [r] velocity
    #   @return [Geom::Vector3d]

    # @!attribute [r] gravity
    #   @return [Numeric]

    # @!attribute [r] scale
    #   @return [Numeric]

    # @!attribute [r] color
    #   @return [Sketchup::Color]

    # @!attribute [r] type
    #   @return [Fixnum]

    # @!attribute [r] life_start
    #   @return [Fixnum]

    # @!attribute [r] life_end
    #   @return [Fixnum]

    # @!attribute [r] life_time
    #   @return [Fixnum]


    attr_reader :pos, :velocity, :gravity, :scale, :color, :type, :life_start, :life_end, :life_time

    private

    def create_geom_instance
      model = Sketchup.active_model
      if @_3d
        if @@def3d[@num_seg].nil? or @@def3d[@num_seg].deleted?
          @@def3d[@num_seg] = model.definitions.add("MSP_P3D_#{@num_seg}")
          e = @@def3d[@num_seg].entities
          c1 = e.add_circle([0,0,0], [1,0,0], 1, @num_seg)
          c2 = e.add_circle([0,0,-10], [0,0,1], 1, @num_seg)
          c1.each { |edge| edge.hidden = true }
          f = e.add_face(c1)
          f.followme(c2)
          c2.each { |edge| edge.erase! }
        end
        cd = @@def3d[@num_seg]
        normal = [Math.cos(@rotate.degrees), Math.sin(@rotate.degrees), 0]
      else
        if @@def2d[@num_seg].nil? or @@def2d[@num_seg].deleted?
          @@def2d[@num_seg] = model.definitions.add("MSP_P2D_#{@num_seg}")
          e = @@def2d[@num_seg].entities
          c = e.add_circle([0,0,0], [0,0,1], 1, @num_seg)
          c.each { |edge| edge.hidden = true }
          e.add_face(c)
        end
        cd = @@def2d[@num_seg]
        eye = model.active_view.camera.eye
        normal = (eye.to_a == @pos.to_a) ? [0,0,1] : @pos.vector_to(eye)
      end
      tra1 = Geom::Transformation.new(@pos, normal)
      tra2 = Geom::Transformation.rotation([0,0,0], [0,0,1], @rotate.degrees)
      tra3 = Geom::Transformation.scaling(@radius)
      tra = tra1*tra2*tra3
      @material = model.materials.add('FX')
      @material.color = @color
      @material.alpha = @color.alpha/255.0
      @group = model.entities.add_instance(cd, tra)
      @group.material = @material
    end

    public

    # @api private
    def update
      return unless @valid
      if @type == 1 and (@group.deleted? or @material.deleted?)
        destroy
        return
      end
      @radius *= @scale
      if @radius < 0.1 or $msp_simulation.frame >= @life_end
        destroy
        return
      end
      uts = $msp_simulation.update_timestep
      @alpha -= @trate
      @color.alpha = @alpha.round
      @velocity.z += @gravity*uts
      @pos.x += @velocity.x.m*uts
      @pos.y += @velocity.y.m*uts
      @pos.z += @velocity.z.m*uts
      if @type == 1
        @material.color = @color
        @material.alpha = @color.alpha/255.0
        if @_3d
          normal = [Math.cos(@rotate.degrees), Math.sin(@rotate.degrees), 0]
        else
          eye = Sketchup.active_model.active_view.camera.eye
          normal = (eye.to_a == @pos.to_a) ? [0,0,1] : @pos.vector_to(eye)
        end
        tra1 = Geom::Transformation.new(@pos, normal)
        tra2 = Geom::Transformation.rotation([0,0,0], [0,0,1], @rotate.degrees)
        tra3 = Geom::Transformation.scaling(@radius)
        @group.move! tra1*tra2*tra3
      end
    end

    # @api private
    # @param [Sketchup::View] view
    def draw(view)
      return unless @valid
      if @type == 1
        return if @group.deleted? or @material.deleted?
        $msp_simulation.bb.add(@group.bounds)
        return
      end
      eye = view.camera.eye
      normal = (eye.to_a == @pos.to_a) ? [0,0,1] : @pos.vector_to(eye)
      pts = MSPhysics.points_on_circle3d(@pos, @radius, normal, @num_seg, @rotate)
      $msp_simulation.bb.add(pts)
      view.drawing_color = @color
      view.draw(GL_POLYGON, pts)
    end

    # Remove particle and its data.
    def destroy
      return unless @valid
      if @type == 1
        if @group.valid?
          @group.material = nil
          @group.erase!
        end
        mats = Sketchup.active_model.materials
        mats.remove(@material) if mats.respond_to?(:remove)
        @material = nil
        @group = nil
      end
      @valid = false
      @@instances.delete self
    end

    # Determine whether the particle is not destroyed.
    # @return [Boolean]
    def valid?
      @valid
    end

  end # class Particle

  module ParticleEffects

    # @!visibility private
    @scatter = 10

    module_function

    # Get particle scatter radius.
    # @return [Numeric]
    def scatter
      @scatter
    end

    # Set particle scatter radius.
    # @param [Numeric] value
    def scatter=(value)
      @scatter = MSPhysics.clamp(0, nil)
    end

    # Create smoke.
    # @param [Geom::Point3d] pos
    # @param [Numeric] scale
    # @param [Fixnum] density Particle count
    # @param [Sketchup::Color, String, Array] color
    # @param [Fixnum] type
    #   +1+ - Particles from geometry,
    #   +2+ - Particles from view GL_POLYGON.
    def create_smoke(pos, scale = 1, density = 20, color = 'Gray', type = 1)
      scale = MSPhysics.clamp(scale.to_i, 0.1, nil)
      density = MSPhysics.clamp(density.to_i, 1, nil)
      for i in 0...density
        pt = Geom::Point3d.new(pos)
        theta = rand(360).degrees
        dist = rand
        pt.x += Math.cos(theta)*@scatter*scale*dist
        pt.y += Math.sin(theta)*@scatter*scale*dist
        pt.z += (rand-0.5)*@scatter*0.25*scale
        v = MSPhysics.scale_vector([rand-0.5, rand-0.5, rand], scale*0.05)
        radius = (rand(5)+1)/5.0*scale
        Particle.new(pt, v, radius, 100, 0, 1.005, color, 1.0, 12, false, type)
      end
    end

    # Create dust.
    # @param (see create_smoke)
    def create_dust(pos, scale = 1, density = 20, color = 'Gray', type = 1)
    end

    # Create fire.
    # @param (see create_smoke)
    def create_fire(pos, scale = 1, density = 20, color = 'Gray', type = 1)
    end

    # Create smoking fire.
    # @param (see create_smoke)
    def create_smoking_fire(pos, scale = 1, density = 20, color = 'Gray', type = 1)
    end

    # Create simple explosion.
    # @param (see create_smoke)
    def create_explosion_sq(pos, scale = 1, density = 20, color = 'Gray', type = 1)
    end

    # Create complex explosion.
    # @param (see create_smoke)
    def create_explosion_hq(pos, scale = 1, density = 20, color = 'Gray', type = 1)
    end

  end # class ParticleEffects
end # module MSPhysics
