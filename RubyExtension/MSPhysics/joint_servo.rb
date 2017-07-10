module MSPhysics

  # @since 1.0.0
  class Servo < Joint

    DEFAULT_MIN = -180.0.degrees
    DEFAULT_MAX = 180.0.degrees
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_RATE = 360.degrees
    DEFAULT_POWER = 0.0
    DEFAULT_REDUCTION_RATIO = 0.1
    DEFAULT_CONTROLLER = nil

    # Create a servo joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation
    #   in global space. Matrix origin is interpreted as the pin position.
    #   Matrix Z-axis is interpreted as the pin direction.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, group)
      MSPhysics::Newton::Servo.create(@address)
      MSPhysics::Newton::Servo.set_min(@address, DEFAULT_MIN)
      MSPhysics::Newton::Servo.set_max(@address, DEFAULT_MAX)
      MSPhysics::Newton::Servo.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Servo.set_rate(@address, DEFAULT_RATE)
      MSPhysics::Newton::Servo.set_power(@address, DEFAULT_POWER)
      MSPhysics::Newton::Servo.set_reduction_ratio(@address, DEFAULT_REDUCTION_RATIO)
      MSPhysics::Newton::Servo.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get current angle in radians.
    # @return [Numeric]
    def cur_angle
      MSPhysics::Newton::Servo.get_cur_angle(@address)
    end

    # Get current angular velocity in radians per second.
    # @return [Numeric]
    def cur_omega
      MSPhysics::Newton::Servo.get_cur_omega(@address)
    end

    # Get current angular acceleration in radians per second per second.
    # @return [Numeric]
    def cur_alpha
      MSPhysics::Newton::Servo.get_cur_alpha(@address)
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
    # @note This option has no effect if SP mode is enabled.
    # @return [Boolean]
    def limits_enabled?
      MSPhysics::Newton::Servo.limits_enabled?(@address)
    end

    # Enable/disable min & max angle limits.
    # @note This option has no effect if SP mode is enabled.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Servo.enable_limits(@address, state)
    end

    # Get maximum angular rate in radians per second.
    # @return [Numeric]
    def rate
      MSPhysics::Newton::Servo.get_rate(@address)
    end

    # Set maximum angular rate in radians per second.
    # @param [Numeric] value A value greater than or equal to zero.
    def rate=(value)
      MSPhysics::Newton::Servo.set_rate(@address, value)
    end

    # Get rotational power in Watts.
    # @note A power value of zero represents maximum power.
    # @return [Numeric]
    def power
      MSPhysics::Newton::Servo.get_power(@address)
    end

    # Set rotational power in Watts.
    # @note A power value of zero represents maximum power.
    # @param [Numeric] value A value greater than or equal to zero.
    def power=(value)
      MSPhysics::Newton::Servo.set_power(@address, value)
    end

    # Get angular reduction ratio.
    # @note Reduction ratio is a feature that reduces angular rate of the joint
    #   when its current angle nears its desired angle. Angular reduction ratio
    #   starts acting upon the angular rate of the joint when the difference
    #   between the current angle and the desired angle of the joint is less
    #   than <tt>rate * reduction_ratio</tt> radians.
    # @note A reduction ratio of zero disables the reduction feature.
    # @note A typical reduction ratio value is 0.1.
    # @return [Numeric] A value between 0.0 and 1.0.
    def reduction_ratio
      MSPhysics::Newton::Servo.get_reduction_ratio(@address)
    end

    # Set angular reduction ratio.
    # @note Reduction ratio is a feature that reduces angular rate of the joint
    #   when its current angle nears its desired angle. Angular reduction ratio
    #   starts acting upon the angular rate of the joint when the difference
    #   between the current angle and the desired angle of the joint is less
    #   than <tt>rate * reduction_ratio</tt> radians.
    # @note A reduction ratio of zero disables the reduction feature.
    # @note A typical reduction ratio value is 0.1.
    # @param [Numeric] value A value between 0.0 and 1.0.
    def reduction_ratio=(value)
      MSPhysics::Newton::Servo.set_reduction_ratio(@address, value)
    end

    # Get servo controller.
    # @return [Numeric, nil] Desired angle in radians or +nil+ if servo is
    #   turned off.
    def controller
      MSPhysics::Newton::Servo.get_controller(@address)
    end

    # Set servo controller.
    # @param [Numeric, nil] value Desired angle in radians or +nil+ to turn off
    #   the servo.
    def controller=(value)
      MSPhysics::Newton::Servo.set_controller(@address, value)
    end

  end # class Servo < Joint
end # module MSPhysics
