module MSPhysics
  class Contact

    # Create a Contact object.
    # @param [AMS::FFI::Pointer] material A contact material.
    # @param [AMS::FFI::Pointer] body_ptr A body in contact.
    def initialize(material, body_ptr)
      @mat = material
      @_body_ptr = body_ptr
    end

    # Get the contact force vector.
    # @note The force value is only valid when the colliding bodies are at rest.
    # @return [Geom::Vector3d]
    def get_force
      force = 0.chr*12
      Newton.materialGetContactForce(@mat, @_body_ptr, force)
      Geom::Vector3d.new(force.unpack('FFF'))
    end

    # Get the contact position.
    # @return [Geom::Point3d]
    def get_position
      pos = 0.chr*12
      normal = 0.chr*12
      Newton.materialGetContactPositionAndNormal(@mat, @_body_ptr, pos, normal)
      Geometry.convert_point(pos.unpack('FFF'), :m, :in)
    end

    # Get the contact normal vector.
    # @return [Geom::Vector3d]
    def get_normal_direction
      pos = 0.chr*12
      normal = 0.chr*12
      Newton.materialGetContactPositionAndNormal(@mat, @_body_ptr, pos, normal)
      Geom::Vector3d.new(normal.unpack('FFF'))
    end

    # Get the speed of contact along the normal vector.
    # @return [Numeric]
    def get_normal_speed
      Newton.materialGetContactNormalSpeed(@mat)
    end

    # Get the tangent vector to the contact point.
    # @param [Fixnum] index Primary (0) or secondary (1).
    # @return [Geom::Vector3d]
    def get_tangent_direction(index = 0)
      dir0 = 0.chr*12
      dir1 = 0.chr*12
      Newton.materialGetContactTangentDirections(@mat, @_body_ptr, dir0, dir1)
      if index == 0
        Geom::Vector3d.new(dir0.unpack('FFF'))
      else
        Geom::Vector3d.new(dir1.unpack('FFF'))
      end
    end

    # Get the contact speed along the tangent vector.
    # @param [Fixnum] index Primary (0) or secondary (1).
    # @return [Numeric]
    def get_tangent_speed(index = 0)
      Newton.materialGetContactTangentSpeed(@mat, index)
    end

    # Get the maximum normal impact of the contact.
    # @return [Numeric]
    def get_max_normal_impact
      Newton.materialGetContactMaxNormalImpact(@mat)
    end

    # Get the maximum tangent impact of the contact.
    # @param [Fixnum] index Primary (0) or secondary (1).
    # @return [Numeric]
    def get_max_tangent_impact(index = 0)
      Newton.materialGetContactMaxTangentImpact(@mat, index)
    end

    # Override the default softness value for the contact.
    # @param [Numeric] softness A value between 0.01 and 0.7.
    def set_softness(softness)
      Newton.materialSetContactSoftness(@mat, softness)
    end

    # Override the default elasticity (coefficient of restitution) value for the
    # contact.
    # @param [Numeric] restitution A value between 0.01 and 2.0.
    def set_elasticity(restitution)
      Newton.materialSetContactElasticity(@mat, restitution)
    end

    # Enable or disable friction calculation for this contact.
    # @param [Boolean] state +true+ to enable, or +false+ to disable.
    # @param [Fixnum] index Primary (0) or secondary (1).
    def set_friction_state(state, index = 0)
      Newton.materialSetContactFrictionState(@mat, state ? 1 : 0, index)
    end

    # Override the default value of the kinetic and static coefficient of
    # friction for this contact.
    # @note You must increase the static friction first to set a kinetic
    #   friction higher than the current static friction.
    # @param [Numeric] static A value between 0.01 and 2.0.
    # @param [Numeric] kinetic A value between 0.01 and 2.0.
    # @param [Fixnum] index Primary (0) or secondary (1).
    def set_friction_coef(static, kinetic, index = 0)
      Newton.materialSetContactFrictionCoef(@mat, static, kinetic, index)
    end

    # Force the contact point to have a non-zero acceleration.
    # @note This function can be used for spacial effects like implementing jump
    #   or an explosive.
    # @param [Numeric] accel Desired contact acceleration, must be a positive
    #   value.
    def set_normal_acceleration(accel)
      Newton.materialSetContactNormalAcceleration(@mat, accel)
    end

    # Set the new direction of the contact point.
    # @param [Array, Geom::Vector3d] dir
    def set_normal_direction(dir)
      Newton.materialSetContactNormalDirection(@mat, dir.to_a.pack('FFF'))
    end

    # Force the contact point to have a non-zero acceleration along the surface
    # plane.
    # @param [Numeric] accel Desired contact acceleration, must be a positive
    #   value.
    # @param [Fixnum] index Primary (0) or secondary (1).
    def set_tangent_acceleration(accel, index = 0)
      Newton.materialSetContactNormalAcceleration(@mat, accel, index)
    end

    # Rotate the tangent direction of the contacts until the primary direction
    # is aligned with the +align_vector+. This function can be used in
    # conjunction with {#set_tangent_acceleration} in order to create special
    # effects. For example, conveyor belts, cheap low LOD vehicles, slippery
    # surfaces, etc.
    # @param [Array, Geom::Vector3d] align_vector
    def set_rotate_tangent_direction(align_vector)
      Newton.materialSetContactNormalDirection(@mat, align_vector.to_a.pack('FFF'))
    end

  end # class Contact
end # module MSPhysics
