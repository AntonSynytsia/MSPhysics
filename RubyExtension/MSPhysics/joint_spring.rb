module MSPhysics

  # @since 1.0.0
  class Spring < Joint

    DEFAULT_MIN = -10.0
    DEFAULT_MAX = 10.0
    DEFAULT_ACCEL = 40.0
    DEFAULT_DAMP = 10.0
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_STRONG_MODE_ENABLED = true
    DEFAULT_START_POSITION = 0.0
    DEFAULT_CONTROLLER = 1.0

    # Create a spring joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-axis should represent pin up.
    def initialize(world, parent, pin_tra)
      super(world, parent, pin_tra, 6)
      MSPhysics::Newton::Spring.create(@address)
      MSPhysics::Newton::Spring.set_min(@address, DEFAULT_MIN)
      MSPhysics::Newton::Spring.set_max(@address, DEFAULT_MAX)
      MSPhysics::Newton::Spring.set_accel(@address, DEFAULT_ACCEL)
      MSPhysics::Newton::Spring.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::Spring.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Spring.enable_strong_mode(@address, DEFAULT_STRONG_MODE_ENABLED)
      MSPhysics::Newton::Spring.set_start_position(@address, DEFAULT_START_POSITION)
      MSPhysics::Newton::Spring.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get minimum position in meters with respect to the starting position.
    # @return [Numeric]
    def min
      MSPhysics::Newton::Spring.get_min(@address)
    end

    # Set minimum position in meters with respect to the starting position.
    # @param [Numeric] value
    def min=(value)
      MSPhysics::Newton::Spring.set_min(@address, value)
    end

    # Get maximum position in meters with respect to the starting position.
    # @return [Numeric]
    def max
      MSPhysics::Newton::Spring.get_max(@address)
    end

    # Set maximum position in meters with respect to the starting position.
    def max=(value)
      MSPhysics::Newton::Spring.set_max(@address, value)
    end

    # Determine whether min and max position limits are enabled.
    # @return [Boolean]
    def limits_enabled?
      MSPhysics::Newton::Spring.limits_enabled?(@address)
    end

    # Enable/disable min and max position limits.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Spring.enable_limits(@address, state)
    end

    # Determine whether strong stiff and damp mode is enabled.
    # @return [Boolean]
    def strong_mode_enabled?
      MSPhysics::Newton::Spring.strong_mode_enabled?(@address)
    end

    # Enable/disable strong stiff and damp mode.
    # @param [Boolean] state
    def strong_mode_enabled=(state)
      MSPhysics::Newton::Spring.enable_strong_mode(@address, state)
    end

    # Get movement acceleration.
    # @note Higher acceleration makes movement faster.
    # @return [Numeric]
    def accel
      MSPhysics::Newton::Spring.get_accel(@address)
    end

    # Set movement acceleration.
    # @note Higher acceleration makes movement faster.
    # @param [Numeric] value A value greater than or equal to zero.
    def accel=(value)
      MSPhysics::Newton::Spring.set_accel(@address, value)
    end

    # Get movement damper.
    # @note Higher damper makes movement stronger.
    # @return [Numeric]
    def damp
      MSPhysics::Newton::Spring.get_damp(@address)
    end

    # Set movement damper.
    # @note Higher damper makes movement stronger.
    # @param [Numeric] value A value greater than or equal to zero.
    def damp=(value)
      MSPhysics::Newton::Spring.set_damp(@address, value)
    end

    # Get current position in meters with respect to the starting position.
    # @return [Numeric]
    def cur_position
      MSPhysics::Newton::Spring.get_cur_position(@address)
    end

    # Get current velocity in meters per second.
    # @return [Numeric]
    def cur_velocity
      MSPhysics::Newton::Spring.get_cur_velocity(@address)
    end

    # Get current acceleration in meters per second per second.
    # @return [Numeric]
    def cur_acceleration
      MSPhysics::Newton::Spring.get_cur_acceleration(@address)
    end

    # Get starting position along joint Z-axis in meters.
    # @note The actual starting position is
    #   <tt>start_posistion * controller</tt>.
    # @return [Numeric]
    def start_position
      MSPhysics::Newton::Spring.get_start_position(@address)
    end

    # Set starting position along joint Z-axis in meters.
    # @note The actual starting position is
    #   <tt>start_posistion * controller</tt>.
    # @param [Numeric] position
    def start_position=(position)
      MSPhysics::Newton::Spring.set_start_position(@address, position)
    end

    # Get spring controller, magnitude and direction of the starting position.
    # By default, controller value is 1.
    # @note The actual starting position is
    #   <tt>start_posistion * controller</tt>.
    # @return [Numeric]
    def controller
      MSPhysics::Newton::Spring.get_controller(@address)
    end

    # Set spring controller, magnitude and direction of the starting position.
    # By default, controller value is 1.
    # @note The actual starting position is
    #   <tt>start_posistion * controller</tt>.
    # @param [Numeric] value
    def controller=(value)
      MSPhysics::Newton::Spring.set_controller(@address, value)
    end

  end # class Spring < Joint
end # module MSPhysics
