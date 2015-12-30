module MSPhysics

  # @since 1.0.0
  class Piston < Joint

    DEFAULT_MIN = -10.0
    DEFAULT_MAX = 10.0
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_LINEAR_RATE = 10.0
    DEFAULT_MAX_ACCEL = 100.0
    DEFAULT_CONTROLLER = nil

    # Create a piston joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-AXIS should represent pin up.
    def initialize(world, parent, pin_tra)
      super(world, parent, pin_tra, 6)
      MSPhysics::Newton::Piston.create(@address)
      MSPhysics::Newton::Piston.set_min(@address, DEFAULT_MIN.degrees)
      MSPhysics::Newton::Piston.set_max(@address, DEFAULT_MAX.degrees)
      MSPhysics::Newton::Piston.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Piston.set_linear_rate(@address, DEFAULT_LINEAR_RATE.degrees)
      MSPhysics::Newton::Piston.set_max_accel(@address, DEFAULT_MAX_ACCEL.degrees)
      MSPhysics::Newton::Piston.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get minimum position in meters.
    # @return [Numeric]
    def min
      MSPhysics::Newton::Piston.get_min(@address)
    end

    # Set minimum position in meters.
    # @param [Numeric] value
    def min=(value)
      MSPhysics::Newton::Piston.set_min(@address, value)
    end

    # Get maximum position in meters.
    # @return [Numeric]
    def max
      MSPhysics::Newton::Piston.get_max(@address)
    end

    # Set maximum position in meters.
    # @param [Numeric] value
    def max=(value)
      MSPhysics::Newton::Piston.set_max(@address, value)
    end

    # Determine whether min & max position limits are enabled.
    # @return [Boolean]
    def limits_enabled?
      MSPhysics::Newton::Piston.are_limits_enabled?(@address)
    end

    # Enable/Disable min & max position limits.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Piston.enable_limits(@address, state)
    end

    # Get maximum linear rate in meters per second.
    # @return [Numeric]
    def linear_rate
      MSPhysics::Newton::Piston.get_linear_rate(@address)
    end

    # Set maximum linear rate in meters per second.
    # @param [Numeric] value A value greater than or equal to zero.
    def linear_rate=(value)
      MSPhysics::Newton::Piston.set_linear_rate(@address, value)
    end

    # Get maximum linear acceleration in meters per second per second.
    # @return [Numeric]
    def max_accel
      MSPhysics::Newton::Piston.get_max_accel(@address)
    end

    # Set maximum linear acceleration in meters per second per second.
    # @param [Numeric] value A value greater than or equal to zero.
    def max_accel=(value)
      MSPhysics::Newton::Piston.set_max_accel(@address, value)
    end

    # Get current position in meters.
    # @return [Numeric]
    def cur_position
      MSPhysics::Newton::Piston.get_cur_position(@address)
    end

    # Get current velocity in meters per second.
    # @return [Numeric]
    def cur_velocity
      MSPhysics::Newton::Piston.get_cur_velocity(@address)
    end

    # Get current acceleration in meters per second per second.
    # @return [Numeric]
    def cur_acceleration
      MSPhysics::Newton::Piston.get_cur_acceleration(@address)
    end

    # Get piston controller, desired position in meters. Nil is returned if
    # piston is 'off'.
    # @return [Numeric, nil]
    def controller
      MSPhysics::Newton::Piston.get_controller(@address)
    end

    # Set piston controller, desired position in meters. Pass nil to 'turn off'
    # the piston.
    # @param [Numeric, nil] value
    def controller=(value)
      MSPhysics::Newton::Piston.set_controller(@address, value)
    end

  end # class Piston < Joint
end # module MSPhysics
