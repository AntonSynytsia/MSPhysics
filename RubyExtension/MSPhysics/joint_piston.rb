module MSPhysics

  # @since 1.0.0
  class Piston < Joint

    DEFAULT_MIN = -10.0
    DEFAULT_MAX = 10.0
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_LINEAR_RATE = 40.0
    DEFAULT_STRENGTH = 0.0
    DEFAULT_REDUCTION_RATIO = 0.1
    DEFAULT_CONTROLLER = nil

    # Create a piston joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-axis should represent pin up.
    # @param [Sketchup::Group, Sketchup::ComponentInstance, nil] group
    def initialize(world, parent, pin_tra, group = nil)
      super(world, parent, pin_tra, 6, group)
      MSPhysics::Newton::Piston.create(@address)
      MSPhysics::Newton::Piston.set_min(@address, DEFAULT_MIN)
      MSPhysics::Newton::Piston.set_max(@address, DEFAULT_MAX)
      MSPhysics::Newton::Piston.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Piston.set_linear_rate(@address, DEFAULT_LINEAR_RATE)
      MSPhysics::Newton::Piston.set_strength(@address, DEFAULT_STRENGTH)
      MSPhysics::Newton::Piston.set_reduction_ratio(@address, DEFAULT_REDUCTION_RATIO)
      MSPhysics::Newton::Piston.set_controller(@address, DEFAULT_CONTROLLER)
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
      MSPhysics::Newton::Piston.limits_enabled?(@address)
    end

    # Enable/disable min & max position limits.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Piston.enable_limits(@address, state)
    end

    # Get maximum linear rate in meters per second.
    # @return [Numeric] A value greater than or equal to zero.
    def linear_rate
      MSPhysics::Newton::Piston.get_linear_rate(@address)
    end

    # Set maximum linear rate in meters per second,
    # @param [Numeric] value A value greater than or equal to zero.
    def linear_rate=(value)
      MSPhysics::Newton::Piston.set_linear_rate(@address, value)
    end

    # Get joint power to mass ratio.
    # @note The actual power in Joules per second can be determined by
    #   multiplying strength with the mass of the connected body. If the mass of
    #   the connected body is zero or if its static, then the power can be
    #   determined by multiplying strength with the mass of the parent body.
    #   However, if it turns out that both parent and child bodies are static
    #   or have a mass of zero, which is unlikely, the strength resembles power.
    # @note A strength value of zero represents infinite power.
    # @return [Numeric] A value greater than or equal to zero.
    def strength
      MSPhysics::Newton::Piston.get_strength(@address)
    end

    # Set joint power to mass ratio.
    # @note The actual power in Joules per second can be determined by
    #   multiplying strength with the mass of the connected body. If the mass of
    #   the connected body is zero or if its static, then the power can be
    #   determined by multiplying strength with the mass of the parent body.
    #   However, if it turns out that both parent and child bodies are static
    #   or have a mass of zero, which is unlikely, the strength resembles power.
    # @note A strength value of zero represents infinite power.
    # @param [Numeric] value A value greater than or equal to zero.
    def strength=(value)
      MSPhysics::Newton::Piston.set_strength(@address, value)
    end

    # Get linear reduction ratio.
    # @note Reduction ratio is a feature that reduces linear rate of a piston
    #   joint when piston current position nears its desired position. Linear
    #   reduction starts acting upon the linear rate of a piston joint when the
    #   difference between the current position and the desired position of the
    #   piston joint is less than <tt>linear_rate * reduction_ratio</tt> meters.
    # @note A reduction ratio of zero will disable the reduction.
    # @note A typical reduction ratio is 0.1.
    # @return [Numeric] A value between 0.0 and 1.0.
    def reduction_ratio
      MSPhysics::Newton::Piston.get_reduction_ratio(@address)
    end

    # Get linear reduction ratio.
    # @note Reduction ratio is a feature that reduces linear rate of a piston
    #   joint when piston current position nears its desired position. Linear
    #   reduction starts acting upon the linear rate of a piston joint when the
    #   difference between the current position and the desired position of the
    #   piston joint is less than <tt>linear_rate * reduction_ratio</tt> meters.
    # @note A reduction ratio of zero will disable the reduction.
    # @note A typical reduction ratio is 0.1.
    # @param [Numeric] value A value between 0.0 and 1.0.
    def reduction_ratio=(value)
      MSPhysics::Newton::Piston.set_reduction_ratio(@address, value)
    end

    # Get piston controller, a desired position in meters. Nil is returned if
    # piston is turned off.
    # @return [Numeric, nil]
    def controller
      MSPhysics::Newton::Piston.get_controller(@address)
    end

    # Set piston controller, a desired position in meters. Pass nil to
    # turn off the piston.
    # @param [Numeric, nil] value
    def controller=(value)
      MSPhysics::Newton::Piston.set_controller(@address, value)
    end

  end # class Piston < Joint
end # module MSPhysics
