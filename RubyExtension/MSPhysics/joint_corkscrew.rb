module MSPhysics

  # @since 1.0.0
  class Corkscrew < Joint

    DEFAULT_MIN_POSITION = -10.0
    DEFAULT_MAX_POSITION = 10.0
    DEFAULT_MIN_ANGLE = -180.0.degrees
    DEFAULT_MAX_ANGLE = 180.0.degrees
    DEFAULT_LINEAR_LIMITS_ENABLED = false
    DEFAULT_ANGULAR_LIMITS_ENABLED = false
    DEFAULT_LINEAR_FRICTION = 0.0
    DEFAULT_ANGULAR_FRICTION = 0.0

    # Create a corkscrew joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-axis should represent pin up.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, 6, group)
      MSPhysics::Newton::Corkscrew.create(@address)
      MSPhysics::Newton::Corkscrew.set_min_position(@address, DEFAULT_MIN_POSITION)
      MSPhysics::Newton::Corkscrew.set_max_position(@address, DEFAULT_MAX_POSITION)
      MSPhysics::Newton::Corkscrew.enable_linear_limits(@address, DEFAULT_LINEAR_LIMITS_ENABLED)
      MSPhysics::Newton::Corkscrew.set_linear_friction(@address, DEFAULT_LINEAR_FRICTION)
      MSPhysics::Newton::Corkscrew.set_min_angle(@address, DEFAULT_MIN_ANGLE)
      MSPhysics::Newton::Corkscrew.set_max_angle(@address, DEFAULT_MAX_ANGLE)
      MSPhysics::Newton::Corkscrew.enable_angular_limits(@address, DEFAULT_ANGULAR_LIMITS_ENABLED)
      MSPhysics::Newton::Corkscrew.set_angular_friction(@address, DEFAULT_ANGULAR_FRICTION)
    end

    # Get current position in meters.
    # @return [Numeric]
    def cur_position
      MSPhysics::Newton::Corkscrew.get_cur_position(@address)
    end

    # Get current velocity in meters per second.
    # @return [Numeric]
    def cur_velocity
      MSPhysics::Newton::Corkscrew.get_cur_velocity(@address)
    end

    # Get current linear acceleration in meters per second per second.
    # @return [Numeric]
    def cur_linear_acceleration
      MSPhysics::Newton::Corkscrew.get_cur_linear_acceleration(@address)
    end

    # Get minimum position in meters.
    # @return [Numeric]
    def min_position
      MSPhysics::Newton::Corkscrew.get_min_position(@address)
    end

    # Set minimum position in meters.
    # @param [Numeric] value
    def min_position=(value)
      MSPhysics::Newton::Corkscrew.set_min_position(@address, value)
    end

    # Get maximum position in meters.
    # @return [Numeric]
    def max_position
      MSPhysics::Newton::Corkscrew.get_max_position(@address)
    end

    # Set maximum position in meters.
    def max_position=(value)
      MSPhysics::Newton::Corkscrew.set_max_position(@address, value)
    end

    # Determine whether min and max position limits are enabled.
    # @return [Boolean]
    def linear_limits_enabled?
      MSPhysics::Newton::Corkscrew.linear_limits_enabled?(@address)
    end

    # Enable/disable min and max position limits.
    # @param [Boolean] state
    def linear_limits_enabled=(state)
      MSPhysics::Newton::Corkscrew.enable_linear_limits(@address, state)
    end

    # Get movement friction.
    # @return [Numeric] A value greater than or equal to zero.
    def linear_friction
      MSPhysics::Newton::Corkscrew.get_linear_friction(@address)
    end

    # Set movement friction.
    # @param [Numeric] value A value greater than or equal to zero.
    def linear_friction=(value)
      MSPhysics::Newton::Corkscrew.set_linear_friction(@address, value)
    end

    # Get current angle in radians.
    # @return [Numeric]
    def cur_angle
      MSPhysics::Newton::Corkscrew.get_cur_angle(@address)
    end

    # Get current omega in radians per second.
    # @return [Numeric]
    def cur_omega
      MSPhysics::Newton::Corkscrew.get_cur_omega(@address)
    end

    # Get current angular acceleration in radians per second per second.
    # @return [Numeric]
    def cur_angular_acceleration
      MSPhysics::Newton::Corkscrew.get_cur_angular_acceleration(@address)
    end

    # Get minimum angle in radians.
    # @return [Numeric]
    def min_angle
      MSPhysics::Newton::Corkscrew.get_min_angle(@address)
    end

    # Set minimum angle in radians.
    # @param [Numeric] value
    def min_angle=(value)
      MSPhysics::Newton::Corkscrew.set_min_angle(@address, value)
    end

    # Get maximum angle in radians.
    # @return [Numeric]
    def max_angle
      MSPhysics::Newton::Corkscrew.get_max_angle(@address)
    end

    # Set maximum angle in radians.
    # @param [Numeric] value
    def max_angle=(value)
      MSPhysics::Newton::Corkscrew.set_max_angle(@address, value)
    end

    # Determine whether min & max angle limits are enabled.
    # @return [Boolean]
    def angular_limits_enabled?
      MSPhysics::Newton::Corkscrew.angular_limits_enabled?(@address)
    end

    # Enable/disable min & max angle limits.
    # @param [Boolean] state
    def angular_limits_enabled=(state)
      MSPhysics::Newton::Corkscrew.enable_angular_limits(@address, state)
    end

    # Get rotational friction.
    # @return [Numeric] A value greater than or equal to zero.
    def angular_friction
      MSPhysics::Newton::Corkscrew.get_angular_friction(@address)
    end

    # Set rotational friction.
    # @param [Numeric] value A value greater than or equal to zero.
    def angular_friction=(value)
      MSPhysics::Newton::Corkscrew.set_angular_friction(@address, value)
    end

  end # class Corkscrew < Joint
end # module MSPhysics
