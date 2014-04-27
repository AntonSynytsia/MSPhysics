module MSPhysics
  class Body

    TYPES = [
      :dynamic,
      :kinematic,
      :deformable
    ].freeze

    # @!visibility private
    COLLISION_ITERATOR = Proc.new { |user_data, vertex_count, face_array, face_id|
      vc = face_array.get_array_of_float(0, vertex_count*3)
      face = []
      for i in 0...vertex_count
        face.push Conversion.convert_point(vc[i*3,3], :m, :in)
      end
      @@_faces.push face
    }

    # @!visibility private
    @@_instances ||= {}
    # @!visibility private
    @@_faces ||= []

    class << self

      private

      def check_validity(body)
        e = 'The body is invalid!'
        raise e unless body.is_a?(Body)
        raise e unless body.valid?
      end

      public

      # Get body by group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @return [Body, NilClass]
      def get_body_by_entity(entity)
        @@_instances.values.each{ |body|
          return body if body._entity == entity
        }
        nil
      end

      # Get body by body pointer.
      # @param [AMS::FFI::Pointer, Fixnum] body_ptr
      # @return [Body, NilClass]
      def get_body_by_body_ptr(body_ptr)
        if body_ptr.is_a?(AMS::FFI::Pointer)
          key = body_ptr.address
        else
          key = body_ptr
        end
        @@_instances[key]
      end

      # @!visibility private
      def reset_data
        @@_instances.clear
      end

      # Drag the body to a specific position by applying force and torque.
      # @param [Body] body
      # @param [Array<Numeric>, Geom::Point3d] pick_pt Attachment point relative
      #   to the body's coordinate system.
      # @param [Array<Numeric>, Geom::Point3d] dest_pt Destination point relative
      #   to the global coordinate system.
      # @param [Numeric] stiffness Drag stiffness.
      # @param [Numeric] damp Drag damp.
      def apply_pick_force(body, pick_pt, dest_pt, stiffness = 100, damp = 10)
        check_validity(body)
        # Convert points
        matrix = body.get_matrix.to_a
        matrix[12..14] = Conversion.convert_point(matrix[12..14], :in, :m).to_a
        tra = Geom::Transformation.new(matrix)
        pick_pt = Conversion.convert_point(pick_pt, :in, :m)
        dest_pt = Conversion.convert_point(dest_pt, :in, :m)
        glob_pick_pt = pick_pt.transform(tra)
        # Get data
        com = Conversion.convert_point(body.get_centre_of_mass, :in, :m)
        vel = body.get_velocity
        omega = body.get_omega
        mass = body.get_mass
        # Calculate force
        force = glob_pick_pt.vector_to(dest_pt)
        return if force.length.zero?
        force.length *= mass*stiffness
        # Add some force damp
        damp_force = vel
        damp_force.length *= mass*damp if (damp_force.length != 0)
        force -= damp_force
        # Calculate torque
        point = tra*Geom::Vector3d.new(pick_pt-com)
        point.normalize! if point.length > 1
        torque = point*force
        # Add some torque damp
        #torque_damp = omega
        #torque_damp.length *= mass*0.1 if torque_damp.length != 0
        #torque -= torque_damp
        # Apply forces
        body.add_force(force)
        body.add_torque(torque)
        # Make sure body is unfrozen.
        body.frozen = false
      end

      # Get force between two bodies.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Geom::Vector3d] force in newtons (kg*m/s/s).
      def get_force_between_bodies(body1, body2)
        check_validity(body1)
        check_validity(body2)
        b1 = body1._body_ptr
        b2 = body2._body_ptr
        reaction_force = Geom::Vector3d.new(0,0,0)
        joint = Newton.bodyGetFirstContactJoint(b1)
        while !joint.null?
          if [b1, b2].include?(Newton.jointGetBody0(joint))
            contact = Newton.contactJointGetFirstContact(joint)
            while !contact.null?
              material = Newton.contactGetMaterial(contact)
              contact_force = 0.chr*12
              Newton.materialGetContactForce(material, b1, contact_force)
              force = Geom::Vector3d.new(*contact_force.unpack('FFF'))
              reaction_force += force
              contact = Newton.contactJointGetNextContact(joint, contact)
            end
          end
          joint = Newton.bodyGetNextContactJoint(b1, joint)
        end
        reaction_force
      end

      # Get the closest collision points between two convex bodies. This won't
      # work with the <i>compound</i> and <i>static mesh</i> bodies.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Array<Geom::Point3d>] +[contact_pt1, contact_pt2]+
      def get_closest_points(body1, body2)
        check_validity(body1)
        check_validity(body2)
        b1 = body1._body_ptr
        b2 = body2._body_ptr
        world = Newton.bodyGetWorld(b1)
        col1 = Newton.bodyGetCollision(b1)
        col2 = Newton.bodyGetCollision(b2)
        matrix1 = 0.chr*64
        Newton.bodyGetMatrix(b1, matrix1)
        matrix2 = 0.chr*64
        Newton.bodyGetMatrix(b2, matrix2)
        contact1 = 0.chr*12
        contact2 = 0.chr*12
        normal = 0.chr*12
        Newton.collisionClosestPoint(world, col1, matrix1, col2, matrix2, contact1, contact2, normal, 0)
        pt1 = Conversion.convert_point(contact1.unpack('F*'), :m, :in)
        pt2 = Conversion.convert_point(contact2.unpack('F*'), :m, :in)
        [pt1, pt2]
      end

      # Determine whether the two bodies are in contact.
      # @param [Body] body1
      # @param [Body] body2
      # return [Boolean]
      def bodies_collide?(body1, body2)
        check_validity(body1)
        check_validity(body2)
        b1 = body1._body_ptr
        b2 = body2._body_ptr
        joint = Newton.bodyGetFirstContactJoint(b1)
        while !joint.null?
          if Newton.jointGetBody0(joint) == b2 or Newton.jointGetBody1(joint) == b2
            return true
          end
          joint = Newton.bodyGetNextContactJoint(b1, joint)
        end
        false
      end

    end # proxy class

    # @overload initialize(world, ent, type, shape, material, bb_tra)
    #   Create a body from scratch.
    #   @param [AMS::FFI:::Pointer] world A pointer to the newton world.
    #   @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    #   @param [Symbol, String] type Body type. See {TYPES}.
    #   @param [Symbol, String] shape Collision shape. See {Collision::SHAPES}.
    #   @param [Material] material
    # @overload initialize(body, tra)
    #   Create a copy of the body.
    #   @param [Body] body A body Object.
    #   @param [Array<Numeric>, Geom::Transformation, Geom::Point3d] tra New
    #     position or new transformation.
    def initialize(*args)
      if args.size == 2
        # Create a copy of the body.
        body, tra = args
        tra = tra.to_a
        if tra.size == 3
          t = body._entity.transformation.to_a
          t[12..14] = tra
          tra = Geom::Transformation.new(t)
        else
          tra = Geom::Transformation.new(tra)
        end
        if body.entity.respond_to?(:definition)
          ent = Sketchup.active_model.entities.add_instance(body.entity.definition, tra)
        else
          ent = body.entity.copy
          ent.transformation = tra
        end
        ent.material = body.entity.material
        world = body._world_ptr
        col = Newton.collisionCreateInstance(body._collision_ptr)
        type = body.get_type
        shape = body.get_shape
        density = body.get_density
        static_friction = body.get_static_friction
        kinetic_friction = body.get_kinetic_friction
        elasticity = body.get_elasticity
        softness = body.get_softness
      else
        # Create a new body.
        world, ent, type, shape, material = args
        type = type.to_s.downcase.to_sym
        shape = Collision.optimize_shape_name(shape)
        col = Collision.create(world, ent, shape)
        tra = ent.transformation
        density = material.density
        static_friction = material.static_friction
        kinetic_friction = material.kinetic_friction
        elasticity = material.elasticity
        softness = material.softness
      end
      # Extract scale
      scale = Geometry.get_scale(tra)
      matrix = Geometry.extract_scale(tra).to_a
      matrix[12..14] = Conversion.convert_point(matrix[12..14], :in, :m).to_a
      buffer = matrix.pack('F*')
      volume = Newton.convexCollisionCalculateVolume(col)
      mass = volume*density.to_f
      # Create body
      case type
        when :kinematic
          body_ptr = Newton.createKinematicBody(world, col, buffer)
        when :deformable
          body_ptr = Newton.createDeformableBody(world, col, buffer)
        else
          type = :dynamic
          body_ptr = Newton.createDynamicBody(world, col, buffer)
      end
      # Calculate centre of mass and moments of inertia.
      Newton.bodySetMassProperties(body_ptr, mass, col)
      # Destroy reference to collision.
      Newton.destroyCollision(col)
      Newton.bodySetMaterialGroupID(body_ptr, 0)
      # Instance variables
      @_world_ptr         = world
      @_body_ptr          = body_ptr
      @_collision_ptr     = Newton.bodyGetCollision(body_ptr)
      @_entity            = ent
      @_type              = type
      @_shape             = shape
      @_static            = (shape == :static_mesh or mass.zero?)
      @_collidable        = true
      @_magnetic          = false
      @_magnet_force      = 0
      @_mass              = mass
      @_volume            = volume
      @_density           = density
      @_static_friction   = static_friction
      @_kinetic_friction  = kinetic_friction
      @_friction_enabled  = true
      @_elasticity        = elasticity
      @_softness          = softness
      @_scale             = scale
      @_destroy_called    = false
      @_applied_forces    = {}
      @_up_vector         = nil
      @@_instances[body_ptr.address] = self
      BodyObserver.call_event(:on_create, self)
    end

    # @!visibility private
    attr_reader :_applied_forces, :_world_ptr, :_body_ptr, :_collision_ptr, :_up_vector

    private

    def check_validity
      raise 'The body is destroyed!' if invalid?
    end

    public

    # @return [Sketchup::Group, Sketchup::ComponentInstance]
    def entity
      check_validity
      @_entity
    end

    # Get the type of the body.
    # @return [Symbol] See {TYPES}.
    def get_type
      check_validity
      @_type
    end

    # Get the shape of the body.
    # @return [Symbol] See {Collision::SHAPES}.
    def get_shape
      check_validity
      @_shape
    end

    # Get the volume of the body.
    # @return [Numeric] in cubic meters.
    def get_volume
      check_validity
      @_volume
    end

    # Get the mass of the body.
    # @return [Numeric] in kilograms.
    def get_mass
      check_validity
      @_mass
    end

    # Set the mass of the body.
    # @param [Numeric] mass in kilograms.
    # @return [Boolean] +true+ (if successful).
    def set_mass(mass)
      check_validity
      return false if (mass <= 0 or @_static)
      @_mass = mass
      @_density = @_mass/@_volume.to_f
      mass = @_static ? 0 : @_mass
      Newton.bodySetMassProperties(@_body_ptr, mass, @_collision_ptr)
      @_static = false
      true
    end

    # Get the density of the body.
    # @return [Numeric] in kilograms per cubic meter.
    def get_density
      check_validity
      @_density
    end

    # Set the density of the body.
    # @param [Numeric] density in kilograms per cubic meter.
    # @return [Boolean] +true+ (if successful).
    def set_density(density)
      check_validity
      return false if (density <= 0 or @_shape == :staticmesh)
      @_density = density
      @_mass = @_density*@_volume
      mass = @_static ? 0 : @_mass
      Newton.bodySetMassProperties(@_body_ptr, mass, @_collision_ptr)
      @_static = false
      true
    end

    # Get static friction coefficient of the body.
    # @return [Numeric]
    def get_static_friction
      check_validity
      @_static_friction
    end

    # Set static friction coefficient of the body.
    # @return [Numeric] coefficient
    #   This value is clamped between +0.01+ and +2.0+.
    def set_static_friction(coefficient)
      check_validity
      @_static_friction = MSPhysics.clamp(coefficient, 0.01, 2.0)
    end

    # Get dynamic friction coefficient of the body.
    # @return [Numeric]
    def get_kinetic_friction
      check_validity
      @_kinetic_friction
    end

    # Set dynamic friction coefficient of the body.
    # @return [Numeric] coefficient
    #   This value is clamped between +0.01+ and +2.0+.
    def set_kinetic_friction(coefficient)
      check_validity
      @_kinetic_friction = MSPhysics.clamp(coefficient, 0.01, 2.0)
    end

    # Enable/Disable contact friction of the body.
    # @param [Boolean] state
    def enable_friction(state = true)
      check_validity
      @_friction_enabled = state ? true : false
    end

    # Determine whether body friction calculations are enabled.
    # @return [Boolean]
    def friction_enabled?
      check_validity
      @_friction_enabled
    end

    # Get the coefficient of restitution of the body.
    # @return [Numeric]
    def get_elasticity
      check_validity
      @_elasticity
    end

    # Set the coefficient of restitution of the body. This is the rebound ratio. A
    # basketball has a rebound ratio of 0.83. This means the new height of a
    # basketball is 83% of original height within each bounce.
    # @param [Numeric] coefficient
    #   This value is clamped between +0.01+ and +1.00+.
    def set_elasticity(coefficient)
      check_validity
      @_elasticity = MSPhysics.clamp(coefficient, 0.01, 1.0)
    end

    # Get contact softness coefficient of the body.
    # @return [Numeric]
    def get_softness
      check_validity
      @_softness
    end

    # Set contact softness coefficient of the body.
    # @param [Numeric] coefficient
    #   This value is clamped between +0.01+ and +1.00+.
    def set_softness(coefficient)
      check_validity
      @_softness = MSPhysics.clamp(coefficient, 0.01, 1.0)
    end

    # Get the centre of mass of the body.
    # @note The coordinates are returned relative to the body's coordinate
    #   system. Use {#get_position} function to get body's centre of mass in
    #   global coordinates.
    # @return [Geom::Point3d]
    def get_centre_of_mass
      check_validity
      buffer = 0.chr*12
      Newton.bodyGetCentreOfMass(@_body_ptr, buffer)
      centre = Conversion.convert_point(buffer.unpack('F*'), :m, :in)
      centre.x /= @_scale[0]
      centre.y /= @_scale[1]
      centre.z /= @_scale[2]
      centre
    end

    # Set the centre of mass of the body.
    # @param [Geom::Point3d] centre in body coordinate system.
    def set_centre_of_mass(centre)
      buffer = Conversion.convert_point(centre, :in, :m).to_a.pack('F*')
      Newton.bodySetCentreOfMass(@_body_ptr, buffer)
    end

    # Get the moment of inertia of the body.
    # @return [Array<Numeric>] +[Ixx, Iyy, Izz]+
    def get_moment_of_inertia
      check_validity
      mass = 0.chr*4
      ixx = 0.chr*4
      iyy = 0.chr*4
      izz = 0.chr*4
      Newton.bodyGetMassMatrix(@_body_ptr, mass, ixx, iyy, izz)
      ixx = ixx.unpack('F')[0]
      iyy = iyy.unpack('F')[0]
      izz = izz.unpack('F')[0]
      [ixx, iyy, izz]
    end

    # Get the world axis aligned bounding box (AABB) of the body.
    # @return [Array<Geom::Point3d>] +[min_pt, max_pt]+
    def get_bounding_box
      check_validity
      min_pt = 0.chr*12
      max_pt = 0.chr*12
      Newton.bodyGetAABB(@_body_ptr, min_pt, max_pt)
      min = Conversion.convert_point(min_pt.unpack('F*'), :m, :in)
      max = Conversion.convert_point(max_pt.unpack('F*'), :m, :in)
      [min, max]
    end

    # Get the global linear velocity of the body.
    # @return [Geom::Vector3d] The magnitude is in meters per second.
    def get_velocity
      check_validity
      buffer = 0.chr*12
      Newton.bodyGetVelocity(@_body_ptr, buffer)
      Geom::Vector3d.new(buffer.unpack('FFF'))
    end

    # Set the global linear velocity of the body.
    # @param [Array<Numeric>, Geom::Vector3d] velocity The magnitude is in
    #   meters per second.
    def set_velocity(velocity)
      check_validity
      Newton.bodySetVelocity(@_body_ptr, velocity.to_a.pack('FFF'))
    end

    # Get the global angular velocity of the body.
    # @return [Geom::Vector3d]
    def get_omega
      check_validity
      buffer = 0.chr*12
      Newton.bodyGetOmega(@_body_ptr, buffer)
      Geom::Vector3d.new(buffer.unpack('FFF'))
    end

    # Set the global angular velocity of the body.
    # @param [Array<Numeric>, Geom::Vector3d] omega
    def set_omega(omega)
      check_validity
      Newton.bodySetOmega(@_body_ptr, omega.to_a.pack('FFF'))
    end

    # Get the linear viscous damping coefficient applied to the body.
    # @return [Numeric] The linear damping coefficient.
    def get_linear_damping
      check_validity
      Newton.bodyGetLinearDamping(@_body_ptr)
    end

    # Set the linear viscous damping coefficient applied to the body.
    # @param [Numeric] damp
    def set_linear_damping(damp)
      check_validity
      Newton.bodySetLinearDamping(@_body_ptr, damp)
    end

    # Get the angular viscous damping coefficient applied to the body.
    # @return [Geom::Vector3d]
    def get_angular_damping
      check_validity
      buffer = 0.chr*12
      Newton.bodyGetAngularDamping(@_body_ptr, buffer)
      Geom::Vector3d.new(buffer.unpack('FFF'))
    end

    # Set the angular viscous damping coefficient applied to the body.
    # @param [Array<Numeric>, Geom::Vector3d] damp
    def set_angular_damping(damp)
      check_validity
      Newton.bodySetAngularDamping(@_body_ptr, damp.to_a.pack('FFF'))
    end

    # Add an impulse to a specific point on the body.
    # @param [Array<Numeric>, Geom::Point3d] center The center of an impulse in
    #   global space.
    # @param [Array<Numeric>, Geom::Vector3d] delta_vel The desired change in
    #   velocity.
    def add_impulse(center, delta_vel)
      check_validity
      center = Conversion.convert_point(center, :in, :m)
      Newton.bodyAddImpulse(@_body_ptr, delta_vel.to_a.pack('FFF'), center.to_a.pack('F*'))
    end

    # Get the net torque applied to the body.
    # @return [Array<Numeric>] An array of three numeric values.
    def get_torque
      check_validity
      buffer = 0.chr*12
      Newton.bodyGetTorque(@_body_ptr, buffer)
      buffer.unpack('F*')
    end

    # Get the torque allied to the body.
    # @return [Array<Numeric>] An array of three numeric values.
    def get_torque_acc
      check_validity
      buffer = 0.chr*12
      Newton.bodyGetTorqueAcc(@_body_ptr, buffer)
      buffer.unpack('F*')
    end

    # Set the net torque allied to the body.
    # @param [Array<Numeric>] torque An array of three numeric values.
    def set_torque(torque)
      check_validity
      # Works when called inside update force callback only!
      @_applied_forces[:set_torque] = torque.to_a.pack('FFF')
    end

    # Add the net torque allied to the body.
    # @param [Array<Numeric>] torque An array of three numeric values.
    def add_torque(torque)
      check_validity
      # Works when called inside update force callback only!
      @_applied_forces[:add_torque] ||= [0,0,0]
      @_applied_forces[:add_torque][0] += torque[0]
      @_applied_forces[:add_torque][1] += torque[1]
      @_applied_forces[:add_torque][2] += torque[2]
    end

    # Get the net force applied to the body.
    # @return [Geom::Vector3d] in newtons (kg*m/s/s).
    def get_force
      check_validity
      buffer = 0.chr*12
      Newton.bodyGetForce(@_body_ptr, buffer)
      Geom::Vector3d.new(buffer.unpack('F*'))
    end

    # Get the force applied to the body.
    # @return [Geom::Vector3d] in newtons (kg*m/s/s).
    def get_force_acc
      check_validity
      buffer = 0.chr*12
      Newton.bodyGetForceAcc(@_body_ptr, buffer)
      Geom::Vector3d.new(buffer.unpack('F*'))
    end

    # Set the net force applied to the body.
    # @param [Array<Numeric>, Geom::Vector3d] force in newtons (kg*m/s/s).
    def set_force(force)
      check_validity
      # Works when called inside update force callback only!
      @_applied_forces[:set_force] = force.to_a.pack('FFF')
    end

    # Add the net force applied to the body.
    # @param [Array<Numeric>, Geom::Vector3d] force in newtons (kg*m/s/s).
    def add_force(force)
      check_validity
      # Works when called inside update force callback only!
      @_applied_forces[:add_force] ||= [0,0,0]
      @_applied_forces[:add_force][0] += force[0]
      @_applied_forces[:add_force][1] += force[1]
      @_applied_forces[:add_force][2] += force[2]
    end

    # Get the transformation matrix of the body.
    # @return [Geom::Transformation]
    def get_matrix
      check_validity
      buffer = 0.chr*64
      Newton.bodyGetMatrix(@_body_ptr, buffer)
      matrix = buffer.unpack('F*')
      matrix[12..14] = Conversion.convert_point(matrix[12..14], :m, :in).to_a
      tra = Geom::Transformation.new(matrix)
      Geometry.set_scale(tra, @_scale)
    end

    # Set the transformation matrix of the body.
    # @param [Array<Numeric>, Geom::Transformation] tra An array of 16 numeric
    #   values or a Geom::Transformation object.
    def set_matrix(tra)
      check_validity
      scale = Geometry.get_scale(tra)
      if @_shape == :static_mesh or @_shape == :compound_from_mesh
        tra = Geometry.set_scale(tra, @_scale)
      elsif scale != @_scale
        Newton.collisionSetScale(@_collision_ptr, *scale)
        buffer = 0.chr*64
        Newton.collisionGetMatrix(@_collision_ptr, buffer)
        offset_matrix = buffer.unpack('F*')
        for i in 0..2
          r = scale[i]/@_scale[i].to_f
          offset_matrix[12+i] *= r
          @_volume *= r
        end
        Newton.collisionSetMatrix(@_collision_ptr, offset_matrix.pack('F*'))
        @_scale = scale
        @_mass = @_volume*@_density
        mass = @_static ? 0 : @_mass
        Newton.bodySetMassProperties(@_body_ptr, mass, @_collision_ptr)
      end
      matrix = Geometry.extract_scale(tra).to_a
      matrix[12..14] = Conversion.convert_point(matrix[12..14], :in, :m).to_a
      Newton.bodySetMatrix(@_body_ptr, matrix.pack('F*'))
      @_entity.transformation = tra
    end

    # Get the global position of the body.
    # @param [Numeric] mode Get the global position of the body's origin (0) or
    #   centre of mass (1).
    # @return [Geom::Point3d]
    def get_position(mode = 0)
      check_validity
      tra = get_matrix
      if mode == 1
        get_centre_of_mass.transform(tra)
      else
        tra.origin
      end
    end

    # Set the global position of the body.
    # @param [Array<Numeric>, Geom::Point3d] pos
    # @param [Numeric] mode Set the global position of the body's origin (0) or
    #   centre of mass (1).
    def set_position(pos, mode = 0)
      check_validity
      tra = get_matrix
      matrix = tra.to_a
      if mode == 1
        centre = get_centre_of_mass
        tra2 = Geometry.extract_scale(tra)
        pos = Geom::Point3d.new(pos.to_a).transform(tra2.inverse)
        pos.x -= centre.x
        pos.y -= centre.y
        pos.z -= centre.z
        pos.transform!(tra2)
      end
      matrix[12..14] = pos.to_a[0..2]
      set_matrix(matrix)
    end

    # Get the rotation part of the transformation matrix of the body, in form of
    # a unit quaternion.
    # @return [Array<Numeric>] An array of four numeric values:
    #   +[q0, q1, q2, q3]+ - [(x,y,z),w].
    def get_rotation
      check_validity
      buffer = 0.chr*16
      Newton.bodyGetRotation(@_body_ptr, buffer)
      buffer.unpack('F*')
    end

    # Get the three Euler angles of the transformation matrix of the body.
    # @return [Array<Numeric>] An array of three Euler angles expressed in
    #   radians: +[roll, yaw, pitch]+.
    def get_euler_angle
      check_validity
      matrix = 0.chr*64
      Newton.bodyGetMatrix(@_body_ptr, matrix)
      angles = 0.chr *12
      Newton.getEulerAngle(matrix, angles)
      angles.unpack('F*')
    end

    # Set the three Euler angles of the transformation matrix of the body.
    # @param [Array<Numeric>] angles An array of three Euler angles expressed in
    #   radians: +[roll, yaw, pitch]+.
    def set_euler_angle(angles)
      check_validity
      matrix = 0.chr*64
      Newton.setEulerAngle(angles.pack('FFF'), matrix)
      tra = matrix.unpack('F*')
      tra[12..14] = get_position(0).to_a
      set_matrix(tra)
    end

    # Get the freeze state of the body.
    # @return [Boolean] Whether the body is frozen.
    def frozen?
      check_validity
      Newton.bodyGetFreezeState(@_body_ptr) == 1
    end

    # Set the freeze state of the body.
    # @param [Boolean] state +true+ to freeze, +false+ to unfreeze.
    def frozen=(state)
      check_validity
      Newton.bodySetFreezeState(@_body_ptr, state ? 1 : 0)
    end

    # Get the collidable state of the body.
    # @return [Boolean] Whether the body is collidable.
    def collidable?
      check_validity
      @_collidable
    end

    # Set the collidable state of the body.
    # @param [Boolean] state
    def collidable=(state)
      check_validity
      @_collidable = state ? true : false
      Newton.collisionSetCollisonMode(@_collision_ptr, @_collidable ? 1 : 0)
    end

    # Get the static state of the body.
    # @return [Boolean] Whether the body is static.
    def static?
      check_validity
      @_static
    end

    # Set the static state of the body.
    # @param [Boolean] state +true+ to set static, +false+ to set dynamic.
    def static=(state)
      check_validity
      return if @_shape == :static_mesh
      @_static = state ? true : false
      mass = @_static ? 0 : @_mass
      Newton.bodySetMassProperties(@_body_ptr, mass, @_collision_ptr)
    end

    # Get the sleep mode of the body.
    # @return [Boolean] Whether the body is sleeping.
    def get_sleep_state
      check_validity
      Newton.bodyGetSleepState(@_body_ptr) == 1
    end

    # Set the sleep mode of the body.
    # @param [Boolean] state +true+ to set body sleeping, +false+ to set body
    #   active.
    def set_sleep_state(state)
      check_validity
      state = state ? true : false
      Newton.bodySetSleepState(@_body_ptr, state ? 0 : 1)
    end

    # Get the auto-sleep mode of the body.
    # @return [Boolean] +true+ if auto-sleep is on, +false+ if auto-sleep is
    #   off.
    def get_auto_sleep
      check_validity
      Newton.bodyGetAutoSleep(@_body_ptr) == 1
    end

    # Set the auto-sleep mode for the body.
    # @note Keeping auto sleep *on* is a huge performance plus for simulation.
    #   Auto sleep enabled is the default state for a created body; however, for
    #   player control, AI control or some other special circumstance, the
    #   application may want to control the activation/deactivation of the body.
    # @param [Boolean] state +true+ to set auto-sleep on, or +false+ to set
    #   auto-sleep off.
    def set_auto_sleep(state)
      check_validity
      state = state ? true : false
      Newton.bodySetAutoSleep(@_body_ptr, state ? 1 : 0)
    end

    # Get continuous collision mode for the body.
    # @return [Boolean] +true+ if the continuous collision check is on, or
    #   +false+ if off.
    def get_continuous_collision_mode
      check_validity
      Newton.bodyGetContinuousCollisionMode(@_body_ptr) == 1
    end

    # Set continuous collision mode for the body.
    # @param [Boolean] state +true+ to set continuous collision check on, or
    #   +false+ to set continuous collision check off.
    def set_continuous_collision_mode(state)
      check_validity
      state = state ? true : false
      Newton.bodySetContinuousCollisionMode(@_body_ptr, state ? 1 : 0)
    end

    # Get the magnetic state of the body.
    # @return [Boolean] Whether the body is magnetic.
    def magnetic?
      check_validity
      @_magnetic
    end

    # Set the magnetic state of the body. A magnetic body will have a
    # tendency to attract/repel to/from magnets.
    # @param [Boolean] state Magnetic state of the body.
    def magnetic=(state)
      check_validity
      @_magnetic = state ? true : false
    end

    # Get the magnet force of the body.
    # @return [Numeric]
    def get_magnet_force
      check_validity
      @_magnet_force
    end

    # Set the magnet force of the body, the force with which to attract/repel
    # other magnetic bodies.
    # @param [Numeric] force
    def set_magnet_force(force)
      check_validity
      @_magnet_force = force.to_f
    end

    # Get all bodies touching this body.
    # @return [Array<Body>]
    def get_bodies_in_contact
      check_validity
      bodies = []
      joint = Newton.bodyGetFirstContactJoint(@_body_ptr)
      while !joint.null?
        body_ptr1 = Newton.jointGetBody1(joint)
        body1 = Body.get_body_by_body_ptr(body_ptr1)
        bodies.push(body1) if body1
        joint = Newton.bodyGetNextContactJoint(@_body_ptr, joint)
      end
      bodies
    end

    # Get all body contact points.
    # @return [Array<SimpleContact>]
    def get_contacts
      check_validity
      contacts = []
      joint = Newton.bodyGetFirstContactJoint(@_body_ptr)
      while !joint.null?
        contact = Newton.contactJointGetFirstContact(joint)
        while !contact.null?
          material = Newton.contactGetMaterial(contact)
          contacts.push SimpleContact.new(material, @_body_ptr)
          contact = Newton.contactJointGetNextContact(joint, contact)
        end
        joint = Newton.bodyGetNextContactJoint(@_body_ptr, joint)
      end
      contacts
    end

    # Get collision faces of the body.
    # @return [Array<Array<Geom::Point3d>>] An array of faces. Each face
    #   represents an array of points.
    def get_collision_faces
      check_validity
      matrix = 0.chr*64
      Newton.bodyGetMatrix(@_body_ptr, matrix)
      @@_faces.clear
      Newton.collisionForEachPolygonDo(@_collision_ptr, matrix, COLLISION_ITERATOR, nil)
      @@_faces.dup
    end

    # Turn the body with a constraint.
    # @param [Geom::Point3d, NilClass] target A point to look at. Pass +nil+ to
    #   stop looking.
    # @param [Geom::Vector3d] pin The vector to rotate.
    def look_at(target, pin = self.entity.transformation.zaxis)
      check_validity
      unless target
        Newton.destroyJoint(@_world_ptr, @_up_vector[0]) if @_up_vector
        @_up_vector = nil
        return
      end
      unless @_up_vector
        @_up_vector = []
        pin = Geom::Vector3d.new(pin.to_a).normalize
        @_up_vector[0] = Newton.constraintCreateUpVector(@_world_ptr, pin.to_a.pack('FFF'), @_body_ptr)
      end
      @_up_vector[1] = target.to_a
      dir = get_position(1).vector_to(target).normalize
      Newton.upVectorSetPin(@_up_vector[0], dir.to_a.pack('FFF'))
      self.frozen = false
    end

    # Create a copy of the body.
    # @param [Array<Numeric>, Geom::Transformation] tra New position or transformation.
    # @return [Body] New body.
    def copy(tra = self.get_matrix)
      check_validity
      self.class.new(self, tra)
    end

    # Enable simulation of the body.
    def enable
      check_validity
      Newton.bodyEnableSimulation(@_body_ptr)
    end

    # Disable simulation of the body.
    def disable
      check_validity
      Newton.bodyDisableSimulation(@_body_ptr)
    end

    # Destroy the body.
    # @param [Boolean] delete_entity Whether to delete the entity belonging to
    #   the body.
    # @param [Boolean] destroy_pointer Whether to destroy the body pointer.
    # @return [Boolean] +true+ (if successful).
    def destroy(delete_entity = true, destroy_pointer = true)
      return false if @_destroy_called
      @_destroy_called = true
      Newton.destroyBody(@_body_ptr) if destroy_pointer
      BodyObserver.call_event(:on_destroy, self)
      @@_instances.delete(@_body_ptr)
      @_entity.erase! if delete_entity and @_entity.valid?
      @_entity = nil
      @_body_ptr = nil
      @_collision_ptr = nil
      @_world_ptr = nil
      true
    end

    # Determines whether the body is valid.
    # @return [Boolean]
    def valid?
      @_entity and @_body_ptr and @_entity.valid?
    end

    # Determines whether the body is invalid.
    # @return [Boolean]
    def invalid?
      !valid?
    end

  end # class Body
end # module MSPhysics
