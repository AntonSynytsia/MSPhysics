# @since 1.0.0
module MSPhysics::Settings

  @debug_collision = false
  @debug_axes = false
  @debug_aabb = false
  @debug_contact_points = false
  @debug_contact_forces = false
  @debug_bodies = true

  @settings = {}

  class << self

    # @!visibility private
    def init
      Sketchup.add_observer(MSPhysics::Settings::AppObserver.new)
      self.load_options
    end

    # Save settings to registry
    def save_options
      Sketchup.write_default('MSPhysics', 'Settings', @settings.inspect.inspect[1..-2])
    end

    # Load settings from registry
    def load_options
      v = Sketchup.read_default('MSPhysics', 'Settings', '{}')
      begin
        res = eval(v)
        @settings.merge!(res) if res.is_a?(Hash)
      rescue Exception => e
        # Do nothing; use default options.
      end
    end

    # Get the state of keyboard navigation.
    # @return [Integer] State:
    #   * 0 - off/stop
    #   * 1 - normal
    #   * 2 - upright
    def key_nav_state
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:key_nav_state]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Keyboard Nav State', default)
      return AMS.clamp(attr.to_i, 0, 2)
    end

    # Set the state of keyboard navigation.
    # @param [Integer] state State:
    #   * 0 - off/stop
    #   * 1 - normal
    #   * 2 - upright
    def key_nav_state=(state)
      state = AMS.clamp(state.to_i, 0, 2)
      Sketchup.active_model.set_attribute('MSPhysics', 'Keyboard Nav State', state)
    end

    # Get the maximum velocity of keyboard camera navigation in m/s.
    # @return [Numeric]
    def key_nav_velocity
      if @settings.has_key?(:key_nav_velocity)
        @settings[:key_nav_velocity]
      else
        MSPhysics::DEFAULT_SIMULATION_SETTINGS[:key_nav_velocity]
      end
    end

    # Set the maximum velocity of keyboard camera navigation in m/s.
    # @param [Numeric] value
    def key_nav_velocity=(value)
      @settings[:key_nav_velocity] = AMS.clamp(value.to_f, 0.01, nil)
    end

    # Get the maximum omega of keyboard camera navigation in rad/s.
    # @return [Numeric]
    def key_nav_omega
      if @settings.has_key?(:key_nav_omega)
        @settings[:key_nav_omega]
      else
        MSPhysics::DEFAULT_SIMULATION_SETTINGS[:key_nav_omega]
      end
    end

    # Set the maximum omega of keyboard camera navigation in rad/s.
    # @param [Numeric] value
    def key_nav_omega=(value)
      @settings[:key_nav_omega] = AMS.clamp(value.to_f, 0.01, nil)
    end

    # Get the time to accelerate, in seconds, from zero to the desired
    # velocity or omega.
    # @return [Numeric]
    def key_nav_atime
      if @settings.has_key?(:key_nav_atime)
        @settings[:key_nav_atime]
      else
        MSPhysics::DEFAULT_SIMULATION_SETTINGS[:key_nav_atime]
      end
    end

    # Set the time to accelerate, in seconds, from zero to the desired
    # velocity or omega.
    # @param [Numeric] value
    def key_nav_atime=(value)
      @settings[:key_nav_atime] = AMS.clamp(value.to_f, 0.0, nil)
    end

    # Get the state of scenes animation.
    # @return [Integer] State:
    #   * 0 - off/stop
    #   * 1 - one way
    #   * 2 - repeat forth and back
    #   * 3 - loop around
    def animate_scenes_state
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:animate_scenes_state]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Animate Scenes State', default)
      return AMS.clamp(attr.to_i, 0, 3)
    end

    # Set the state of scenes animation.
    # @param [Integer] state State:
    #   * 0 - off/stop
    #   * 1 - one way
    #   * 2 - repeat forth and back
    #   * 3 - loop around
    def animate_scenes_state=(state)
      state = AMS.clamp(state.to_i, 0, 3)
      Sketchup.active_model.set_attribute('MSPhysics', 'Animate Scenes State', state)
      sim = MSPhysics::Simulation.instance
      sim.animate_scenes(state) if sim
    end

    # Get the time to wait before starting to animate scenes.
    # @return [Numeric] Time in seconds.
    def animate_scenes_delay
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:animate_scenes_delay]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Animate Scenes Delay', default)
      return AMS.clamp(attr.to_f, 0, nil)
    end

    # Set the time to wait before starting to animate scenes.
    # @param [Numeric] value Time in seconds.
    def animate_scenes_delay=(value)
      value = AMS.clamp(value.to_f, 0, nil)
      Sketchup.active_model.set_attribute('MSPhysics', 'Animate Scenes Delay', value)
    end

    # Determine whether the animation of scenes is reversed.
    # @return [Boolean]
    def animate_scenes_reversed?
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:animate_scenes_reversed]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Animate Scenes Reversed', default)
      return attr ? true : false
    end

    # Reverse/normalize the animation of scenes.
    # @param [Boolean] state
    def animate_scenes_reversed=(state)
      state = state ? true : false
      Sketchup.active_model.set_attribute('MSPhysics', 'Animate Scenes Reversed', state)
    end

    # Determine whether continuous collision check is enabled.
    # @return [Boolean]
    def continuous_collision_check_enabled?
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:continuous_collision_check]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Continuous Collision Mode', default)
      return attr ? true : false
    end

    # Enable/disable continuous collision check.
    # @param [Boolean] state
    def continuous_collision_check_enabled=(state)
      state = state ? true : false
      Sketchup.active_model.set_attribute('MSPhysics', 'Continuous Collision Mode', state)
      sim = MSPhysics::Simulation.instance
      sim.continuous_collision_check_enabled = state if sim
    end

    # Determine whether fullscreen mode is enabled.
    # @return [Boolean]
    def full_screen_mode_enabled?
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:full_screen_mode]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Fullscreen Mode', default)
      return attr ? true : false
    end

    # Enable/disable fullscreen mode.
    # @param [Boolean] state
    def full_screen_mode_enabled=(state)
      state = state ? true : false
      Sketchup.active_model.set_attribute('MSPhysics', 'Fullscreen Mode', state)
      sim = MSPhysics::Simulation.instance
      sim.view_full_screen(state) if sim
    end

    # Determine whether hidden instances are ignored.
    # @return [Boolean]
    def ignore_hidden_instances?
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:ignore_hidden_instances]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Ignore Hidden Instances', default)
      return attr ? true : false
    end

    # Enable/disable fancy operation.
    # @param [Boolean] state
    def ignore_hidden_instances=(state)
      state = state ? true : false
      Sketchup.active_model.set_attribute('MSPhysics', 'Ignore Hidden Instances', state)
    end

    # Determine whether the navigation of camera with keyboard is enabled.
    # @return [Boolean]
    def keyboard_navigation_enabled?
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:keyboard_navigation]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Keyboard Navigation', default)
      return attr ? true : false
    end

    # Enable/disable the navigation of camera with keyboard.
    # @param [Boolean] state
    def keyboard_navigation_enabled=(state)
      state = state ? true : false
      Sketchup.active_model.set_attribute('MSPhysics', 'Keyboard Navigation', state)
    end

    # Determine whether game mode is enabled.
    # @return [Boolean]
    def game_mode_enabled?
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:game_mode]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Game Mode', default)
      return attr ? true : false
    end

    # Enable/disable game mode.
    # @param [Boolean] state
    def game_mode_enabled=(state)
      state = state ? true : false
      Sketchup.active_model.set_attribute('MSPhysics', 'Game Mode', state)
      sim = MSPhysics::Simulation.instance
      sim.mode = (state ? 1 : 0) if sim
    end

    # Determine whether the hide-joint-layer option is enabled.
    # @return [Boolean]
    def hide_joint_layer_enabled?
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:hide_joint_layer]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Hide Joint Layer', default)
      return attr ? true : false
    end

    # Enable/disable the hide-joint-layer option.
    # @param [Boolean] state
    def hide_joint_layer_enabled=(state)
      state = state ? true : false
      Sketchup.active_model.set_attribute('MSPhysics', 'Hide Joint Layer', state)
      if MSPhysics::Simulation.instance
        layer = Sketchup.active_model.layers['MSPhysics Joints']
        layer.visible = !state if layer && layer.visible? == state
      end
    end

    # Determine whether the undo-on-end option is enabled.
    # @return [Boolean]
    def undo_on_end_enabled?
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:undo_on_end]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Undo on End', default)
      return attr ? true : false
    end

    # Enable/disable the undo-on-end option.
    # @param [Boolean] state
    def undo_on_end_enabled=(state)
      state = state ? true : false
      Sketchup.active_model.set_attribute('MSPhysics', 'Undo on End', state)
      sim = MSPhysics::Simulation.instance
      sim.undo_on_reset = state if sim
    end

    # Get simulation solver model.
    # @return [Integer] Number of passes, a value between 1 and 256.
    def solver_model
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:solver_model]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Solver Model', default)
      return AMS.clamp(attr.to_i, 1, 256)
    end

    # Set simulation solver model.
    # @param [Integer] model Number of passes, a value between 1 and 256.
    def solver_model=(model)
      model = AMS.clamp(model.to_i, 1, 256)
      Sketchup.active_model.set_attribute('MSPhysics', 'Solver Model', model)
      sim = MSPhysics::Simulation.instance
      sim.world.solver_model = model if sim
    end

    # Get joint solver model.
    # @return [Integer] 0 - Accurate; 1 - Don't Use; 2 - Fast.
    def joint_algorithm
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:joint_algorithm]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Joint Algorithm', default)
      return AMS.clamp(attr.to_i, 0, 2)
    end

    # Set joint solver model.
    # @note Changing this property won't have an effect until simulation is
    #   restarted.
    # @param [Integer] model Joint solver model:
    #   * 0 - Accurate: Slow but robust
    #   * 1 - Don't Use
    #   * 2 - Fast: Fast but flexible
    def joint_algorithm=(model)
      model = AMS.clamp(model.to_i, 0, 2)
      Sketchup.active_model.set_attribute('MSPhysics', 'Joint Algorithm', model)
    end

    # Get simulation update timestep in seconds.
    # @return [Numeric] A value between 1/1200 and 1/30.
    def update_timestep
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:update_timestep]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Update Timestep', default)
      return AMS.clamp(attr.to_f, 1/1200.0, 1/30.0)
    end

    # Set simulation update timestep in seconds.
    # @param [Numeric] timestep A value between 1/1200 and 1/30.
    def update_timestep=(timestep)
      timestep = AMS.clamp(timestep.to_f, 1/1200.0, 1/30.0)
      Sketchup.active_model.set_attribute('MSPhysics', 'Update Timestep', timestep)
      sim = MSPhysics::Simulation.instance
      sim.update_timestep = timestep if sim
    end

    # Get simulation update rate, the number of times to update newton world
    # per frame.
    # @return [Integer] A value between 1 and 100.
    def update_rate
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:update_rate]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Update Rate', default)
      return AMS.clamp(attr.to_i, 1, 100)
    end

    # Set simulation update rate, the number of times to update newton world
    # per frame.
    # @param [Integer] rate A value between 1 and 100.
    def update_rate=(rate)
      rate = AMS.clamp(rate.to_i, 1, 100)
      Sketchup.active_model.set_attribute('MSPhysics', 'Update Rate', rate)
      sim = MSPhysics::Simulation.instance
      sim.update_rate = rate if sim
    end

    # Get simulation gravitational acceleration along Z-axis, in meters per
    # second per second (m/s/s).
    # @return [Numeric]
    def gravity
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:gravity]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Gravity', default)
      return attr.to_f
    end

    # Set simulation gravitational acceleration along Z-axis, in meters per
    # second per second (m/s/s).
    # @param [Numeric] acceleration
    def gravity=(acceleration)
      acceleration = acceleration.to_f
      Sketchup.active_model.set_attribute('MSPhysics', 'Gravity', acceleration)
      sim = MSPhysics::Simulation.instance
      sim.world.set_gravity(0, 0, acceleration) if sim
    end

    # Get simulation material thickness in meters.
    # @return [Numeric] A value between 0 and 1/32.
    def material_thickness
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:material_thickness]
      attr = Sketchup.active_model.get_attribute('MSPhysics', 'Material Thickness', default)
      return AMS.clamp(attr.to_f, 0, 1/32.0)
    end

    # Set simulation material thickness in meters.
    # @param [Numeric] thickness A value between 0 and 1/32.
    def material_thickness=(thickness)
      thickness = AMS.clamp(thickness.to_f, 0, 1/32.0)
      Sketchup.active_model.set_attribute('MSPhysics', 'Material Thickness', thickness)
      sim = MSPhysics::Simulation.instance
      sim.world.material_thickness = thickness if sim
    end

    # Show/hide bodies' collision wireframe.
    # @param [Boolean] state
    def collision_wireframe_visible=(state)
      @debug_collision = state ? true : false
      sim = MSPhysics::Simulation.instance
      sim.collision_wireframe_visible = @debug_collision if sim
    end

    # Determine whether bodies' collision wireframe is visible.
    # @return [Boolean]
    def collision_wireframe_visible?
      @debug_collision
    end

    # Show/hide bodies' centre of mass axes.
    # @param [Boolean] state
    def axes_visible=(state)
      @debug_axes = state ? true : false
      sim = MSPhysics::Simulation.instance
      sim.axes_visible = @debug_axes if sim
    end

    # Determine whether bodies' centre of mass axes are visible.
    # @return [Boolean]
    def axes_visible?
      @debug_axes
    end

    # Show/hide bodies' axes aligned bounding box (AABB).
    # @param [Boolean] state
    def aabb_visible=(state)
      @debug_aabb = state ? true : false
      sim = MSPhysics::Simulation.instance
      sim.aabb_visible = @debug_aabb if sim
    end

    # Determine whether bodies' axes aligned bonding box (AABB) is visible.
    # @return [Boolean]
    def aabb_visible?
      @debug_aabb
    end

    # Show/hide bodies' contact points.
    # @param [Boolean] state
    def contact_points_visible=(state)
      @debug_contact_points = state ? true : false
      sim = MSPhysics::Simulation.instance
      sim.contact_points_visible = @debug_contact_points if sim
    end

    # Determine whether bodies' contact points are visible.
    # @return [Boolean]
    def contact_points_visible?
      @debug_contact_points
    end

    # Show/hide bodies' contact forces.
    # @param [Boolean] state
    def contact_forces_visible=(state)
      @debug_contact_forces = state ? true : false
      sim = MSPhysics::Simulation.instance
      sim.contact_forces_visible = @debug_contact_forces if sim
    end

    # Determine whether bodies contact forces are visible.
    # @return [Boolean]
    def contact_forces_visible?
      @debug_contact_forces
    end

    # Show/hide all bodies.
    # @param [Boolean] state
    def bodies_visible=(state)
      @debug_bodies = state ? true : false
      sim = MSPhysics::Simulation.instance
      sim.bodies_visible = @debug_bodies if sim
      @debug_bodies
    end

    # Determine whether bodies are visible.
    # @return [Boolean]
    def bodies_visible?
      @debug_bodies
    end

  end # class << self

  # @!visibility private
  class AppObserver < ::Sketchup::AppObserver

    def onQuit
      ::MSPhysics::Settings.save_options
    end

  end # class AppObserver

end # module MSPhysics::Settings
