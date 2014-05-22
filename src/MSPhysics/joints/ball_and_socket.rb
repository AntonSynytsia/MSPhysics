module MSPhysics
  class BallAndSocket < Joint

    # @param [Array<Numeric>, Geom::Point3d] pos Attach point in global space.
    # @param [Body, NilClass] parent Pass +nil+ to create joint without a parent
    #   body.
    # @param [Body, NilClass] child Pass +nil+ to create an initially
    #   disconnected joint.
    def initialize(pos, pin_dir, parent, child)
      super(pos, pin_dir, parent, child, 6, false)
      @max_cone_angle = 0
      @max_twist_angle = 0
      @connect_proc = Proc.new {
        world_ptr = Newton.bodyGetWorld(@child._body_ptr)
        parent_ptr = @parent ? @parent._body_ptr : nil
        pos = get_position(false).to_a.pack('F*')
        @joint_ptr = Newton.constraintCreateBall(world_ptr, pos, @child._body_ptr, parent_ptr)
      }
      connect(child)
    end

    private

    def update_limits
      return unless connected?
      dir = get_direction.to_a.pack('F*')
      Newton.ballSetConeLimits(@joint_ptr, dir, @max_cone_angle, @max_twist_angle)
    end

    def on_connect
      update_limits
    end

    public

    # Set joint position in global space.
    # @param [Array<Numeric>, Geom::Point3d] pos
    def set_position(pos)
      super(pos)
      if connected?
        disconnect
        connect
      end
    end

    # Modify joint pin direction.
    # @param [Array<Numeric>, Geom::Vector3d] dir
    def set_direction(dir)
      super(dir)
      if connected?
        disconnect
        connect
      end
    end

    # Get joint Euler angles in degrees.
    # @return [Array<Numeric>, NilClass] An array of 3 Euler angles.
    def angles
      return unless connected?
      ptr = 0.chr*12
      Newton.ballGetJointAngle(@joint_ptr, ptr)
      ptr.unpack('F*')
    end

    # Get joint omega in degrees per second.
    # @return [Geom::Vector3d, NilClass]
    def omega
      return unless connected?
      ptr = 0.chr*12
      Newton.ballGetJointOmega(@joint_ptr, ptr)
      Geom::Vector3d.new(ptr.unpack('F*'))
    end

    # Get max cone angle in degrees.
    # @return [Numeric]
    def max_cone_angle
      @max_cone_angle.radians
    end

    # Set max cone angle in degrees.
    # @note This value is clamped between 5 and 175 degrees.
    # @param [Numeric] value Pass +0+ to disable cone limits.
    def max_cone_angle=(value)
      if value.zero?
        @max_cone_angle = 0
      else
        value = MSPhysics.clamp(value, 5, 175)
        @max_cone_angle = value.degrees
      end
      update_limits
    end

    # Get max twist angle in degrees.
    # @return [Numeric]
    def max_twist_angle
      @max_twist_angle.radians
    end

    # Set max twist angle in degrees.
    # @note This value is clamped between 5 and 90 degrees.
    # @param [Numeric] value Pass +0+ to disable twist limits.
    def max_twist_angle=(value)
      if value.zero?
        @max_twist_angle = 0
      else
        value = MSPhysics.clamp(value, 5, 90)
        @max_twist_angle = value.degrees
      end
      update_limits
    end

  end # class BallAndSocket
end # module MSPhysics
