module MSPhysics

  # @since 1.0.0
  class World < Entity
    class << self

      # Verify that world is valid.
      # @api private
      # @param [World] world
      # @return [void]
      # @raise [TypeError] if the world is invalid or destroyed.
      def validate(world)
        AMS.validate_type(world, MSPhysics::World)
        unless world.valid?
          raise(TypeError, "World #{world} is invalid/destroyed!", caller)
        end
      end

      # Get world by world address.
      # @param [Fixnum] address
      # @return [World, nil] A World object if successful.
      def world_by_address(address)
        data = MSPhysics::Newton::World.get_user_data(address.to_i)
        data.is_a?(MSPhysics::World) ? data : nil
      end

      # Get all worlds.
      # @note Worlds that do not have a {World} instance are not included in the
      #   array.
      # @return [Array<World>]
      def all_worlds
        MSPhysics::Newton.get_all_worlds() { |ptr, data| data.is_a?(MSPhysics::World) ? data : nil }
      end

    end # class << self

    def initialize
      @address = MSPhysics::Newton::World.create()
      MSPhysics::Newton::World.set_user_data(@address, self)
    end

    # Determine if the world is valid (not destroyed).
    # @return [Boolean]
    def valid?
      MSPhysics::Newton::World.is_valid?(@address)
    end

    # Get pointer to the world.
    # @return [Fixnum]
    def address
      @address
    end

    # Destroy the world.
    # @return [nil]
    def destroy
      MSPhysics::Newton::World.destroy(@address)
    end

    # Get world default contact material id.
    # @return [Fixnum]
    def default_material_id
      MSPhysics::Newton::World.get_default_material_id(@address)
    end

    # Get the maximum possible number of threads to be used by the world.
    # @return [Fixnum]
    def max_possible_threads_count
      MSPhysics::Newton::World.get_max_possible_threads_count(@address)
    end

    # Get the desired maximum number of threads to be used by the world.
    # @return [Fixnum]
    def max_threads_count
      MSPhysics::Newton::World.get_max_threads_count(@address)
    end

    # Set the desired maximum number of threads to be used by the world.
    # @param [Fixnum] count This value is clamped between one and the maximum
    #   number of CPUs can be used in the system.
    def max_threads_count=(count)
      MSPhysics::Newton::World.set_max_threads_count(@address, count.to_i)
    end

    # Get the number of threads currently used by the world.
    # @return [Fixnum]
    def cur_threads_count
      MSPhysics::Newton::World.get_cur_threads_count(@address)
    end

    # Update world by a time step in seconds.
    # @note The smaller the time step the more accurate the simulation will be.
    # @param [Numeric] timestep This value is clamped between 1/30.0 and 1/1200.0.
    # @return [Numeric] The update time step.
    def update(timestep)
      MSPhysics::Newton::World.update(@address, timestep)
    end

    # Get all bodies in the world.
    # @note Bodies that do not have a {Body} instance are not included in the
    #   array.
    # @return [Array<Body>]
    def bodies
      MSPhysics::Newton::World.get_bodies(@address) { |ptr, data| data.is_a?(MSPhysics::Body) ? data : nil }
    end

    # Get all joints in the world.
    # @note Joints that do not have a {Joint} instance are not included in the
    #   array.
    # @return [Array<Joint>]
    def joints
      MSPhysics::Newton::World.get_joints(@address) { |ptr, data| data.is_a?(MSPhysics::Joint) ? data : nil }
    end

    # Get all gears in the world.
    # @note Gears that do not have a {Gear} instance are not included in the
    #   array.
    # @return [Array<Gear>]
    def gears
      MSPhysics::Newton::World.get_gears(@address) { |ptr, data| data.is_a?(MSPhysics::Gear) ? data : nil }
    end

    # Get all bodies in a particular bounds.
    # @param [Geom::Point3d, Array<Numeric>] min Minimum point in the bounding box.
    # @param [Geom::Point3d, Array<Numeric>] max Maximum point in the bounding box.
    # @return [Array<Body>]
    def bodies_in_aabb(min, max)
      MSPhysics::Newton::World.get_bodies_in_aabb(@address, min, max) { |ptr, data| data.is_a?(MSPhysics::Body) ? data : nil }
    end

    # Get the number of bodies in the world.
    # @return [Fixnum]
    def body_count
      MSPhysics::Newton::World.get_body_count(@address)
    end

    # Get the number of constraints in the world.
    # @return [Fixnum]
    def constraint_count
      MSPhysics::Newton::World.get_constraint_count(@address)
    end

    # Destroy all bodies in the world.
    # @return [Fixnum] The number of bodies destroyed.
    def destroy_all_bodies
      MSPhysics::Newton::World.destroy_all_bodies(@address)
    end

    # Get world gravity.
    # @return [Geom::Vector3d] Gravitational acceleration vector. The magnitude
    #   of the vector is expressed in meters per second per second (m/s/s).
    def get_gravity
      MSPhysics::Newton::World.get_gravity(@address)
    end

    # Set world gravity.
    # @overload set_gravity(acceleration)
    #   @param [Geom::Vector3d, Array<Numeric>] acceleration Gravitational
    #     acceleration vector. The magnitude of the vector is assumed in meters
    #     per second per second (m/s/s).
    # @overload set_gravity(ax, ay, az)
    #   @param [Numeric] ax Acceleration along X-axis in m/s/s.
    #   @param [Numeric] ay Acceleration along Y-axis in m/s/s.
    #   @param [Numeric] az Acceleration along Z-axis in m/s/s.
    # @return [nil]
    def set_gravity(*args)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments but got #{args.size}.", caller)
      end
      MSPhysics::Newton::World.set_gravity(@address, data)
    end

    # Get world solver model.
    # @return [Fixnum] Number of passes, a value between 1 and 256.
    def solver_model
      MSPhysics::Newton::World.get_solver_model(@address)
    end

    # Set world solver model.
    # @param [Fixnum] model Number of passes, a value between 1 and 256.
    def solver_model=(model)
      MSPhysics::Newton::World.set_solver_model(@address, model.to_i)
    end

    # Get world material thickness in meters, an imaginary thickness between the
    # collision geometry of two colliding bodies.
    # @return [Numeric] A value between 0.0 and 1/32.0.
    def material_thickness
      MSPhysics::Newton::World.get_material_thickness(@address)
    end

    # Set world material thickness in meters, an imaginary thickness between the
    # collision geometry of two colliding bodies.
    # @param [Numeric] thickness This value is clamped between 0.0 and 1/32.0.
    def material_thickness=(thickness)
      MSPhysics::Newton::World.set_material_thickness(@address, thickness.to_f)
    end

    # Shoot a ray from point1 to point2 and get the closest intersection.
    # @param [Geom::Point3d, Array<Numeric>] point1 Ray starting point.
    # @param [Geom::Point3d, Array<Numeric>] point2 Ray destination point.
    # @return [Hit, nil] A Hit object or nil if no intersections occurred.
    def ray_cast(point1, point2)
      res = MSPhysics::Newton::World.ray_cast(@address, point1, point2)
      res && res[1].is_a?(MSPhysics::Body) ? Hit.new(res[1], res[2], res[3]) : nil
    end

    # Shoot a ray from point1 to point2 and get all intersections.
    # @param [Geom::Point3d, Array<Numeric>] point1 Ray starting point.
    # @param [Geom::Point3d, Array<Numeric>] point2 Ray destination point.
    # @return [Array<Hit>]
    def continuous_ray_cast(point1, point2)
      MSPhysics::Newton::World.continuous_ray_cast(@address, point1, point2) { |ptr, data, point, vector|
        data.is_a?(MSPhysics::Body) ? Hit.new(data, point, vector) : nil
      }
    end

    # Shoot a convex body from point1 to point2 and get the closest
    # intersection.
    # @note If given body does not have a convex collision, the function will
    #   return nil.
    # @param [Body] body A convex body.
    # @param [Geom::Transformation, Array<Numeric>, nil] transformation Body
    #   orientation and start position. Pass nil to ray cast from body's current
    #   transformation.
    # @param [Geom::Point3d, Array<Numeric>] target Ray destination point.
    # @return [Hit, nil] A Hit object or nil if no intersections occurred.
    def convex_ray_cast(body, transformation, target)
      MSPhysics::Body.validate(body, self)
      transformation = body.get_matrix unless transformation
      res = MSPhysics::Newton::World.convex_ray_cast(@address, body.collision_address, transformation, target)
      res && res[1].is_a?(MSPhysics::Body) ? Hit.new(res[1], res[2], res[3]) : nil
    end

    # Shoot a convex body from point1 to point2 and get all intersections.
    # @note If given body does not have a convex collision, the function will
    #   return an empty array.
    # @param [Geom::Transformation, Array<Numeric>, nil] transformation Body
    #   orientation and start position. Pass nil to ray cast from body's current
    #   transformation.
    # @param [Geom::Point3d, Array<Numeric>] target Ray destination point.
    # @param [Fixnum] max_hits Maximum number of hits, a value b/w 1 and 256.
    # @return [Array<Hit>]
    def continuous_convex_ray_cast(body, transformation, target, max_hits = 16)
      MSPhysics::Body.validate(body, self)
      transformation = body.get_matrix unless transformation
      MSPhysics::Newton::World.continuous_convex_ray_cast(@address, body.collision_address, transformation, target, max_hits) { |ptr, data, point, vector, penetration|
        data.is_a?(MSPhysics::Body) ? Hit.new(data, point, vector) : nil
      }
    end

    # Add an explosion impulse at particular point in the world.
    # @param [Geom::Point3d, Array<Numeric>] center_point A point of impulse.
    # @param [Numeric] blast_radius A blast radius in meters. Objects beyond the
    #   blast radius are not affected.
    # @param [Numeric] blast_force Maximum blast force in Newtons. The force is
    #   distributed linearly along the blast radius.
    # @return [Boolean] success
    # @example
    #   # Adding an impulsive explosion
    #   onStart {
    #     center_point = this.get_position(1)
    #     blast_radius = 100
    #     blast_force = 1000 / simulation.update_timestep
    #     world.add_explosion(center_point, blast_radius, blast_force)
    #   }
    def add_explosion(center_point, blast_radius, blast_force)
      MSPhysics::Newton::World.add_explosion(@address, center_point, blast_radius.to_f, blast_force.to_f)
    end

    # Get world axes aligned bounding box, a bounding box in which all the
    # bodies are included.
    # @return [Geom::BoundingBox, nil] A bounding box object, containing the
    #   minimum and maximum points of the world bounding box. Nil is returned if
    #   the world has no bodies.
    def aabb
      mm = MSPhysics::Newton::World.get_aabb(@address)
      return unless mm
      bb = Geom::BoundingBox.new
      bb.add(mm)
      bb
    end

    # Get world elapsed time in seconds.
    # @return [Numeric]
    def time
      MSPhysics::Newton::World.get_time(@address)
    end

    # Serialize world into file.
    # @param [String] full_path
    # @return [nil]
    def serialize_to_file(full_path)
      MSPhysics::Newton::World.serialize_to_file(@address, full_path)
    end

    # Get world contact merge tolerance.
    # @return [Numeric]
    def contact_merge_tolerance
      MSPhysics::Newton::World.get_contact_merge_tolerance(@address)
    end

    # Set world contact merge tolerance.
    # @param [Numeric] tolerance Default value is 0.001. Minimum value is 0.001.
    def contact_merge_tolerance=(tolerance)
      MSPhysics::Newton::World.set_contact_merge_tolerance(@address, tolerance)
    end

  end # class World
end # module MSPhysics
