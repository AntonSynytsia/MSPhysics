module MSPhysics

  # @since 1.0.0
  class BallAndSocket < Joint

    DEFAULT_MAX_CONE_ANGLE = 30.0.degrees
    DEFAULT_CONE_LIMITS_ENABLED = false
    DEFAULT_MIN_TWIST_ANGLE = -180.0.degrees
    DEFAULT_MAX_TWIST_ANGLE = 180.0.degrees
    DEFAULT_TWIST_LIMITS_ENABLED = false
    DEFAULT_FRICTION = 0.0
    DEFAULT_CONTROLLER = 1.0

    # Create a ball & socket joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, group)
      MSPhysics::Newton::BallAndSocket.create(@address)
      MSPhysics::Newton::BallAndSocket.set_max_cone_angle(@address, DEFAULT_MAX_CONE_ANGLE)
      MSPhysics::Newton::BallAndSocket.enable_cone_limits(@address, DEFAULT_CONE_LIMITS_ENABLED)
      MSPhysics::Newton::BallAndSocket.set_min_twist_angle(@address, DEFAULT_MIN_TWIST_ANGLE)
      MSPhysics::Newton::BallAndSocket.set_max_twist_angle(@address, DEFAULT_MAX_TWIST_ANGLE)
      MSPhysics::Newton::BallAndSocket.enable_twist_limits(@address, DEFAULT_TWIST_LIMITS_ENABLED)
      MSPhysics::Newton::BallAndSocket.set_friction(@address, DEFAULT_FRICTION)
      MSPhysics::Newton::BallAndSocket.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get maximum cone angle in radians.
    # @return [Numeric] A value between 0.0 and PI.
    def max_cone_angle
      MSPhysics::Newton::BallAndSocket.get_max_cone_angle(@address)
    end

    # Set maximum cone angle in radians.
    # @param [Numeric] angle A value between 0.0 and PI.
    def max_cone_angle=(angle)
      MSPhysics::Newton::BallAndSocket.set_max_cone_angle(@address, angle)
    end

    # Determine whether cone angle limits are enabled.
    # @return [Boolean]
    def cone_limits_enabled?
      MSPhysics::Newton::BallAndSocket.cone_limits_enabled?(@address)
    end

    # Enable/disable cone angle limits.
    # @param [Boolean] state
    def cone_limits_enabled=(state)
      MSPhysics::Newton::BallAndSocket.enable_cone_limits(@address, state)
    end

    # Get minimum twist angle in radians.
    # @return [Numeric]
    def min_twist_angle
      MSPhysics::Newton::BallAndSocket.get_min_twist_angle(@address)
    end

    # Set minimum twist angle in radians.
    # @param [Numeric] angle
    def min_twist_angle=(angle)
      MSPhysics::Newton::BallAndSocket.set_min_twist_angle(@address, angle)
    end

    # Get maximum twist angle in radians.
    # @return [Numeric]
    def max_twist_angle
      MSPhysics::Newton::BallAndSocket.get_max_twist_angle(@address)
    end

    # Set maximum twist angle in radians.
    # @param [Numeric] angle
    def max_twist_angle=(angle)
      MSPhysics::Newton::BallAndSocket.set_max_twist_angle(@address, angle)
    end

    # Determine whether twist angle limits are enabled.
    # @return [Boolean]
    def twist_limits_enabled?
      MSPhysics::Newton::BallAndSocket.twist_limits_enabled?(@address)
    end

    # Enable/disable twist angle limits.
    # @param [Boolean] state
    def twist_limits_enabled=(state)
      MSPhysics::Newton::BallAndSocket.enable_twist_limits(@address, state)
    end

    # Get current cone angle in radians.
    # @return [Numeric]
    def cur_cone_angle
      MSPhysics::Newton::BallAndSocket.get_cur_cone_angle(@address)
    end

    # Get current twist angle in radians.
    # @return [Numeric]
    def cur_twist_angle
      MSPhysics::Newton::BallAndSocket.get_cur_twist_angle(@address)
    end

    # Get current twist omega in radians per second.
    # @return [Numeric]
    def cur_twist_omega
      MSPhysics::Newton::BallAndSocket.get_cur_twist_omega(@address)
    end

    # Get current twist acceleration in radians per second per second.
    # @return [Numeric]
    def cur_twist_acceleration
      MSPhysics::Newton::BallAndSocket.get_cur_twist_acceleration(@address)
    end

    # Get angular friction.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @return [Numeric] A value greater than or equal to zero.
    def friction
      MSPhysics::Newton::BallAndSocket.get_friction(@address)
    end

    # Set angular friction.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @param [Numeric] value A value greater than or equal to zero.
    def friction=(value)
      MSPhysics::Newton::BallAndSocket.set_friction(@address, value)
    end

    # Get magnitude of the angular friction.
    # @note Default controller value is 1.0.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @return [Numeric]
    def controller
      MSPhysics::Newton::BallAndSocket.get_controller(@address)
    end

    # Set magnitude of the angular friction.
    # @note Default controller value is 1.0.
    # @note The actual friction is <tt>friction * controller</tt>.
    # @param [Numeric] value
    def controller=(value)
      MSPhysics::Newton::BallAndSocket.set_controller(@address, value)
    end

  end # class BallAndSocket < Joint
end # module MSPhysics
