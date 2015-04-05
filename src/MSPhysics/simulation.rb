module MSPhysics

  # @since 1.0.0
  class Simulation

    @@instance ||= nil

    class << self

      # Get {Simulation} instance.
      # @return [Simulation, nil]
      def instance
        @@instance
      end

      # Determine if simulation is running.
      # @return [Boolean]
      def is_active?
        @@instance ? true : false
      end

      # Start simulation.
      # @return [Boolean] success
      def start
        return false if is_active?
        Sketchup.active_model.select_tool(Simulation.new)
        true
      end

      # Reset simulation.
      # @return [Boolean] success
      def reset
        return false unless is_active?
        Sketchup.active_model.select_tool nil
        true
      end

    end # class << self

    def initialize
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS
      @world = nil
      @update_rate = default[:update_rate]
      @update_timestep = default[:update_timestep]
      @mode = 0
      @frame = 0
      @fps = 0
      @time_info = { :start => 0, :end => 0, :last => 0, :sim => 0, :total => 0 }
      @fps_info = { :update_rate => 10, :last => 0, :change => 0 }
      @camera = { :original => nil, :follow => nil, :target => nil, :offset => nil }
      @cursor_id = MSPhysics::CURSORS[:hand]
      @cursor_pos = [0,0]
      @interactive_note = "Interactive mode: Click and drag a physics body to move. Hold SHIFT while dragging to lift."
      @game_note = "Game mode: All control over bodies and camera via mouse is restricted as the mouse is reserved for gaming."
      @general_note = "PAUSE - toggle play  ESC - reset"
      @paused = false
      @pause_updated = false
      @suspended = false
      @mouse_over = true
      @menu_enter = false
      @update_timer = nil
      @ip1 = Sketchup::InputPoint.new
      @ip = Sketchup::InputPoint.new
      @picked = []
      @clicked = nil
      @error = nil
      @saved_transformations = {}
      @mlt = nil
      @mlt_mat = nil
      @emitted_bodies = {}
      @created_entities = []
      @bb = Geom::BoundingBox.new
      @draw_queue = []
      @points_queue = []
      @ccm = false
      @show_bodies = true
      @hidden_entities = []
      @contact_points = {
        :show           => false,
        :point_size     => 3,
        :point_style    => 2,
        :point_color    => Sketchup::Color.new(153, 68, 95)
      }
      @contact_forces = {
        :show           => false,
        :line_width     => 1,
        :line_stipple   => '',
        :line_color     => Sketchup::Color.new(100, 160, 255)
      }
      @aabb = {
        :show           => false,
        :line_width     => 1,
        :line_stipple   => '',
        :line_color     => Sketchup::Color.new(68, 53, 165)
      }
      @collision_wireframe = {
        :show           => false,
        :line_width     => 1,
        :line_stipple   => '',
        :active         => Sketchup::Color.new(221, 38, 165),
        :sleeping       => Sketchup::Color.new(255, 255, 100),
        :show_edges     => nil,
        :show_profiles  => nil
      }
      @axis = {
        :show           => false,
        :line_width     => 2,
        :line_stipple   => '',
        :size           => 20,
        :xaxis          => Sketchup::Color.new(255, 0, 0),
        :yaxis          => Sketchup::Color.new(0, 255, 0),
        :zaxis          => Sketchup::Color.new(0, 0, 255)
      }
      @pick_and_drag = {
        :line_width     => 2,
        :line_stipple   => '_',
        :line_color     => Sketchup::Color.new(60, 60, 60),
        :point_size     => 10,
        :point_style    => 4,
        :point_color    => Sketchup::Color.new(4, 4, 4),
        :vline_width    => 2,
        :vline_stipple  => '',
        :vline_color    => Sketchup::Color.new(0, 40, 255)
      }
      @controller = MSPhysics::Controller.new
      @thrusters = {}
      @emitters = {}
      @buoyancy_planes = {}
      @@instance = self
    end

    # Get simulation world.
    # @return [World]
    def get_world
      @world
    end

    # Get simulation frame.
    # @return [Fixnum]
    def get_frame
      @frame
    end

    # Get simulation update rate in frames per second.
    # @return [Fixnum]
    def get_fps
      @fps
    end

    # Play simulation.
    # @return [Boolean] success
    def play
      return false unless @paused
      @paused = false
      call_event(:onPlay)
      true
    end

    # Pause simulation.
    # @return [Boolean] success
    def pause
      return false if @paused
      @paused = true
      call_event(:onPause)
      true
    end

    # Play/pause simulation.
    # @return [Boolean] success
    def toggle_play
      @paused ? play : pause
    end

    # Determine if simulation is playing.
    # @return [Boolean]
    def is_playing?
      !@paused
    end

    # Determine if simulation is paused.
    # @return [Boolean]
    def is_paused?
      @paused
    end

    # Get entity transformation update rate. Entity transformations are updated
    # every n frames.
    # @return [Fixnum]
    def get_update_rate
      @update_rate
    end

    # Set entity transformation update rate. Entity transformations are updated
    # every n frames.
    # @param [Fixnum] n
    # @return [Fixnum] The new update rate.
    def set_update_rate(n)
      @update_rate = AMS.clamp(n.to_i, 1, 100)
    end

    # Get simulation update time step in seconds.
    # @return [Numeric]
    def get_update_timestep
      @update_timestep
    end

    # Set simulation update time step in seconds.
    # @param [Numeric] time_step This value is clamped between +1/1200.0+ and
    #   +1/30.0+. Normal update time step is +1/60.0+.
    # @return [Numeric] The new update time step.
    def set_update_timestep(time_step)
      @update_timestep = AMS.clamp(time_step, 1/1200.0, 1/30.0)
    end

    # Get simulation mode.
    # * 0 - Interactive mode: The pick and drag tool and orbiting camera via the
    #   middle mouse button is enabled.
    # * 1 - Game mode: The pick and drag tool and orbiting camera via the middle
    #   mouse button is disabled.
    # @return [Fixnum]
    def get_mode
      @mode
    end

    # Set simulation mode.
    # * 0 - Interactive mode: The pick and drag tool and orbiting camera via the
    #   middle mouse button is enabled.
    # * 1 - Game mode: The pick and drag tool and orbiting camera via the middle
    #   mouse button is disabled.
    # @param [Fixnum] mode
    # @return [Fixnum] The new mode.
    def set_mode(mode)
      @mode = mode == 1 ? 1 : 0
    end

    # Get active cursor.
    # @return [Fixnum] Cursor id.
    def get_cursor
      @cursor_id
    end

    # Set active cursor.
    # @example
    #   onStart {
    #     # Set game mode.
    #     simulation.set_mode 1
    #     # Set target cursor.
    #     simulation.set_cursor MSPhysics::CURSORS[:target]
    #   }
    # @param [Fixnum] id Cursor id.
    # @return [Fixnum] The new cursor id.
    # @see MSPhysics::CURSORS
    def set_cursor(id)
      @cursor_id = id.to_i
    end

    # Get cursor position in view coordinates.
    # @return [Array<Fixnum>] +[x,y]+
    def get_cursor_pos
      @cursor_pos.dup
    end

    # Set cursor position in view coordinates.
    # @param [Fixnum] x
    # @param [Fixnum] y
    # @return [Array<Fixnum>] An array of two integer values containing the new
    #   cursor position - +[x,y]+.
    def set_cursor_pos(x,y)
      AMS::Cursor.set_pos(x.to_i, y.to_i, 2)
      @cursor_pos = AMS::Cursor.get_pos(2)
      @cursor_pos.dup
    end

    # Enable/disable the drawing of collision contact points.
    # @param [Boolean] state
    # @return [Boolean] The new state.
    def show_contact_points(state)
      @contact_points[:show] = state ? true : false
    end

    # Determine if the drawing of collision contact points is enabled.
    # @return [Boolean]
    def is_contact_points_visible?
      @contact_points[:show]
    end

    # Enable/disable the drawing of collision contact forces.
    # @param [Boolean] state
    # @return [Boolean] The new state.
    def show_contact_forces(state)
      @contact_forces[:show] = state ? true : false
    end

    # Determine if the drawing of collision contact forces is enabled.
    # @return [Boolean]
    def is_contact_forces_visible?
      @contact_forces[:show]
    end

    # Enable/disable the drawing of body world axis aligned bounding box.
    # @param [Boolean] state
    # @return [Boolean] The new state.
    def show_aabb(state)
      @aabb[:show] = state ? true : false
    end

    # Determine if the drawing of body world axis aligned bounding box is
    # enabled.
    # @return [Boolean]
    def is_aabb_visible?
      @aabb[:show]
    end

    # Enable/disable the drawing of body collision wireframe.
    # @param [Boolean] state
    # @return [Boolean] The new state.
    def show_collision_wireframe(state)
      state = state ? true : false
      return state if state == @collision_wireframe[:show]
      ro = Sketchup.active_model.rendering_options
      if state
        @collision_wireframe[:show_edges] = ro['EdgeDisplayMode']
        @collision_wireframe[:show_profiles] = ro['DrawSilhouettes']
        ro['EdgeDisplayMode'] = false
        ro['DrawSilhouettes'] = false
      else
        ro['EdgeDisplayMode'] = @collision_wireframe[:show_edges]
        ro['DrawSilhouettes'] = @collision_wireframe[:show_profiles]
      end
      @collision_wireframe[:show] = state
    end

    # Determine if the drawing of body collision wireframe is enabled.
    # @return [Boolean]
    def is_collision_wireframe_visible?
      @collision_wireframe[:show]
    end

    # Enable/disable the drawing of body centre of mass axis.
    # @param [Boolean] state
    # @return [Boolean] The new state.
    def show_axis(state)
      @axis[:show] = state ? true : false
    end

    # Determine if the drawing of body centre of mass axis is enabled.
    # @return [Boolean]
    def is_axis_visible?
      @axis[:show]
    end

    # Enable/disable recording of simulation.
    # @param [Boolean] state
    # @return [Boolean] The new state.
    def record_animation(state)
      @record_animation = state ? true : false
    end

    # Determine if recording of simulation is enabled.
    # @return [Boolean]
    def is_animation_recording?
      @record_animation
    end

    # Get continuous collision state for all bodies. Continuous collision
    # prevents bodies from passing each other at high speeds.
    # @return [Boolean]
    def get_continuous_collision_state
      @ccm
    end

    # Set continuous collision state for all bodies. Continuous collision
    # prevents bodies from passing each other at high speeds.
    # @param [Boolean] state
    # @return [Boolean] The new state.
    def set_continuous_collision_state(state)
      @ccm = state ? true : false
      body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
      while body_address
        MSPhysics::Newton::Body.set_continuous_collision_state(body_address, @ccm)
        body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
      end
      @ccm
    end

    # Show/hide all entities associated with the bodies.
    # @param [Boolean] state
    # @return [Boolean] The new state.
    def show_bodies(state)
      @show_bodies = state ? true : false
      body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
      if @show_bodies
        @hidden_entities.each { |e|
          e.visible = true if e.valid?
        }
        @hidden_entities.clear
      else
        while body_address
          data = MSPhysics::Newton::Body.get_user_data(body_address)
          if data.is_a?(MSPhysics::Body) && data.get_entity.visible?
            data.get_entity.visible = false
            @hidden_entities << data.get_entity
          end
          body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
        end
      end
      @show_bodies
    end

    # Determine if all entities associated with the bodies are visible.
    # @return [Boolean]
    def is_bodies_visible?
      @show_bodies
    end

    # Get body by group/component.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    # @return [Body, nil]
    def get_body_by_entity(entity)
      AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
      body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
      while body_address
        data = MSPhysics::Newton::Body.get_user_data(body_address)
        return data if data.is_a?(MSPhysics::Body) && data.get_entity == entity
        body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
      end
      nil
    end

    # Add a group/component to simulation.
    # @raise [TypeError] if the specified entity is already part of simulation.
    # @raise [TypeError] if the entity doesn't meet demands for being a valid
    #   physics body.
    # @raise [MSPhysics::ScriptException] if there is an error in body script.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    # @return [Body]
    def add_entity(entity)
      AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)

      if get_body_by_entity(entity)
        raise(TypeError, "Entity #{entity} is already part of simulation!", caller)
      end

      default = MSPhysics::DEFAULT_BODY_SETTINGS
      handle = 'MSPhysics Body'

      shape = entity.get_attribute(handle, 'Shape', default[:shape])
      body = MSPhysics::Body.new(@world, entity, shape)

      body.set_density entity.get_attribute(handle, 'Density', default[:density])
      body.set_static_friction entity.get_attribute(handle, 'Static Friction', default[:static_friction])
      body.set_dynamic_friction entity.get_attribute(handle, 'Dynamic Friction', default[:dynamic_friction])
      body.set_elasticity entity.get_attribute(handle, 'Elasticity', default[:elasticity])
      body.set_softness entity.get_attribute(handle, 'Softness', default[:softness])
      body.set_friction_state entity.get_attribute(handle, 'Enable Friction', default[:enable_friction])
      body.set_magnet_force entity.get_attribute(handle, 'Magnet Force', default[:magnet_force])
      body.set_magnet_range entity.get_attribute(handle, 'Magnet Range', default[:magnet_range])
      body.set_static entity.get_attribute(handle, 'Static', default[:static])
      body.set_frozen entity.get_attribute(handle, 'Frozen', default[:frozen])
      body.set_magnetic entity.get_attribute(handle, 'Magnetic', default[:magnetic])
      body.set_collidable entity.get_attribute(handle, 'Collidable', default[:collidable])
      body.set_auto_sleep_state entity.get_attribute(handle, 'Auto Sleep', default[:auto_sleep])
      body.set_continuous_collision_state entity.get_attribute(handle, 'Continuous Collision', default[:continuous_collision])
      body.set_linear_damping entity.get_attribute(handle, 'Linear Damping', default[:linear_damping])
      ad = entity.get_attribute(handle, 'Angular Damping', default[:angular_damping])
      body.set_angular_damping([ad,ad,ad])

      if entity.get_attribute(handle, 'Enable Script', default[:enable_script])
        script = entity.get_attribute('MSPhysics Script', 'Value')
        begin
          Kernel.eval(script, body.get_binding, MSPhysics::SCRIPT_NAME, 1)
        rescue Exception => e
          ref = nil
          test = MSPhysics::SCRIPT_NAME + ':'
          e.backtrace.each { |location|
            if location.include?(test)
              ref = location
              break
            end
          }
          ref = e.message if !ref && e.message.include?(test)
          line = ref ? ref.split(test, 2)[1].split(':', 2)[0].to_i : nil
          msg = "#{e.class.to_s[0] =~ /a|e|i|o|u/i ? 'An' : 'A'} #{e.class} has occurred while evaluating entity script#{line ? ', line ' + line.to_s : nil}:\n#{e.message}"
          raise MSPhysics::ScriptException.new(msg, entity, line)
        end if script.is_a?(String)
      end

      controller = entity.get_attribute(handle, 'Thruster Controller')
      if controller.is_a?(String) && !controller.empty?
        lock_axis = entity.get_attribute(handle, 'Thruster Lock Axis', default[:thruster_lock_axis])
        @thrusters[body] = { :controller => controller, :lock_axis => lock_axis }
      end
      controller = entity.get_attribute(handle, 'Emitter Controller')
      if controller.is_a?(String) && !controller.empty?
        lock_axis = entity.get_attribute(handle, 'Emitter Lock Axis', default[:emitter_lock_axis])
        rate = AMS.clamp(entity.get_attribute(handle, 'Emitter Rate', default[:emitter_rate]).to_i, 1, nil)
        lifetime = AMS.clamp(entity.get_attribute(handle, 'Emitter Lifetime', default[:emitter_lifetime]).to_i, 0, nil)
        @emitters[body] = { :controller => controller, :lock_axis => lock_axis, :rate => rate, :lifetime => lifetime }
      end

      @saved_transformations[entity] = entity.transformation
      body
    end

    # Remove a group/component from simulation.
    # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    # @return [Boolean] success
    def remove_entity(entity)
      AMS.validate_type(entity, Sketchup::Group, Sketchup::ComponentInstance)
      body = get_body_by_entity(entity)
      return false unless body
      body.destroy
      true
    end

    # @overload emit_body(body, force, life_time)
    #   Create a copy of the body and apply force to it.
    #   @param [Body] body The body to emit.
    #   @param [Geom::Vector3d, Array<Numeric>] force in Newtons.
    #   @param [Fixnum] life_time Body life time in frames. A life of 0 will
    #     give the body an endless life.
    #   @return [Body] A new body object.
    # @overload emit_body(body, transformation, force, life_time)
    #   Create a copy of the body at the specified transformation and apply
    #   force to it.
    #   @param [Body] body A body to emit.
    #   @param [Geom::Vector3d, Array<Numeric>] force A force to apply in
    #     Newtons.
    #   @param [Geom::Transformation, Array<Numeric>] transformation
    #   @param [Fixnum] life_time Body life time in frames. A life of 0 will
    #     give a new body an endless lifetime.
    #   @return [Body] A new body object.
    # @example
    #   onUpdate {
    #     # Emit body every 5 frames if key 'space' is down.
    #     if key('space') == 1 && frame % 5 == 0
    #       dir = this.get_entity.transformation.yaxis
    #       dir.length = 1000
    #       simulation.emit_body(this, dir, 100)
    #     end
    #   }
    def emit_body(*args)
      if args.size == 3
        body, force, life_time = args
      elsif args.size == 4
        body, tra, force, life_time = args
      else
        raise(ArgumentError, "Expected 3 or 4 parameters, but got #{args.size}.", caller)
      end
      life_time = life_time.to_i.abs
      new_body = args.size == 3 ? body.copy(true) : body.copy(tra, true)
      new_body.set_static(false)
      new_body.set_collidable(true)
      new_body.set_continuous_collision_state(true)
      new_body.add_force(force)
      @emitted_bodies[new_body] = life_time == 0 ? 0 : @frame + life_time
      @created_entities << new_body.get_entity
      new_body
    end

    # Destroy all emitted bodies and the entities associated with them.
    # @return [Fixnum] The number of emitted bodies destroyed.
    def destroy_all_emitted_bodies
      count = 0
      @emitted_bodies.each { |body, life|
        if body.is_valid?
          body.destroy
          count += 1
        end
      }
      @emitted_bodies.clear
      @created_entities.each { |e|
        e.erase! if e.valid?
      }
      @created_entities.clear
      count
    end

    # Erase group/component when simulation resets. This method is commonly used
    # for copied bodies. <tt>Body.#copy</tt> method doesn't register created
    # entity to the "erase" queue. When simulation resets created entities
    # remain un-deleted. To erase these entities, one could simply use this
    # method.
    # @param [Sketchup::Drawingelement] entity
    # @return [void]
    # @example Erasing copied entities.
    #   onUpdate {
    #     if frame % 10 == 0 && key('space') == 1
    #       pt = Geom::Point3d.new( rand(1000), rand(1000), rand(1000) )
    #       tra = Geom::Transformation.new(pt)
    #       body = this.copy(tra, true)
    #       simulation.erase_on_end(entity)
    #     end
    #   }
    def erase_on_end(entity)
      AMS.validate_type(entity, Sketchup::Drawingelement)
      @created_entities << entity
    end

    # Add text to the log line.
    # @param [String] text
    # @return [void]
    def log_line(text)
      @mlt.puts(text)
    end

    # Remove all text from the log line.
    # @return [void]
    def clear_log_line
      @mlt.clear
    end

    # Draw 2D geometry into view.
    # @param [String, Symbol] type Drawing type. Use one of the following:
    #   * <tt>"points"</tt> - Draw a collection of points. Each vertex is
    #     treated as a single point. Vertex n defines point n. N points are
    #     drawn.
    #   * <tt>"lines"</tt> - Draw a collection of independent lines. Each pair
    #     of vertices is treated as a single line. Vertices 2n-1 and 2n define
    #     line n. N/2 lines are drawn.
    #   * <tt>"line_strip"</tt> - Draw a connected group of line segments from
    #     the first vertex to the last. Vertices n and n+1 define line n. N-1
    #     lines are drawn.
    #   * <tt>"line_loop"</tt> - Draw a connected group of line segments from
    #     the first vertex to the last, then back to the first. Vertices n and
    #     n+1 define line n. The last line, however, is defined by vertices N
    #     and 1. N lines are drawn.
    #   * <tt>"triangles"</tt> - Draw a group of independent triangles. Each
    #     triplet of vertices is considered a single triangle. Vertices 3n-2,
    #     3n-1, and 3n define triangle n. N/3 triangles are drawn.
    #   * <tt>"triangle_strip"</tt> - Draw a connected group of triangles. One
    #     triangle is defined for each vertex presented after the first two
    #     vertices. For odd n, vertices n, n+1, and n+2 define triangle n. For
    #     even n, vertices n+1, n, and n+2 define triangle n. N-2 triangles are
    #     drawn.
    #   * <tt>"triangle_fan"</tt> - Draw a connected group of triangles. One
    #     triangle is defined for each vertex presented after the first two
    #     vertices. Vertices 1, n+1, and n+2 define triangle n. N-2 triangles
    #     are drawn.
    #   * <tt>"quads"</tt> - Draw a collection of independent quadrilaterals. A
    #     group of four vertices is treated as a single quadrilateral. Vertices
    #     4n-3, 4n-2, 4n-1, and 4n define quadrilateral n. N/4 quadrilaterals
    #     are drawn.
    #   * <tt>"quad_strip"</tt> - Draw a collection of connected quadrilaterals.
    #     One quadrilateral is defined for each pair of vertices presented after
    #     the first pair. Vertices 2n-1, 2n, 2n+2, and 2n+1 define quadrilateral n.
    #     N/2-1 quadrilaterals are drawn. Note that the order in which
    #     vertices are used to construct a quadrilateral from strip data is
    #     different from that used with independent data.
    #   * <tt>"polygon"</tt> - Draws a single convex polygon. Vertices 1 through
    #     N define this polygon.
    # @param [Array<Geom::Point3d, Array<Numeric>>] points An array of points.
    # @param [Sketchup::Color, Array, String] color Drawing color.
    # @param [Fixnum] width Line width in pixels.
    # @param [String] stipple Line stipple. Use one of the following:
    #  * <tt>"."</tt> - dotted line
    #  * <tt>"-"</tt> - short-dashed line
    #  * <tt>"_"</tt> - long-dashed line
    #  * <tt>"-.-"</tt> - dash dot dash line
    #  * <tt>""</tt> - solid line
    # @return [void]
    def draw2d(type, points, color = 'black', width = 1, stipple = '')
      type = case type.to_s.downcase.gsub(/\s/i, '_').to_sym
        when :points
          GL_POINTS
        when :lines
          GL_LINES
        when :line_strip
          GL_LINE_STRIP
        when :line_loop
          GL_LINE_LOOP
        when :triangles
          GL_TRIANGLES
        when :triangle_strip
          GL_TRIANGLE_STRIP
        when :triangle_fan
          GL_TRIANGLE_FAN
        when :quads
          GL_QUADS
        when :quad_strip
          GL_QUAD_STRIP
        when :polygon
          GL_POLYGON
      else
        raise(TypeError, 'Invalid type!', caller)
      end
      @draw_queue << [type, points, color, width, stipple, 0]
    end

    # Draw 3D geometry into view.
    # @param (see #draw2d)
    # @return (see #draw2d)
    def draw3d(type, points, color = 'black', width = 1, stipple = '')
      type = case type.to_s.downcase.gsub(/\s/i, '_').to_sym
        when :points
          GL_POINTS
        when :lines
          GL_LINES
        when :line_strip
          GL_LINE_STRIP
        when :line_loop
          GL_LINE_LOOP
        when :triangles
          GL_TRIANGLES
        when :triangle_strip
          GL_TRIANGLE_STRIP
        when :triangle_fan
          GL_TRIANGLE_FAN
        when :quads
          GL_QUADS
        when :quad_strip
          GL_QUAD_STRIP
        when :polygon
          GL_POLYGON
      else
        raise(TypeError, 'Invalid type!', caller)
      end
      @draw_queue << [type, points, color, width, stipple, 1]
    end

    # Draw 3D points with custom style.
    # @param [Array<Geom::Point3d, Array<Numeric>>] points An array of points.
    # @param [Fixnum] size Point size in pixels.
    # @param [Fixnum] style Point style. Use one of the following:
    #   0. none
    #   1. open square
    #   2. filled square
    #   3. + cross
    #   4. x cross
    #   5. star
    #   6. open triangle
    #   7. filled triangle
    # @param [Sketchup::Color, Array, String] color Point color.
    # @param [Fixnum] width Line width in pixels.
    # @param [String] stipple Line stipple. Use one of the following:
    #  * <tt>"."</tt> - dotted line
    #  * <tt>"-"</tt> - short-dashed line
    #  * <tt>"_"</tt> - long-dashed line
    #  * <tt>"-.-"</tt> - dash dot dash line
    #  * <tt>""</tt> - solid line
    # @return [void]
    def draw_points(points, size = 1, style = 0, color = 'black', width = 1, stipple = '')
      @points_queue << [points, size, style, color, width, stipple]
    end

    # Set view full screen.
    # @param [Boolean] state
    # @return [void]
    # @example
    #   onStart {
    #     simulation.view_full_screen(true)
    #   }
    #   onEnd {
    #     simulation.view_full_screen(false)
    #   }
    def view_full_screen(state)
      AMS::Sketchup.show_toolbar_container(5, !state, false)
      AMS::Sketchup.show_scenes_bar(!state, false)
      AMS::Sketchup.show_status_bar(!state, false)
      AMS::Sketchup.set_viewport_border(!state)
      r1 = AMS::Sketchup.set_menu_bar(!state)
      r2 = AMS::Sketchup.switch_full_screen(state)
      AMS::Sketchup.refresh unless r1 || r2
      #~ AMS::Sketchup.show_dialogs(!state)
      #~ AMS::Sketchup.show_toolbars(!state)
    end

    private

    def do_on_update
      model = Sketchup.active_model
      view = model.active_view
      cam = view.camera
      # Handle simulation play/pause events.
      if @paused
        unless @pause_updated
          @pause_updated = true
          @time_info[:sim] += Time.now - @time_info[:last]
          @fps_info[:change] += Time.now - @fps_info[:last]
        end
        #~ view.show_frame
        return
      end
      if @pause_updated
        @time_info[:last] = Time.now
        @fps_info[:last] = Time.now
        @pause_updated = false
      end
      # Clear drawing queues
      @draw_queue.clear
      @points_queue.clear
      # Increment frame
      @frame += 1
      # Call onPreUpdate event
      call_event(:onPreUpdate)
      return unless Simulation.is_active?
      # Process thrusters
      @thrusters.reject! { |body, data|
        next true unless body.is_valid?
        value = nil
        begin
          value = Kernel.eval(data[:controller], @controller.get_binding, CONTROLLER_NAME, 1)
        rescue Exception => e
          puts e.inspect
        end
        if value.is_a?(Numeric)
          value = Geom::Vector3d.new(0, 0, value)
        elsif value.is_a?(Array)
          value = Geom::Vector3d.new(value)
        end
        if value.is_a?(Geom::Vector3d) && value.length != 0
          value = AMS.scale_vector(value, 1.0/@update_timestep)
          body.add_force( data[:lock_axis] ? value.transform(body.get_normal_matrix) : value )
        end
        false
      }
      # Process emitters
      @emitters.reject! { |body, data|
        next true unless body.is_valid?
        value = nil
        begin
          value = Kernel.eval(data[:controller], @controller.get_binding, CONTROLLER_NAME, 1)
        rescue Exception => e
          puts e.inspect
        end
        if value.is_a?(Numeric)
          value = Geom::Vector3d.new(0, 0, value)
        elsif value.is_a?(Array)
          value = Geom::Vector3d.new(value)
        end
        if value.is_a?(Geom::Vector3d) && value.length != 0
          value = AMS.scale_vector(value, 1.0/@update_timestep)
          if @frame % data[:rate] == 0
            self.emit_body(body, data[:lock_axis] ? value.transform(body.get_normal_matrix) : value, data[:lifetime])
          end
        end
        false
      }
      # Process buoyancy planes
      @buoyancy_planes.reject! { |entity, data|
        return true unless entity.valid?
        tra = entity.transformation
        normal = tra.zaxis
        height = tra.origin.z
        body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
        while body_address
          MSPhysics::Newton::Body.add_buoyancy(body_address, normal, height, data[:density], data[:viscosity])
          body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
        end
        false
      }
      # Update newton world
      @world.update(@update_timestep)
      # Update entities' transformations
      if @frame % @update_rate == 0
        body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
        while body_address
          if MSPhysics::Newton::Body.matrix_changed?(body_address)
            data = MSPhysics::Newton::Body.get_user_data(body_address)
            if data.is_a?(MSPhysics::Body) && data.get_entity.valid?
              data.get_entity.move!(data.get_matrix)
            end
          end
          body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
        end
      end
      # Call onTouch event
      count = MSPhysics::Newton::World.get_touch_data_count(@world.get_address)
      for i in 0...count
        data = MSPhysics::Newton::World.get_touch_data_at(@world.get_address, i)
        body1 = MSPhysics::Newton::Body.get_user_data(data[0])
        body2 = MSPhysics::Newton::Body.get_user_data(data[1])
        if body1.is_a?(MSPhysics::Body) && body2.is_a?(MSPhysics::Body)
          begin
            body1.call_event(:onTouch, body2, data[2], data[3], data[4], data[5])
          rescue Exception => e
            abort(e)
          end
          return unless Simulation.is_active?
        end
      end
      # Call onTouching event
      count = MSPhysics::Newton::World.get_touching_data_count(@world.get_address)
      for i in 0...count
        data = MSPhysics::Newton::World.get_touching_data_at(@world.get_address, i)
        body1 = MSPhysics::Newton::Body.get_user_data(data[0])
        body2 = MSPhysics::Newton::Body.get_user_data(data[1])
        if body1.is_a?(MSPhysics::Body) && body2.is_a?(MSPhysics::Body)
          begin
            body1.call_event(:onTouching, body2)
          rescue Exception => e
            abort(e)
          end
          return unless Simulation.is_active?
        end
      end
      # Call onUntouch event
      count = MSPhysics::Newton::World.get_untouch_data_count(@world.get_address)
      for i in 0...count
        data = MSPhysics::Newton::World.get_untouch_data_at(@world.get_address, i)
        body1 = MSPhysics::Newton::Body.get_user_data(data[0])
        body2 = MSPhysics::Newton::Body.get_user_data(data[1])
        if body1.is_a?(MSPhysics::Body) && body2.is_a?(MSPhysics::Body)
          begin
            body1.call_event(:onUntouch, body2)
          rescue Exception => e
            abort(e)
          end
          return unless Simulation.is_active?
        end
      end
      # Call onUpdate event
      call_event(:onUpdate)
      return unless Simulation.is_active?
      # Call onPostUpdate event
      call_event(:onPostUpdate)
      return unless Simulation.is_active?
      # Process emitted bodies.
      @emitted_bodies.reject! { |body, life_end|
        next false if life_end == 0 || @frame < life_end
        if body.is_valid?
          @created_entities.delete(body.get_entity)
          body.destroy(true)
        end
        true
      }
      # Update camera
      ent = @camera[:follow]
      if ent
        if ent.deleted?
          @camera[:follow] = nil
        else
          eye = ent.bounds.center + @camera[:offset]
          tar = eye + cam.direction.to_a
          cam.set(eye, tar, [0,0,1])
        end
      end
      ent = @camera[:target]
      if ent
        if ent.deleted?
          @camera[:target] = nil
        else
         dir = cam.eye.vector_to(ent.bounds.center)
         cam.set(cam.eye, dir, [0,0,1])
        end
      end
      # Process dragged body
      unless @picked.empty?
        if @picked[0].is_valid?
          pick_pt = @picked[1].transform(@picked[0].get_matrix)
          dest_pt = @picked[2]
          MSPhysics::Newton::Body.apply_pick_and_drag(@picked[0].get_address, pick_pt, dest_pt, 120, 10)
          #Newton::Body.apply_pick_and_drag2(@picked[0].get_address, pick_pt, dest_pt, 0.3, 0.95, @update_timestep)
        else
          @picked.clear
        end
      end
      # Update FPS
      if @frame % @fps_info[:update_rate] == 0
        @fps_info[:change] += Time.now - @fps_info[:last]
        @fps = ( @fps_info[:change] == 0 ? 0 : (@fps_info[:update_rate] / @fps_info[:change]).round )
        @fps_info[:last] = Time.now
        @fps_info[:change] = 0
      end
      # Update status bar text
      update_status_text
      # Redraw view
      view.show_frame
    end

    def draw_contact_points(view)
      return unless @contact_points[:show]
      body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
      while body_address
        points = MSPhysics::Newton::Body.get_contact_points(body_address, true)
        if points.size > 0
          view.draw_points(points, @contact_points[:point_size], @contact_points[:point_style], @contact_points[:point_color])
        end
        body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
      end
    end

    def draw_contact_forces(view)
      return unless @contact_forces[:show]
      view.drawing_color = @contact_forces[:line_color]
      view.line_width = @contact_forces[:line_width]
      view.line_stipple = @contact_forces[:line_stipple]
      body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
      while body_address
        mass = MSPhysics::Newton::Body.get_mass(body_address)
        if mass > 0
          MSPhysics::Newton::Body.get_contacts(body_address, false).each { |contact|
            for i in 0..2
              x = contact[3][i] / mass.to_f
              next if x.abs < 1
              pt = contact[1].clone
              pt[i] += x
              view.draw(GL_LINES, [contact[1], pt])
            end
          }
        end
        body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
      end
    end

    def draw_collision_wireframe(view)
      return unless @collision_wireframe[:show]
      view.line_width = @collision_wireframe[:line_width]
      view.line_stipple = @collision_wireframe[:line_stipple]
      body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
      while body_address
        sleeping = MSPhysics::Newton::Body.is_sleeping?(body_address)
        view.drawing_color = @collision_wireframe[sleeping ? :sleeping : :active]
        MSPhysics::Newton::Body.get_collision_faces(body_address).each { |face|
          view.draw(GL_LINE_LOOP, face)
        }
        body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
      end
    end

    def draw_axis(view)
      return unless @axis[:show]
      view.line_width = @axis[:line_width]
      view.line_stipple = @axis[:line_stipple]
      body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
      while body_address
        pos = MSPhysics::Newton::Body.get_position(body_address, 1)
        tra = MSPhysics::Newton::Body.get_matrix(body_address)
        # Draw xaxis
        l = tra.xaxis
        l.length = @axis[:size]
        pt = pos + l
        view.drawing_color = @axis[:xaxis]
        view.draw_line(pos, pt)
        # Draw yaxis
        l = tra.yaxis
        l.length = @axis[:size]
        pt = pos + l
        view.drawing_color = @axis[:yaxis]
        view.draw_line(pos, pt)
        # Draw zaxis
        l = tra.zaxis
        l.length = @axis[:size]
        pt = pos + l
        view.drawing_color = @axis[:zaxis]
        view.draw_line(pos, pt)
        # Get next body
        body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
      end
    end

    def draw_aabb(view)
      return unless @aabb[:show]
      view.drawing_color = @aabb[:line_color]
      view.line_width = @aabb[:line_width]
      view.line_stipple = @aabb[:line_stipple]
      body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
      while body_address
        min, max = MSPhysics::Newton::Body.get_aabb(body_address)
        view.draw(GL_LINE_LOOP, [min, [min.x, max.y, min.z], [max.x, max.y, min.z], [max.x, min.y, min.z]])
        view.draw(GL_LINE_LOOP, [[min.x, min.y, max.z], [min.x, max.y, max.z], max, [max.x, min.y, max.z]])
        view.draw(GL_LINES, [min, [min.x, min.y, max.z], [min.x, max.y, min.z], [min.x, max.y, max.z], [max.x, max.y, min.z], max, [max.x, min.y, min.z], [max.x, min.y, max.z]])
        body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
      end
    end

    def draw_pick_and_drag(view)
      return if @picked.empty?
      if @picked[0].is_valid?
        pt1 = @picked[1].transform(@picked[0].get_matrix)
        pt2 = @picked[2]
        @bb.add(pt2)
        view.line_width = @pick_and_drag[:line_width]
        view.line_stipple = @pick_and_drag[:line_stipple]
        view.drawing_color = @pick_and_drag[:line_color]
        view.draw_line(pt1, pt2)
        view.line_stipple = ''
        view.draw_points(pt1, @pick_and_drag[:point_size], @pick_and_drag[:point_style], @pick_and_drag[:point_color])
        if AMS::Keyboard.shift_down?
          view.line_width = @pick_and_drag[:vline_width]
          view.line_stipple = @pick_and_drag[:vline_stipple]
          view.drawing_color = @pick_and_drag[:vline_color]
          view.draw(GL_LINES, [pt2.x, pt2.y, 0], pt2)
        end
      else
        @picked.clear
      end
    end

    def draw_queues(view)
      @draw_queue.each { |type, points, color, width, stipple, mode|
        view.drawing_color = color
        view.line_width = width
        view.line_stipple = stipple
        if mode == 1
          @bb.add(points)
          view.draw(type, points)
        else
          view.draw2d(type, points)
        end
      }
      @points_queue.each{ |points, size, style, color, width, stipple|
        @bb.add(points)
        view.line_width = width
        view.line_stipple = stipple
        view.draw_points(points, size, style, color)
      }
    rescue Exception => e
      @draw_queue.clear
      @points_queue.clear
    end

    def update_status_text
      if @mouse_over && !@suspended
        change = @fps.zero? ? 0 : (1000.0/@fps).round
        Sketchup.status_text = "Frame: #{@frame}   Time: #{sprintf("%.2f", @world.get_time)} s   FPS: #{@fps}   Change: #{change} ms   Thread Count: #{@world.get_threads_count}   #{@mode == 0 ? @interactive_note : @game_note}   #{@general_note}"
      end
    end

    def call_event(evt, *args)
      return if @world.nil? || !@world.is_valid?
      body_address = MSPhysics::Newton::World.get_first_body(@world.get_address)
      while body_address
        data = MSPhysics::Newton::Body.get_user_data(body_address)
        data.call_event(evt, *args) if data.is_a?(MSPhysics::Body)
        return if @world.nil? || !@world.is_valid?
        body_address = MSPhysics::Newton::World.get_next_body(@world.get_address, body_address)
      end
    rescue Exception => e
      abort(e)
    end

    def abort(e)
      @error = e
      Simulation.reset
    end

    public

    # @!visibility private

    # SketchUp Tool Events

    def activate
      model = Sketchup.active_model
      view = model.active_view
      cam = view.camera
      default_sim = MSPhysics::DEFAULT_SIMULATION_SETTINGS
      default_buoyancy = MSPhysics::DEFAULT_BUOYANCY_PLANE_SETTINGS
      # Close active path
      state = true
      while state
        state = model.close_active
      end
      # Wrap operations
      if Sketchup.version.to_i > 6
        model.start_operation('MSPhysics Simulation', true)
      else
        model.start_operation('MSPhysics Simulation')
      end
      # Clear selection
      model.selection.clear
      # Stop any running animation
      view.animation = nil
      # Save camera orientation
      @camera[:original] = [cam.eye, cam.target, cam.up, cam.fov, cam.aspect_ratio]
      # Activate observer
      AMS::Sketchup.add_observer(self)
      # Initialize timers
      @time_info[:start] = Time.now
      @time_info[:last] = Time.now
      @fps_info[:last] = Time.now
      # Do on start
      @update_rate = default_sim[:update_rate]
      @update_timestep = default_sim[:update_timestep]
      # Create world
      @world = MSPhysics::World.new
      @world.set_solver_model(default_sim[:solver_model])
      @world.set_friction_model(default_sim[:friction_model])
      @world.set_gravity(default_sim[:gravity])
      @world.set_material_thickness(default_sim[:thickness])
      @world.set_threads_count(@world.get_max_threads_count)
      destructor = Proc.new {
        Simulation.reset
      }
      MSPhysics::Newton::World.set_destructor_proc(@world.get_address, destructor)
      # Create multi-line text
      @mlt = AMS::MultiLineText.new(10,10)
      @mlt.set_limit(10)
      @mlt.show_line_numbers(false)
      @mlt_mat = model.materials.add('MultiLineText')
      @mlt_mat.color = [0,0,220]
      @mlt.entity.material = @mlt_mat
      # Add entities
      ents = model.entities.to_a
      ents.each { |entity|
        next unless entity.is_a?(Sketchup::Group) || entity.is_a?(Sketchup::ComponentInstance)
        next unless entity.valid?
        type = entity.get_attribute('MSPhysics', 'Type', 'Body')
        if type == 'Body'
          next if entity.get_attribute('MSPhysics Body', 'Ignore')
          begin
            body = add_entity(entity)
          rescue MSPhysics::ScriptException => e
            abort(e)
            return
          rescue StandardError => e
            index = ents.index(entity)
            puts "Entity at index #{index} was not added to simulation:\n#{e.inspect}\n"
          end
        elsif type == 'Joint'
        elsif type == 'Buoyancy Plane'
          density = entity.get_attribute('MSPhysics Buoyancy Plane', 'Density', default_buoyancy[:density]).to_f
          viscosity = entity.get_attribute('MSPhysics Buoyancy Plane', 'Viscosity', default_buoyancy[:viscosity]).to_f
          @buoyancy_planes[entity] = { :density => AMS.clamp(density, 0.001, nil), :viscosity => AMS.clamp(viscosity, 0, 1) }
        end
        return unless @world
      }
      # Apply settings
      MSPhysics::Settings.apply_settings
      # Call onStart event
      call_event(:onStart)
      # Refresh view
      view.invalidate
      # Start the update timer
      t = ::UI.start_timer(0.1, false){
        ::UI.stop_timer(t)
        if Simulation.is_active?
          @update_timer = ::UI.start_timer(0, true){ do_on_update }
        end
      }
    end

    def deactivate(view)
      model = Sketchup.active_model
      # Stop any running animation
      view.animation = nil
      # Stop the update timer
      ::UI.stop_timer(@update_timer) if @update_timer
      @update_timer = nil
      # Call onEnd event
      orig_error = @error
      call_event(:onEnd)
      @error = orig_error if orig_error
      # Destroy all emitted bodies
      destroy_all_emitted_bodies
      # Destroy world
      @world.destroy if @world.is_valid?
      @world = nil
      # Erase multi-line text
      @mlt.entity.material = nil
      if model.materials.respond_to?(:remove)
        model.materials.remove(@mlt_mat)
      end
      @mlt_mat = nil
      @mlt.remove
      @mlt = nil
      # Reset entity transformations
      @saved_transformations.each { |e, t|
        e.move!(t) if e.valid?
      }
      @saved_transformations.clear
      # Show hidden entities
      @hidden_entities.each { |e|
        e.visible = true if e.valid?
      }
      @hidden_entities.clear
      # Undo changed style made by the show collision function
      show_collision_wireframe(false)
      # Remove observer
      AMS::Sketchup.remove_observer(self)
      # Finish all operations
      model.commit_operation
      # Set camera to original placement.
      cam = @camera[:original]
      view.camera.set(cam[0], cam[1], cam[2])
      view.camera.fov = cam[3]
      view.camera.aspect_ratio = cam[4]
      # Clear selection
      model.selection.clear
      # Refresh view
      view.invalidate
      # Show info
      if @error
        msg = "MSPhysics Simulation has been aborted due to an error!\n#{@error.class}: #{@error}"
        puts msg + "\n\n"
        ::UI.messagebox(msg)
        if @error.is_a?(MSPhysics::ScriptException)
          MSPhysics::Dialog.locate_error(@error)
        end
      else
        @time_info[:end] = Time.now
        @time_info[:total] = @time_info[:end] - @time_info[:start]
        @time_info[:sim] += @time_info[:end] - @time_info[:last] unless @paused
        average_fps = (@time_info[:sim].zero? ? 0 : (@frame / @time_info[:sim]).round)
        puts 'MSPhysics Simulation Results:'
        printf("  frames          : %d\n", @frame)
        printf("  average FPS     : %d\n", average_fps)
        printf("  simulation time : %.2f seconds\n", @time_info[:sim])
        printf("  total time      : %.2f seconds\n\n", @time_info[:total])
      end
      # Free some variables
      @controller = nil
      @emitters.clear
      @thrusters.clear
      @buoyancy_planes.clear
      @@instance = nil
      # Start garbage collection
      ::GC.start
    end

    def onCancel(reason, view)
      Simulation.reset
    end

    def resume(view)
      @suspended = false
      if @camera[:follow] && @camera[:follow].valid?
        @camera[:offset] = view.camera.eye - @camera[:follow].bounds.center
      end
      update_status_text
      view.invalidate
    end

    def suspend(view)
      @suspended = true
    end

    def onMouseEnter(view)
      @mouse_over = true
      return unless @menu_enter
      sel = Sketchup.active_model.selection
      sel.clear
      sel.add @camera[:follow] if @camera[:follow] && @camera[:follow].valid?
      sel.add @camera[:target] if @camera[:target] && @camera[:target].valid?
      @menu_enter = false
    end

    def onMouseLeave(view)
      @mouse_over = false
    end

    def onMouseMove(flags, x, y, view)
      @cursor_pos = [x,y]
      call_event(:onMouseMove, x, y) unless @paused
      return unless Simulation.is_active?
      @ip.pick(view, x, y)
      return if @ip == @ip1
      @ip1.copy! @ip
      # view.tooltip = @ip1.tooltip
      return if @picked.empty?
      if @mode == 1
        @picked.clear
        return
      end
      if @picked[0].is_valid?
        begin
          @picked[0].call_event(:onDrag) if @picked[4] != @frame
        rescue Exception => e
          abort(e)
          return
        end
        return unless Simulation.is_active?
        @picked[4] = @frame
      else
        @picked.clear
        return
      end
      cam = view.camera
      line = [cam.eye, @ip1.position]
      if AMS::Keyboard.shift_down?
        normal = view.camera.zaxis
        normal.z = 0
        normal.normalize!
      else
        normal = Z_AXIS
      end
      plane = [@picked[2], normal]
      vector = cam.eye.vector_to(@ip1.position)
      theta = vector.angle_between(normal).radians
      if (90 - theta).abs > 1
        pt = Geom.intersect_line_plane(line, plane)
        v = cam.eye.vector_to(pt)
        @picked[2] = pt if cam.zaxis.angle_between(v).radians < 90
      end
      #~ @picked[2] = @ip1.position
    end

    def onLButtonDown(flags, x, y, view)
      return if @paused
      @ip1.pick(view, x, y)
      return unless @ip1.valid?
      pos = @ip1.position
      # Use ray test as it determines positions more accurate than input point.
      res = view.model.raytest( view.pickray(x,y) )
      pos = res[0] if res
      ph = view.pick_helper
      ph.do_pick(x,y)
      ent = ph.best_picked
      return unless ent.is_a?(Sketchup::Group) || ent.is_a?(Sketchup::ComponentInstance)
      body = get_body_by_entity(ent)
      return unless body
