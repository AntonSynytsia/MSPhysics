module MSPhysics

  # The Body class represents a physics body in simulation. Every body in
  # simulation is designed to have its own Body object. The Body class consists
  # of various functions that allow you to control it in physics world.
  # @since 1.0.0
  class Body < Common
    class << self

      # Verify that body is valid.
      # @api private
      # @param [Body] body
      # @raise [TypeError] if body is invalid or destroyed.
      # @return [void]
      def validate(body)
        AMS.validate_type(body, MSPhysics::Body)
        unless body.is_valid?
          raise(TypeError, "Body #{body} is invalid/destroyed!", caller)
        end
      end

      # Verify that two bodies are valid and unique.
      # @api private
      # @param [Body] body1
      # @param [Body] body2
      # @return [void]
      # @raise [TypeError] if at least one body is invalid or destroyed.
      # @raise [TypeError] if both bodies link to the same address.
      def validate2(body1, body2)
        AMS.validate_type(body1, MSPhysics::Body)
        AMS.validate_type(body2, MSPhysics::Body)
        unless body1.is_valid?
          raise(TypeError, "Body1 #{body1} is invalid/destroyed!", caller)
        end
        unless body2.is_valid?
          raise(TypeError, "Body1 #{body1} is invalid/destroyed!", caller)
        end
        if body1.get_address == body2.get_address
          raise(TypeError, "Body1 #{body1} and body2 #{body2} link to the same address. Expected two unique bodies!", caller)
        end
      end

      # Get body by body address.
      # @param [Fixnum] address
      # @return [Body, nil]
      def get_body_by_address(address)
        data = MSPhysics::Newton::Body.get_user_data(address.to_i)
        data.is_a?(Body) ? data : nil
      end

      # Determine if the bounding boxes of two bodies overlap.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Boolean]
      def bodies_aabb_overlap?(body1, body2)
        validate2(body1, body2)
        MSPhysics::Newton::Bodies.aabb_overlap?(body1.get_address, body2.get_address)
      end

      # Determine if two bodies can collide with each other.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Boolean]
      def bodies_collidable?(body1, body2)
        validate2(body1, body2)
        MSPhysics::Newton::Bodies.collidable?(body1.get_address, body2.get_address)
      end

      # Determine if two bodies are in contact.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Boolean]
      def bodies_touching?(body1, body2)
        validate2(body1, body2)
        MSPhysics::Newton::Bodies.touching?(body1.get_address, body2.get_address)
      end

      # Get closest collision points between two bodies.
      # @note This works with convex and compound bodies only. Nil will be
      #   returned if one the passed bodies have a static mesh or a null
      #   collision.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Array<Geom::Point3d>, nil] +[contact_pt1, contact_pt2]+
      def get_closest_points(body1, body2)
        validate2(body1, body2)
        MSPhysics::Newton::Bodies.get_closest_points(body1.get_address, body2.get_address)
      end

      # Get contact force between two bodies.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Geom::Vector3d] force in newtons (kg*m/s/s).
      def get_force_between_bodies(body1, body2)
        validate2(body1, body2)
        MSPhysics::Newton::Bodies.get_force_in_between(body1.get_address, body2.get_address)
      end

      # Get all bodies.
      # @note Bodies that don't have a {Body} instance are not included in the
      #   array.
      # @return [Array<World>]
      def get_all_bodies
        bodies = []
        ovs = MSPhysics::Newton.is_object_validation_enabled?
        MSPhysics::Newton.enable_object_validation(false)
        Newton.get_all_bodies.each { |address|
          data = MSPhysics::Newton::Body.get_user_data(address)
          bodies << data if data.is_a?(Body)
        }
        MSPhysics::Newton.enable_object_validation(ovs)
        bodies
      end

    end # class << self

    # @overload initialize(world, entity, collision_shape)
    #   Create a new body.
    #   @note The specified entity must have all axis perpendicular to each
    #     other; otherwise, it is considered invalid. The entity will also be
    #     considered invalid if its collision shape turns out flat (not
    #     considering +"null"+ or +"static_mesh"+ collision shapes).
    #   @param [World] world
    #   @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    #   @param [Symbol, String] collision_shape Use one of the provided shapes:
    #     * box
    #     * sphere
    #     * cone
    #     * cylinder
    #     * chamfer_cylinder
    #     * capsule
    #     * convex_hull
    #     * null
    #     * compound
    #     * compound_from_cd
    #     * static_mesh
    #   @raise [TypeError] if the specified world is invalid.
    #   @raise [TypeError] if the specified entity is invalid.
    #   @raise [TypeError] if the specified collision shape is invalid.
    # @overload initialize(body, transformation, reapply_forces)
    #   Create a clone of an existing body.
    #   @param [Body] body A body Object.
    #   @param [Geom::Transformation, Array<Numeric>, nil] transformation New
    #     transformation matrix or +nil+ to create new body at the current
    #     location.
    #   @param [Boolean] reapply_forces Whether to reapply force and torque.
    #   @raise [TypeError] if the specified body is invalid.
    #   @raise [TypeError] if the specified transformation matrix is not acceptable.
    def initialize(*args)
      if args.size != 3
        raise(ArgumentError, "Wrong number of arguments! Expected 3 arguments, but got #{args.size}.", caller)
      end
      if args[0].is_a?(MSPhysics::World)
        MSPhysics::World.validate(args[0])
        MSPhysics::Collision.validate_entity(args[1])
        @_group = args[1]
        @_collision_shape = args[2].to_s.downcase.gsub(' ', '_')
        collision = MSPhysics::Collision.create(args[0], @_group, @_collision_shape)
        @_address = MSPhysics::Newton::Body.create_dynamic(args[0].get_address, collision, @_group.transformation, args[0].get_default_material_id)
        MSPhysics::Newton::Collision.destroy(collision)
      else
        # Create a clone of an existing body.
        Body.validate(args[0])
        unless args[0].get_group.valid?
          raise(TypeError, 'The specified body references a deleted entity. Copying bodies with erased entities is not acceptable!', caller)
        end
        @_address = MSPhysics::Newton::Body.copy(args[0].get_address, args[1], args[2])
        @_collision_shape = args[0].get_collision_shape
        definition = MSPhysics::Group.get_definition(args[0].get_group)
        @_group = Sketchup.active_model.entities.add_instance(definition, MSPhysics::Newton::Body.get_matrix(@_address))
        @_group.material = args[0].get_group.material
      end
      destructor = Proc.new {
        @_group = nil
        @_events.clear
      }
      MSPhysics::Newton::Body.set_destructor_proc(@_address, destructor)
      MSPhysics::Newton::Body.set_user_data(@_address, self)
      @_events = {
        :onStart                => nil,
        :onUpdate               => nil,
        :onPreUpdate            => nil,
        :onPostUpdate           => nil,
        :onEnd                  => nil,
        :onDraw                 => nil,
        :onPlay                 => nil,
        :onPause                => nil,
        :onTouch                => nil,
        :onTouching             => nil,
        :onUntouch              => nil,
        :onClick                => nil,
        :onDrag                 => nil,
        :onUnclick              => nil,
        :onKeyDown              => nil,
        :onKeyUp                => nil,
        :onKeyExtended          => nil,
        :onMouseMove            => nil,
        :onLButtonDown          => nil,
        :onLButtonUp            => nil,
        :onLButtonDoubleClick   => nil,
        :onRButtonDown          => nil,
        :onRButtonUp            => nil,
        :onRButtonDoubleClick   => nil,
        :onMButtonDown          => nil,
        :onMButtonUp            => nil,
        :onMButtonDoubleClick   => nil,
        :onXButton1Down         => nil,
        :onXButton1Up           => nil,
        :onXButton1DoubleClick  => nil,
        :onXButton2Down         => nil,
        :onXButton2Up           => nil,
        :onXButton2DoubleClick  => nil,
        :onMouseWheelRotate     => nil,
        :onMouseWheelTilt       => nil
      }
      @_script_state = true
      @_look_at_constraint = nil
    end

    # Determine if the body is valid - not destroyed.
    # @return [Boolean]
    def is_valid?
      MSPhysics::Newton::Body.is_valid?(@_address)
    end

    # Get body pointer.
    # @return [Fixnum]
    def get_address
      Body.validate(self)
      @_address
    end

    # Get world in which the body was created.
    # @return [World]
    def get_world
      Body.validate(self)
      world_address = MSPhysics::Newton::Body.get_world(@_address)
      MSPhysics::Newton::World.get_user_data(world_address)
    end

    # Get contact material id.
    # @return [Fixnum]
    def get_material_id
      Body.validate(self)
      MSPhysics::Newton::Body.get_material_id(@_address)
    end

    # Set contact material id.
    # @param [Fixnum] id
    # @return [Fixnum]
    def set_material_id(id)
      Body.validate(self)
      MSPhysics::Newton::Body.set_material_id(@_address, id)
    end

    # Get an address of the collision associated with the body.
    # @return [Fixnum]
    def get_collision_address
      Body.validate(self)
      MSPhysics::Newton::Body.get_collision(@_address)
    end

    # Get collision shape of the body.
    # @return [String]
    def get_collision_shape
      Body.validate(self)
      @_collision_shape.clone
    end

    # Get the group/component associated with the body.
    # @return [Sketchup::Group, Sketchup::ComponentInstance]
    def get_group
      Body.validate(self)
      @_group
    end

    # Destroy the body.
    # @param [Boolean] erase_entity Whether to erase an entity associated with
    #   the body.
    # @return [nil]
    def destroy(erase_entity = false)
      Body.validate(self)
      @_group.erase! if @_group.valid? && erase_entity
      MSPhysics::Newton::Body.destroy(@_address)
    end

    # Get simulation state of the body.
    # @return [Boolean] +true+ if body simulation is enabled, +false+ if body
    #   simulation is disabled.
    def get_simulation_state
      Body.validate(self)
      MSPhysics::Newton::Body.get_simulation_state(@_address)
    end

    # Set simulation state of the body.
    # @param [Boolean] state +true+ to enable body simulation, +false+ to
    #   disable body simulation.
    # @return [Boolean] The newly assigned state.
    def set_simulation_state(state)
      Body.validate(self)
      MSPhysics::Newton::Body.set_simulation_state(@_address, state)
    end

    # Get continuous collision state of the body. If continuous collision check
    # is enabled, the body avoids passing other bodies at high speeds and
    # avoids penetrating into other bodies.
    # @return [Boolean] +true+ if continuous collision check is on, +false+ if
    #    continuous collision check is off.
    def get_continuous_collision_state
      Body.validate(self)
      MSPhysics::Newton::Body.get_continuous_collision_state(@_address)
    end

    # Set continuous collision state of the body. Enabling continuous collision
    # prevents the body from passing other bodies at high speeds and prevents
    # the body from penetrating into other bodies.
    # @note This is known to affect performance. Be cautions when using. When
    #   performing box stacks it's better to reduce simulation update step, to
    #   1/256 for instance, rather than enabling continuous collision mode as
    #   smaller update step will keep simulation running smoothly and avoid
    #   penetration at the same time.
    # @param [Boolean] state +true+ to set continuous collision check on,
    #   +false+ to set continuous collision check off.
    # @return [Boolean] The newly assigned state.
    def set_continuous_collision_state(state)
      Body.validate(self)
      MSPhysics::Newton::Body.set_continuous_collision_state(@_address, state)
    end

    # Get body transformation matrix.
    # @return [Geom::Transformation]
    def get_matrix
      Body.validate(self)
      MSPhysics::Newton::Body.get_matrix(@_address)
    end

    # Get body matrix with no scale factors.
    # @return [Geom::transformation]
    def get_normal_matrix
      Body.validate(self)
      MSPhysics::Newton::Body.get_normal_matrix(@_address)
    end

    # Set body transformation matrix.
    # @param [Geom::Transformation, Array<Numeric>] matrix
    # @return [Geom::Transformation] The newly assigned transformation matrix.
    # @raise [TypeError] if some or all matrix axis are not perpendicular to
    #   each other.
    # @raise [TypeError] if some or all matrix axis have a scale of zero.
    def set_matrix(matrix)
      Body.validate(self)
      res = MSPhysics::Newton::Body.set_matrix(@_address, matrix)
      @_group.move!(MSPhysics::Newton::Body.get_matrix(@_address)) if @_group.valid?
      res
    end

    # Get body position.
    # @param [Fixnum] mode
    #   * 0 - get body's origin in global space.
    #   * 1 - get body's centre of mass in global space.
    # @return [Geom::Point3d]
    def get_position(mode = 0)
      Body.validate(self)
      MSPhysics::Newton::Body.get_position(@_address, mode.to_i)
    end

    # Set body position.
    # @overload set_position(position, mode = 0)
    #   @param [Geom::Point3d, Array<Numeric>] position A point in global space.
    #   @param [Fixnum] mode
    #     * 0 - reposition body origin to a desired location in global space.
    #     * 1 - reposition body centre of mass to a desired location in global
    #       space.
    # @overload set_position(px, py, pz, mode = 0)
    #   @param [Numeric] px X position in global space.
    #   @param [Numeric] py Y position in global space.
    #   @param [Numeric] pz Z position in global space.
    #   @param [Fixnum] mode
    #     * 0 - reposition body origin to a desired location in global space.
    #     * 1 - reposition body centre of mass to a desired location in global
    #       space.
    # @return [Geom::Point3d] The newly assigned position.
    def set_position(*args)
      Body.validate(self)
      mode = 0
      if args.size == 4
        point = [args[0], args[1], args[2]]
        mode = args[3]
      elsif args.size == 3
        point = [args[0], args[1], args[2]]
      elsif args.size == 2
        point = args[0]
        mode = args[3]
      elsif args.size == 1
        point = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1..4 arguments, but got #{args.size}.", caller)
      end
      res = MSPhysics::Newton::Body.set_position(@_address, point, mode)
      @_group.move!(MSPhysics::Newton::Body.get_matrix(@_address)) if @_group.valid?
      res
    end

    # Get body orientation in form of the unit quaternion.
    # @return [Array<Numeric>] An array of four numeric values:
    #   +[q0, q1, q2, q3]+ - +[x,y,z,w]+.
    def get_rotation
      Body.validate(self)
      MSPhysics::Newton::Body.get_rotation(@_address)
    end

    # Get body orientation in form of the three Euler angles.
    # @return [Geom::Vector3d] An vector of three Euler angles expressed in
    #   radians: +(roll, yaw, pitch)+.
    def get_euler_angles
      Body.validate(self)
      MSPhysics::Newton::Body.get_euler_angles(@_address)
    end

    # Set body orientation via the three Euler angles.
    # @note The angles are assumed in radians.
    # @overload set_euler_angles(angles)
    #   @param [Geom::Vector3d, Array<Numeric>] angles A vector representing the
    #     roll, yaw, and pitch Euler angles in radians.
    # @overload set_euler_angles(roll, yaw, pitch)
    #   @param [Numeric] roll
    #   @param [Numeric] yaw
    #   @param [Numeric] pitch
    # @return [Geom::Vector3d] The newly assigned Euler angles.
    def set_euler_angles(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      res = MSPhysics::Newton::Body.set_euler_angles(@_address, data)
      @_group.move!(MSPhysics::Newton::Body.get_matrix(@_address)) if @_group.valid?
      res
    end

    # Get global linear velocity of the body.
    # @return [Geom::Vector3d] The magnitude of the velocity vector is
    #   represented in meters per second.
    def get_velocity
      Body.validate(self)
      MSPhysics::Newton::Body.get_velocity(@_address)
    end

    # Set global linear velocity of the body.
    # @overload set_velocity(velocity)
    #   @param [Geom::Vector3d, Array<Numeric>] velocity The magnitude of the
    #     velocity vector is assumed in meters per second.
    # @overload set_velocity(vx, vy, vz)
    #   @param [Numeric] vx Velocity in meters per second along xaxis.
    #   @param [Numeric] vy Velocity in meters per second along yaxis.
    #   @param [Numeric] vz Velocity in meters per second along zaxis.
    # @return [Geom::Vector3d] The newly assigned velocity vector.
    def set_velocity(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_velocity(@_address, data)
    end

    # Get global angular velocity of the body.
    # @example
    #   Each value of the omega vector represents angular velocity in radians
    #   per second along xaxis, yaxis, or zaxis in global space. For example, if
    #   omega of a body is (0,0,PI), it means that the body rotates along zaxis
    #   in global space at an angular velocity of 360 degrees per second.
    # @return [Geom::Vector3d] The magnitude of the omega vector is represented
    #   in radians per second.
    def get_omega
      Body.validate(self)
      MSPhysics::Newton::Body.get_omega(@_address)
    end

    # Set global angular velocity of the body.
    # @overload set_omega(omega)
    #   @param [Geom::Vector3d, Array<Numeric>] omega The magnitude of the omega
    #     vector is assumed in radians per second.
    # @overload set_omega(vx, vy, vz)
    #   @param [Numeric] vx Omega in radians per second along xaxis.
    #   @param [Numeric] vy Omega in radians per second along yaxis.
    #   @param [Numeric] vz Omega in radians per second along zaxis.
    # @return [Geom::Vector3d] The newly assigned omega vector.
    def set_omega(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_omega(@_address, data)
    end

    # Get centre of mass of the body in local coordinates.
    # @example Getting centre of mass in global space.
    #   centre = body.get_cetre_of_mass.transform( body.get_matrix )
    # @return [Geom::Point3d]
    # @see {#get_position}
    def get_centre_of_mass
      Body.validate(self)
      MSPhysics::Newton::Body.get_centre_of_mass(@_address)
    end

    # Set centre of mass of the body in local coordinates.
    # @overload set_centre_of_mass(centre)
    #   @param [Geom::Point3d, Array<Numeric>] centre A point is assumed in body
    #     coordinates.
    # @overload set_velocity(px, py, pz)
    #   @param [Numeric] px X position in local space.
    #   @param [Numeric] py Y position in local space.
    #   @param [Numeric] pz Z position in local space.
    # @return [Geom::Point3d] The newly assigned centre of mass.
    def set_centre_of_mass(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_centre_of_mass(@_address, data)
    end

    # Get body mass in kilograms (kg).
    # @return [Numeric]
    def get_mass
      Body.validate(self)
      MSPhysics::Newton::Body.get_mass(@_address)
    end

    # Set body mass in kilograms (kg).
    # @note Mass and density are correlated. If you change mass the density will
    #   automatically be recalculated.
    # @param [Numeric] mass
    # @return [Numeric] The newly assigned mass.
    def set_mass(mass)
      Body.validate(self)
      MSPhysics::Newton::Body.set_mass(@_address, mass)
    end

    # Get body density in kilograms per cubic meter (kg / m^3).
    # @return [Numeric]
    def get_density
      Body.validate(self)
      MSPhysics::Newton::Body.get_density(@_address)
    end

    # Set body density in kilograms per cubic meter (kg / m^3).
    # @note Density and mass are correlated. If you change density the mass will
    #   automatically be recalculated.
    # @param [Numeric] density
    # @return [Numeric] The newly assigned density.
    def set_density(density)
      Body.validate(self)
      MSPhysics::Newton::Body.set_density(@_address, density)
    end

    # Get body volume in cubic meters (m^3).
    # @return [Numeric]
    def get_volume
      Body.validate(self)
      MSPhysics::Newton::Body.get_volume(@_address)
    end

    # Set body volume in cubic meters (m^3).
    # @note Volume and mass are correlated. If you change volume the mass will
    #   automatically be recalculated.
    # @param [Numeric] volume
    # @return [Numeric] The newly assigned volume.
    def set_volume(volume)
      Body.validate(self)
      MSPhysics::Newton::Body.set_volume(@_address, volume.to_f)
    end

    # Reset/recalculate body volume and mass.
    # @param [Numeric] density
    # @return [Boolean] success
    def reset_mass_properties(density)
      Body.validate(self)
      MSPhysics::Newton::Body.reset_mass_properties(@_address, density.to_f)
    end

    # Determine whether body is static.
    # @return [Boolean]
    def is_static?
      Body.validate(self)
      MSPhysics::Newton::Body.is_static?(@_address)
    end

    # Set body static.
    # @param [Boolean] state +true+ to set body static, +false+ to set body
    #   dynamic.
    # @return [Boolean] The newly assigned state.
    def set_static(state)
      Body.validate(self)
      MSPhysics::Newton::Body.set_static(@_address, state)
    end

    # Determine whether body is collidable.
    # @return [Boolean]
    def is_collidable?
      Body.validate(self)
      MSPhysics::Newton::Body.is_collidable?(@_address)
    end

    # Set body collidable.
    # @param [Boolean] state +true+ to set body collidable, +false+ to set body
    #   non-collidable.
    # @return [Boolean] The newly assigned state.
    def set_collidable(state)
      Body.validate(self)
      MSPhysics::Newton::Body.set_collidable(@_address, state)
    end

    # Determine whether body is frozen.
    # @return [Boolean]
    def is_frozen?
      Body.validate(self)
      MSPhysics::Newton::Body.is_frozen?(@_address)
    end

    # Set body collidable.
    # @param [Boolean] state +true+ to freeze the body, +false+ unfreeze the
    #   body.
    # @return [Boolean] The newly assigned state.
    def set_frozen(state)
      Body.validate(self)
      MSPhysics::Newton::Body.set_frozen(@_address, state)
    end

    # Determine whether body is sleeping. Sleeping bodies are bodies at rest.
    # @return [Boolean]
    def is_sleeping?
      Body.validate(self)
      MSPhysics::Newton::Body.is_sleeping?(@_address)
    end

    # Set body sleeping.
    # @note This function can only set body active, the sleeping is controlled
    #   by equilibrium.
    # @param [Boolean] state +true+ to set body sleeping, +false+ to set body
    #   active.
    # @return [Boolean] The newly assigned state.
    def set_sleeping(state)
      Body.validate(self)
      MSPhysics::Newton::Body.set_sleeping(@_address, state)
    end

    # Get the auto-sleep state of the body.
    # @return [Boolean] +true+ if body auto-sleep is on, +false+ if body
    #   auto-sleep is off.
    def get_auto_sleep_state
      Body.validate(self)
      MSPhysics::Newton::Body.get_auto_sleep_state(@_address)
    end

    # Set auto sleep state of the body. Auto sleep enables body to automatically
    # go to sleep mode when at rest or become active when activated.
    # @note Keeping auto sleep *on* is a huge performance plus for simulation.
    #   Auto sleep enabled is the default state for a created body; however, for
    #   player control, AI control or some other special circumstance, the
    #   application may want to control the activation/deactivation of the body.
    # @param [Boolean] state +true+ to set body auto-sleep on, or +false+ to set
    #   body auto-sleep off.
    # @return [Boolean] The newly assigned state.
    def set_auto_sleep_state(state)
      Body.validate(self)
      MSPhysics::Newton::Body.set_auto_sleep_state(@_address, state)
    end

    # Determine whether this body is non-collidable with a particular body.
    # @param [Body] body The body to test.
    # @return [Boolean] +true+ if this body is non-collidable with the given
    #   body, +false+ if this body is collidable with the given body.
    def is_non_collidable_with?(body)
      Body.validate2(self, body)
      MSPhysics::Newton::Body.is_non_collidable_with?(@_address, body.get_address)
    end

    # Set this body non-collidable with a particular body.
    # @param [Body] body
    # @param [Boolean] state +true+ to set this body non-collidable with
    #   another, +false+ to set this body collidable with another.
    # @return [Boolean] The newly assigned state.
    def set_non_collidable_with(body, state)
      Body.validate2(self, body)
      MSPhysics::Newton::Body.set_non_collidable_with(@_address, body.get_address, state)
    end

    # Get all bodies that are non-collidable with this body; the bodies that
    # were set non-collidable by the {#set_non_collidable_with} function.
    # @return [Array<Body>] An array of non-collidable bodies.
    def get_non_collidable_bodies
      Body.validate(self)
      bodies = []
      MSPhysics::Newton::Body.get_non_collidable_bodies(@_address).each { |address|
        body = MSPhysics::Newton::Body.get_user_data(address)
        bodies << body if body.is_a?(Body)
      }
      bodies
    end

    # Remove all bodies from the non-collidable list; the bodies that were set
    # non-collidable by the {#set_non_collidable_with} function.
    # @return [Fixnum] The number of bodies unmarked.
    def clear_non_collidable_bodies
      Body.validate(self)
      MSPhysics::Newton::Body.clear_non_collidable_bodies(@_address)
    end

    # Get body coefficient of restitution - bounciness - rebound ratio.
    # @return [Numeric] A value between 0.01 and 2.00.
    def get_elasticity
      Body.validate(self)
      MSPhysics::Newton::Body.get_elasticity(@_address)
    end

    # Set body coefficient of restitution - bounciness - rebound ratio.
    # @example A basketball has a rebound ratio of 0.83. This means the new
    #   height of a basketball is 83% of original height within each bounce.
    # @param [Numeric] coefficient A value between 0.01 and 2.00.
    # @return [Numeric] The newly assigned coefficient ratio.
    def set_elasticity(coefficient)
      Body.validate(self)
      MSPhysics::Newton::Body.set_elasticity(@_address, coefficient.to_f)
    end

    # Get contact softness coefficient of the body.
    # @return [Numeric] A value between 0.01 and 1.00.
    def get_softness
      Body.validate(self)
      MSPhysics::Newton::Body.get_softness(@_address)
    end

    # Set contact softness coefficient of the body.
    # @param [Numeric] coefficient A value between 0.01 and 1.00.
    # @return [Numeric] The newly assigned coefficient.
    def set_softness(coefficient)
      Body.validate(self)
      MSPhysics::Newton::Body.set_softness(@_address, coefficient.to_f)
    end

    # Get static friction coefficient of the body.
    # @return [Numeric] A value between 0.01 and 2.00.
    def get_static_friction
      Body.validate(self)
      MSPhysics::Newton::Body.get_static_friction(@_address)
    end

    # Set static friction coefficient of the body.
    # @param [Numeric] coefficient A value between 0.01 and 2.00.
    # @return [Numeric] The newly assigned coefficient.
    def set_static_friction(coefficient)
      Body.validate(self)
      MSPhysics::Newton::Body.set_static_friction(@_address, coefficient.to_f)
    end

    # Get dynamic friction coefficient of the body.
    # @return [Numeric] A value between 0.01 and 2.00.
    def get_dynamic_friction
      Body.validate(self)
      MSPhysics::Newton::Body.get_dynamic_friction(@_address)
    end

    # Set dynamic friction coefficient of the body.
    # @param [Numeric] coefficient A value between 0.01 and 2.00.
    # @return [Numeric] The newly assigned coefficient.
    def set_dynamic_friction(coefficient)
      Body.validate(self)
      MSPhysics::Newton::Body.set_dynamic_friction(@_address, coefficient.to_f)
    end

    # Get friction state of the body.
    # @return [Boolean] +true+ if body friction is enabled, +false+ if body
    #   friction is disabled.
    def get_friction_state
      Body.validate(self)
      MSPhysics::Newton::Body.get_friction_state(@_address)
    end

    # Set friction state of the body.
    # @param [Boolean] state +true+ to enable body friction, +false+ to disable
    #   body friction.
    # @return [Boolean] The newly assigned state.
    def set_friction_state(state)
      Body.validate(self)
      MSPhysics::Newton::Body.set_friction_state(@_address, state)
    end

    # Get the maximum magnet force in Newton to be applied on surrounding
    # magnetic bodies.
    # @return [Numeric]
    def get_magnet_force
      Body.validate(self)
      MSPhysics::Newton::Body.get_magnet_force(@_address)
    end

    # Set the maximum magnet force in Newton to be applied on surrounding
    # magnetic bodies.
    # @param [Numeric] force
    # @return [Numeric] The newly assigned magnet force.
    def set_magnet_force(force)
      Body.validate(self)
      MSPhysics::Newton::Body.set_magnet_force(@_address, force.to_f)
    end

    # Get the maximum magnet range in meters. Magnet force is distributed along
    # the magnet range. Magnetic bodies outside the magnet range are not
    # affected.
    # @return [Numeric]
    def get_magnet_range
      Body.validate(self)
      MSPhysics::Newton::Body.get_magnet_range(@_address)
    end

    # Set the maximum magnet range in meters. Magnet force is distributed along
    # the magnet range. Magnetic bodies outside the magnet range are not
    # affected.
    # @param [Numeric] range
    # @return [Numeric] The newly assigned magnet range.
    def set_magnet_range(range)
      Body.validate(self)
      MSPhysics::Newton::Body.set_magnet_range(@_address, range.to_f)
    end

    # Determine whether body is magnetic.
    # @return [Boolean]
    def is_magnetic?
      Body.validate(self)
      MSPhysics::Newton::Body.is_magnetic?(@_address)
    end

    # Set body magnetic. Magnetic bodies will be affected by other bodies with
    # magnetism.
    # @param [Boolean] state +true+ to set body magnetic, +false+ to set body
    #   non-magnetic.
    # @return [Boolean] The newly assigned state.
    def set_magnetic(state)
      Body.validate(self)
      MSPhysics::Newton::Body.set_magnetic(@_address, state)
    end

    # Get world axis aligned bounding box (AABB) of the body.
    # @return [Geom::BoundingBox]
    def get_bounding_box
      Body.validate(self)
      bb = Geom::BoundingBox.new
      bb.add MSPhysics::Newton::Body.get_aabb(@_address)
      bb
    end

    alias get_aabb get_bounding_box

    # Get the linear viscous damping coefficient applied to the body.
    # @return [Numeric] A coefficient value between 0.0 and 1.0.
    def get_linear_damping
      Body.validate(self)
      MSPhysics::Newton::Body.get_linear_damping(@_address)
    end

    # Set the linear viscous damping coefficient applied to the body.
    # @param [Numeric] damp A coefficient value between 0.0 and 1.0.
    # @return [Numeric] The newly assigned linear damping.
    def set_linear_damping(damp)
      Body.validate(self)
      MSPhysics::Newton::Body.set_linear_damping(@_address, damp.to_f)
    end

    # Get the angular viscous damping coefficient applied to the body.
    # @return [Geom::Vector3d] Each axis represents a coefficient value between
    #   0.0 and 1.0.
    def get_angular_damping
      Body.validate(self)
      MSPhysics::Newton::Body.get_angular_damping(@_address)
    end

    # Set the angular viscous damping coefficient applied to the body.
    # @overload set_angular_damping(damp)
    #   @param [Geom::Vector3d, Array<Numeric>] damp Each value of the damp
    #     vector is assumed as an angular coefficient, a value b/w 0.0 and 1.0.
    # @overload set_angular_damping(dx, dy, dz)
    #   @param [Numeric] dx Xaxis damping coefficient, a value b/w 0.0 and 1.0.
    #   @param [Numeric] dy Yaxis damping coefficient, a value b/w 0.0 and 1.0.
    #   @param [Numeric] dz Zaxis damping coefficient, a value b/w 0.0 and 1.0.
    # @return [Geom::Vector3d] The newly assigned angular damping.
    def set_angular_damping(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_angular_damping(@_address, data)
    end

    # Get velocity at a specific point on the body.
    # @param [Geom::Point3d, Array<Numeric>] point A point in global space.
    # @return [Geom::Vector3d] Velocity at the given point. The magnitude of
    #   velocity is retrieved in meters per second (m/s).
    def get_point_velocity(point)
      Body.validate(self)
      MSPhysics::Newton::Body.get_point_velocity(@_address, point)
    end

    # Add force to a specific point on the body.
    # @param [Geom::Point3d, Array<Numeric>] point Point on the body in global
    #   space.
    # @param [Geom::Vector3d, Array<Numeric>] force The magnitude of force is
    #   assumed in newtons (kg*m/s/s).
    # @return [Boolean] success
    def add_point_force(point, force)
      Body.validate(self)
      MSPhysics::Newton::Body.add_point_force(@_address, point, force)
    end

    # Add an impulse to a specific point on the body.
    # @param [Geom::Point3d, Array<Numeric>] center The center of an impulse in
    #   global space.
    # @param [Geom::Vector3d, Array<Numeric>] delta_vel The desired change in
    #   velocity. The magnitude of velocity is assumed in meters per second
    #   (m/s).
    # @return [Boolean] success
    def add_impulse(center, delta_vel)
      Body.validate(self)
      MSPhysics::Newton::Body.add_impulse(@_address, center, delta_vel)
    end

    # @!group Force Control Functions

    # Get the net force, in Newtons, applied on the body after the last world
    # update.
    # @return [Geom::Vector3d]
    def get_force
      Body.validate(self)
      MSPhysics::Newton::Body.get_force(@_address)
    end

    # Get the net force, in Newtons, applied on the body on the last world
    # update.
    # @return [Geom::Vector3d] The magnitude of force is retrieved in newtons
    #   (kg*m/s/s).
    def get_force_acc
      Body.validate(self)
      MSPhysics::Newton::Body.get_force_acc(@_address)
    end

    # Apply force on the body in Newtons (kg * m/s/s).
    # @note The {#add_force} applies force on an object for one world update.
    #   For example, if the world update rate is 3, i.e if world updates 3 times
    #   per frame, the force accumulated by the {#add_force} function will be
    #   applied on the first update but not on the consequent two updates. Use
    #   the {#add_force2} function to apply force throughout the whole world
    #   update rate.
    # @note Unlike the {#set_force}, this function doesn't overwrites original
    #   force, but rather adds force to the force accumulator.
    # @overload add_force(force)
    #   @param [Geom::Vector3d, Array<Numeric>] force
    # @overload add_force(fx, fy, fz)
    #   @param [Numeric] fx
    #   @param [Numeric] fy
    #   @param [Numeric] fz
    # @return [Boolean] success
    # @see {#add_force2}
    def add_force(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.add_force(@_address, data)
    end

    # Apply force on the body in Newtons (kg * m/s/s).
    # @note The {#add_force} applies force on an object for one world update.
    #   For example, if the world update rate is 3, i.e if world updates 3 times
    #   per frame, the force accumulated by the {#add_force} function will be
    #   applied on the first update but not on the consequent two updates. Use
    #   the {#add_force2} function to apply force throughout the whole world
    #   update rate.
    # @note Unlike the {#set_force}, this function doesn't overwrites original
    #   force, but rather adds force to the force accumulator.
    # @overload add_force2(force)
    #   @param [Geom::Vector3d, Array<Numeric>] force
    # @overload add_force2(fx, fy, fz)
    #   @param [Numeric] fx
    #   @param [Numeric] fy
    #   @param [Numeric] fz
    # @return [Boolean] success
    # @see {#add_force}
    def add_force2(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.add_force2(@_address, data)
    end

    # Apply force on the body in Newton (kg * m/s/s).
    # @note The {#set_force} applies force on an object for one world update.
    #   For example, if the world update rate is 3, i.e if world updates 3 times
    #   per frame, the force accumulated by the {#set_force} function will be
    #   applied on the first update but not on the consequent two updates. Use
    #   the {#set_force2} function to apply force throughout the whole world
    #   update rate.
    # @note Unlike the {#add_force}, this function overwrites original force,
    #   thus discarding the previously applied force.
    # @overload set_force(force)
    #   @param [Geom::Vector3d, Array<Numeric>] force
    # @overload set_force(fx, fy, fz)
    #   @param [Numeric] fx
    #   @param [Numeric] fy
    #   @param [Numeric] fz
    # @return [Boolean] success
    # @see {#set_force2}
    def set_force(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_force(@_address, data)
    end

    # Apply force on the body in Newton (kg * m/s/s).
    # @note The {#set_force} applies force on an object for one world update.
    #   For example, if the world update rate is 3, i.e if world updates 3 times
    #   per frame, the force accumulated by the {#set_force} function will be
    #   applied on the first update but not on the consequent two updates. Use
    #   the {#set_force2} function to apply force throughout the whole world
    #   update rate.
    # @note Unlike the {#add_force}, this function overwrites original force,
    #   thus discarding the previously applied force.
    # @overload set_force2(force)
    #   @param [Geom::Vector3d, Array<Numeric>] force
    # @overload set_force2(fx, fy, fz)
    #   @param [Numeric] fx
    #   @param [Numeric] fy
    #   @param [Numeric] fz
    # @return [Boolean] success
    # @see {#set_force}
    def set_force2(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_force2(@_address, data)
    end

    # Get the net torque, in Newton-meters, applied on the body after the last
    # world update.
    # @return [Geom::Vector3d]
    def get_torque
      Body.validate(self)
      MSPhysics::Newton::Body.get_torque(@_address)
    end

    # Get the net torque, in Newton-meters, applied on the body on the last
    # world update.
    # @return [Geom::Vector3d]
    def get_torque_acc
      Body.validate(self)
      MSPhysics::Newton::Body.get_torque_acc(@_address)
    end

    # Apply torque on the body in Newton-meters (kg * m/s/s * m).
    # @note The {#add_torque} applies torque on an object for one world update.
    #   For example, if the world update rate is 3, i.e if world updates 3 times
    #   per frame, the torque accumulated by the {#add_torque} function will be
    #   applied on the first update but not on the consequent two updates. Use
    #   the {#add_torque2} function to apply torque throughout the whole world
    #   update rate.
    # @note Unlike the {#set_torque}, this function doesn't overwrites original
    #   torque, but rather adds torque to the torque accumulator.
    # @overload add_torque(torque)
    #   @param [Geom::Vector3d, Array<Numeric>] torque
    # @overload add_torque(tx, ty, tz)
    #   @param [Numeric] tx
    #   @param [Numeric] ty
    #   @param [Numeric] tz
    # @return [Boolean] success
    # @see {#add_torque2}
    def add_torque(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.add_torque(@_address, data)
    end

    # Apply torque on the body in Newton-meters (kg * m/s/s * m).
    # @note The {#add_torque} applies torque on an object for one world update.
    #   For example, if the world update rate is 3, i.e if world updates 3 times
    #   per frame, the torque accumulated by the {#add_torque} function will be
    #   applied on the first update but not on the consequent two updates. Use
    #   the {#add_torque2} function to apply torque throughout the whole world
    #   update rate.
    # @note Unlike the {#set_torque}, this function doesn't overwrites original
    #   torque, but rather adds torque to the torque accumulator.
    # @overload add_torque2(torque)
    #   @param [Geom::Vector3d, Array<Numeric>] torque
    # @overload add_torque2(tx, ty, tz)
    #   @param [Numeric] tx
    #   @param [Numeric] ty
    #   @param [Numeric] tz
    # @return [Boolean] success
    # @see {#add_torque}
    def add_torque2(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.add_torque2(@_address, data)
    end

    # Apply torque on the body in Newton-meters (kg * m/s/s * m).
    # @note The {#set_torque} applies torque on an object for one world update.
    #   For example, if the world update rate is 3, i.e if world updates 3 times
    #   per frame, the torque accumulated by the {#set_torque} function will be
    #   applied on the first update but not on the consequent two updates. Use
    #   the {#set_torque2} function to apply torque throughout the whole world
    #   update rate.
    # @note Unlike the {#add_torque}, this function overwrites original torque,
    #   thus discarding the previously applied torque.
    # @overload set_torque(torque)
    #   @param [Geom::Vector3d, Array<Numeric>] torque
    # @overload set_torque(tx, ty, tz)
    #   @param [Numeric] tx
    #   @param [Numeric] ty
    #   @param [Numeric] tz
    # @return [Boolean] success
    # @see {#set_torque2}
    def set_torque(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_torque(@_address, data)
    end

    # Apply torque on the body in Newton-meters (kg * m/s/s * m).
    # @note The {#set_torque} applies torque on an object for one world update.
    #   For example, if the world update rate is 3, i.e if world updates 3 times
    #   per frame, the torque accumulated by the {#set_torque} function will be
    #   applied on the first update but not on the consequent two updates. Use
    #   the {#set_torque2} function to apply torque throughout the whole world
    #   update rate.
    # @note Unlike the {#add_torque}, this function overwrites original torque,
    #   thus discarding the previously applied torque.
    # @overload set_torque2(torque)
    #   @param [Geom::Vector3d, Array<Numeric>] torque
    # @overload set_torque2(tx, ty, tz)
    #   @param [Numeric] tx
    #   @param [Numeric] ty
    #   @param [Numeric] tz
    # @return [Boolean] success
    # @see {#set_torque}
    def set_torque2(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_torque2(@_address, data)
    end

    # @!endgroup
    # @!group Contact Related Functions

    # Get total force generated from contacts on the body.
    # @return [Geom::Vector3d] Magnitude of the net force is retrieved in
    #   newtons (kg*m/s/s).
    def get_net_contact_force
      Body.validate(self)
      MSPhysics::Newton::Body.get_net_contact_force(@_address)
    end

    # Get all contacts on the body.
    # @param [Boolean] inc_non_collidable Whether to include contacts from
    #   non-collidable bodies.
    # @return [Array<Contact>]
    def get_contacts(inc_non_collidable)
      Body.validate(self)
      contacts = []
      ovs = MSPhysics::Newton.is_object_validation_enabled?
      MSPhysics::Newton.enable_object_validation(false)
      MSPhysics::Newton::Body.get_contacts(@_address, inc_non_collidable).each { |data|
        body = MSPhysics::Newton::Body.get_user_data(data[0])
        if body.is_a?(Body)
          contacts << Contact.new(body, data[1], data[2], data[3], data[4])
        end
      }
      MSPhysics::Newton.enable_object_validation(ovs)
      contacts
    end

    # Get all bodies that are in contact with this body.
    # @param [Boolean] inc_non_collidable Whether to include contacts from
    #   non-collidable bodies.
    # @return [Array<Body>]
    def get_touching_bodies(inc_non_collidable)
      Body.validate(self)
      bodies = []
      ovs = MSPhysics::Newton.is_object_validation_enabled?
      MSPhysics::Newton.enable_object_validation(false)
      MSPhysics::Newton::Body.get_touching_bodies(@_address, inc_non_collidable).each { |address|
        data = MSPhysics::Newton::Body.get_user_data(address)
        bodies << data if data.is_a?(Body)
      }
      MSPhysics::Newton.enable_object_validation(ovs)
      bodies
    end

    # Determine if this body is in contact with another body.
    # @param [Body] body A body to test.
    # @return [Boolean]
    def is_touching_with?(body)
      Body.validate2(self, body)
      MSPhysics::Newton::Bodies.touching?(@_address, body.get_address)
    end

    # Get all contact points on the body.
    # @param [Boolean] inc_non_collidable Whether to included contacts from
    #   non-collidable bodies.
    # @return [Array<Geom::Point3d>]
    def get_contact_points(inc_non_collidable)
      Body.validate(self)
      points = []
      MSPhysics::Newton::Body.get_contact_points(@_address, inc_non_collidable)
    end

    # @!endgroup

    # Get collision faces of the body.
    # @return [Array<Array<Geom::Point3d>>] An array of faces. Each face
    #   represents an array of points. Points are coordinated in global space.
    def get_collision_faces
      Body.validate(self)
      MSPhysics::Newton::Body.get_collision_faces(@_address)
    end

    # Get collision faces of the body.
    # @return [Array<Array<(Array<Geom::Point3d>, Geom::Point3d, Geom::Vector3d, Numeric)>>]
    #  An array of face data. Each face data contains four elements. The first
    #  element contains the array of face vertices (sorted counterclockwise) in
    #  global space. The second element represents face centroid in global
    #  space. The third element represents face normal in global space. And the
    #  last element represents face area in inches squared.
    def get_collision_faces2
      Body.validate(self)
      MSPhysics::Newton::Body.get_collision_faces2(@_address)
    end

    # Get collision faces of the body.
    # @return [Array<Array<(Geom::Point3d, Geom::Vector3d, Numeric)>>] An array
    #  of face data. Each face data contains three elements. The first element
    #  represents face centroid in global space. The second element represents
    #  face normal in global space. And the last element represents face area in
    #  inches squared.
    def get_collision_faces3
      Body.validate(self)
      MSPhysics::Newton::Body.get_collision_faces3(@_address)
    end

    # Apply pick and drag on the body.
    # @param [Geom::Point3d, Array<Numeric>] pick_pt Pick point, usually on the
    #   surface of the body, in global space.
    # @param [Geom::Point3d, Array<Numeric>] dest_pt Destination point in global
    #   space.
    # @param [Numeric] stiffness Pick and drag stiffness.
    # @param [Numeric] damp Pick and drag damper.
    # @return [Boolean] success
    def apply_pick_and_drag(pick_pt, dest_pt, stiffness, damp)
      Body.validate(self)
      MSPhysics::Newton::Body.apply_pick_and_drag(@_address, pick_pt, dest_pt, stiffness, damp)
    end

    # Apply buoyancy on the body.
    # @param [Geom::Point3d, Array<Numeric>] origin Plane origin.
    # @param [Geom::Vector3d, Array<Numeric>] normal Plane normal.
    # @param [Geom::Vector3d, Array<Numeric>] current Plane acceleration in
    #   global space.
    # @param [Numeric] density Fluid density in kilograms per cubic meter
    #   (kg/m^3).
    # @param [Numeric] linear_viscosity Linear viscosity, a value
    #   between 0.0 and 1.0.
    # @param [Numeric] angular_viscosity Angular viscosity, a value
    #   between 0.0 and 1.0.
    # @return [Boolean] success
    def apply_buoyancy(plane_origin, plane_normal = Z_AXIS, current = [0,0,0], density = 997.04, linear_viscosity = 0.01, angular_viscosity = 0.01)
      Body.validate(self)
      MSPhysics::Newton::Body.apply_buoyancy(@_address, plane_origin, plane_normal, density, linear_viscosity, angular_viscosity)
    end

    # Apply fluid resistance on a body. The resistance force and torque is based
    # upon the body's velocity, omega, and orientation of its collision faces.
    # @param [Numeric] density Fluid density in kg/m^3.
    # @return [Array<(Geom::Vector3d, Geom::Vector3d)>, nil] The net force and
    #   torque applied on the body or nil if body is static or not dynamic.
    def apply_fluid_resistance(density = 1.225)
      Body.validate(self)
      MSPhysics::Newton::Body.apply_fluid_resistance(@_address, density)
    end

    # Create a copy of the body.
    # @overload copy(reapply_forces)
    #   @param [Boolean] reapply_forces Whether to reapply force and torque.
    # @overload copy(transformation, reapply_forces)
    #   @param [Geom::Transformation, Array<Numeric>] transformation New
    #     transformation matrix.
    #   @param [Boolean] reapply_forces Whether to reapply force and torque.
    #   @raise [TypeError] if the specified transformation matrix is not
    #     uniform.
    # @return [Body] A new body object.
    def copy(*args)
      Body.validate(self)
      if args.size == 1
        Body.new(self, nil, args[0])
      elsif args.size == 2
        Body.new(self, args[0], args[1])
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 2 arguments, but got #{args.size}.", caller)
      end
    end

    # Enable/Disable gravitational force on this body.
    # @param [Boolean] state
    # @return [Boolean] The new state
    def enable_gravity(state)
      Body.validate(self)
      MSPhysics::Newton::Body.enable_gravity(@_address, state)
    end

    # Determine if gravitational force is enabled on this body.
    # @return [Boolean]
    def is_gravity_enabled?
      Body.validate(self)
      MSPhysics::Newton::Body.is_gravity_enabled?(@_address)
    end

    # Get joints whose parent bodies associate to this body.
    # @return [Array<Joint>]
    def get_contained_joints
      Body.validate(self)
      joints = []
      ovs = MSPhysics::Newton.is_object_validation_enabled?
      MSPhysics::Newton.enable_object_validation(false)
      MSPhysics::Newton::Body.get_contained_joints(@_address).each { |joint_address|
        data = MSPhysics::Newton::Joint.get_user_data(joint_address)
        joints << data if data.is_a?(MSPhysics::Joint)
      }
      MSPhysics::Newton.enable_object_validation(ovs)
      joints
    end

    # Get joints whose child bodies associate to this body.
    # @return [Array<Joint>]
    def get_connected_joints
      Body.validate(self)
      joints = []
      ovs = MSPhysics::Newton.is_object_validation_enabled?
      MSPhysics::Newton.enable_object_validation(false)
      MSPhysics::Newton::Body.get_connected_joints(@_address).each { |joint_address|
        data = MSPhysics::Newton::Joint.get_user_data(joint_address)
        joints << data if data.is_a?(MSPhysics::Joint)
      }
      MSPhysics::Newton.enable_object_validation(ovs)
      joints
    end

    # Get all bodies connected to this body through joints.
    # @return [Array<Body>]
    def get_connected_bodies
      Body.validate(self)
      bodies = []
      ovs = MSPhysics::Newton.is_object_validation_enabled?
      MSPhysics::Newton.enable_object_validation(false)
      MSPhysics::Newton::Body.get_connected_bodies(@_address).each { |body_address|
        data = MSPhysics::Newton::Body.get_user_data(body_address)
        bodies << data if data.is_a?(MSPhysics::Body)
      }
      MSPhysics::Newton.enable_object_validation(ovs)
      bodies
    end

    # Get body collision scale.
    # @note Does not include group scale.
    # @return [Geom::Vector3d] A vector representing the xaxis, yaxis, and zaxis
    #   scale factors of the collision.
    def get_collision_scale
      Body.validate(self)
      MSPhysics::Newton::Body.get_collision_scale(@_address)
    end

    # Set body collision scale.
    # @note Does not include group scale.
    # @overload set_collision_scale(scale)
    #   @param [Geom::Vector3d, Array<Numeric>] scale A vector representing
    #     xaxis, yaxis, and zaxis scale factors of the body collision.
    # @overload set_collision_scale(sx, sy, sz)
    #   @param [Numeric] sx Scale along the xaxis of the body, a value between
    #     0.01 and 100.
    #   @param [Numeric] sy Scale along the yaxis of the body, a value between
    #     0.01 and 100.
    #   @param [Numeric] sz Scale along the zaxis of the body, a value between
    #     0.01 and 100.
    # @return [Geom::Vector3d] A vector representing the new xaxis, yaxis, and
    #   zaxis scale factors of the body collision.
    def set_collision_scale(*args)
      Body.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      res = MSPhysics::Newton::Body.set_collision_scale(@_address, data)
      @_group.move!(MSPhysics::Newton::Body.get_matrix(@_address)) if @_group.valid?
      res
    end

    # Get default scale of the body collision.
    # @note Does not include group scale.
    # @return [Geom::Vector3d] A vector representing the default xaxis, yaxis,
    #   and zaxis scale factors of the collision.
    def get_default_collision_scale
      Body.validate(self)
      MSPhysics::Newton::Body.get_default_collision_scale(@_address)
    end

    # Get scale of the body matrix that is a product of group scale and
    # collision scale.
    # @return [Geom::Vector3d] A vector representing the xaxis, yaxis, and zaxis
    #   scale factors of the actual body matrix.
    def get_actual_matrix_scale
      Body.validate(self)
      MSPhysics::Newton::Body.get_actual_matrix_scale(@_address)
    end

    # Make the body's Z-AXIS to look in a particular direction.
    # @param [Geom::Vector3d, nil] pin_dir Direction in global space. Pass nil
    #   to disable the look at constraint.
    # @param [Numeric] stiff Rotational stiffness.
    # @param [Numeric] damp Rotational damper.
    # @return [void]
    # @example
    #   onUpdate {
    #     if (key(' ') == 1)
    #       look_at(this.get_group.transformation.origin.vector_to(ORIGIN), 1500, 200)
    #     else
    #       look_at(nil)
    #     end
    #   }
    def look_at(pin_dir, stiff = 1500, damp = 200)
      Body.validate(self)
      if pin_dir.nil?
        if @_look_at_constraint && @_look_at_constraint.valid?
          @_look_at_constraint.destroy
          @_look_at_constraint = nil
        end
        return
      end
      pin_dir = Geom::Vector3d.new(pin_dir) unless pin_dir.is_a?(Geom::Vector3d)
      if @_look_at_constraint.nil? || !@_look_at_constraint.valid?
        @_look_at_constraint = MSPhysics::UpVector.new(get_world, nil, Geom::Transformation.new(ORIGIN, @_group.transformation.zaxis))
        @_look_at_constraint.connect(self)
      end
      @_look_at_constraint.set_pin_dir(pin_dir.transform(@_look_at_constraint.get_pin_matrix.inverse))
      @_look_at_constraint.stiff = stiff
      @_look_at_constraint.damp = damp
      @_look_at_constraint.damper_enabled = true
    end

    # Assign a block of code to an event or a list of events.
    # @example
    #   on(:keyDown, :keyUp, :keyExtended){ |key, value, char|
    #     simulation.log_line(key)
    #   }
    # @param [Symbol, String] events
    # @yield A block of code.
    # @return [Fixnum] The number of events assigned.
    def on(*events, &block)
      Body.validate(self)
      count = 0
      events.flatten.each{ |evt|
        evt = evt.to_s.downcase
        evt.insert(0, 'on') if evt[0..1] != 'on'
        found = false
        @_events.keys.each{ |key|
          next if key.to_s.downcase != evt
          evt = key
          found = true
          break
        }
        next unless found
        @_events[evt] = block
        count += 1
      }
      state = @_events[:onTouch] || @_events[:onTouching] || @_events[:onUntouch]
      MSPhysics::Newton::Body.set_record_touch_data_state(@_address, state)
      count
    end

    # Get a Proc object assigned to an event.
    # @param [Symbol, String] event
    # @return [Proc, nil] A Proc object or +nil+ if there is no procedure to an
    #   event.
    def get_proc(event)
      Body.validate(self)
      @_events[event.to_sym]
    end

    # Assign a Proc object to an event.
    # @param [Symbol, String] event
    # @param [Proc, nil] proc A Proc object or +nil+ to remove procedure from an
    #   event.
    # @return [Boolean] success
    def set_proc(event, proc)
      Body.validate(self)
      AMS.validate_type(proc, Proc, NilClass)
      return false unless @_events.keys.include?(event.to_sym)
      @_events[event.to_sym] = proc
      if event.to_s =~ /onTouch|onTouching|onUntouch/
        state = @_events[:onTouch] || @_events[:onTouching] || @_events[:onUntouch]
        MSPhysics::Newton::Body.set_record_touch_data_state(@_address, state)
      end
      true
    end

    # Determine whether particular event has a Proc object.
    # @param [Symbol, String] event
    # @return [Boolean]
    def is_proc_assigned?(event)
      Body.validate(self)
      @_events[event.to_sym] != nil
    end

    # Trigger an event.
    # @api private
    # @param [Symbol, String] event Event name.
    # @param [*args] args Event arguments.
    # @return [Boolean] success
    def call_event(event, *args)
      Body.validate(self)
      return false unless @_script_state
      evt = @_events[event.to_sym]
      return false unless evt
      begin
        evt.call(*args)
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
        msg = "#{e.class.to_s[0] =~ /a|e|i|o|u/i ? 'An' : 'A'} #{e.class} has occurred while calling body #{event} event#{line ? ', line ' + line.to_s : nil}:\n#{e.message}"
        raise MSPhysics::ScriptException.new(msg, e.backtrace, @_group, line)
      end
      true
    end

    # Determine whether body script is enabled.
    # @return [Boolean] +true+ if enabled, +false+ if disabled.
    def get_script_state
      Body.validate(self)
      @_script_state
    end

    # Enable/Disable body script. Disabling script will prevent all events of
    # the body from being called.
    # @param [Boolean] state
    # @return [Boolean] The newly assigned state.
    def set_script_state(state)
      Body.validate(self)
      @_script_state = state ? true : false
    end

    # @!group Simulation Events

    # Assign a block of code to the onStart event.
    # @yield This event is triggered once when simulation starts, hence when the
    #   frame is zero. No transformation updates are made at this point.
    def onStart(&block)
      set_proc(:onStart, block)
    end

    # Assign a block of code to the onUpdate event.
    # @yield This event is triggered every frame after the simulation starts,
    #   hence when the frame is greater than zero. Specifically, it is called
    #   after the world update.
    def onUpdate(&block)
      set_proc(:onUpdate, block)
    end

    # Assign a block of code to the onPreUpdate event.
    # @yield This event is triggered every frame prior to the world update.
    def onPreUpdate(&block)
      set_proc(:onPreUpdate, block)
    end

    # Assign a block of code to the onUpdate event.
    # @yield This event is triggered every frame after the {#onUpdate} event is
    #   called.
    def onPostUpdate(&block)
      set_proc(:onPostUpdate, block)
    end

    # Assign a block of code to the onEnd event.
    # @yield This event is triggered once when simulation ends; right before the
    #   bodies are moved back to their starting transformation.
    def onEnd(&block)
      set_proc(:onEnd, block)
    end

    # Assign a block of code to the onDraw event.
    # @yield This event is triggered whenever the view is redrawn, even when
    #   simulation is paused.
    # @yieldparam [Sketchup::View] view
    # @yieldparam [Geom::BoundingBox] bb
    # @example
    #   onDraw { |view, bb|
    #     pts = [[0,0,100], [100,100,100]]
    #     # Add points to the view bounding box to prevent the line from being
    #     # clipped.
    #     bb.add(pts)
    #     # Now, draw the line in red.
    #     view.drawing_color = 'red'
    #     view.draw(GL_LINES, pts)
    #   }
    def onDraw(&block)
      set_proc(:onDraw, block)
    end

    # Assign a block of code to the onPlay event.
    # @yield This event is triggered when simulation is played. It is not called
    #   when simulation starts.
    def onPlay(&block)
      set_proc(:onPlay, block)
    end

    # Assign a block of code to the onPause event.
    # @yield This event is triggered when simulation is paused.
    def onPause(&block)
      set_proc(:onPause, block)
    end

    # Assign a block of code to the onTouch event.
    # @yield This event is triggered when this body comes in contact with
    #   another body.
    # @yieldparam [Body] toucher
    # @yieldparam [Geom::Point3d] point
    # @yieldparam [Geom::Vector3d] normal
    # @yieldparam [Geom::Vector3d] force in Newtons.
    # @yieldparam [Numeric] speed in meters per second.
    def onTouch(&block)
      set_proc(:onTouch, block)
    end

    # Assign a block of code to the onTouching event.
    # @yield This event is triggered every frame when the body is in an extended
    #   contact with another body.
    # @yieldparam [Body] toucher
    # @yieldparam [Geom::Point3d] point
    # @yieldparam [Geom::Vector3d] normal
    def onTouching(&block)
      set_proc(:onTouching, block)
    end

    # Assign a block of code to the onUntouch event.
    # @yield This event is triggered when particular body is no longer in
    #   contact with another body. When this procedure is triggered it doesn't
    #   always mean the body is free from all contacts. This means particular
    #   +toucher+ has stopped touching the body.
    # @note Sometimes you may want to know whether particular body is in contact
    #   with another body. Relying on events is not always the best technique.
    #   To determine whether this body is in contact with another body, use
    #   {#is_touching_with?}, or {#get_touching_bodies} to get all contacting
    #   bodies in particular.
    # @yieldparam [Body] toucher
    def onUntouch(&block)
      set_proc(:onUntouch, block)
    end

    # Assign a block of code to the onClick event.
    # @yield This event is triggered when the body is clicked.
    # @yieldparam [Geom::Point3d] pos Clicked position in global space.
    def onClick(&block)
      set_proc(:onClick, block)
    end

    # Assign a block of code to the onDrag event.
    # @yield This event is triggered whenever the body is dragged by a mouse.
    def onDrag(&block)
      set_proc(:onDrag, block)
    end

    # Assign a block of code to the onUnclick event.
    # @yield This event is triggered when the body is unclicked.
    def onUnclick(&block)
      set_proc(:onUnclick, block)
    end

    # @!endgroup
    # @!group User Input Events

    # Assign a block of code to the onKeyDown event.
    # @yield This event is called when the key is pressed.
    # @yieldparam [String] key Virtual key name.
    # @yieldparam [Fixnum] val Virtual key constant value.
    # @yieldparam [String] char Actual key character.
    def onKeyDown(&block)
      set_proc(:onKeyDown, block)
    end

    # Assign a block of code to the onKeyUp event.
    # @yield This event is called when the key is released.
    # @yieldparam [String] key Virtual key name.
    # @yieldparam [Fixnum] val Virtual key constant value.
    # @yieldparam [String] char Actual key character.
    def onKeyUp(&block)
      set_proc(:onKeyUp, block)
    end
    # Assign a block of code to the onKeyExtended event.
    # @yield This event is called when the key is held down.
    # @yieldparam [String] key Virtual key name.
    # @yieldparam [Fixnum] val Virtual key constant value.
    # @yieldparam [String] char Actual key character.
    def onKeyExtended(&block)
      set_proc(:onKeyExtended, block)
    end

    # Assign a block of code to the onMouseMove event.
    # @yield This event is called when the mouse is moved.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onMouseMove(&block)
      set_proc(:onMouseMove, block)
    end

    # Assign a block of code to the onLButtonDown event.
    # @yield This event is called when the left mouse button is pressed.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onLButtonDown(&block)
      set_proc(:onLButtonDown, block)
    end

    # Assign a block of code to the onLButtonUp event.
    # @yield This event is called when the left mouse button is released.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onLButtonUp(&block)
      set_proc(:onLButtonUp, block)
    end

    # Assign a block of code to the onLButtonDoubleClick event.
    # @yield This event is called when the left mouse button is double clicked.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onLButtonDoubleClick(&block)
      set_proc(:onLButtonDoubleClick, block)
    end

    # Assign a block of code to the onRButtonDown event.
    # @yield This event is called when the right mouse button is pressed.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onRButtonDown(&block)
      set_proc(:onRButtonDown, block)
    end

    # Assign a block of code to the onRButtonUp event.
    # @yield This event is called when the right mouse button is released.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onRButtonUp(&block)
      set_proc(:onRButtonUp, block)
    end

    # Assign a block of code to the onRButtonDoubleClick event.
    # @yield This event is called when the right mouse button is double clicked.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onRButtonDoubleClick(&block)
      set_proc(:onRButtonDoubleClick, block)
    end

    # Assign a block of code to the onMButtonDown event.
    # @yield This event is called when the middle mouse button is pressed.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onMButtonDown(&block)
      set_proc(:onMButtonDown, block)
    end

    # Assign a block of code to the onMButtonUp event.
    # @yield This event is called when the middle mouse button is released.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onMButtonUp(&block)
      set_proc(:onMButtonUp, block)
    end

    # Assign a block of code to the onMButtonDoubleClick event.
    # @yield This event is called when the middle mouse button is double clicked.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onMButtonDoubleClick(&block)
      set_proc(:onMButtonDoubleClick, block)
    end

    # Assign a block of code to the onXButton1Down event.
    # @yield This event is called when the X1 mouse button is pressed.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onXButton1Down(&block)
      set_proc(:onXButton1Down, block)
    end

    # Assign a block of code to the onXButton1Up event.
    # @yield This event is called when the X1 mouse button is released.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onXButton1Up(&block)
      set_proc(:onXButton1Up, block)
    end

    # Assign a block of code to the onXButton1DoubleClick event.
    # @yield This event is called when the X1 mouse button is double clicked.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onXButton1DoubleClick(&block)
      set_proc(:onXButton1DoubleClick, block)
    end

    # Assign a block of code to the onXButton2Down event.
    # @yield This event is called when the X2 mouse button is pressed.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onXButton2Down(&block)
      set_proc(:onXButton2Down, block)
    end

    # Assign a block of code to the onXButton2Up event.
    # @yield This event is called when the X2 mouse button is released.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onXButton2Up(&block)
      set_proc(:onXButton2Up, block)
    end

    # Assign a block of code to the onXButton2DoubleClick event.
    # @yield This event is called when the X2 mouse button is double clicked.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    def onXButton2DoubleClick(&block)
      set_proc(:onXButton2DoubleClick, block)
    end

    # Assign a block of code to the onMouseWheelRotate event.
    # @yield This event is called when the mouse wheel is rotated.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    # @yieldparam [Fixnum] dir Rotate direction: +-1+ - down, +1+ - up.
    def onMouseWheelRotate(&block)
      set_proc(:onMouseWheelRotate, block)
    end

    # Assign a block of code to the onMouseWheelTilt event.
    # @yield This event is called when the mouse wheel is tilted.
    # @yieldparam [Fixnum] x
    # @yieldparam [Fixnum] y
    # @yieldparam [Fixnum] dir Tilt direction: +-1+ - left, +1+ - right.
    def onMouseWheelTilt(&block)
      set_proc(:onMouseWheelTilt, block)
    end

    # @!endgroup
  end # class Body
end # module MSPhysics
