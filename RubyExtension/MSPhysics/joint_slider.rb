module MSPhysics

  # @since 1.0.0
  class Slider < Joint

    DEFAULT_MIN = -10.0
    DEFAULT_MAX = 10.0
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_FRICTION = 0.0
    DEFAULT_CONTROLLER = 1.0

    # Create a slider joint.
    # @param [MSPhysics::World] world
    # @param [MSPhysics::Body, nil] parent
    # @param [Geom::Transformation, Array<Numeric>] pin_tra Pin transformation.
    #   Of the given matrix, matrix origin should represent pin origin, and
    #   matrix Z-AXIS should represent pin up.
    def initialize(world, parent, pin_tra)
      super(world, parent, pin_tra, 6)
      MSPhysics::Newton::Slider.create(@address)
      MSPhysics::Newton::Slider.set_min(@address, DEFAULT_MIN)
      MSPhysics::Newton::Slider.set_max(@address, DEFAULT_MAX)
      MSPhysics::Newton::Slider.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Slider.set_friction(@address, DEFAULT_FRICTION)
      MSPhysics::Newton::Slider.set_controller(@address, DEFAULT_CONTROLLER)
    end

    # Get current position in meters.
    # @return [Numeric]
    def cur_position
      MSPhysics::Newton::Slider.get_cur_position(@address)
    end

    # Get current velocity in meters per second.
    # @return [Numeric]
    def cur_velocity
      MSPhysics::Newton::Slider.get_cur_velocity(@address)
    end

    # Get current acceleration in meters per second per second.
    # @return [Numeric]
    def cur_acceleration
      MSPhysics::Newton::Slider.get_cur_acceleration(@address)
    end

    # Get minimum position in meters.
    # @return [Numeric]
    def min
      MSPhysics::Newton::Slider.get_min(@address)
    end

    # Set minimum position in meters.
    # @param [Numeric] value
    def min=(value)
      MSPhysics::Newton::Slider.set_min(@address, value)
    end

    # Get maximum position in meters.
    # @return [Numeric]
    def max
      MSPhysics::Newton::Slider.get_max(@address)
    end

    # Set maximum position in meters.
    def max=(value)
      MSPhysics::Newton::Slider.set_max(@address, value)
    end

    # Determine whether min and max position limits are enabled.
    # @return [Boolean]
    def limits_enabled?
      MSPhysics::Newton::Slider.limits_enabled?(@address)
    end

    # Enable/Disable min and max position limits.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Slider.enable_limits(@address, state)
    end

    # Get movement friction.
    # @return [Numeric] A value greater than or equal to zero.
    def friction
      MSPhysics::Newton::Slider.get_friction(@address)
    end

    # Set movement friction.
    # @param [Numeric] value A value greater than or equal to zero.
    def friction=(value)
      MSPhysics::Newton::Slider.set_friction(@address, value)
    end

    # Get slider controller, the magnitude of the linear friction.
    # @note By default, the controller value is 1.0.
    # @return [Numeric]
    def controller
      MSPhysics::Newton::Slider.get_controller(@address)
    end

    # Set slider controller, the magnitude of the linear friction.
    # @note By default, the controller value is 1.0.
    # @param [Numeric] value
    def controller=(value)
      MSPhysics::Newton::Slider.set_controller(@address, value)
    end

  end # class Slider < Joint
end # module MSPhysics