=begin
      # Correct input point position if is not located on the picked entity.
      # 1. Transform input point position relative to the deepest element
      # coordinate system.
      path = ph.path_at(0)[0..-2]
      path.each { |e|
        pos.transform!(e.transformation.inverse)
      }
      deepest = ph.leaf_at(0)
      # 2. Verify that input point is located on the picked entity. If point
      # is not on the entity, then use deepest element position as the new
      # reference to the point.
      case deepest
      when Sketchup::ConstructionPoint
        pos = deepest.position
      when Sketchup::Edge
        unless MSPhysics::Geometry.is_point_on_edge?(pos, deepest)
          pos = MSPhysics::Geometry.calc_edge_centre(deepest)
        end
      when Sketchup::Face
        unless MSPhysics::Geometry.is_point_on_face?(pos, deepest)
          pos = MSPhysics::Geometry.calc_face_centre(deepest)
        end
      end
      # 3. Transform input point back into global coordinate system for
      # implementation.
      path.reverse.each { |e|
        pos.transform!(e.transformation)
      }
=end
      # Call the onClick event.
      begin
        @clicked = body
        @clicked.call_event(:onClick, pos.clone)
      rescue Exception => e
        abort(e)
        return
      end
      return unless Simulation.is_active?
      # Pick body if the body is not static.
      return if body.is_static? or @mode == 1
      pick_pt = pos.transform(body.get_matrix.inverse)
      cc = body.get_continuous_collision_state
      body.set_continuous_collision_state(true)
      @picked = [body, pick_pt, pos, cc]
      view.lock_inference
    end

    def onLButtonUp(flags, x, y, view)
      unless @picked.empty?
        @picked[0].set_continuous_collision_state(@picked[3]) if @picked[0].is_valid?
        @picked.clear
      end
      begin
        @clicked.call_event(:onUnclick)
      rescue Exception => e
        abort(e)
      end if @clicked and @clicked.is_valid?
      @clicked = nil
    end

    def getMenu(menu)
      menu.add_item(@paused ? 'Play' : 'Pause'){
        self.toggle_play
      }
      menu.add_item('Reset'){
        Simulation.reset
      }

      menu.add_separator

      model = Sketchup.active_model
      view = model.active_view
      sel = model.selection
      ph = view.pick_helper
      ph.do_pick *@cursor_pos
      ent = ph.best_picked
      return unless ent.is_a?(Sketchup::Group) || ent.is_a?(Sketchup::ComponentInstance)
      body = get_body_by_entity(ent)
      return unless body
      @menu_enter = true
      sel.add ent
      item = menu.add_item('Camera Follow'){
        next unless body.is_valid?
        if @camera[:follow] == ent
          @camera[:follow] = nil
        else
          @camera[:follow] = ent
          @camera[:offset] = view.camera.eye - ent.bounds.center
        end
      }
      menu.set_validation_proc(item){
        @camera[:follow] == ent ? MF_CHECKED : MF_UNCHECKED
      }
      item = menu.add_item('Camera Target'){
        next unless body.is_valid?
        if @camera[:target] == ent
          @camera[:target] = nil
        else
          @camera[:target] = ent
        end
      }
      menu.set_validation_proc(item){
        @camera[:target] == ent ? MF_CHECKED : MF_UNCHECKED
      }
      if @camera[:target] or @camera[:follow]
        menu.add_item('Camera Clear'){
          @camera[:follow] = nil
          @camera[:target] = nil
        }
      end

      menu.add_separator

      menu.add_item('Freeze Body'){
        next unless body.is_valid?
        body.set_frozen(true)
      }
    end

    def onSetCursor
      ::UI.set_cursor(@cursor_id)
    end

    def getInstructorContentDirectory
    end

    def getExtents
      if Sketchup.version.to_i > 6
        Sketchup.active_model.entities.each { |e|
          next if e.is_a?(Sketchup::Text)
          @bb.add(e.bounds)
        }
      end
      @bb
    end

    def draw(view)
      @bb.clear
      draw_contact_points(view)
      draw_contact_forces(view)
      draw_collision_wireframe(view)
      draw_axis(view)
      draw_aabb(view)
      draw_pick_and_drag(view)
      draw_queues(view)
      return unless Simulation.is_active?
      view.drawing_color = 'black'
      view.line_width = 5
      view.line_stipple = ''
      call_event(:onDraw, view, @bb)
    end

    # AMS SketchUp Observer

    def swo_activate
    end

    def swo_deactivate
      Simulation.reset
    end


    def swp_on_key_down(key, val, char)
      case key
        when 'escape'
          Simulation.reset
          return
        when 'pause'
          toggle_play
      end
      call_event(:onKeyDown, key, val, char) unless @paused
      1
    end

    def swp_on_key_extended(key, val, char)
      call_event(:onKeyExtended, key, val, char) unless @paused
      1
    end

    def swp_on_key_up(key, val, char)
      call_event(:onKeyUp, key, val, char) unless @paused
      1
    end


    def swp_on_lbutton_down(x,y)
      call_event(:onLButtonDown, x, y) unless @paused
      0
    end

    def swp_on_lbutton_up(x,y)
      call_event(:onLButtonUp, x, y) unless @paused
      0
    end

    def swp_on_lbutton_double_click(x,y)
      call_event(:onLButtonDoubleClick, x, y) unless @paused
      0
    end


    def swp_on_rbutton_down(x,y)
      call_event(:onRButtonDown, x, y) unless @paused
      # Prevent the menu from showing up if user selects anything, other than
      # simulation bodies.
      if @mode == 0 and !@suspended
        ph = Sketchup.active_model.active_view.pick_helper
        ph.do_pick x,y
        ent = ph.best_picked
        return 1 unless ent.is_a?(Sketchup::Group) || ent.is_a?(Sketchup::ComponentInstance)
        return get_body_by_entity(ent) ? 0 : 1
      end
      @mode
    end

    def swp_on_rbutton_up(x,y)
      call_event(:onRButtonUp, x, y) unless @paused
      @mode
    end

    def swp_on_rbutton_double_click(x,y)
      call_event(:onRButtonDoubleClick, x, y) unless @paused
      @mode
    end


    def swp_on_mbutton_down(x,y)
      call_event(:onMButtonDown, x, y) unless @paused
      @mode
    end

    def swp_on_mbutton_up(x,y)
      call_event(:onMButtonUp, x, y) unless @paused
      @mode
    end

    def swp_on_mbutton_double_click(x,y)
      call_event(:onMButtonDoubleClick, x, y) unless @paused
      @mode
    end


    def swp_on_xbutton1_down(x,y)
      call_event(:onXButton1Down, x, y) unless @paused
      0
    end

    def swp_on_xbutton1_up(x,y)
      call_event(:onXButton1Up, x, y) unless @paused
      0
    end

    def swp_on_xbutton1_double_click(x,y)
      call_event(:onXButton1DoubleClick, x, y) unless @paused
      0
    end


    def swp_on_xbutton2_down(x,y)
      call_event(:onXButton2Down, x, y) unless @paused
      0
    end

    def swp_on_xbutton2_up(x,y)
      call_event(:onXButton2Up, x, y) unless @paused
      0
    end

    def swp_on_xbutton2_double_click(x,y)
      call_event(:onXButton2DoubleClick, x, y) unless @paused
      0
    end


    def swp_on_mouse_wheel_rotate(x,y, dir)
      call_event(:onMouseWheelRotate, x, y, dir) unless @paused
      @mode
    end

    def swp_on_mouse_wheel_tilt(x,y, dir)
      call_event(:onMouseWheelTilt, x, y, dir) unless @paused
      0
    end

  end # class Simulation
end # module MSPhysics
