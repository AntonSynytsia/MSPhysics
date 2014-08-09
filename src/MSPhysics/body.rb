module MSPhysics
  class Body

    TYPES = [
      :dynamic,
      :kinematic,
      :deformable
    ].freeze

    # @!visibility private
    @@_instances ||= {}

    class << self

      # Get all body instances.
      # @return [Array<Body>]
      def instances
        @@_instances.values
      end

      # @api private
      # @param [Body] body
      # @raise [StandardError] if the body is invalid or destroyed.
      def check_validity(body)
        MSPhysics.validate_type(body, MSPhysics::Body)
        raise 'The body is destroyed!' unless body.valid?
      end

      # Get body by group/component.
      # @param [Sketchup::Group, Sketchup::ComponentInstance] entity
      # @return [Body, NilClass]
      def get_body_by_entity(entity)
        @@_instances.values.each{ |body|
          return body if body.entity == entity
        }
        nil
      end

      # Get body by body pointer.
      # @param [AMS::FFI::Pointer, Fixnum] body_ptr
      # @return [Body, NilClass]
      def get_body_by_body_ptr(body_ptr)
        key = body_ptr.is_a?(AMS::FFI::Pointer) ? body_ptr.address : body_ptr.to_i
        @@_instances[key]
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

      # Determine whether the collision bounding box of one body overlaps with
      # the collision bounding box of another body.
      # @param [Body] body1
      # @param [Body] body2
      # return [Boolean]
      def bodies_aabb_overlap?(body1, body2)
        check_validity(body1)
        check_validity(body2)
        ab1 = body1.get_aabb
        ab2 = body2.get_aabb
        for i in 0..2
          if (ab1.min[i].between?(ab2.min[i], ab2.max[i]) ||
              ab1.max[i].between?(ab2.min[i], ab2.max[i]) ||
              ab2.min[i].between?(ab1.min[i], ab1.max[i]) ||
              ab2.max[i].between?(ab1.min[i], ab1.max[i]))
            next
          end
          return false
        end
        true
      end

      # Determine whether the two bodies can collide with each other.
      # @param [Body] body1
      # @param [Body] body2
      # @return [Boolean]
      def bodies_collidable?(body1, body2)
        return false unless body1.collidable?
        return false unless body2.collidable?
        body1.collidable_with?(body2) and body2.collidable_with?(body1)
      end

    end # proxy class

    # @overload initialize(world, ent, type, shape, material, bb_tra)
    #   Create a body from scratch.
    #   @param [AMS::FFI:::Pointer] world A pointer to the newton world.
    #   @param [Sketchup::Group, Sketchup::ComponentInstance] ent
    #   @param [Symbol, String] type Body type. See {TYPES}. Use dynamic type as
    #     other types are not done yet.
    #   @param [Symbol, String] shape Collision shape. See {Collision::SHAPES}.
    #   @param [Material] material
    # @overload initialize(body, tra)
    #   Create a copy of the body.
    #   @param [Body] body A body Object.
    #   @param [Geom::Transformation, Array<Numeric>] tra
    def initialize(*args)
      if args.size == 2
        # Create a copy of the body.
        body, tra = args
        tra = Geom::Transformation.new(tra.to_a)
        if body.entity.respond_to?(:definition)
          ent = Sketchup.active_model.entities.add_instance(body.entity.definition, tra)
        else
          ent = body.entity.copy
          ent.move! tra
        end
        ent.material        = body.entity.material
        world               = body._world_ptr
        col                 = Newton.collisionCreateInstance(body._collision_ptr)
        type                = body.get_type
        shape               = body.get_shape
        _density            = body.get_density
        _static_friction    = body.get_static_friction
        _dynamic_friction   = body.get_dynamic_friction
        _elasticity         = body.get_elasticity
        _softness           = body.get_softness
        _friction_enabled   = body.friction_enabled?
        _magnetic           = body.magnetic?
        _magnet_force       = body.get_magnet_force
        _magnet_range       = body.get_magnet_range
        _collidable         = body.collidable?
        _contact_mode       = body.instance_variable_get(:@_contact_mode)
        _contact_bodies     = body.instance_variable_get(:@_contact_bodies).clone
      else
        # Create a new body.
        world, ent, type, shape, material = args
        tra                 = ent.transformation
        type                = type.to_s.downcase.to_sym
        shape               = Collision.optimize_shape_name(shape)
        col                 = Collision.create(world, ent, shape)
        _density            = material.density
        _static_friction    = material.static_friction
        _dynamic_friction   = material.dynamic_friction
        _elasticity         = material.elasticity
        _softness           = material.softness
        _friction_enabled   = true
        _magnetic           = false
        _magnet_force       = 0
        _magnet_range       = 10
        _collidable         = true
        _contact_mode       = 0
        _contact_bodies     = []
      end
      # Extract scale
      scale = Geometry.get_scale(tra)
      matrix = Geometry.extract_scale(tra).to_a
      matrix[12..14] = Conversion.convert_point(matrix[12..14], :in, :m).to_a
      buffer = matrix.pack('F*')
      volume = Newton.convexCollisionCalculateVolume(col)
      mass = volume*_density.to_f
      mass = 0.001 if mass < 0.001
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
      @_magnetic          = _magnetic
      @_magnet_force      = _magnet_force
      @_magnet_range      = _magnet_range
      @_collidable        = _collidable
      @_mass              = mass
      @_volume            = volume
      @_density           = _density
      @_static_friction   = _static_friction
      @_dynamic_friction  = _dynamic_friction
      @_friction_enabled  = _friction_enabled
      @_elasticity        = _elasticity
      @_softness          = _softness
      @_scale             = scale
      @_applied_forces    = {}
      @_up_vector         = nil
      @_enabled           = true
      @_contact_mode      = _contact_mode
      @_contact_bodies    = _contact_bodies
      @_faces             = []
      @_collision_iterator = Proc.new { |user_data, vertex_count, face_array, face_id|
        vc = face_array.get_array_of_float(0, vertex_count*3)
        face = []
        for i in 0...vertex_count
          face << Conversion.convert_point(vc[i*3,3], :m, :in)
        end
        @_faces << face
      }
      @_destructor_callback = Proc.new { |body_ptr|
        BodyObserver.call_event(:on_body_removed, self)
        @_world_ptr     = nil
        @_body_ptr      = nil
        @_collision_ptr = nil
        @_applied_forces.clear
        @_contact_bodies.clear
        @_faces.clear
        @@_instances.delete(body_ptr.address)
        @_entity.remove_observer(self) if @_entity.valid?
      }
      Newton.bodySetDestructorCallback(@_body_ptr, @_destructor_callback)
      @@_instances[body_ptr.address] = self
      @_entity.add_observer(self)
      BodyObserver.call_event(:on_body_added, self)
    end

    # @!visibility private
    attr_reader :_world_ptr, :_body_ptr, :_collision_ptr, :_up_vector, :_applied_forces

    # @api private
    # @raise [StandardError] if the body is invalid or destroyed.
    def check_validity
      raise 'The body is destroyed!' unless valid?
    end

    # @return [Sketchup::Group, Sketchup::ComponentInstance]
    def entity
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

    # Set the volume of the body.
    # @note Volume and mass are connected. If you change volume the mass will
    #   automatically be recalculated.
    # @param [Numeric] volume in cubic meters.
    # @return [Boolean] +true+ (if successful).
    def set_volume(volume)
      check_validity
      return false if volume <= 0 or @_volume == 0
      @_volume = volume
      set_mass(@_volume*@_density)
      true
    end

    # Get the mass of the body.
    # @return [Numeric] in kilograms.
    def get_mass
      check_validity
      @_mass
    end

    # Set the mass of the body.
    # @note Mass and density are connected. If you change mass the density will
    #   automatically be recalculated.
    # @param [Numeric] mass in kilograms. Min mass is 0.001 kilograms.
    # @return [Boolean] +true+ (if successful).
    def set_mass(mass)
      check_validity
      return false if mass <= 0 or @_volume == 0
      @_mass = (mass < 0.001) ? 0.001 : mass
      @_density = @_mass/@_volume.to_f
      Newton.bodySetMassProperties(@_body_ptr, @_mass, @_collision_ptr) unless @_static
      true
    end

    # Get the density of the body.
    # @return [Numeric] in kilograms per cubic meter.
    def get_density
      check_validity
      @_density
    end

    # Set the density of the body.
    # @note Density and mass are connected. If you change density the mass will
    #   automatically be recalculated.
    # @param [Numeric] density in kilograms per cubic meter.
    # @return [Boolean] +true+ (if successful).
    def set_density(density)
      check_validity
      return false if density <= 0 or @_volume == 0
      @_density = density
      @_mass = @_density*@_volume
      Newton.bodySetMassProperties(@_body_ptr, @_mass, @_collision_ptr) unless @_static
      true
    end

    # Modify body density, static friction, kinetic friction, coefficient of
    # restitution, and softness at once by setting material.
    # @param [Material] mat
    def set_material(mat)
      set_density(mat.density)
      set_static_friction(mat.static_friction)
      set_dynamic_friction(mat.dynamic_friction)
      set_elasticity(mat.elasticity)
      set_softness(mat.softness)
    end

    # Get static friction coefficient of the body.
    # @return [Numeric]
    def get_static_friction
      check_validity
      @_static_friction
    end

    # Set static friction coefficient of the body.
    # @return [Numeric] coefficient
    #   This value is clamped between +0.01+ and +2.00+.
    def set_static_friction(coefficient)
      check_validity
      @_static_friction = MSPhysics.clamp(coefficient, 0.01, 2.00)
    end

    # Get dynamic friction coefficient of the body.
    # @return [Numeric]
    def get_dynamic_friction
      check_validity
      @_dynamic_friction
    end

    # Set dynamic friction coefficient of the body.
    # @return [Numeric] coefficient
    #   This value is clamped between +0.01+ and +2.00+.
    def set_dynamic_friction(coefficient)
      check_validity
      @_dynamic_friction = MSPhysics.clamp(coefficient, 0.01, 2.00)
    end

    # Enable/Disable contact friction of the body.
    # @param [Boolean] state
    def friction_enabled=(state)
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
    #   This value is clamped between +0.01+ and +2.00+.
    def set_elasticity(coefficient)
      check_validity
      @_elasticity = MSPhysics.clamp(coefficient, 0.01, 2.00)
    end

    # Get contact softness coefficient of the body.
    # @return [Numeric]
    def get_softness
      check_validity
      @_softness
    end

    # Set contact softness coefficient of the body. The higher the softness
    # coefficient the higher the stiffness is.
    # @param [Numeric] coefficient
    #   This value is clamped between +0.01+ and +1.00+.
    def set_softness(coefficient)
      check_validity
      @_softness = MSPhysics.clamp(coefficient, 0.01, 1.00)
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
    # @return [Geom::BoundingBox]
    def get_bounding_box
      check_validity
      min_pt = 0.chr*12
      max_pt = 0.chr*12
      Newton.bodyGetAABB(@_body_ptr, min_pt, max_pt)
      min = Conversion.convert_point(min_pt.unpack('F*'), :m, :in)
      max = Conversion.convert_point(max_pt.unpack('F*'), :m, :in)
      bb = Geom::BoundingBox.new
      bb.add(min,max)
      bb
    end

    alias get_aabb get_bounding_box

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
    # @param [Fixnum] mode
    #   0 - specify position units in meters,
    #   1 - specify position units in inches.
    # @return [Geom::Transformation]
    def get_matrix(mode = 1)
      check_validity
      buffer = 0.chr*64
      Newton.bodyGetMatrix(@_body_ptr, buffer)
      matrix = buffer.unpack('F*')
      if mode == 1
        matrix[12..14] = Conversion.convert_point(matrix[12..14], :m, :in).to_a
      end
      Geometry.set_scale(matrix, @_scale)
    end

    # Set body transformation matrix.
    # @note You may scale body by encrypting the scale factors within the
    #   transformation matrix. Keep in mind that <b>static_mesh</b> and
    #   <b>compound_from_mesh</b> bodies cannot be scaled as NewtonDynamics
    #   has some bugs in scaling. You may apply new scale to them, but
    #   it will be overwritten with original scale.
    # @param [Geom::Transformation, Array<Numeric>] tra
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
        for i in 0..2; @_scale[i] = scale[i] * (@_scale[i] <=> 0) end
        @_mass = @_volume*@_density
        mass = @_static ? 0 : @_mass
        Newton.bodySetMassProperties(@_body_ptr, mass, @_collision_ptr)
      end
      matrix = Geometry.extract_scale(tra).to_a
      matrix[12..14] = Conversion.convert_point(matrix[12..14], :in, :m).to_a
      Newton.bodySetMatrix(@_body_ptr, matrix.pack('F*'))
      @_entity.move! tra
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
      return true if this.static?
      Newton.bodyGetFreezeState(@_body_ptr) == 1
    end

    # Set the freeze state of the body.
    # @param [Boolean] state +true+ to freeze, +false+ to unfreeze.
    def frozen=(state)
      check_validity
      Newton.bodySetFreezeState(@_body_ptr, state ? 1 : 0)
    end

    # Determine whether the body is collidable.
    # @return [Boolean]
    def collidable?
      check_validity
      #Newton.bodyGetCollidable(@_body_ptr) == 1
      @_collidable
    end

    # Set body collidable/non-collidable.
    # @param [Boolean] state
    def collidable=(state)
      check_validity
      @_collidable = state ? true : false
      #Newton.bodySetCollidable(@_body_ptr, state ? 1 : 0)
      Newton.collisionSetCollisionMode(@_collision_ptr, state ? 1 : 0)
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
    def get_sleep_mode
      check_validity
      Newton.bodyGetSleepState(@_body_ptr) == 1
    end

    # Set the sleep mode of the body.
    # @note You can force the body to become active by passing +false+, but you
    #   can't force body to go to sleep, so passing +true+ will do nothing.
    # @param [Boolean] state +true+ to set body sleeping, +false+ to set body
    #   active.
    # @see #frozen=
    def set_sleep_mode(state)
      check_validity
      @_applied_forces[:sleep] = (state ? 1 : 0)
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

    # Set continuous collision mode for the body. Enabling continuous collision
    # ensures that stacks of bodies don't penetrate into each other.
    # @note This will prevent this body from passing other bodies at high speed,
    #   and prevent this body from penetrating into other bodies. This is useful
    #   for performing box stacks, or testing falling objects at heights.
    # @note Huge blocks stacks are known to affect performance significantly.
    #   To simulate wall stacks it is recommended to use small update rate, for
    #   instance, setting an update rate to 1/256.0 of a second, rather than
    #   enabling continuous collision mode. Small update rate will ensure smooth
    #   performance.
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

    # Get magnet range in meters.
    # @return [Numeric]
    def get_magnet_range
      @_magnet_range
    end

    # Set magnet range in meters.
    # @param [Numeric] range
    def set_magnet_range(range)
      @_magnet_range = range.to_f.abs
    end

    # Get all bodies touching this body.
    # @param [Boolean] inc_noncol_bodies Whether to include non-collidable
    #   bodies.
    # @return [Array<Body>]
    def get_bodies_in_contact(inc_noncol_bodies = false)
      check_validity
      bodies = []
      if inc_noncol_bodies
        colA = self._collision_ptr
        matA = 0.chr*64
        Newton.bodyGetMatrix(self._body_ptr, matA)
        @@_instances.values.each { |body|
          next if body == self
          next if body._world_ptr.address != self._world_ptr.address
          next if Body.bodies_collidable?(self, body)
          next unless Body.bodies_aabb_overlap?(self, body)
          colB = body._collision_ptr
          matB = 0.chr*64
          Newton.bodyGetMatrix(body._body_ptr, matB)
          res = Newton.collisionIntersectionTest(@_world_ptr, colA, matA, colB, matB, 0)
          bodies << body if res == 1
        }
      end
      joint = Newton.bodyGetFirstContactJoint(@_body_ptr)
      while !joint.null?
        toucher_ptr = Newton.jointGetBody0(joint)
        if @_body_ptr.address == toucher_ptr.address
          toucher_ptr = Newton.jointGetBody1(joint)
        end
        toucher = Body.get_body_by_body_ptr(toucher_ptr)
        bodies << toucher if toucher
        joint = Newton.bodyGetNextContactJoint(@_body_ptr, joint)
      end
      bodies
    end

    # Get all body contacts.
    # @param [Boolean] inc_noncol_bodies Whether to include non-collidable
    #   bodies.
    # @return [Array<Contact>]
    def get_contacts(inc_noncol_bodies = false)
      check_validity
      contacts = []
      if inc_noncol_bodies
      begin
        colA = self._collision_ptr
        matA = 0.chr*64
        Newton.bodyGetMatrix(self._body_ptr, matA)
        limit = 30
        @@_instances.values.each { |body|
          next if body == self
          next if body._world_ptr.address != self._world_ptr.address
          next if Body.bodies_collidable?(self, body)
          next unless Body.bodies_aabb_overlap?(self, body)
          colB = body._collision_ptr
          matB = 0.chr*64
          Newton.bodyGetMatrix(body._body_ptr, matB)
          buf1 = 0.chr*12*limit
          buf2 = 0.chr*12*limit
          buf3 = 0.chr*12*limit
          attrA = 0.chr*4*limit
          attrB = 0.chr*4*limit
          count = Newton.collisionCollide(@_world_ptr, limit, colA, matA, colB, matB, buf1, buf2, buf3, attrA, attrB, 0)
          next if count == 0
          points = buf1.unpack('F'*3*count)
          normals = buf2.unpack('F'*3*count)
          for i in 0...count
            contacts << Contact2.new(body, points[i*3,3], normals[i*3,3])
          end
        }
        rescue Exception => e
          puts "#{e}\n#{e.backtrace.first}"
        end
      end
      joint = Newton.bodyGetFirstContactJoint(@_body_ptr)
      while !joint.null?
        toucher_ptr = Newton.jointGetBody0(joint)
        if @_body_ptr.address == toucher_ptr.address
          toucher_ptr = Newton.jointGetBody1(joint)
        end
        toucher = Body.get_body_by_body_ptr(toucher_ptr)
        if toucher
          contact = Newton.contactJointGetFirstContact(joint)
          while !contact.null?
            material = Newton.contactGetMaterial(contact)
            contacts << Contact.new(material, @_body_ptr, toucher)
            contact = Newton.contactJointGetNextContact(joint, contact)
          end
        end
        joint = Newton.bodyGetNextContactJoint(@_body_ptr, joint)
      end
      contacts
    end

    # Get collision faces of the body.
    # @return [Array<Array<Geom::Point3d>>] An array of faces. Each face
    #   represents an array of points. The points are coordinated in global
    #   space.
    def get_collision_faces
      check_validity
      matrix = 0.chr*64
      Newton.bodyGetMatrix(@_body_ptr, matrix)
      @_faces.clear
      Newton.collisionForEachPolygonDo(@_collision_ptr, matrix, @_collision_iterator, nil)
      @_faces.dup
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
    # @param [Geom::Transformation, Array<Numeric>] tra
    # @return [Body] New body instance.
    def copy(tra = self.get_matrix)
      check_validity
      self.class.new(self, tra)
    end

    # Enable/Disable simulation of the body.
    # @param [Boolean] state
    def simulation_enabled=(state)
      check_validity
      @_enabled = state ? true : false
      if state
        Newton.bodyEnableSimulation(@_body_ptr)
      else
        Newton.bodyDisableSimulation(@_body_ptr)
      end
    end

    # Determine whether the body is enabled in simulation.
    # @return [Boolean]
    def simulation_enabled?
      check_validity
      @_enabled
    end

    # Get joints created within the body.
    # @return [Array<Joint>]
    def get_joints
      check_validity
      Joint.get_joints(self)
    end

    # Get joints the body is connected to.
    # @return [Array<Joint>]
    def get_connected_joints
      check_validity
      Joint.get_connected_joints(self)
    end

    # Get all collidable bodies.
    # @return [Array<Body>]
    def get_collidable_bodies
      check_validity
      return @_contact_bodies if @_contact_mode == 1
      bodies = []
      @@_instances.values.each { |body|
        next if body.world_ptr == @_world_ptr
        next if body == self
        bodies << body
      }
      @_contact_mode == 0 ? bodies : bodies - @_contact_bodies
    end

    # Get all non-collidable bodies.
    # @return [Array<Body>]
    def get_noncollidable_bodies
      check_validity
      return [] if @_contact_mode == 0
      return @_contact_bodies if @_contact_mode == 2
      bodies = []
      @@_instances.values.each { |body|
        next if body.world_ptr == @_world_ptr
        next if body == self
        bodies << body
      }
      bodies - @_contact_bodies
    end

    # Determine whether the body is collidable with another body.
    # @param [Body] body
    # @return [Boolean]
    def collidable_with?(body)
      check_validity
      return true if @_contact_mode == 0
      return @_contact_bodies.include?(body) if @_contact_mode == 1
      return !@_contact_bodies.include?(body) if @_contact_mode == 2
    end

    # Set collidable bodies.
    # @param [Array<Body>] bodies
    def set_collidable_bodies(bodies)
      check_validity
      @_contact_bodies = bodies.to_a.dup
      @_contact_mode = 1
      bodies = []
      @@_instances.values.each { |body|
        next if body.world_ptr == @_world_ptr
        next if body == self
        bodies << body
      }
      if (bodies - @_contact_bodies).empty?
        @_contact_bodies.clear
        @_contact_mode = 0
      end
    end

    # Set non-collidable bodies.
    # @param [Array<Body>] bodies
    def set_noncollidable_bodies(bodies)
      check_validity
      @_contact_bodies = bodies.to_a.dup
      @_contact_mode = @_contact_bodies.empty? ? 0 : 2
    end

    # Set all bodies collidable.
    # Another way to set all bodies collidable is by writing
    # <tt>this.set_noncollidable_bodies([])</tt>.
    def set_all_bodies_collidable
      set_noncollidable_bodies([])
    end

    # Set all bodies non-collidable.
    # Another way to set all bodies non-collidable is by simply writing
    # <tt>this.set_collidable_bodies([])</tt> or
    # <tt>this.collidable = false</tt>.
    def set_all_bodies_noncollidable
      set_collidable_bodies([])
    end

    # Add some buoyancy to the body.
    # @param [Geom::Vector3d, Array] gravity
    # @param [Geom::Vector3d, Array] plane
    # @param [Numeric] density Fluid density in cubic meters per kilogram.
    # @param [Numeric] viscosity Fluid viscosity.
    # @return [Boolean] success
    def add_buoyancy(gravity = [0,0,-9.8], plane = [0,0,1], density = 997.04, viscosity = 0.894)
      check_validity
      return false if @_static
      matrix = 0.chr*64
      Newton.bodyGetMatrix(@_body_ptr, matrix)
      tra = Geom::Transformation.new(matrix.unpack('F*'))
      cog = 0.chr*12
      Newton.bodyGetCentreOfMass(@_body_ptr, cog)
      centre = Geom::Point3d.new(cog.unpack('F*'))
      centre.transform!(tra)
      cog = centre.to_a.pack('F*')
      grav = gravity.to_a.pack('FFF')
      norm = plane.to_a.pack('FFF')
      dens = 1.0/(0.9*@_volume)
      accel_per_unit_mass = 0.chr*12
      torque_per_unit_mass = 0.chr*12
      Newton.convexCollisionCalculateBuoyancyAcceleration(@_collision_ptr, matrix, cog, grav, norm, dens, 0.1, accel_per_unit_mass, torque_per_unit_mass)
      accel = MSPhysics.scale_vector(accel_per_unit_mass.unpack('F*'), @_mass)
      torque = MSPhysics.scale_vector(torque_per_unit_mass.unpack('F*'), @_mass)
      add_force(accel)
      add_torque(torque)
      true
    end

    # Destroy the body.
    # @param [Boolean] erase_ent Whether to delete body entity.
    def destroy(erase_ent = true)
      check_validity
      Newton.destroyBody(@_body_ptr)
      @_entity.erase! if erase_ent and @_entity.valid?
      @_entity = nil
    end

    # Determine whether the body is valid.
    # @return [Boolean]
    def valid?
      @_body_ptr ? true : false
    end

    # @!visibility private
    def onEraseEntity(ent)
      destroy(false)
    end

  end # class Body
end # module MSPhysics
