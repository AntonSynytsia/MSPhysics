module MSPhysics

  # @since 1.0.0
  module Settings

    @debug_collision = false
    @debug_axes = false
    @debug_aabb = false
    @debug_contact_points = false
    @debug_contact_forces = false
    @debug_bodies = true

    class << self

      # Determine whether continuous collision check is enabled.
      # @return [Boolean]
      def continuous_collision_check_enabled?
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Continuous Collision Mode', false)
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
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Fullscreen Mode', false)
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

      # Determine whether game mode is enabled.
      # @return [Boolean]
      def game_mode_enabled?
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Game Mode', false)
        return attr ? true : false
      end

      # Enable/disable game mode.
      # @param [Boolean] state
      def game_mode_enabled=(state)
        state = state ? true : false
        Sketchup.active_model.set_attribute('MSPhysics', 'Game Mode', state)
        sim = MSPhysics::Simulation.instance
        sim.mode = state ? 1 : 0 if sim
      end

      # Determine whether the hide-joint-layer option is enabled.
      # @return [Boolean]
      def hide_joint_layer_enabled?
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Hide Joint Layer', false)
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
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Undo on End', false)
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
      # @return [Fixnum] 0 - exact, n - iterative.
      def solver_model
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:solver_model]
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Solver Model', default)
        return attr.to_i.abs
      end

      # Set simulation solver model.
      # @param [Fixnum] model 0 - exact, n - iterative.
      def solver_model=(model)
        model = model.to_i.abs
        Sketchup.active_model.set_attribute('MSPhysics', 'Solver Model', model)
        sim = MSPhysics::Simulation.instance
        sim.world.solver_model = model if sim
      end

      # Get simulation friction model.
      # @return [Fixnum] 0 - exact coulomb, n - adaptive coulomb.
      def friction_model
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:friction_model]
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Friction Model', default)
        return attr.zero? ? 0 : 1
      end

      # Set simulation friction model.
      # @param [Fixnum] model 0 - exact coulomb, 1 - adaptive coulomb.
      def friction_model=(model)
        model = model.zero? ? 0 : 1
        Sketchup.active_model.set_attribute('MSPhysics', 'Friction Model', model)
        sim = MSPhysics::Simulation.instance
        sim.world.friction_model = model if sim
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
      # @return [Fixnum] A value between 1 and 100.
      def update_rate
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:update_rate]
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Update Rate', default)
        return AMS.clamp(attr.to_i, 1, 100)
      end

      # Set simulation update rate, the number of times to update newton world
      # per frame.
      # @param [Fixnum] rate A value between 1 and 100.
      def update_rate=(rate)
        rate = AMS.clamp(rate.to_i, 1, 100)
        Sketchup.active_model.set_attribute('MSPhysics', 'Update Rate', rate)
        sim = MSPhysics::Simulation.instance
        sim.update_rate = rate if sim
      end

      # Get simulation gravitational acceleration along z-axis, in meters per
      # second per second (m/s/s).
      # @return [Numeric]
      def gravity
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:gravity].z
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Gravity', default)
        return attr.to_f
      end

      # Set simulation gravitational acceleration along z-axis, in meters per
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

      # Get world scale.
      # @return [Numeric] A value between 0.1 and 100.
      def world_scale
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:world_scale]
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'World Scale', default)
        return AMS.clamp(attr.to_f, 0.1, 100.0)
      end

      # Set world scale.
      # @note This has an effect only when simulation is restarted.
      # @param [Numeric] scale A value between 0.1 and 100.
      def world_scale=(scale)
        scale = AMS.clamp(scale.to_f, 0.1, 100.0)
        Sketchup.active_model.set_attribute('MSPhysics', 'World Scale', scale)
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

      # Apply all settings to the active simulation.
      # @return [Boolean] success
      def apply_settings
        return false unless MSPhysics::Simulation.active?
        self.continuous_collision_check_enabled = self.continuous_collision_check_enabled?
        self.full_screen_mode_enabled = self.full_screen_mode_enabled?
        self.game_mode_enabled = self.game_mode_enabled?
        self.hide_joint_layer_enabled = true if self.hide_joint_layer_enabled?
        self.undo_on_end_enabled = self.undo_on_end_enabled?
        self.solver_model = self.solver_model
        self.friction_model = self.friction_model
        self.update_timestep = self.update_timestep
        self.gravity = self.gravity
        self.material_thickness = self.material_thickness
        self.world_scale = self.world_scale
        self.update_rate = self.update_rate
        self.collision_wireframe_visible = self.collision_wireframe_visible?
        self.axes_visible = self.axes_visible?
        self.contact_points_visible = self.contact_points_visible?
        self.contact_forces_visible = self.contact_forces_visible?
        self.bodies_visible = self.bodies_visible?
        true
      end

    end # class << self
  end # module Settings
end # module MSPhysics
