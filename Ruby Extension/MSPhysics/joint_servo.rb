module MSPhysics

  # @since 1.0.0
  class Servo < Joint

    DEFAULT_MIN = -180.0
    DEFAULT_MAX = 180.0
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_ANGULAR_RATE = 600.0
    DEFAULT_MAX_ACCEL = 3600.0
    DEFAULT_CONTROLLER = nil

    # Create a servo joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-AXIS should represent pin up.
    def initialize(world, parent, pin_tra)
      super(world, parent, pin_tra, 6)
      MSPhysics::Newton::Servo.create(@address)
      MSPhysics::Newton::Servo.set_min(@address, DEFAULT_MIN.degrees)
      MSPhysics::Newton::Servo.set_max(@address, DEFAULT_MAX.degrees)
      MSPhysics::Newton::Servo.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Servo.set_angular_rate(@address, DEFAULT_ANGULAR_RATE.degrees)
      MSPhysics::Newton::Servo.set_max_accel(@address, DEFAULT_MAX_ACCEL.degrees)
      MSPhysics::Newton::Servo.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get minimum angle in radians.
    # @return [Numeric]
    def min
      MSPhysics::Newton::Servo.get_min(@address)
    end

    # Set minimum angle in radians.
    # @param [Numeric] value
    def min=(value)
      MSPhysics::Newton::Servo.set_min(@address, value)
    end

    # Get maximum angle in radians.
    # @return [Numeric]
    def max
      MSPhysics::Newton::Servo.get_max(@address)
    end

    # Set maximum angle in radians.
    # @param [Numeric] value
    def max=(value)
      MSPhysics::Newton::Servo.set_max(@address, value)
    end

    # Determine whether min & max angle limits are enabled.
    # @return [Boolean]
    def limits_enabled?
      MSPhysics::Newton::Servo.are_limits_enabled?(@address)
    end

    # Enable/Disable min & max angle limits.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Servo.enable_limits(@address, state)
    end

    # Get maximum angular rate in radians per second.
    # @return [Numeric]
    def angular_rate
      MSPhysics::Newton::Servo.get_angular_rate(@address)
    end

    # Set maximum angular rate in radians per second.
    # @param [Numeric] value A value greater than or equal to zero.
    def angular_rate=(value)
      MSPhysics::Newton::Servo.set_angular_rate(@address, value)
    end

    # Get maximum angular acceleration in radians per second per second.
    # @return [Numeric]
    def max_accel
      MSPhysics::Newton::Servo.get_max_accel(@address)
    end

    # Set maximum angular acceleration in radians per second per second.
    # @param [Numeric] value A value greater than or equal to zero.
    def max_accel=(value)
      MSPhysics::Newton::Servo.set_max_accel(@address, value)
    end

    # Get current angle in radians.
    # @return [Numeric]
    def cur_angle
      MSPhysics::Newton::Servo.get_cur_angle(@address)
    end

    # Get current omega in radians per second.
    # @return [Numeric]
    def cur_omega
      MSPhysics::Newton::Servo.get_cur_omega(@address)
    end

    # Get current acceleration in radians per second per second.
    # @return [Numeric]
    def cur_acceleration
      MSPhysics::Newton::Servo.get_cur_acceleration(@address)
    end

    # Get servo controller, desired angle in radians. Nil is returned if
    # servo is 'off'.
    # @return [Numeric, nil]
    def controller
      MSPhysics::Newton::Servo.get_controller(@address)
    end

    # Set servo controller, desired angle in radians. Pass nil to 'turn off' the
    # servo.
    # @param [Numeric, nil] value
    def controller=(value)
      MSPhysics::Newton::Servo.set_controller(@address, value)
    end

  end # class Servo < Joint
end # module MSPhysics
