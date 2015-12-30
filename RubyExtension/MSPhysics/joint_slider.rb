module MSPhysics

  # @since 1.0.0
  class Slider < Joint

    DEFAULT_MIN = -10.0
    DEFAULT_MAX = 10.0
    DEFAULT_STIFF = 40.0
    DEFAULT_DAMP = 10.0
    DEFAULT_LIMITS_ENABLED = false
    DEFAULT_DAMPER_ENABLED = false

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
      MSPhysics::Newton::Slider.set_stiff(@address, DEFAULT_STIFF)
      MSPhysics::Newton::Slider.set_damp(@address, DEFAULT_DAMP)
      MSPhysics::Newton::Slider.enable_limits(@address, DEFAULT_LIMITS_ENABLED)
      MSPhysics::Newton::Slider.enable_damper(@address, DEFAULT_DAMPER_ENABLED)
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
      MSPhysics::Newton::Slider.are_limits_enabled?(@address)
    end

    # Enable/Disable min and max position limits.
    # @param [Boolean] state
    def limits_enabled=(state)
      MSPhysics::Newton::Slider.enable_limits(@address, state)
    end

    # Get movement stiffness. Higher stiffness makes movement stronger.
    # @return [Numeric]
    def stiff
      MSPhysics::Newton::Slider.get_stiff(@address)
    end

    # Set movement stiffness. Higher stiffness makes movement stronger.
    # @param [Numeric] value A value greater than or equal to zero.
    def stiff=(value)
      MSPhysics::Newton::Slider.set_stiff(@address, value)
    end

    # Get movement damper. Higher damper makes movement slower.
    # @return [Numeric]
    def damp
      MSPhysics::Newton::Slider.get_damp(@address)
    end

    # Set movement damper. Higher damper makes movement slower.
    # @param [Numeric] value A value greater than or equal to zero.
    def damp=(value)
      MSPhysics::Newton::Slider.set_damp(@address, value)
    end

    # Determine whether movement stiff & damp parameters are enabled.
    # @return [Boolean]
    def damper_enabled?
      MSPhysics::Newton::Slider.is_damper_enabled?(@address)
    end

    # Enable/Disable movement stiff & damp parameters.
    # @param [Boolean] state
    def damper_enabled=(state)
      MSPhysics::Newton::Slider.enable_damper(@address, state)
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

  end # class Slider < Joint
end # module MSPhysics
