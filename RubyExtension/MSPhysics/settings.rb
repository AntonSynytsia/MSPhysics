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

      # Get continuous collision state.
      # @return [Boolean]
      def get_continuous_collision_state
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Continuous Collision Mode', false)
        return attr ? true : false
      end

      # Set continuous collision state.
      # @param [Boolean] state
      # @return [Boolean] The new state.
      def set_continuous_collision_state(state)
        state = state ? true : false
        Sketchup.active_model.set_attribute('MSPhysics', 'Continuous Collision Mode', state)
        sim = MSPhysics::Simulation.instance
        sim.set_continuous_collision_state(state) if sim
        return state
      end


      # Get simulation solver model.
      # @return [Fixnum] 0 - exact, n - iterative.
      def get_solver_model
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:solver_model]
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Solver Model', default)
        return attr.to_i.abs
      end

      # Set simulation solver model.
      # @param [Fixnum] model 0 - exact, n - iterative.
      # @return [Fixnum] The new solver model.
      def set_solver_model(model)
        model = model.to_i.abs
        Sketchup.active_model.set_attribute('MSPhysics', 'Solver Model', model)
        sim = MSPhysics::Simulation.instance
        sim.world.set_solver_model(model) if sim
        return model
      end

      # Get simulation friction model.
      # @return [Fixnum] 0 - exact coulomb, n - adaptive coulomb.
      def get_friction_model
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:friction_model]
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Friction Model', default)
        return attr.zero? ? 0 : 1
      end

      # Set simulation friction model.
      # @param [Fixnum] model 0 - exact coulomb, 1 - adaptive coulomb.
      # @return [Fixnum] The new friction model.
      def set_friction_model(model)
        model = model.zero? ? 0 : 1
        Sketchup.active_model.set_attribute('MSPhysics', 'Friction Model', model)
        sim = MSPhysics::Simulation.instance
        sim.world.set_friction_model(model) if sim
        return model
      end

      # Get simulation update timestep in seconds.
      # @return [Numeric] A value between 1/1200 and 1/30.
      def get_update_timestep
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:update_timestep]
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Update Timestep', default)
        return AMS.clamp(attr.to_f, 1/1200.0, 1/30.0)
      end

      # Set simulation update timestep in seconds.
      # @param [Numeric] timestep A value between 1/1200 and 1/30.
      # @return [Numeric] The new timestep.
      def set_update_timestep(timestep)
        timestep = AMS.clamp(timestep.to_f, 1/1200.0, 1/30.0)
        Sketchup.active_model.set_attribute('MSPhysics', 'Update Timestep', timestep)
        sim = MSPhysics::Simulation.instance
        sim.set_update_timestep(timestep) if sim
        return timestep
      end

      # Get simulation update rate, the number of times to update newton world
	  # per frame.
      # @return [Fixnum] A value between 1 and 100.
      def get_update_rate
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:update_rate]
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Update Rate', default)
        return AMS.clamp(attr.to_i, 1, 100)
      end

      # Set simulation update rate, the number of times to update newton world
	  # per frame.
      # @param [Fixnum] rate A value between 1 and 100.
      # @return [Fixnum] The new update rate.
      def set_update_rate(rate)
        rate = AMS.clamp(rate.to_i, 1, 100)
        Sketchup.active_model.set_attribute('MSPhysics', 'Update Rate', rate)
        sim = MSPhysics::Simulation.instance
        sim.set_update_rate(rate) if sim
        return rate
      end

      # Get simulation gravity in meters per second per second (m/s/s).
      # @return [Numeric]
      def get_gravity
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:gravity].z
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Gravity', default)
        return attr.to_f
      end

      # Set simulation gravity in meters per second per second (m/s/s).
      # @param [Numeric] gravity
      # @return [Numeric] The new gravity.
      def set_gravity(gravity)
        gravity = gravity.to_f
        Sketchup.active_model.set_attribute('MSPhysics', 'Gravity', gravity)
        sim = MSPhysics::Simulation.instance
        sim.world.set_gravity([0, 0, gravity]) if sim
        return gravity
      end

      # Get material thickness in meters.
      # @return [Numeric] A value between 0 and 1/32.
      def get_material_thickness
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:material_thickness]
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'Material Thickness', default)
        return AMS.clamp(attr.to_f, 0, 1/32.0)
      end

      # Set material thickness in meters.
      # @param [Numeric] thickness A value between 0 and 1/32.
      # @return [Numeric] The new thickness.
      def set_material_thickness(thickness)
        thickness = AMS.clamp(thickness.to_f, 0, 1/32.0)
        Sketchup.active_model.set_attribute('MSPhysics', 'Material Thickness', thickness)
        sim = MSPhysics::Simulation.instance
        sim.world.set_material_thickness(thickness) if sim
        return thickness
      end

      # Get world scale.
      # @return [Numeric] A value between 0.1 and 100.
      def get_world_scale
        default = MSPhysics::DEFAULT_SIMULATION_SETTINGS[:world_scale]
        attr = Sketchup.active_model.get_attribute('MSPhysics', 'World Scale', default)
        return AMS.clamp(attr.to_f, 0.1, 100.0)
      end

      # Set world scale.
      # @note This has an effect only when simulation is restarted.
      # @param [Numeric] scale A value between 0.1 and 100.
      # @return [Numeric] New scale.
      def set_world_scale(scale)
        scale = AMS.clamp(scale.to_f, 0.1, 100.0)
        Sketchup.active_model.set_attribute('MSPhysics', 'World Scale', scale)
        return scale
      end

      # Show/hide body collision wireframe.
      # @param [Boolean] state
      # @return [Boolean] The new state.
      def show_collision_wireframe(state)
        @debug_collision = state ? true : false
        sim = MSPhysics::Simulation.instance
        sim.show_collision_wireframe(@debug_collision) if sim
        @debug_collision
      end

      # Determine if body collision wireframe is visible.
      # @return [Boolean]
      def collision_wireframe_visible?
        @debug_collision
      end

      # Show/hide body centre of mass axes.
      # @param [Boolean] state
      # @return [Boolean] The new state.
      def show_axes(state)
        @debug_axes = state ? true : false
        sim = MSPhysics::Simulation.instance
        sim.show_axes(@debug_axes) if sim
        @debug_axes
      end

      # Determine if body centre of mass axes are visible.
      # @return [Boolean]
      def axes_visible?
        @debug_axes
      end

      # Show/hide body axes aligned bounding box (AABB).
      # @param [Boolean] state
      # @return [Boolean] The new state.
      def show_aabb(state)
        @debug_aabb = state ? true : false
        sim = MSPhysics::Simulation.instance
        sim.show_aabb(@debug_aabb) if sim
        @debug_aabb
      end

      # Determine if body axes aligned bonding box (AABB) is visible.
      # @return [Boolean]
      def aabb_visible?
        @debug_aabb
      end

      # Show/hide body contact points.
      # @param [Boolean] state
      # @return [Boolean] The new state.
      def show_contact_points(state)
        @debug_contact_points = state ? true : false
        sim = MSPhysics::Simulation.instance
        sim.show_contact_points(@debug_contact_points) if sim
        @debug_contact_points
      end

      # Determine if body contact points are visible.
      # @return [Boolean]
      def contact_points_visible?
        @debug_contact_points
      end

      # Show/hide body contact forces.
      # @param [Boolean] state
      # @return [Boolean] The new state.
      def show_contact_forces(state)
        @debug_contact_forces = state ? true : false
        sim = MSPhysics::Simulation.instance
        sim.show_contact_forces(@debug_contact_forces) if sim
        @debug_contact_forces
      end

      # Determine if body contact forces are visible.
      # @return [Boolean]
      def contact_forces_visible?
        @debug_contact_forces
      end

      # Show/hide all bodies.
      # @param [Boolean] state
      # @return [Boolean] The new state.
      def show_bodies(state)
        @debug_bodies = state ? true : false
        sim = MSPhysics::Simulation.instance
        sim.show_bodies(@debug_bodies) if sim
        @debug_bodies
      end

      # Determine whether bodies are set visible.
      # @return [Boolean]
      def bodies_visible?
        @debug_bodies
      end

      # Apply all settings to the active simulation.
      # @return [Boolean] success
      def apply_settings
        sim = MSPhysics::Simulation.instance
        return false unless sim
        set_continuous_collision_state( get_continuous_collision_state )
        set_solver_model( get_solver_model )
        set_friction_model( get_friction_model )
        set_update_timestep( get_update_timestep )
        set_gravity( get_gravity )
        set_material_thickness( get_material_thickness )
        set_world_scale( get_world_scale )
        set_update_rate( get_update_rate )
        show_collision_wireframe( collision_wireframe_visible? )
        show_axes( axes_visible? )
        show_aabb( aabb_visible? )
        show_contact_points( contact_points_visible? )
        show_contact_forces( contact_forces_visible? )
        show_bodies( bodies_visible? )
        true
      end

    end # class << self
  end # module Settings
end # module MSPhysics
