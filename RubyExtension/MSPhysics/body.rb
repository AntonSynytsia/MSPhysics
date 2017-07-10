module MSPhysics

  # The Body class represents a physics body in simulation. Every body in
  # simulation is designed to have its own Body object.
  # @since 1.0.0
  class Body < Entity
    class << self

      # Verify that body is valid.
      # @api private
      # @param [Body] body
      # @param [World, nil] world A world the body ought to belong to or +nil+.
      # @raise [TypeError] if body is invalid or destroyed.
      # @return [void]
      def validate(body, world = nil)
        AMS.validate_type(body, MSPhysics::Body)
        unless body.valid?
          raise(TypeError, "Body #{body} is invalid/destroyed!", caller)
        end
        if world != nil
          AMS.validate_type(world, MSPhysics::World)
          if body.world.address != world.address
            raise(TypeError, "Body #{body} belongs to a different world!", caller)
          end
        end
      end

      # Verify that two bodies are valid and unique.
      # @api private
      # @param [Body] body1
      # @param [Body] body2
      # @param [World, nil] world A world the body ought to belong to or +nil+.
      # @return [void]
      # @raise [TypeError] if at least one body is invalid or destroyed.
      # @raise [TypeError] if both bodies link to the same address.
      def validate2(body1, body2, world = nil)
        AMS.validate_type(body1, MSPhysics::Body)
        AMS.validate_type(body2, MSPhysics::Body)
        unless body1.valid?
          raise(TypeError, "Body1 #{body1} is invalid/destroyed!", caller)
        end
        unless body2.valid?
          raise(TypeError, "Body1 #{body1} is invalid/destroyed!", caller)
        end
        if body1.address == body2.address
          raise(TypeError, "Body1 #{body1} and body2 #{body2} link to the same address. Expected two unique bodies!", caller)
        end
        if world != nil
          AMS.validate_type(world, MSPhysics::World)
          if body1.world.address != world.address
            raise(TypeError, "Body1 #{body1} belongs to a different world!", caller)
          end
          if body2.world.address != world.address
            raise(TypeError, "Body2 #{body2} belongs to a different world!", caller)
          end
        end
      end

      # Get body by body address.
      # @param [Fixnum] address
      # @return [Body, nil]
      def body_by_address(address)
        data = MSPhysics::Newton::Body.get_user_data(address.to_i)
        data.is_a?(MSPhysics::Body) ? data : nil
      end

      # Determine if the bounding boxes of two bodies overlap.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Boolean]
      def bodies_aabb_overlap?(body1, body2)
        validate2(body1, body2)
        MSPhysics::Newton::Bodies.aabb_overlap?(body1.address, body2.address)
      end

      # Determine if two bodies can collide with each other.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Boolean]
      def bodies_collidable?(body1, body2)
        validate2(body1, body2)
        MSPhysics::Newton::Bodies.collidable?(body1.address, body2.address)
      end

      # Determine if two bodies are in contact.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Boolean]
      def bodies_touching?(body1, body2)
        validate2(body1, body2)
        MSPhysics::Newton::Bodies.touching?(body1.address, body2.address)
      end

      # Get closest collision points between two bodies.
      # @note This works with convex and compound bodies only. Nil will be
      #   returned if one the passed bodies have a static mesh or a null
      #   collision.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Array<Geom::Point3d>, nil] +[contact_pt1, contact_pt2]+
      def closest_points(body1, body2)
        validate2(body1, body2)
        MSPhysics::Newton::Bodies.get_closest_points(body1.address, body2.address)
      end

      # Get contact force between two bodies.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Geom::Vector3d] force in newtons (kg*m/s/s).
      def force_between_bodies(body1, body2)
        validate2(body1, body2)
        MSPhysics::Newton::Bodies.get_force_in_between(body1.address, body2.address)
      end

      # Get all bodies.
      # @note Bodies that do not have a {Body} instance are not included in the
      #   array.
      # @return [Array<Body>]
      def all_bodies
        MSPhysics::Newton.get_all_bodies() { |ptr, data| data.is_a?(MSPhysics::Body) ? data : nil }
      end

    end # class << self

    # @overload initialize(world, entity, shape_id, offset_tra, type_id)
    #   Create a new body.
    #   @note The specified entity must have all axis perpendicular to each
    #     other; otherwise, it is considered invalid. The entity will also be
    #     considered invalid if its collision shape turns out flat (not
    #     considering +"null"+ or +"static_mesh"+ collision shapes).
    #   @param [World] world
    #   @param [Sketchup::Group, Sketchup::ComponentInstance] entity
    #   @param [Fixnum] shape_id Shape ID. See {MSPhysics::SHAPE_NAMES}
    #   @param [Geom::Transformation, nil] offset_tra A local transform to apply
    #     to the collision.
    #   @param [Fixnum] type_id Body type: 0 -> dynamic; 1 -> kinematic.
    #   @raise [TypeError] if the specified world is invalid.
    #   @raise [TypeError] if the specified entity is invalid.
    #   @raise [TypeError] if the specified collision shape is invalid.
    # @overload initialize(body, transformation, reapply_forces, type_id)
    #   Create a clone of an existing body.
    #   @param [Body] body A body Object.
    #   @param [Geom::Transformation, Array<Numeric>, nil] transformation New
    #     transformation matrix or +nil+ to create new body at the current
    #     location.
    #   @param [Boolean] reapply_forces Whether to reapply force and torque.
    #   @param [Fixnum] type_id Body type: 0 -> dynamic; 1 -> kinematic.
    #   @raise [TypeError] if the specified body is invalid.
    #   @raise [TypeError] if the specified transformation matrix is not acceptable.
    def initialize(*args)
      if args.size == 5
        MSPhysics::World.validate(args[0])
        MSPhysics::Collision.validate_entity(args[1])
        @group = args[1]
        collision = MSPhysics::Collision.create(args[0], @group, args[2], args[3])
        @address = MSPhysics::Newton::Body.create(args[0].address, collision, @group.transformation, args[4], args[0].default_material_id, @group)
        if args[2] == 0
          bb = AMS::Group.get_bounding_box_from_faces(@group, true, nil, &MSPhysics::Collision::ENTITY_VALIDATION_PROC)
          scale = AMS::Geometry.get_matrix_scale(@group.transformation)
          c = bb.center
          c.x *= scale.x
          c.y *= scale.y
          c.z *= scale.z
          MSPhysics::Newton::Body.set_centre_of_mass(@address, c)
          if bb.width.to_f < MSPhysics::EPSILON || bb.height.to_f < MSPhysics::EPSILON || bb.depth.to_f < MSPhysics::EPSILON
            MSPhysics::Newton::Body.set_volume(@address, 1.0)
          else
            v = bb.width * bb.height * bb.depth * 0.0254**3
            MSPhysics::Newton::Body.set_volume(@address, v)
          end
        end
        MSPhysics::Newton::Collision.destroy(collision)
      elsif args.size == 4
        # Create a clone of an existing body.
        Body.validate(args[0])
        orig_group = args[0].group
        unless orig_group.valid?
          raise(TypeError, 'The specified body references a deleted group/component. Copying bodies with invalid linked entities is not acceptable!', caller)
        end
        definition = AMS::Group.get_definition(orig_group)
        @group = Sketchup.active_model.entities.add_instance(definition, orig_group.transformation)
        @group.name = orig_group.name
        @group.layer = orig_group.layer
        @group.material = orig_group.material
        @address = MSPhysics::Newton::Body.copy(args[0].address, args[1], args[2], args[3], @group)
        @group.transformation = MSPhysics::Newton::Body.get_matrix(@address)
        # Preserve source definition
        if MSPhysics::Replay.record_enabled? && @group.is_a?(Sketchup::Group)
          MSPhysics::Replay.preset_definition(@group, definition)
        end
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 4..5 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_user_data(@address, self)
      @context = MSPhysics::BodyContext.new(self)
      @look_at_joint = nil
      @attached_bodies = {}
    end

    # Determine whether this body is valid - not destroyed.
    # @return [Boolean]
    def valid?
      MSPhysics::Newton::Body.is_valid?(@address)
    end

    # Get pointer to the body.
    # @return [Fixnum]
    def address
      @address
    end

    # Get pointer to the collision associated with the body.
    # @return [Fixnum]
    def collision_address
      MSPhysics::Newton::Body.get_collision(@address)
    end

    # Get the group/component associated with the body.
    # @return [Sketchup::Group, Sketchup::ComponentInstance]
    def group
      @group
    end

    # Get body type.
    # @return [Fixnum] Type: 0 -> dynamic; 1 -> kinematic
    def type
      MSPhysics::Newton::Body.get_type(@address)
    end

    # Get world in which the body was created.
    # @return [World]
    def world
      world_address = MSPhysics::Newton::Body.get_world(@address)
      MSPhysics::Newton::World.get_user_data(world_address)
    end

    # Get the associated context.
    # @return [BodyContext]
    def context
      @context
    end

    # Destroy the body.
    # @param [Boolean] erase_entity Whether to erase the group/component
    #   associated with the body.
    # @return [nil]
    def destroy(erase_entity = false)
      @group.erase! if @group.valid? && erase_entity
      MSPhysics::Newton::Body.destroy(@address)
    end

    # Determine whether continuous collision check is enabled for this body.
    # Continuous collision check prevents this body from passing through other
    # bodies at high speeds.
    # @return [Boolean] Returns +true+ if continuous collision check is on;
    #   +false+ if continuous collision check is off.
    def continuous_collision_check_enabled?
      MSPhysics::Newton::Body.get_continuous_collision_state(@address)
    end

    # Enable/disable continuous collision check for this body. Continuous
    # collision check prevents this body from passing through other bodies at
    # high speeds.
    # @note Continuous collision check is known to affect performance. Be
    #   cautions when using it. When performing box stacks it's better to reduce
    #   simulation update step, to 1/256 for instance, rather than enabling
    #   continuous collision check as smaller update step will keep simulation
    #   running smoothly while avoiding penetration at the same time.
    # @param [Boolean] state Pass +true+ to enable continuous collision check;
    #   +false+ to disable continuous collision check.
    def continuous_collision_check_enabled=(state)
      MSPhysics::Newton::Body.set_continuous_collision_state(@address, state)
    end

    # Get body matrix with no scale factors.
    # @return [Geom::transformation]
    def normal_matrix
      MSPhysics::Newton::Body.get_normal_matrix(@address)
    end

    # Get body transformation matrix.
    # @return [Geom::Transformation]
    def get_matrix
      MSPhysics::Newton::Body.get_matrix(@address)
    end

    # Set body transformation matrix.
    # @param [Geom::Transformation, Array<Numeric>] matrix
    # @return [nil]
    # @raise [TypeError] if some or all matrix axis are not perpendicular to
    #   each other.
    # @raise [TypeError] if some or all matrix axis have a scale of zero.
    def set_matrix(matrix)
      MSPhysics::Newton::Body.set_matrix(@address, matrix)
    end

    # Get body position.
    # @param [Fixnum] mode
    #   * 0 - get body's origin in global space.
    #   * 1 - get body's centre of mass in global space.
    # @return [Geom::Point3d]
    def get_position(mode = 0)
      MSPhysics::Newton::Body.get_position(@address, mode.to_i)
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
    # @return [nil]
    def set_position(*args)
      mode = 0
      if args.size == 4
        point = [args[0], args[1], args[2]]
        mode = args[3]
      elsif args.size == 3
        point = [args[0], args[1], args[2]]
      elsif args.size == 2
        point = args[0]
        mode = args[1]
      elsif args.size == 1
        point = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1..4 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_position(@address, point, mode)
    end

    # Get body orientation in form of the unit quaternion.
    # @return [Array<Numeric>] An array of four numeric values:
    #   +[q0, q1, q2, q3]+ - +[x,y,z,w]+.
    def rotation
      MSPhysics::Newton::Body.get_rotation(@address)
    end

    # Get body orientation in form of the three Euler angles.
    # @return [Geom::Vector3d] An vector of three Euler angles expressed in
    #   radians: +(roll, yaw, pitch)+.
    def get_euler_angles
      MSPhysics::Newton::Body.get_euler_angles(@address)
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
    # @return [nil]
    def set_euler_angles(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_euler_angles(@address, data)
    end

    # Get global linear velocity of the body.
    # @return [Geom::Vector3d] The magnitude of the velocity vector is
    #   represented in meters per second.
    def get_velocity
      MSPhysics::Newton::Body.get_velocity(@address)
    end

    # Set global linear velocity of the body.
    # @overload set_velocity(velocity)
    #   @param [Geom::Vector3d, Array<Numeric>] velocity The magnitude of the
    #     velocity vector is assumed in meters per second.
    # @overload set_velocity(vx, vy, vz)
    #   @param [Numeric] vx Velocity in meters per second along X-axis.
    #   @param [Numeric] vy Velocity in meters per second along Y-axis.
    #   @param [Numeric] vz Velocity in meters per second along Z-axis.
    # @return [nil]
    def set_velocity(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_velocity(@address, data)
    end

    # Integrate linear and angular velocity of the body.
    # @note For this function to have an effect, the body must be kinematic and
    #   have a non-zero mass (non-static).
    # @param [Numeric] timestep Timestep in seconds.
    # @return [nil]
    # @example
    #   # Assuming this body is kinematic
    #   onStart {
    #     @velocity = Geom::Vector3d.new(2,0,0)
    #     this.set_velocity(2,0,0)
    #   }
    #   onPreUpdate {
    #     this.integrate_velocity(simulation.update_timestep)
    #   }
    def integrate_velocity(timestep)
      MSPhysics::Newton::Body.integrate_velocity(@address, timestep)
    end

    # Get global angular velocity of the body.
    # @example
    #   Each value of the omega vector represents angular velocity in radians
    #   per second along X-axis, Y-axis, or Z-axis in global space. For example,
    #   if omega of a body is (0,0,PI), it means that the body rotates along
    #   Z-axis in global space at an angular velocity of 360 degrees per second.
    # @return [Geom::Vector3d] The magnitude of the omega vector is represented
    #   in radians per second.
    def get_omega
      MSPhysics::Newton::Body.get_omega(@address)
    end

    # Set global angular velocity of the body.
    # @overload set_omega(omega)
    #   @param [Geom::Vector3d, Array<Numeric>] omega The magnitude of the omega
    #     vector is assumed in radians per second.
    # @overload set_omega(vx, vy, vz)
    #   @param [Numeric] vx Omega in radians per second along X-axis.
    #   @param [Numeric] vy Omega in radians per second along Y-axis.
    #   @param [Numeric] vz Omega in radians per second along Z-axis.
    # @return [nil]
    def set_omega(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_omega(@address, data)
    end

    # Get centre of mass of the body in local coordinates.
    # @example Getting centre of mass in global space.
    #   centre = body.get_cetre_of_mass.transform( body.get_matrix )
    # @return [Geom::Point3d]
    # @see #get_position
    def get_centre_of_mass
      MSPhysics::Newton::Body.get_centre_of_mass(@address)
    end

    # Set centre of mass of the body in local coordinates.
    # @overload set_centre_of_mass(centre)
    #   @param [Geom::Point3d, Array<Numeric>] centre A point is assumed in body
    #     coordinates.
    # @overload set_velocity(px, py, pz)
    #   @param [Numeric] px X position in local space.
    #   @param [Numeric] py Y position in local space.
    #   @param [Numeric] pz Z position in local space.
    # @return [nil]
    def set_centre_of_mass(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_centre_of_mass(@address, data)
    end

    # Get body local inertia.
    # @note Inertia is the rotational equivalent of mass. It may be used for
    #   damping angular velocity through applying dissipative torque.
    # @note If the body is static or has a zero mass, the magnitude of the
    #   return inertia will be zero.
    # @return [Geom::Vector3d]
    # @example Applying angular damping
    #   onUpdate {
    #     # Damping ratio, a value between 0.00 - 1.00
    #     ratio = 0.1
    #     # Current angular velocity
    #     omega = this.get_omega
    #     # Get the rotational equivalent of mass
    #     inertia = this.inertia
    #     # Dissipative torque
    #     torque = Geom::Vector3d.new(
    #       -omega.x * inertia.x * ratio,
    #       -omega.y * inertia.y * ratio,
    #       -omega.z * inertia.z * ratio)
    #     # Apply torque
    #     this.add_torque(torque)
    #   }
    def inertia
      MSPhysics::Newton::Body.get_inertia(@address)
    end

    # Get body mass in kilograms (kg).
    # @return [Numeric]
    def mass
      MSPhysics::Newton::Body.get_mass(@address)
    end

    # Set body mass in kilograms (kg).
    # @note Mass and density are correlated. If you change mass the density will
    #   automatically be recalculated.
    # @param [Numeric] value
    def mass=(value)
      MSPhysics::Newton::Body.set_mass(@address, value)
    end

    # Get body density in kilograms per cubic meter (kg / m^3).
    # @return [Numeric]
    def density
      MSPhysics::Newton::Body.get_density(@address)
    end

    # Set body density in kilograms per cubic meter (kg / m^3).
    # @note Density and mass are correlated. If you change density the mass will
    #   automatically be recalculated.
    # @param [Numeric] value
    def density=(value)
      MSPhysics::Newton::Body.set_density(@address, value)
    end

    # Get body volume in cubic meters (m^3).
    # @return [Numeric]
    def volume
      MSPhysics::Newton::Body.get_volume(@address)
    end

    # Set body volume in cubic meters (m^3).
    # @note Volume and mass are correlated. If you change volume the mass will
    #   automatically be recalculated.
    # @param [Numeric] value
    def volume=(value)
      MSPhysics::Newton::Body.set_volume(@address, value)
    end

    # Reset/recalculate body volume and mass.
    # @param [Numeric] density
    # @return [Boolean] success
    def reset_mass_properties(density)
      MSPhysics::Newton::Body.reset_mass_properties(@address, density)
    end

    # Determine whether body is static.
    # @return [Boolean]
    def static?
      MSPhysics::Newton::Body.is_static?(@address)
    end

    # Set body static.
    # @param [Boolean] state +true+ to set body static, +false+ to set body
    #   dynamic.
    def static=(state)
      MSPhysics::Newton::Body.set_static(@address, state)
    end

    # Determine whether body is collidable.
    # @return [Boolean]
    def collidable?
      MSPhysics::Newton::Body.is_collidable?(@address)
    end

    # Set body collidable.
    # @param [Boolean] state +true+ to set body collidable, +false+ to set body
    #   non-collidable.
    def collidable=(state)
      MSPhysics::Newton::Body.set_collidable(@address, state)
    end

    # Determine whether body is frozen.
    # @return [Boolean]
    def frozen?
      MSPhysics::Newton::Body.is_frozen?(@address)
    end

    # Set body collidable.
    # @param [Boolean] state +true+ to freeze the body, +false+ unfreeze the
    #   body.
    def frozen=(state)
      MSPhysics::Newton::Body.set_frozen(@address, state)
    end

    # Determine whether body is sleeping. Sleeping bodies are bodies at rest.
    # @return [Boolean]
    def sleeping?
      MSPhysics::Newton::Body.is_sleeping?(@address)
    end

    # Set body sleeping.
    # @note This function can only set body active, the sleeping is controlled
    #   by equilibrium.
    # @param [Boolean] state +true+ to set body sleeping, +false+ to set body
    #   active.
    def sleeping=(state)
      MSPhysics::Newton::Body.set_sleeping(@address, state)
    end

    # Get the auto-sleep state of the body.
    # @return [Boolean] +true+ if body auto-sleep is on, +false+ if body
    #   auto-sleep is off.
    def auto_sleep_enabled?
      MSPhysics::Newton::Body.get_auto_sleep_state(@address)
    end

    # Set auto sleep state of the body. Auto sleep enables body to automatically
    # go to sleep mode when at rest or become active when activated.
    # @note Keeping auto sleep *on* is a huge performance plus for simulation.
    #   Auto sleep enabled is the default state for a created body; however, for
    #   player control, AI control or some other special circumstance, the
    #   application may want to control the activation/deactivation of the body.
    # @param [Boolean] state +true+ to set body auto-sleep on, or +false+ to set
    #   body auto-sleep off.
    def auto_sleep_enabled=(state)
      MSPhysics::Newton::Body.set_auto_sleep_state(@address, state)
    end

    # Determine whether this body is non-collidable with a particular body.
    # @param [Body] body The body to test.
    # @return [Boolean] +true+ if this body is non-collidable with the given
    #   body, +false+ if this body is collidable with the given body.
    def non_collidable_with?(body)
      MSPhysics::Newton::Body.is_non_collidable_with?(@address, body.address)
    end

    # Set this body non-collidable with a particular body.
    # @param [Body] body
    # @param [Boolean] state +true+ to set this body non-collidable with
    #   another, +false+ to set this body collidable with another.
    # @return [nil]
    def set_non_collidable_with(body, state)
      Body.validate2(self, body, self.world)
      MSPhysics::Newton::Body.set_non_collidable_with(@address, body.address, state)
    end

    # Get all bodies that are non-collidable with this body; the bodies that
    # were set non-collidable by the {#set_non_collidable_with} function.
    # @return [Array<Body>] An array of non-collidable bodies.
    def get_non_collidable_bodies
      MSPhysics::Newton::Body.get_non_collidable_bodies(@address) { |ptr, data|
        data.is_a?(MSPhysics::Body) ? data : nil
      }
    end

    # Remove all bodies from the non-collidable list; the bodies that were set
    # non-collidable by the {#set_non_collidable_with} function.
    # @return [Fixnum] The number of bodies unmarked.
    def clear_non_collidable_bodies
      MSPhysics::Newton::Body.clear_non_collidable_bodies(@address)
    end

    # Get body coefficient of restitution - bounciness - rebound ratio.
    # @return [Numeric] A value between 0.01 and 2.00.
    def elasticity
      MSPhysics::Newton::Body.get_elasticity(@address)
    end

    # Set body coefficient of restitution - bounciness - rebound ratio.
    # @example A basketball has a rebound ratio of 0.83. This means the new
    #   height of a basketball is 83% of original height within each bounce.
    # @param [Numeric] coefficient A value between 0.01 and 2.00.
    def elasticity=(coefficient)
      MSPhysics::Newton::Body.set_elasticity(@address, coefficient)
    end

    # Get contact softness coefficient of the body.
    # @return [Numeric] A value between 0.01 and 1.00.
    def softness
      MSPhysics::Newton::Body.get_softness(@address)
    end

    # Set contact softness coefficient of the body.
    # @param [Numeric] coefficient A value between 0.01 and 1.00.
    def softness=(coefficient)
      MSPhysics::Newton::Body.set_softness(@address, coefficient)
    end

    # Get static friction coefficient of the body.
    # @return [Numeric] A value between 0.01 and 2.00.
    def static_friction
      MSPhysics::Newton::Body.get_static_friction(@address)
    end

    # Set static friction coefficient of the body.
    # @param [Numeric] coefficient A value between 0.01 and 2.00.
    def static_friction=(coefficient)
      MSPhysics::Newton::Body.set_static_friction(@address, coefficient)
    end

    # Get kinetic friction coefficient of the body.
    # @return [Numeric] A value between 0.01 and 2.00.
    def kinetic_friction
      MSPhysics::Newton::Body.get_kinetic_friction(@address)
    end

    # Set kinetic friction coefficient of the body.
    # @param [Numeric] coefficient A value between 0.01 and 2.00.
    def kinetic_friction=(coefficient)
      MSPhysics::Newton::Body.set_kinetic_friction(@address, coefficient)
    end

    # Get friction state of the body.
    # @return [Boolean] +true+ if body friction is enabled, +false+ if body
    #   friction is disabled.
    def friction_enabled?
      MSPhysics::Newton::Body.get_friction_state(@address)
    end

    # Set friction state of the body.
    # @param [Boolean] state +true+ to enable body friction, +false+ to disable
    #   body friction.
    def friction_enabled=(state)
      MSPhysics::Newton::Body.set_friction_state(@address, state)
    end

    # Get the mode for controlling the way this magnet should work.
    # @return [Fixnum] Returns one of the following values:
    # - 1; to have the magnet force be calculated with the following
    #   equation: actual_magnet_force = f * (d - r)^2 / r^2; where f is the
    #   maximum magnet force (in Newtons), d is the distance between this magnet
    #   and a magnetic body (in meters), and r is the maximum magnet range
    #   (in meters). In this mode, the magnet_force and magnet_range methods are
    #   used for controlling the magnet.
    # - 2; to have magnet force be computed with a slightly different
    #   equation: actual_magnet_force = s / d^2; where s is the magnet strength
    #   and d is the distance between this magnet and a magnetic body
    #   (in meters). In this mode, the magnet_strength method must be used for
    #   controlling the magnet.
    def magnet_mode
      MSPhysics::Newton::Body.get_magnet_mode(@address)
    end

    # Set the mode for controlling the way this magnet should work.
    # @param [Fixnum] mode
    # - Pass 1 to have the magnet force be calculated with the following
    #   equation: actual_magnet_force = f * (d - r)^2 / r^2; where f is the
    #   maximum magnet force (in Newtons), d is the distance between this magnet
    #   and a magnetic body (in meters), and r is the maximum magnet range
    #   (in meters). In this mode, the magnet_force and magnet_range methods are
    #   used for controlling the magnet.
    # - Pass 2 to have magnet force be computed with a slightly different
    #   equation: actual_magnet_force = s / d^2; where s is the magnet strength
    #   and d is the distance between this magnet and a magnetic body
    #   (in meters). In this mode, the magnet_strength method must be used for
    #   controlling the magnet.
    def magnet_mode=(mode)
      MSPhysics::Newton::Body.set_magnet_mode(@address, mode)
    end

    # Get the maximum magnet force in Newton to be applied on the surrounding
    # magnetic bodies.
    # @note This option has an effect if and only if magnet_mode is set to 1.
    # @return [Numeric]
    # @see magnet_mode
    def magnet_force
      MSPhysics::Newton::Body.get_magnet_force(@address)
    end

    # Set the maximum magnet force in Newton to be applied on the surrounding
    # magnetic bodies.
    # @note This option has an effect if and only if magnet_mode is set to 1.
    # @param [Numeric] force
    # @see magnet_mode=
    def magnet_force=(force)
      MSPhysics::Newton::Body.set_magnet_force(@address, force)
    end

    # Get the maximum magnet range in meters. Magnet force is distributed along
    # the magnet range. Magnetic bodies outside the magnet range are not
    # affected.
    # @note This option has an effect if and only if magnet_mode is set to 1.
    # @return [Numeric]
    # @see magnet_mode
    def magnet_range
      MSPhysics::Newton::Body.get_magnet_range(@address)
    end

    # Set the maximum magnet range in meters. Magnet force is distributed along
    # the magnet range. Magnetic bodies outside the magnet range are not
    # affected.
    # @note This option has an effect if and only if magnet_mode is set to 1.
    # @param [Numeric] range
    # @see magnet_mode=
    def magnet_range=(range)
      MSPhysics::Newton::Body.set_magnet_range(@address, range)
    end

    # Get the magnet force magnitude to be applied on the surrounding magnetic
    # bodies.
    # @note This option has an effect if and only if magnet_mode is set to 2.
    # @return [Numeric]
    # @see magnet_mode
    def magnet_strength
      MSPhysics::Newton::Body.get_magnet_strength(@address)
    end

    # Set the magnet force magnitude to be applied on the surrounding
    # magnetic bodies.
    # @note This option has an effect if and only if magnet_mode is set to 2.
    # @param [Numeric] magnitude
    # @see magnet_mode=
    def magnet_strength=(magnitude)
      MSPhysics::Newton::Body.set_magnet_strength(@address, magnitude)
    end

    # Determine whether body is magnetic.
    # @return [Boolean]
    def magnetic?
      MSPhysics::Newton::Body.is_magnetic?(@address)
    end

    # Set body magnetic. Magnetic bodies will be affected by other bodies with
    # magnetism.
    # @param [Boolean] state +true+ to set body magnetic, +false+ to set body
    #   non-magnetic.
    def magnetic=(state)
      MSPhysics::Newton::Body.set_magnetic(@address, state)
    end

    # Get world axes aligned bounding box (AABB) of the body.
    # @return [Geom::BoundingBox]
    def aabb
      bb = Geom::BoundingBox.new
      bb.add MSPhysics::Newton::Body.get_aabb(@address)
      bb
    end

    # Get the viscous damping coefficient applied to the velocity of the body.
    # @return [Geom::Vetor3d] A vector representing the local damping
    #   coefficients of the axes, each between 0.0 and 1.0.
    def get_linear_damping
      MSPhysics::Newton::Body.get_linear_damping(@address)
    end

    # Set the viscous damping coefficient applied to the velocity of the body.
    # @overload set_linear_damping(damp)
    #   @param [Geom::Vector3d, Array<Numeric>] damp Each value of the damp
    #     vector is assumed as a local damping coefficient,
    #     a value b/w 0.0 and 1.0.
    # @overload set_linear_damping(dx, dy, dz)
    #   @param [Numeric] dx Local X-axis damping coefficient,
    #     a value b/w 0.0 and 1.0.
    #   @param [Numeric] dy Local Y-axis damping coefficient,
    #     a value b/w 0.0 and 1.0.
    #   @param [Numeric] dz Local Z-axis damping coefficient,
    #     a value b/w 0.0 and 1.0.
    def set_linear_damping(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_linear_damping(@address, data)
    end

    # Get the viscous damping coefficient applied to the omega of the body.
    # @return [Geom::Vetor3d] A vector representing the local damping
    #   coefficients of the axes, each between 0.0 and 1.0.
    def get_angular_damping
      MSPhysics::Newton::Body.get_angular_damping(@address)
    end

    # Set the viscous damping coefficient applied to the omega of the body.
    # @overload set_angular_damping(damp)
    #   @param [Geom::Vector3d, Array<Numeric>] damp Each value of the damp
    #     vector is assumed as an angular coefficient, a value b/w 0.0 and 1.0.
    # @overload set_angular_damping(dx, dy, dz)
    #   @param [Numeric] dx Local X-axis damping coefficient,
    #     a value b/w 0.0 and 1.0.
    #   @param [Numeric] dy Local Y-axis damping coefficient,
    #     a value b/w 0.0 and 1.0.
    #   @param [Numeric] dz Local Z-axis damping coefficient,
    #     a value b/w 0.0 and 1.0.
    # @return [nil]
    def set_angular_damping(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_angular_damping(@address, data)
    end

    # Get velocity at a specific point on the body.
    # @param [Geom::Point3d, Array<Numeric>] point A point in global space.
    # @return [Geom::Vector3d] Velocity at the given point. The magnitude of
    #   velocity is retrieved in meters per second (m/s).
    def point_velocity(point)
      MSPhysics::Newton::Body.get_point_velocity(@address, point)
    end

    # Add force to a specific point on the body.
    # @param [Geom::Point3d, Array<Numeric>] point Point on the body in global
    #   space.
    # @param [Geom::Vector3d, Array<Numeric>] force The magnitude of force is
    #   assumed in newtons (kg*m/s/s).
    # @return [Boolean] success
    def add_point_force(point, force)
      MSPhysics::Newton::Body.add_point_force(@address, point, force)
    end

    # Add an impulse to a specific point on the body.
    # @param [Geom::Point3d, Array<Numeric>] center The center of an impulse in
    #   global space.
    # @param [Geom::Vector3d, Array<Numeric>] delta_vel The desired change in
    #   velocity. The magnitude of velocity is assumed in meters per second
    #   (m/s).
    # @param [Numeric] timestep
    # @return [Boolean] success
    def add_impulse(center, delta_vel, timestep)
      MSPhysics::Newton::Body.add_impulse(@address, center, delta_vel, timestep)
    end

    # @!group Force Control Functions

    # Get the net force, in Newtons, applied on the body after the last world
    # update.
    # @return [Geom::Vector3d]
    def get_force
      MSPhysics::Newton::Body.get_force(@address)
    end

    # Apply force on the body in Newtons (kg * m/s/s).
    # @note Unlike the {#set_force}, this function doesn't overwrite original
    #   force, but rather adds force to the force accumulator.
    # @overload add_force(force)
    #   @param [Geom::Vector3d, Array<Numeric>] force
    # @overload add_force(fx, fy, fz)
    #   @param [Numeric] fx
    #   @param [Numeric] fy
    #   @param [Numeric] fz
    # @return [Boolean] success
    def add_force(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.add_force(@address, data)
    end

    # Apply force on the body in Newton (kg * m/s/s).
    # @note Unlike the {#add_force}, this function overwrites original force,
    #   thus discarding the previously applied force.
    # @overload set_force(force)
    #   @param [Geom::Vector3d, Array<Numeric>] force
    # @overload set_force(fx, fy, fz)
    #   @param [Numeric] fx
    #   @param [Numeric] fy
    #   @param [Numeric] fz
    # @return [Boolean] success
    def set_force(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_force(@address, data)
    end

    # Get the net torque, in Newton-meters, applied on the body after the last
    # world update.
    # @return [Geom::Vector3d]
    def get_torque
      MSPhysics::Newton::Body.get_torque(@address)
    end

    # Apply torque on the body in Newton-meters (kg * m/s/s * m).
    # @note Unlike the {#set_torque}, this function doesn't overwrite original
    #   torque, but rather adds torque to the torque accumulator.
    # @overload add_torque(torque)
    #   @param [Geom::Vector3d, Array<Numeric>] torque
    # @overload add_torque(tx, ty, tz)
    #   @param [Numeric] tx
    #   @param [Numeric] ty
    #   @param [Numeric] tz
    # @return [Boolean] success
    def add_torque(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.add_torque(@address, data)
    end

    # Apply torque on the body in Newton-meters (kg * m/s/s * m).
    # @note Unlike the {#add_torque}, this function overwrites original torque,
    #   thus discarding the previously applied torque.
    # @overload set_torque(torque)
    #   @param [Geom::Vector3d, Array<Numeric>] torque
    # @overload set_torque(tx, ty, tz)
    #   @param [Numeric] tx
    #   @param [Numeric] ty
    #   @param [Numeric] tz
    # @return [Boolean] success
    def set_torque(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_torque(@address, data)
    end

    # @!endgroup
    # @!group Contact Related Functions

    # Get total force generated from contacts on the body.
    # @return [Geom::Vector3d] Magnitude of the net force is retrieved in
    #   newtons (kg*m/s/s).
    def net_contact_force
      MSPhysics::Newton::Body.get_net_contact_force(@address)
    end

    # Get all contacts on the body.
    # @param [Boolean] inc_non_collidable Whether to include contacts from
    #   non-collidable bodies.
    # @return [Array<Contact>]
    def contacts(inc_non_collidable)
      MSPhysics::Newton::Body.get_contacts(@address, inc_non_collidable) { |ptr, data, point, normal, force, speed|
        data.is_a?(MSPhysics::Body) ? MSPhysics::Contact.new(data, point, normal, force, speed) : nil
      }
    end

    # Get all bodies that are in contact with this body.
    # @param [Boolean] inc_non_collidable Whether to include contacts from
    #   non-collidable bodies.
    # @return [Array<Body>]
    def touching_bodies(inc_non_collidable)
      MSPhysics::Newton::Body.get_touching_bodies(@address, inc_non_collidable) { |ptr, data|
        data.is_a?(MSPhysics::Body) ? data : nil
      }
    end

    # Determine if this body is in contact with another body.
    # @param [Body] body A body to test.
    # @return [Boolean]
    def touching_with?(body)
      MSPhysics::Newton::Bodies.touching?(@address, body.address)
    end

    # Get all contact points on the body.
    # @param [Boolean] inc_non_collidable Whether to included contacts from
    #   non-collidable bodies.
    # @return [Array<Geom::Point3d>]
    def contact_points(inc_non_collidable)
      MSPhysics::Newton::Body.get_contact_points(@address, inc_non_collidable)
    end

    # @!endgroup

    # Get collision faces of the body.
    # @return [Array<Array<Geom::Point3d>>] An array of faces. Each face
    #   represents an array of points. Points are coordinated in global space.
    def collision_faces
      MSPhysics::Newton::Body.get_collision_faces(@address)
    end

    # Get collision faces of the body.
    # @return [Array<Array<(Array<Geom::Point3d>, Geom::Point3d, Geom::Vector3d, Numeric)>>]
    #  An array of face data. Each face data contains four elements. The first
    #  element contains the array of face vertices (sorted counterclockwise) in
    #  global space. The second element represents face centroid in global
    #  space. The third element represents face normal in global space. And the
    #  last element represents face area in inches squared.
    def collision_faces2
      MSPhysics::Newton::Body.get_collision_faces2(@address)
    end

    # Get collision faces of the body.
    # @return [Array<Array<(Geom::Point3d, Geom::Vector3d, Numeric)>>] An array
    #  of face data. Each face data contains three elements. The first element
    #  represents face centroid in global space. The second element represents
    #  face normal in global space. And the last element represents face area in
    #  inches squared.
    def collision_faces3
      MSPhysics::Newton::Body.get_collision_faces3(@address)
    end

    # Apply pick and drag to the body.
    # @param [Geom::Point3d, Array<Numeric>] pick_pt Pick point, usually on the
    #   surface of the body, in global space.
    # @param [Geom::Point3d, Array<Numeric>] dest_pt Destination point in global
    #   space.
    # @param [Numeric] stiffness Stiffness, a value b/w 0.0 and 1.0.
    # @param [Numeric] damp Damper, a value b/w 0.0 and 1.0.
    # @param [Numeric] timestep
    # @return [Boolean] success
    def apply_pick_and_drag(pick_pt, dest_pt, stiffness, damp, timestep)
      MSPhysics::Newton::Body.apply_pick_and_drag(@address, pick_pt, dest_pt, stiffness, damp, timestep)
    end

    # Apply buoyancy to the body.
    # @param [Geom::Point3d, Array<Numeric>] plane_origin Plane origin.
    # @param [Geom::Vector3d, Array<Numeric>] plane_normal Plane normal.
    # @param [Numeric] density Fluid density in kilograms per cubic meter
    #   (kg/m^3).
    # @param [Numeric] linear_viscosity Linear viscosity,
    #   a value between 0.0 and 1.0.
    # @param [Numeric] angular_viscosity Angular viscosity,
    #   a value between 0.0 and 1.0.
    # @param [Geom::Vector3d, Array<Numeric>] linear_current Velocity of the
    #   fluid global space.
    # @param [Geom::Vector3d, Array<Numeric>] angular_current Omega of the
    #   fluid global space.
    # @param [Numeric] timestep
    # @return [Boolean] success
    # @example
    #   onUpdate {
    #     plane_origin = Geom::Point3d.new(0,0,100)
    #     plane_normal = Z_AXIS
    #     density = 997.04 # density of water
    #     linear_viscosity = 0.01
    #     angular_viscosity = 0.01
    #     linear_current = Geom::Vector3d.new(5,0,0) # 5 m/s along the X-axis
    #     angular_current = Geom::Vector3d.new(0,0,0)
    #     timestep = simulation.update_timestep
    #     this.apply_buoyancy(plane_origin, plane_normal, density, linear_viscosity, angular_viscosity, linear_current, angular_current, timestep)
    #   }
    def apply_buoyancy(plane_origin, plane_normal, density, linear_viscosity, angular_viscosity, linear_current, angular_current, timestep)
      MSPhysics::Newton::Body.apply_buoyancy(@address, plane_origin, plane_normal, density, linear_viscosity, angular_viscosity, linear_current, angular_current, timestep)
    end

    # Apply fluid resistance to the body. The resistance force and torque is
    # based upon the body's linear and angular velocity, orientation of its
    # collision faces, and the drag coefficient.
    # @note WIP
    # @param [Numeric] drag Drag coefficient.
    # @param [Geom::Vector3d] wind Velocity of the wind in meters per second.
    # @return [Boolean] success
    # @example
    #   onUpdate {
    #     drag = 0.25 # drag coefficient of air
    #     wind = Geom::Vector3d.new(5,0,0) # 5 m/s along the X-axis
    #     this.apply_aerodynamics(drag, wind)
    #   }
    def apply_aerodynamics(drag, wind)
      MSPhysics::Newton::Body.apply_aerodynamics(@address, drag, wind)
    end

    # Create a copy of the body.
    # @overload copy(reapply_forces, type)
    #   @param [Boolean] reapply_forces Whether to reapply force and torque.
    #   @param [Fixnum] type Type: 0 -> dynamic; 1 -> kinematic.
    # @overload copy(transformation, reapply_forces, type)
    #   @param [Geom::Transformation, Array<Numeric>] transformation New
    #     transformation matrix.
    #   @param [Boolean] reapply_forces Whether to reapply force and torque.
    #   @param [Fixnum] type Type: 0 -> dynamic; 1 -> kinematic.
    #   @raise [TypeError] if the specified transformation matrix is not
    #     uniform.
    # @return [Body] A new body object.
    def copy(*args)
      if args.size == 2
        Body.new(self, nil, args[0], args[1])
      elsif args.size == 3
        Body.new(self, args[0], args[1], args[2])
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 2..3 arguments but got #{args.size}.", caller)
      end
    end

    # Enable/disable gravitational force on this body.
    # @param [Boolean] state
    def gravity_enabled=(state)
      MSPhysics::Newton::Body.enable_gravity(@address, state)
    end

    # Determine if gravitational force is enabled on this body.
    # @return [Boolean]
    def gravity_enabled?
      MSPhysics::Newton::Body.is_gravity_enabled?(@address)
    end

    # Get joints whose parent bodies associate to this body.
    # @return [Array<Joint, DoubleJoint>]
    def contained_joints
      MSPhysics::Newton::Body.get_contained_joints(@address) { |ptr, data|
        data.is_a?(MSPhysics::Joint) || data.is_a?(MSPhysics::DoubleJoint) ? data : nil
      }
    end

    # Get joints whose child bodies associate to this body.
    # @return [Array<Joint, DoubleJoint>]
    def connected_joints
      MSPhysics::Newton::Body.get_connected_joints(@address) { |ptr, data|
        data.is_a?(MSPhysics::Joint) || data.is_a?(MSPhysics::DoubleJoint) ? data : nil
      }
    end

    # Get all bodies connected to this body through joints.
    # @return [Array<Body>]
    def connected_bodies
      MSPhysics::Newton::Body.get_connected_bodies(@address) { |ptr, data|
        data.is_a?(MSPhysics::Body) ? data : nil
      }
    end

    # Get body collision scale.
    # @note Does not include group scale.
    # @return [Geom::Vector3d] A vector representing the local X-axis, Y-axis,
    #   and Z-axis scale factors of the collision.
    def get_collision_scale
      MSPhysics::Newton::Body.get_collision_scale(@address)
    end

    # Set body collision scale.
    # @note Does not include group scale.
    # @overload set_collision_scale(scale)
    #   @param [Geom::Vector3d, Array<Numeric>] scale A vector representing the
    #     local X-axis, Y-axis, and Z-axis scale factors of the body collision.
    # @overload set_collision_scale(sx, sy, sz)
    #   @param [Numeric] sx Scale along the local X-axis of the body,
    #     a value between 0.01 and 100.
    #   @param [Numeric] sy Scale along the local Y-axis of the body,
    #     a value between 0.01 and 100.
    #   @param [Numeric] sz Scale along the local Z-axis of the body,
    #     a value between 0.01 and 100.
    # @return [nil]
    def set_collision_scale(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::Body.set_collision_scale(@address, data)
    end

    # Get default scale of the body collision.
    # @note Does not include group scale.
    # @return [Geom::Vector3d] A vector representing the local, default X-axis,
    #   Y-axis, and Z-axis scale factors of the collision.
    def default_collision_scale
      MSPhysics::Newton::Body.get_default_collision_scale(@address)
    end

    # Get scale of the body matrix that is a product of group scale and
    # collision scale.
    # @return [Geom::Vector3d] A vector representing the local X-axis, Y-axis,
    #   and Z-axis scale factors of the actual body matrix.
    def actual_matrix_scale
      MSPhysics::Newton::Body.get_actual_matrix_scale(@address)
    end

    # @!group Joint Associated Functions

    # Make the body's Z-axis to look in a particular direction.
    # @param [Geom::Vector3d, nil] pin_dir Direction in global space. Pass nil
    #   to disable the look at constraint.
    # @param [Numeric] accel Rotational oscillation stiffness.
    # @param [Numeric] damp Rotational oscillation damping coefficient.
    # @param [Numeric] strength Rotational oscillation strength.
    # @return [MSPhysics::UpVector, nil]
    # @example
    #   onTick {
    #     if (key(' ') == 1)
    #       dir = this.group.transformation.origin.vector_to(ORIGIN)
    #       this.look_at(dir, 40, 10, 0.9)
    #     else
    #       this.look_at(nil)
    #     end
    #   }
    def look_at(pin_dir, accel = 40, damp = 10, strength = 0.9)
      if pin_dir.nil?
        if @look_at_joint && @look_at_joint.valid?
          @look_at_joint.destroy
          @look_at_joint = nil
        end
        return
      end
      pin_dir = Geom::Vector3d.new(pin_dir) unless pin_dir.is_a?(Geom::Vector3d)
      if @look_at_joint.nil? || !@look_at_joint.valid?
        @look_at_joint = MSPhysics::UpVector.new(self.world, nil, Geom::Transformation.new(ORIGIN, @group.transformation.zaxis))
        @look_at_joint.connect(self)
      end
      @look_at_joint.set_pin_dir(pin_dir.transform(@look_at_joint.get_pin_matrix.inverse))
      @look_at_joint.accel = accel
      @look_at_joint.damp = damp
      @look_at_joint.strength = strength
      return @look_at_joint
    end

    # Attach a particular body to this body with a Fixed joint.
    # @param [MSPhysics::Body] body The body to attach.
    # @return [MSPhysics::Fixed] The joint representing an attachment.
    # @see detach
    # @see detach_all
    # @see attached?
    def attach(body)
      Body.validate2(self, body, self.world)
      joint = @attached_bodies[body]
      return joint if joint && joint.valid?
      joint = MSPhysics::Fixed.new(self.world, self, body.get_position(1))
      joint.connect(body)
      @attached_bodies[body] = joint
      joint
    end

    # Detach a particular attached body from this body.
    # @param [MSPhysics::Body] body The body to detach.
    # @return [Boolean] success
    # @see attach
    # @see detach_all
    # @see attached?
    def detach(body)
      Body.validate2(self, body, self.world)
      joint = @attached_bodies[body]
      if joint && joint.valid?
        joint.destroy
        @attached_bodies.delete(body)
        true
      else
        false
      end
    end

    # Detach all attached bodies from this body.
    # @return [Fixnum] Number of bodies detached.
    # @see attach
    # @see detach
    # @see attached?
    def detach_all
      count = 0
      @attached_bodies.each { |body, joint|
        if joint.valid?
          joint.destroy
          count += 1
        end
      }
      @attached_bodies.clear
      count
    end

    # Determine whether a particular body is attached to this body.
    # @param [MSPhysics::Body] body The body to check.
    # @return [Boolean]
    # @see attach
    # @see detach
    # @see detach_all
    def attached?(body)
      joint = @attached_bodies[body]
      (joint && joint.valid?) ? true : false
    end

    # @!endgroup

  end # class Body
end # module MSPhysics
