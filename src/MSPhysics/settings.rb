module MSPhysics
  module Settings

    module_function

    # @!visibility private
    @record_animation = false
    # @!visibility private
    @debug_collision = false
    # @!visibility private
    @debug_axis = false
    # @!visibility private
    @debug_aabb = false
    # @!visibility private
    @debug_contact_points = false
    # @!visibility private
    @debug_contact_forces = false
    # @!visibility private
    @debug_bodies = true

    # Enable/disable animation recording.
    # @param [Boolean] state
    def record_animation_enabled=(state)
      @record_animation = state ? true : false
      $msp_simulation.record_animation = state if $msp_simulation
    end

    # Determine whether animation recording is enabled.
    # @return [Boolean]
    def record_animation_enabled?
      @record_animation
    end

    # Enable/disable continuous collision mode.
    # @param [Boolean] state
    def continuous_collision_mode_enabled=(state)
      state = state ? true : false
      Sketchup.active_model.set_attribute('MSPhysics', 'Continuous Collision Mode', state)
      $msp_simulation.continuous_collision_mode_enabled = state if $msp_simulation
    end

    # Determine whether continuous collision mode is enabled.
    # @return [Boolean]
    def continuous_collision_mode_enabled?
      Sketchup.active_model.get_attribute('MSPhysics', 'Continuous Collision Mode', false)
    end

    # Get simulation solver model.
    # @return [Fixnum] 0 - exact, n - interactive
    def solver_model
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:solver_model]
      Sketchup.active_model.get_attribute('MSPhysics', 'Solver Model', default)
    end

    # Set simulation solver model.
    # @param [Fixnum] model 0 - exact, n - interactive
    def solver_model=(model)
      model = model.to_i.abs
      Sketchup.active_model.set_attribute('MSPhysics', 'Solver Model', model)
      $msp_simulation.solver_model = model if $msp_simulation
    end

    # Get simulation friction model.
    # @return [Fixnum] 0 - exact coulomb, n - adaptive coulomb
    def friction_model
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:friction_model]
      Sketchup.active_model.get_attribute('MSPhysics', 'Friction Model', default)
    end

    # Set simulation friction model.
    # @param [Fixnum] model 0 - exact coulomb, 1 - adaptive coulomb
    def friction_model=(model)
      model = model.zero? ? 0 : 1
      Sketchup.active_model.set_attribute('MSPhysics', 'Friction Model', model)
      $msp_simulation.friction_model = model if $msp_simulation
    end

    # Get simulation updated step in seconds.
    # @return [Numeric]
    def update_timestep
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:update_timestep]
      Sketchup.active_model.get_attribute('MSPhysics', 'Update Timestep', default)
    end

    # Set simulation updated step in seconds.
    # @param [Numeric] step
    def update_timestep=(step)
      step = MSPhysics.clamp(step.to_f, 1/1200.0, 1/30.0)
      Sketchup.active_model.set_attribute('MSPhysics', 'Update Timestep', step)
      $msp_simulation.update_timestep = step if $msp_simulation
    end

    # Get simulation gravity in m/s/s.
    # @return [Numeric]
    def gravity
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:gravity]
      Sketchup.active_model.get_attribute('MSPhysics', 'Gravity', default)
    end

    # Set simulation gravity in m/s/s.
    # @param [Numeric] value
    def gravity=(value)
      value = value.to_f
      Sketchup.active_model.set_attribute('MSPhysics', 'Gravity', value)
      $msp_simulation.gravity = value if $msp_simulation
    end

    # Get material thickness in meters.
    # @return [Numeric]
    def material_thickness
      default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:material_thickness]
      Sketchup.active_model.get_attribute('MSPhysics', 'Material Thickness', default)
    end

    # Set material thickness in meters.
    # @param [Numeric] value
    def material_thickness=(value)
      value = MSPhysics.clamp(value.to_f, 0, 1/32.0)
      Sketchup.active_model.set_attribute('MSPhysics', 'Material Thickness', value)
      $msp_simulation.material_thickness = value if $msp_simulation
    end

    # Show/hide body collision wireframe.
    # @param [Boolean] state
    def collision_visible=(state)
      @debug_collision = state ? true : false
      $msp_simulation.collision_visible = state if $msp_simulation
    end

    # Determine whether body collision wireframe is visible.
    # @return [Boolean]
    def collision_visible?
      @debug_collision
    end

    # Show/hide body centre of mass.
    # @param [Boolean] state
    def axis_visible=(state)
      @debug_axis = state ? true : false
      $msp_simulation.axis_visible = state if $msp_simulation
    end

    # Determine whether body centre of mass axis is visible.
    # @return [Boolean]
    def axis_visible?
      @debug_axis
    end

    # Show/hide body bounding box (AABB).
    # @param [Boolean] state
    def bounding_box_visible=(state)
      @debug_aabb = state ? true : false
      $msp_simulation.bounding_box_visible = state if $msp_simulation
    end

    # Determine whether body bonding box (AABB) is visible.
    # @return [Boolean]
    def bounding_box_visible?
      @debug_aabb
    end

    # Show/hide body contact points.
    # @param [Boolean] state
    def contact_points_visible=(state)
      @debug_contact_points = state ? true : false
      $msp_simulation.contact_points_visible = state if $msp_simulation
    end

    # Determine whether body contact points are visible.
    # @return [Boolean]
    def contact_points_visible?
      @debug_contact_points
    end

    # Show/hide body contact forces.
    # @param [Boolean] state
    def contact_forces_visible=(state)
      @debug_contact_forces = state ? true : false
      $msp_simulation.contact_forces_visible = state if $msp_simulation
    end

    # Determine whether body contact forces are visible.
    # @return [Boolean]
    def contact_forces_visible?
      @debug_contact_forces
    end

    # Show/hide all bodies.
    # @param [Boolean] state
    def bodies_visible=(state)
      @debug_bodies = state ? true : false
      $msp_simulation.bodies_visible = state if $msp_simulation
    end

    # Determine whether bodies are set visible.
    # @return [Boolean]
    def bodies_visible?
      @debug_bodies
    end

    # Update all settings.
    # @api private
    def apply_settings
      self.record_animation_enabled = self.record_animation_enabled?
      self.continuous_collision_mode_enabled = self.continuous_collision_mode_enabled?
      self.solver_model = self.solver_model
      self.friction_model = self.friction_model
      self.update_timestep = self.update_timestep
      self.gravity = self.gravity
      self.material_thickness = self.material_thickness
      self.collision_visible = self.collision_visible?
      self.axis_visible = self.axis_visible?
      self.bounding_box_visible = self.bounding_box_visible?
      self.contact_points_visible = self.contact_points_visible?
      self.contact_forces_visible = self.contact_forces_visible?
      self.bodies_visible = self.bodies_visible?
    end

  end # module Settings
end # module MSPhysics
