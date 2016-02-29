module MSPhysics

  # @since 1.0.0
  class World
    class << self

      # Verify that world is valid.
      # @api private
      # @param [World] world
      # @return [void]
      # @raise [TypeError] if the world is invalid or destroyed.
      def validate(world)
        AMS.validate_type(world, MSPhysics::World)
        unless world.is_valid?
          raise(TypeError, "World #{world} is invalid/destroyed!", caller)
        end
      end

      # Get world by world address.
      # @param [Fixnum] address
      # @return [World, nil] A World object if successful.
      def get_world_by_address(address)
        data = MSPhysics::Newton::World.get_user_data(address.to_i)
        data.is_a?(World) ? data : nil
      end

      # Get all worlds.
      # @note Worlds that don't have a {World} instance are not included in the
      #   array.
      # @return [Array<World>]
      def get_all_worlds
        worlds = []
        Newton.get_all_worlds.each { |address|
          data = MSPhysics::Newton::World.get_user_data(address)
          worlds << data if data.is_a?(World)
        }
        worlds
      end

    end # class << self

    # @param [Numeric] scale A value between 0.1 and 100.0. If you're simulating
    #   tiny objects, it's good to set world scale to a big value, like 40.
    def initialize(scale = 1)
      @address = MSPhysics::Newton::World.create(scale)
      MSPhysics::Newton::World.set_user_data(@address, self)
    end

    # Determine if the world is valid (not destroyed).
    # @return [Boolean]
    def is_valid?
      MSPhysics::Newton::World.is_valid?(@address)
    end

    # Get world pointer.
    # @return [Fixnum]
    def get_address
      World.validate(self)
      @address
    end

    # Get world default contact material id.
    # @return [Fixnum]
    def get_default_material_id
      World.validate(self)
      MSPhysics::Newton::World.get_default_material_id(@address)
    end

    # Destroy the world.
    # @return [void]
    def destroy
      World.validate(self)
      MSPhysics::Newton::World.destroy(@address)
    end

    # Get the maximum number of threads can be used by the world.
    # @return [Fixnum]
    def get_max_threads_count
      World.validate(self)
      MSPhysics::Newton::World.get_max_threads_count(@address)
    end

    # Set the maximum number of threads to be used by the world.
    # @param [Fixnum] count This value is clamped between one and the maximum
    #   number of CPUs can be used in the system.
    # @return [Fixnum] The new number of threads to be used.
    def set_max_threads_count(count)
      World.validate(self)
      MSPhysics::Newton::World.set_max_threads_count(@address, count.to_i)
    end

    # Get the number of threads currently used by the world.
    # @return [Fixnum]
    def get_cur_threads_count
      World.validate(self)
      MSPhysics::Newton::World.get_cur_threads_count(@address)
    end

    # Update world by a time step in seconds.
    # @note The smaller the time step the more accurate the simulation will be.
    # @param [Numeric] timestep This value is clamped between 1/30.0 and 1/1200.0.
    # @param [Fixnum] update_rate Number of times to update the world, a fixed
    #   value between 1 and 100.
    # @return [Numeric] Update time step.
    def update(timestep, update_rate = 1)
      World.validate(self)
      MSPhysics::Newton::World.update(@address, timestep, update_rate)
    end

    # Get all bodies in the world.
    # @note Bodies that don't have a {Body} instance are not included in the
    #   array.
    # @return [Array<Body>]
    def get_bodies
      World.validate(self)
      bodies = []
      ovs = MSPhysics::Newton.is_object_validation_enabled?
      MSPhysics::Newton.enable_object_validation(false)
      body_address = MSPhysics::Newton::World.get_first_body(@address)
      while body_address
        data = MSPhysics::Newton::Body.get_user_data(body_address)
        bodies << data if data.is_a?(MSPhysics::Body)
        body_address = MSPhysics::Newton::World.get_next_body(@address, body_address)
      end
      MSPhysics::Newton.enable_object_validation(ovs)
      bodies
    end

    # Get all joints in the world.
    # @note Joints that don't have a {Joint} instance are not included in the
    #   array.
    # @return [Array<Joint>]
    def get_joints
      World.validate(self)
      joints = []
      joint_addresses = MSPhysics::Newton::World.get_joints(@address)
      joint_addresses.each { |joint_address|
        data = MSPhysics::Newton::Joint.get_user_data(joint_address)
        joints << data if data.is_a?(MSPhysics::Joint)
      }
      joints
    end

    # Get all bodies in a bounding box.
    # @param [Geom::Point3d, Array<Numeric>] min Minimum point in the bounding box.
    # @param [Geom::Point3d, Array<Numeric>] max Maximum point in the bounding box.
    # @return [Array<Body>]
    def get_bodies_in_aabb(min, max)
      World.validate(self)
      bodies = []
      ovs = MSPhysics::Newton.is_object_validation_enabled?
      MSPhysics::Newton.enable_object_validation(false)
      MSPhysics::Newton::World.get_bodies_in_aabb(@address, min, max).each { |address|
        data = MSPhysics::Newton::Body.get_user_data(address)
        bodies << body if data.is_a?(MSPhysics::Body)
      }
      MSPhysics::Newton.enable_object_validation(ovs)
      bodies
    end

    # Get the number of bodies in the world.
    # @return [Fixnum]
    def get_body_count
      World.validate(self)
      MSPhysics::Newton::World.get_body_count(@address)
    end

    # Get the number of constraints in the world.
    # @return [Fixnum]
    def get_constraint_count
      World.validate(self)
      MSPhysics::Newton::World.get_constraint_count(@address)
    end

    # Destroy all bodies in the world.
    # @return [Fixnum] The number of bodies destroyed.
    def destroy_all_bodies
      World.validate(self)
      MSPhysics::Newton::World.destroy_all_bodies(@address)
    end

    # Get world gravity.
    # @return [Geom::Vector3d] Gravitational acceleration vector. The magnitude
    #   of the vector is expressed in meters per second per second (m/s/s).
    def get_gravity
      World.validate(self)
      MSPhysics::Newton::World.get_gravity(@address)
    end

    # Set world gravity.
    # @param [Geom::Vector3d, Array<Numeric>] acceleration
    # @overload set_gravity(acceleration)
    #   @param [Geom::Vector3d, Array<Numeric>] acceleration Gravitational
    #     acceleration vector. The magnitude of the vector is assumed in meters
    #     per second per second (m/s/s).
    # @overload set_gravity(ax, ay, az)
    #   @param [Numeric] ax Acceleration along xaxis in m/s/s.
    #   @param [Numeric] ay Acceleration along yaxis in m/s/s.
    #   @param [Numeric] az Acceleration along zaxis in m/s/s.
    # @return [Geom::Vector3d] The newly assigned gravity.
    def set_gravity(*args)
      World.validate(self)
      if args.size == 3
        data = [args[0], args[1], args[2]]
      elsif args.size == 1
        data = args[0]
      else
        raise(ArgumentError, "Wrong number of arguments! Expected 1 or 3 arguments, but got #{args.size}.", caller)
      end
      MSPhysics::Newton::World.set_gravity(@address, data)
    end

    # Get world solver model.
    # * 0 - Exact solver. Exact solver model is used when precision in more
    #   important than speed.
    # * n - Iterative solver. Iterative solver model is used when speed is
    #   more important than precision.
    # @return [Fixnum]
    def get_solver_model
      World.validate(self)
      MSPhysics::Newton::World.get_solver_model(@address)
    end

    # Set world solver model.
    # * 0 - Exact solver. Exact solver model is used when precision in more
    #   important than speed.
    # * n - Iterative solver. Iterative solver model is used when speed is
    #   more important than precision.
    # @param [Fixnum] solver_model
    # @return [Fixnum] The newly assigned solver model.
    def set_solver_model(solver_model)
      World.validate(self)
      MSPhysics::Newton::World.set_solver_model(@address, solver_model.to_i)
    end

    # Get world friction model.
    # * 0 - Exact coulomb. Exact friction model is used when precision in more
    #   important than speed.
    # * 1 - Adaptive coulomb. Adaptive friction model is used when speed is more
    #   important than precision.
    # Generally, adaptive coulomb is 10% faster than exact coulumb.
    # @return [Fixnum]
    def get_friction_model
      World.validate(self)
      MSPhysics::Newton::World.get_friction_model(@address)
    end

    # Set world friction model.
    # * 0 - Exact coulomb. Exact friction model is used when precision in more
    #   important than speed.
    # * 1 - Adaptive coulomb. Adaptive friction model is used when speed is more
    #   important than precision.
    # Generally, adaptive coulomb is 10% faster than exact coulumb.
    # @param [Fixnum] friction_model
    # @return [Fixnum] The newly assigned friction model.
    def set_friction_model(friction_model)
      World.validate(self)
      MSPhysics::Newton::World.set_friction_model(@address, friction_model.to_i)
    end

    # Get world material thickness in meters, an imaginary thickness between the
    # collision geometry of two colliding bodies.
    # @return [Numeric] A value between 0.0 and 1/32.0.
    def get_material_thickness
      World.validate(self)
      MSPhysics::Newton::World.get_material_thickness(@address)
    end

    # Set world material thickness in meters, an imaginary thickness between the
    # collision geometry of two colliding bodies.
    # @param [Numeric] thickness This value is clamped between 0.0 and 1/32.0.
    # @return [Numeric] The newly assigned material thickness.
    def set_material_thickness(thickness)
      World.validate(self)
      MSPhysics::Newton::World.set_material_thickness(@address, thickness.to_f)
    end

    # Shoot a ray from point1 to point2 and get the closest intersection.
    # @param [Geom::Point3d, Array<Numeric>] point1 Ray starting point.
    # @param [Geom::Point3d, Array<Numeric>] point2 Ray destination point.
    # @return [Hit, nil] A Hit object or nil if no intersections occurred.
    def ray_cast(point1, point2)
      World.validate(self)
      res = MSPhysics::Newton::World.ray_cast(@address, point1, point2)
      return unless res
      body = MSPhysics::Newton::Body.get_user_data(res[0])
      return unless body.is_a?(MSPhysics::Body)
      Hit.new(body, res[1], res[2])
    end

    # Shoot a ray from point1 to point2 and get all intersections.
    # @param [Geom::Point3d, Array<Numeric>] point1 Ray starting point.
    # @param [Geom::Point3d, Array<Numeric>] point2 Ray destination point.
    # @return [Array<Hit>]
    def continuous_ray_cast(point1, point2)
      World.validate(self)
      hits = []
      ovs = MSPhysics::Newton.is_object_validation_enabled?
      MSPhysics::Newton.enable_object_validation(false)
      MSPhysics::Newton::World.continuous_ray_cast(@address, point1, point2).each { |address, point, vector|
        body = MSPhysics::Newton::Body.get_user_data(address)
        next unless body.is_a?(MSPhysics::Body)
        hits << Hit.new(body, point, vector)
      }
      MSPhysics::Newton.enable_object_validation(ovs)
      hits
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
      World.validate(self)
      MSPhysics::Body.validate(body)
      transformation = body.get_matrix unless transformation
      res = MSPhysics::Newton::World.convex_ray_cast(@address, body.get_collision_address, transformation, target)
      return unless res
      body = MSPhysics::Newton::Body.get_user_data(res[0])
      return unless body.is_a?(MSPhysics::Body)
      Hit.new(body, res[1], res[2])
    end

    # Shoot a convex body from point1 to point2 and get all intersections.
    # @note If given body does not have a convex collision, the function will
    #   return an empty array.
    # @param [Geom::Transformation, Array<Numeric>, nil] transformation Body
    #   orientation and start position. Pass nil to ray cast from body's current
    #   transformation.
    # @param [Geom::Point3d, Array<Numeric>] target Ray destination point.
    # @return [Array<Hit>]
    def continuous_convex_ray_cast(body, transformation, target)
      World.validate(self)
      MSPhysics::Body.validate(body)
      transformation = body.get_matrix unless transformation
      hits = []
      ovs = MSPhysics::Newton.is_object_validation_enabled?
      MSPhysics::Newton.enable_object_validation(false)
      MSPhysics::Newton::World.continuous_convex_ray_cast(@address, body.get_collision_address, transformation, target).each { |address, point, vector|
        body = MSPhysics::Newton::Body.get_user_data(address)
        next unless body.is_a?(MSPhysics::Body)
        hits << Hit.new(body, point, vector)
      }
      MSPhysics::Newton.enable_object_validation(ovs)
      hits
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
    #     blast_force = 1000 / simulation.get_update_timestep
    #     world.add_explosion(center_point, blast_radius, blast_force)
    #   }
    def add_explosion(center_point, blast_radius, blast_force)
      World.validate(self)
      MSPhysics::Newton::World.add_explosion(@address, center_point, blast_radius.to_f, blast_force.to_f)
    end

    # Get world axis aligned bounding box, a bounding box in which all the
    # bodies are included.
    # @return [Geom::BoundingBox, nil] A bounding box object, containing the
    #   minimum and maximum points of the world bounding box. Nil is returned if
    #   the world has no bodies.
    def get_aabb
      World.validate(self)
      mm = MSPhysics::Newton::World.get_aabb(@address)
      return unless mm
      bb = Geom::BoundingBox.new
      bb.add(mm)
      bb
    end

    # Get world elapsed time in seconds.
    # @return [Numeric]
    def get_time
      World.validate(self)
      MSPhysics::Newton::World.get_time(@address)
    end

    # Serialize world into file.
    # @param [String] full_path
    # @return [nil]
    def serialize_to_file(full_path)
      World.validate(self)
      MSPhysics::Newton::World.serialize_to_file(@address, full_path)
    end

    # Get world contact merge tolerance.
    # @return [Numeric]
    def get_contact_merge_tolerance
      World.validate(self)
      MSPhysics::Newton::World.get_contact_merge_tolerance(@address)
    end

    # Set world contact merge tolerance.
    # @param [Numeric] tolerance Default value is 0.001. Minimum value is 0.001.
    # @return [Numeric] The new contact merge tolerance.
    def set_contact_merge_tolerance(tolerance)
      World.validate(self)
      MSPhysics::Newton::World.set_contact_merge_tolerance(@address, tolerance)
    end

    # Get world scale.
    # @return [Numeric] A value between 0.1 and 100.
    def get_scale
      World.validate(self)
      MSPhysics::Newton::World.get_scale(@address)
    end

  end # class World
end # module MSPhysics
